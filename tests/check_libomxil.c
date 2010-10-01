/*
 * Copyright (C) 2008-2009 Nokia Corporation.
 *
 * Author: Felipe Contreras <felipe.contreras@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation
 * version 2.1 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <check.h>
#include <OMX_Core.h>
#include <OMX_Component.h>

#include <glib.h>
#include <dlfcn.h>

static const char *lib_name;
static void *dl_handle;
static OMX_ERRORTYPE (*init) (void);
static OMX_ERRORTYPE (*deinit) (void);
static OMX_ERRORTYPE (*get_handle) (OMX_HANDLETYPE * handle,
    OMX_STRING name, OMX_PTR data, OMX_CALLBACKTYPE * callbacks);
static OMX_ERRORTYPE (*free_handle) (OMX_HANDLETYPE handle);

typedef struct CustomData CustomData;

struct CustomData
{
  OMX_HANDLETYPE omx_handle;
  OMX_STATETYPE omx_state;
  GCond *omx_state_condition;
  GMutex *omx_state_mutex;
};

static CustomData *
custom_data_new (void)
{
  CustomData *custom_data;
  custom_data = g_new0 (CustomData, 1);
  custom_data->omx_state_condition = g_cond_new ();
  custom_data->omx_state_mutex = g_mutex_new ();
  return custom_data;
}

static void
custom_data_free (CustomData * custom_data)
{
  g_mutex_free (custom_data->omx_state_mutex);
  g_cond_free (custom_data->omx_state_condition);
  g_free (custom_data);
}

static inline void
change_state (CustomData * core, OMX_STATETYPE state)
{
  fail_if (OMX_SendCommand (core->omx_handle, OMX_CommandStateSet, state,
          NULL) != OMX_ErrorNone);
}

static inline void
complete_change_state (CustomData * core, OMX_STATETYPE state)
{
  g_mutex_lock (core->omx_state_mutex);

  core->omx_state = state;
  g_cond_signal (core->omx_state_condition);

  g_mutex_unlock (core->omx_state_mutex);
}

static inline void
wait_for_state (CustomData * core, OMX_STATETYPE state)
{
  g_mutex_lock (core->omx_state_mutex);

  while (core->omx_state != state)
    g_cond_wait (core->omx_state_condition, core->omx_state_mutex);

  g_mutex_unlock (core->omx_state_mutex);
}

static OMX_ERRORTYPE
EventHandler (OMX_HANDLETYPE omx_handle,
    OMX_PTR app_data,
    OMX_EVENTTYPE event, OMX_U32 data_1, OMX_U32 data_2, OMX_PTR event_data)
{
  CustomData *core;

  core = app_data;

  switch (event) {
    case OMX_EventCmdComplete:
    {
      OMX_COMMANDTYPE cmd;

      cmd = (OMX_COMMANDTYPE) data_1;

      switch (cmd) {
        case OMX_CommandStateSet:
          complete_change_state (core, data_2);
          break;
        default:
          break;
      }
      break;
    }
    default:
      break;
  }

  return OMX_ErrorNone;
}

static OMX_CALLBACKTYPE callbacks = { EventHandler, NULL, NULL };

START_TEST (test_basic)
{
  OMX_ERRORTYPE omx_error;
  omx_error = init ();
  fail_if (omx_error != OMX_ErrorNone);
  omx_error = deinit ();
  fail_if (omx_error != OMX_ErrorNone);
}

END_TEST
START_TEST (test_handle)
{
  OMX_ERRORTYPE omx_error;
  OMX_HANDLETYPE omx_handle;

  omx_error = init ();
  fail_if (omx_error != OMX_ErrorNone);

  omx_error = get_handle (&omx_handle, "OMX.check.dummy", NULL, NULL);
  fail_if (omx_error != OMX_ErrorNone);

  omx_error = free_handle (omx_handle);
  fail_if (omx_error != OMX_ErrorNone);

  omx_error = deinit ();
  fail_if (omx_error != OMX_ErrorNone);
}

END_TEST
START_TEST (test_idle)
{
  CustomData *custom_data;
  OMX_ERRORTYPE omx_error;
  OMX_HANDLETYPE omx_handle;

  custom_data = custom_data_new ();

  omx_error = init ();
  fail_if (omx_error != OMX_ErrorNone);

  omx_error =
      get_handle (&omx_handle, "OMX.check.dummy", custom_data, &callbacks);
  fail_if (omx_error != OMX_ErrorNone);

  custom_data->omx_handle = omx_handle;

  change_state (custom_data, OMX_StateIdle);

  /* allocate_buffers */

  wait_for_state (custom_data, OMX_StateIdle);

  change_state (custom_data, OMX_StateLoaded);

  /* free_buffers */

  wait_for_state (custom_data, OMX_StateLoaded);

  omx_error = free_handle (omx_handle);
  fail_if (omx_error != OMX_ErrorNone);

  omx_error = deinit ();
  fail_if (omx_error != OMX_ErrorNone);

  custom_data_free (custom_data);
}

END_TEST static Suite *
util_suite (void)
{
  Suite *s = suite_create ("libomxil");
  TCase *tc_chain = tcase_create ("general");

  lib_name = "libomxil-foo.so";

  if (!g_thread_supported ())
    g_thread_init (NULL);

  {
    dl_handle = dlopen (lib_name, RTLD_LAZY);
    if (!dl_handle) {
            /** @todo report error. */
    }

    init = dlsym (dl_handle, "OMX_Init");
    deinit = dlsym (dl_handle, "OMX_Deinit");
    get_handle = dlsym (dl_handle, "OMX_GetHandle");
    free_handle = dlsym (dl_handle, "OMX_FreeHandle");
  }

  tcase_add_test (tc_chain, test_basic);
  tcase_add_test (tc_chain, test_handle);
  tcase_add_test (tc_chain, test_idle);
  suite_add_tcase (s, tc_chain);

  return s;
}

int
main (void)
{
  int number_failed;
  Suite *s;
  SRunner *sr;

  s = util_suite ();
  sr = srunner_create (s);
  srunner_run_all (sr, CK_NORMAL);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);

  return (number_failed == 0) ? 0 : 1;
}
