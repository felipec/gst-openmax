/*
 * Copyright (C) 2008 Nokia Corporation.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <check.h>
#include <OMX_Core.h>

#include <dlfcn.h>

static const char *lib_name;
static void *dl_handle;
static OMX_ERRORTYPE (*init) (void);
static OMX_ERRORTYPE (*deinit) (void);
static OMX_ERRORTYPE (*get_handle) (OMX_HANDLETYPE *handle,
                                    OMX_STRING name,
                                    OMX_PTR data,
                                    OMX_CALLBACKTYPE *callbacks);
static OMX_ERRORTYPE (*free_handle) (OMX_HANDLETYPE handle);

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

static Suite *
util_suite (void)
{
    Suite *s = suite_create ("libomxil");
    TCase *tc_chain = tcase_create ("general");

    lib_name = "libomxil-foo.so";

    {
        dl_handle = dlopen (lib_name, RTLD_LAZY);
        if (!dl_handle)
        {
            /** @todo report error. */
        }

        init = dlsym (dl_handle, "OMX_Init");
        deinit = dlsym (dl_handle, "OMX_Deinit");
        get_handle = dlsym (dl_handle, "OMX_GetHandle");
        free_handle = dlsym (dl_handle, "OMX_FreeHandle");
    }

    tcase_add_test (tc_chain, test_basic);
    tcase_add_test (tc_chain, test_handle);
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
