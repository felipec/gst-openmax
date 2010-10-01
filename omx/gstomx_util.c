/*
 * Copyright (C) 2006-2007 Texas Instruments, Incorporated
 * Copyright (C) 2007-2009 Nokia Corporation.
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

#include "gstomx_util.h"
#include <dlfcn.h>

#include "gstomx.h"

GST_DEBUG_CATEGORY (gstomx_util_debug);

/*
 * Forward declarations
 */

static inline void change_state (GOmxCore * core, OMX_STATETYPE state);

static inline void wait_for_state (GOmxCore * core, OMX_STATETYPE state);

static inline void
in_port_cb (GOmxPort * port, OMX_BUFFERHEADERTYPE * omx_buffer);

static inline void
out_port_cb (GOmxPort * port, OMX_BUFFERHEADERTYPE * omx_buffer);

static inline void
got_buffer (GOmxCore * core,
    GOmxPort * port, OMX_BUFFERHEADERTYPE * omx_buffer);

static OMX_ERRORTYPE
EventHandler (OMX_HANDLETYPE omx_handle,
    OMX_PTR app_data,
    OMX_EVENTTYPE event, OMX_U32 data_1, OMX_U32 data_2, OMX_PTR event_data);

static OMX_ERRORTYPE
EmptyBufferDone (OMX_HANDLETYPE omx_handle,
    OMX_PTR app_data, OMX_BUFFERHEADERTYPE * omx_buffer);

static OMX_ERRORTYPE
FillBufferDone (OMX_HANDLETYPE omx_handle,
    OMX_PTR app_data, OMX_BUFFERHEADERTYPE * omx_buffer);

static inline const char *omx_state_to_str (OMX_STATETYPE omx_state);

static inline const char *omx_error_to_str (OMX_ERRORTYPE omx_error);

static inline GOmxPort *get_port (GOmxCore * core, guint index);

static void core_deinit (GOmxCore * core);

static inline void port_free_buffers (GOmxPort * port);

static inline void port_allocate_buffers (GOmxPort * port);

static inline void port_start_buffers (GOmxPort * port);

static OMX_CALLBACKTYPE callbacks =
    { EventHandler, EmptyBufferDone, FillBufferDone };

/* protect implementations hash_table */
static GMutex *imp_mutex;
static GHashTable *implementations;
static gboolean initialized;

/*
 * Util
 */

static void
g_ptr_array_clear (GPtrArray * array)
{
  guint index;
  for (index = 0; index < array->len; index++)
    array->pdata[index] = NULL;
}

static void
g_ptr_array_insert (GPtrArray * array, guint index, gpointer data)
{
  if (index + 1 > array->len) {
    g_ptr_array_set_size (array, index + 1);
  }

  array->pdata[index] = data;
}

typedef void (*GOmxPortFunc) (GOmxPort * port);

static inline void
core_for_each_port (GOmxCore * core, GOmxPortFunc func)
{
  guint index;

  for (index = 0; index < core->ports->len; index++) {
    GOmxPort *port;

    port = get_port (core, index);

    if (port)
      func (port);
  }
}

/*
 * Main
 */

static GOmxImp *imp_new (const gchar * name);
static void imp_free (GOmxImp * imp);

static GOmxImp *
imp_new (const gchar * name)
{
  GOmxImp *imp;

  imp = g_new0 (GOmxImp, 1);

  /* Load the OpenMAX IL symbols */
  {
    void *handle;

    GST_DEBUG ("loading: %s", name);

    imp->dl_handle = handle = dlopen (name, RTLD_LAZY);

    GST_DEBUG ("dlopen(%s) -> %p", name, handle);

    if (!handle) {
      g_warning ("%s\n", dlerror ());
      g_free (imp);
      return NULL;
    }

    imp->mutex = g_mutex_new ();
    imp->sym_table.init = dlsym (handle, "OMX_Init");
    imp->sym_table.deinit = dlsym (handle, "OMX_Deinit");
    imp->sym_table.get_handle = dlsym (handle, "OMX_GetHandle");
    imp->sym_table.free_handle = dlsym (handle, "OMX_FreeHandle");
  }

  return imp;
}

static void
imp_free (GOmxImp * imp)
{
  if (imp->dl_handle) {
    dlclose (imp->dl_handle);
  }
  g_mutex_free (imp->mutex);
  g_free (imp);
}

static inline GOmxImp *
request_imp (const gchar * name)
{
  GOmxImp *imp = NULL;

  g_mutex_lock (imp_mutex);
  imp = g_hash_table_lookup (implementations, name);
  if (!imp) {
    imp = imp_new (name);
    if (imp)
      g_hash_table_insert (implementations, g_strdup (name), imp);
  }
  g_mutex_unlock (imp_mutex);

  if (!imp)
    return NULL;

  g_mutex_lock (imp->mutex);
  if (imp->client_count == 0) {
    OMX_ERRORTYPE omx_error;
    omx_error = imp->sym_table.init ();
    if (omx_error) {
      g_mutex_unlock (imp->mutex);
      return NULL;
    }
  }
  imp->client_count++;
  g_mutex_unlock (imp->mutex);

  return imp;
}

static inline void
release_imp (GOmxImp * imp)
{
  g_mutex_lock (imp->mutex);
  imp->client_count--;
  if (imp->client_count == 0) {
    imp->sym_table.deinit ();
  }
  g_mutex_unlock (imp->mutex);
}

void
g_omx_init (void)
{
  if (!initialized) {
    /* safe as plugin_init is safe */
    imp_mutex = g_mutex_new ();
    implementations = g_hash_table_new_full (g_str_hash,
        g_str_equal, g_free, (GDestroyNotify) imp_free);
    initialized = TRUE;
  }
}

void
g_omx_deinit (void)
{
  if (initialized) {
    g_hash_table_destroy (implementations);
    g_mutex_free (imp_mutex);
    initialized = FALSE;
  }
}

/*
 * Core
 */

GOmxCore *
g_omx_core_new (void *object)
{
  GOmxCore *core;

  core = g_new0 (GOmxCore, 1);

  core->object = object;
  core->ports = g_ptr_array_new ();

  core->omx_state_condition = g_cond_new ();
  core->omx_state_mutex = g_mutex_new ();

  core->done_sem = g_sem_new ();
  core->flush_sem = g_sem_new ();
  core->port_sem = g_sem_new ();

  core->omx_state = OMX_StateInvalid;

  return core;
}

void
g_omx_core_free (GOmxCore * core)
{
  core_deinit (core);

  g_sem_free (core->port_sem);
  g_sem_free (core->flush_sem);
  g_sem_free (core->done_sem);

  g_mutex_free (core->omx_state_mutex);
  g_cond_free (core->omx_state_condition);

  g_ptr_array_free (core->ports, TRUE);

  g_free (core);
}

void
g_omx_core_init (GOmxCore * core)
{
  GST_DEBUG_OBJECT (core->object, "loading: %s %s (%s)",
      core->component_name,
      core->component_role ? core->component_role : "", core->library_name);

  core->imp = request_imp (core->library_name);

  if (!core->imp)
    return;

  core->omx_error = core->imp->sym_table.get_handle (&core->omx_handle,
      (char *) core->component_name, core, &callbacks);

  GST_DEBUG_OBJECT (core->object, "OMX_GetHandle(&%p) -> %d",
      core->omx_handle, core->omx_error);

  if (!core->omx_error) {
    core->omx_state = OMX_StateLoaded;

    if (core->component_role) {
      OMX_PARAM_COMPONENTROLETYPE param;

      GST_DEBUG_OBJECT (core->object, "setting component role: %s",
          core->component_role);

      G_OMX_INIT_PARAM (param);

      strncpy ((char *) param.cRole, core->component_role,
          OMX_MAX_STRINGNAME_SIZE);

      OMX_SetParameter (core->omx_handle, OMX_IndexParamStandardComponentRole,
          &param);
    }
  }
}

static void
core_deinit (GOmxCore * core)
{
  if (!core->imp)
    return;

  if (core->omx_state == OMX_StateLoaded || core->omx_state == OMX_StateInvalid) {
    if (core->omx_handle) {
      core->omx_error = core->imp->sym_table.free_handle (core->omx_handle);
      GST_DEBUG_OBJECT (core->object, "OMX_FreeHandle(%p) -> %d",
          core->omx_handle, core->omx_error);
    }
  } else {
    GST_WARNING_OBJECT (core->object, "Incorrect state: %s",
        omx_state_to_str (core->omx_state));
  }

  g_free (core->library_name);
  g_free (core->component_name);
  g_free (core->component_role);

  release_imp (core->imp);
  core->imp = NULL;
}

void
g_omx_core_prepare (GOmxCore * core)
{
  change_state (core, OMX_StateIdle);

  /* Allocate buffers. */
  core_for_each_port (core, port_allocate_buffers);

  wait_for_state (core, OMX_StateIdle);
}

void
g_omx_core_start (GOmxCore * core)
{
  change_state (core, OMX_StateExecuting);
  wait_for_state (core, OMX_StateExecuting);

  if (core->omx_state == OMX_StateExecuting)
    core_for_each_port (core, port_start_buffers);
}

void
g_omx_core_stop (GOmxCore * core)
{
  if (core->omx_state == OMX_StateExecuting ||
      core->omx_state == OMX_StatePause) {
    change_state (core, OMX_StateIdle);
    wait_for_state (core, OMX_StateIdle);
  }
}

void
g_omx_core_pause (GOmxCore * core)
{
  change_state (core, OMX_StatePause);
  wait_for_state (core, OMX_StatePause);
}

void
g_omx_core_unload (GOmxCore * core)
{
  if (core->omx_state == OMX_StateIdle ||
      core->omx_state == OMX_StateWaitForResources ||
      core->omx_state == OMX_StateInvalid) {
    if (core->omx_state != OMX_StateInvalid)
      change_state (core, OMX_StateLoaded);

    core_for_each_port (core, port_free_buffers);

    if (core->omx_state != OMX_StateInvalid)
      wait_for_state (core, OMX_StateLoaded);
  }

  core_for_each_port (core, g_omx_port_free);
  g_ptr_array_clear (core->ports);
}

static inline GOmxPort *
get_port (GOmxCore * core, guint index)
{
  if (G_LIKELY (index < core->ports->len)) {
    return g_ptr_array_index (core->ports, index);
  }

  return NULL;
}

GOmxPort *
g_omx_core_new_port (GOmxCore * core, guint index)
{
  GOmxPort *port = get_port (core, index);

  if (port) {
    GST_WARNING_OBJECT (core->object, "port %d already exists", index);
    return port;
  }

  port = g_omx_port_new (core, index);
  g_ptr_array_insert (core->ports, index, port);

  return port;
}

void
g_omx_core_set_done (GOmxCore * core)
{
  g_sem_up (core->done_sem);
}

void
g_omx_core_wait_for_done (GOmxCore * core)
{
  g_sem_down (core->done_sem);
}

void
g_omx_core_flush_start (GOmxCore * core)
{
  core_for_each_port (core, g_omx_port_pause);
}

void
g_omx_core_flush_stop (GOmxCore * core)
{
  core_for_each_port (core, g_omx_port_flush);
  core_for_each_port (core, g_omx_port_resume);
}

/*
 * Port
 */

/**
 * note: this is not intended to be called directly by elements (which should
 * instead use g_omx_core_new_port())
 */
GOmxPort *
g_omx_port_new (GOmxCore * core, guint index)
{
  GOmxPort *port;
  port = g_new0 (GOmxPort, 1);

  port->core = core;
  port->port_index = index;
  port->num_buffers = 0;
  port->buffer_size = 0;
  port->buffers = NULL;

  port->enabled = TRUE;
  port->queue = async_queue_new ();
  port->mutex = g_mutex_new ();

  return port;
}

void
g_omx_port_free (GOmxPort * port)
{
  g_mutex_free (port->mutex);
  async_queue_free (port->queue);

  g_free (port->buffers);
  g_free (port);
}

void
g_omx_port_setup (GOmxPort * port)
{
  GOmxPortType type = -1;
  OMX_PARAM_PORTDEFINITIONTYPE param;

  G_OMX_INIT_PARAM (param);

  param.nPortIndex = port->port_index;
  OMX_GetParameter (port->core->omx_handle, OMX_IndexParamPortDefinition,
      &param);

  switch (param.eDir) {
    case OMX_DirInput:
      type = GOMX_PORT_INPUT;
      break;
    case OMX_DirOutput:
      type = GOMX_PORT_OUTPUT;
      break;
    default:
      break;
  }

  port->type = type;
    /** @todo should it be nBufferCountMin? */
  port->num_buffers = param.nBufferCountActual;
  port->buffer_size = param.nBufferSize;

  GST_DEBUG_OBJECT (port->core->object,
      "type=%d, num_buffers=%d, buffer_size=%ld, port_index=%d",
      port->type, port->num_buffers, port->buffer_size, port->port_index);

  g_free (port->buffers);
  port->buffers = g_new0 (OMX_BUFFERHEADERTYPE *, port->num_buffers);
}

static void
port_allocate_buffers (GOmxPort * port)
{
  guint i;
  gsize size;

  size = port->buffer_size;

  for (i = 0; i < port->num_buffers; i++) {
    if (port->omx_allocate) {
      GST_DEBUG_OBJECT (port->core->object,
          "%d: OMX_AllocateBuffer(), size=%" G_GSIZE_FORMAT, i, size);
      OMX_AllocateBuffer (port->core->omx_handle, &port->buffers[i],
          port->port_index, NULL, size);
    } else {
      gpointer buffer_data;
      buffer_data = g_malloc (size);
      GST_DEBUG_OBJECT (port->core->object,
          "%d: OMX_UseBuffer(), size=%" G_GSIZE_FORMAT, i, size);
      OMX_UseBuffer (port->core->omx_handle, &port->buffers[i],
          port->port_index, NULL, size, buffer_data);
    }
  }
}

static void
port_free_buffers (GOmxPort * port)
{
  guint i;

  for (i = 0; i < port->num_buffers; i++) {
    OMX_BUFFERHEADERTYPE *omx_buffer;

    omx_buffer = port->buffers[i];

    if (omx_buffer) {
#if 0
            /** @todo how shall we free that buffer? */
      if (!port->omx_allocate) {
        g_free (omx_buffer->pBuffer);
        omx_buffer->pBuffer = NULL;
      }
#endif

      OMX_FreeBuffer (port->core->omx_handle, port->port_index, omx_buffer);
      port->buffers[i] = NULL;
    }
  }
}

static void
port_start_buffers (GOmxPort * port)
{
  guint i;

  for (i = 0; i < port->num_buffers; i++) {
    OMX_BUFFERHEADERTYPE *omx_buffer;

    omx_buffer = port->buffers[i];

    /* If it's an input port we will need to fill the buffer, so put it in
     * the queue, otherwise send to omx for processing (fill it up). */
    if (port->type == GOMX_PORT_INPUT)
      got_buffer (port->core, port, omx_buffer);
    else
      g_omx_port_release_buffer (port, omx_buffer);
  }
}

void
g_omx_port_push_buffer (GOmxPort * port, OMX_BUFFERHEADERTYPE * omx_buffer)
{
  async_queue_push (port->queue, omx_buffer);
}

OMX_BUFFERHEADERTYPE *
g_omx_port_request_buffer (GOmxPort * port)
{
  return async_queue_pop (port->queue);
}

void
g_omx_port_release_buffer (GOmxPort * port, OMX_BUFFERHEADERTYPE * omx_buffer)
{
  switch (port->type) {
    case GOMX_PORT_INPUT:
      OMX_EmptyThisBuffer (port->core->omx_handle, omx_buffer);
      break;
    case GOMX_PORT_OUTPUT:
      OMX_FillThisBuffer (port->core->omx_handle, omx_buffer);
      break;
    default:
      break;
  }
}

void
g_omx_port_resume (GOmxPort * port)
{
  async_queue_enable (port->queue);
}

void
g_omx_port_pause (GOmxPort * port)
{
  async_queue_disable (port->queue);
}

void
g_omx_port_flush (GOmxPort * port)
{
  if (port->type == GOMX_PORT_OUTPUT) {
    OMX_BUFFERHEADERTYPE *omx_buffer;
    while ((omx_buffer = async_queue_pop_forced (port->queue))) {
      omx_buffer->nFilledLen = 0;
      g_omx_port_release_buffer (port, omx_buffer);
    }
  } else {
    OMX_SendCommand (port->core->omx_handle, OMX_CommandFlush, port->port_index,
        NULL);
    g_sem_down (port->core->flush_sem);
  }
}

void
g_omx_port_enable (GOmxPort * port)
{
  GOmxCore *core;

  core = port->core;

  OMX_SendCommand (core->omx_handle, OMX_CommandPortEnable, port->port_index,
      NULL);
  port_allocate_buffers (port);
  if (core->omx_state != OMX_StateLoaded)
    port_start_buffers (port);
  g_omx_port_resume (port);

  g_sem_down (core->port_sem);
}

void
g_omx_port_disable (GOmxPort * port)
{
  GOmxCore *core;

  core = port->core;

  OMX_SendCommand (core->omx_handle, OMX_CommandPortDisable, port->port_index,
      NULL);
  g_omx_port_pause (port);
  g_omx_port_flush (port);
  port_free_buffers (port);

  g_sem_down (core->port_sem);
}

void
g_omx_port_finish (GOmxPort * port)
{
  port->enabled = FALSE;
  async_queue_disable (port->queue);
}

/*
 * Helper functions.
 */

static inline void
change_state (GOmxCore * core, OMX_STATETYPE state)
{
  GST_DEBUG_OBJECT (core->object, "state=%d", state);
  OMX_SendCommand (core->omx_handle, OMX_CommandStateSet, state, NULL);
}

static inline void
complete_change_state (GOmxCore * core, OMX_STATETYPE state)
{
  g_mutex_lock (core->omx_state_mutex);

  core->omx_state = state;
  g_cond_signal (core->omx_state_condition);
  GST_DEBUG_OBJECT (core->object, "state=%d", state);

  g_mutex_unlock (core->omx_state_mutex);
}

static inline void
wait_for_state (GOmxCore * core, OMX_STATETYPE state)
{
  GTimeVal tv;
  gboolean signaled;

  g_mutex_lock (core->omx_state_mutex);

  if (core->omx_error != OMX_ErrorNone)
    goto leave;

  g_get_current_time (&tv);
  g_time_val_add (&tv, 15 * G_USEC_PER_SEC);

  /* try once */
  if (core->omx_state != state) {
    signaled =
        g_cond_timed_wait (core->omx_state_condition, core->omx_state_mutex,
        &tv);

    if (!signaled) {
      GST_ERROR_OBJECT (core->object, "timed out switching from '%s' to '%s'",
          omx_state_to_str (core->omx_state), omx_state_to_str (state));
    }
  }

  if (core->omx_error != OMX_ErrorNone)
    goto leave;

  if (core->omx_state != state) {
    GST_ERROR_OBJECT (core->object,
        "wrong state received: state=%d, expected=%d", core->omx_state, state);
  }

leave:
  g_mutex_unlock (core->omx_state_mutex);
}

/*
 * Callbacks
 */

static inline void
in_port_cb (GOmxPort * port, OMX_BUFFERHEADERTYPE * omx_buffer)
{
    /** @todo remove this */

  if (!port->enabled)
    return;
}

static inline void
out_port_cb (GOmxPort * port, OMX_BUFFERHEADERTYPE * omx_buffer)
{
    /** @todo remove this */

  if (!port->enabled)
    return;

#if 0
  if (omx_buffer->nFlags & OMX_BUFFERFLAG_EOS) {
    g_omx_port_set_done (port);
    return;
  }
#endif
}

static inline void
got_buffer (GOmxCore * core, GOmxPort * port, OMX_BUFFERHEADERTYPE * omx_buffer)
{
  if (G_UNLIKELY (!omx_buffer)) {
    return;
  }

  if (G_LIKELY (port)) {
    g_omx_port_push_buffer (port, omx_buffer);

    switch (port->type) {
      case GOMX_PORT_INPUT:
        in_port_cb (port, omx_buffer);
        break;
      case GOMX_PORT_OUTPUT:
        out_port_cb (port, omx_buffer);
        break;
      default:
        break;
    }
  }
}

/*
 * OpenMAX IL callbacks.
 */

static OMX_ERRORTYPE
EventHandler (OMX_HANDLETYPE omx_handle,
    OMX_PTR app_data,
    OMX_EVENTTYPE event, OMX_U32 data_1, OMX_U32 data_2, OMX_PTR event_data)
{
  GOmxCore *core;

  core = (GOmxCore *) app_data;

  switch (event) {
    case OMX_EventCmdComplete:
    {
      OMX_COMMANDTYPE cmd;

      cmd = (OMX_COMMANDTYPE) data_1;

      GST_DEBUG_OBJECT (core->object, "OMX_EventCmdComplete: %d", cmd);

      switch (cmd) {
        case OMX_CommandStateSet:
          complete_change_state (core, data_2);
          break;
        case OMX_CommandFlush:
          g_sem_up (core->flush_sem);
          break;
        case OMX_CommandPortDisable:
        case OMX_CommandPortEnable:
          g_sem_up (core->port_sem);
        default:
          break;
      }
      break;
    }
    case OMX_EventBufferFlag:
    {
      GST_DEBUG_OBJECT (core->object, "OMX_EventBufferFlag");
      if (data_2 & OMX_BUFFERFLAG_EOS) {
        g_omx_core_set_done (core);
      }
      break;
    }
    case OMX_EventPortSettingsChanged:
    {
      GST_DEBUG_OBJECT (core->object, "OMX_EventPortSettingsChanged");
                /** @todo only on the relevant port. */
      if (core->settings_changed_cb) {
        core->settings_changed_cb (core);
      }
      break;
    }
    case OMX_EventError:
    {
      core->omx_error = data_1;
      GST_ERROR_OBJECT (core->object, "unrecoverable error: %s (0x%lx)",
          omx_error_to_str (data_1), data_1);
      /* component might leave us waiting for buffers, unblock */
      g_omx_core_flush_start (core);
      /* unlock wait_for_state */
      g_mutex_lock (core->omx_state_mutex);
      g_cond_signal (core->omx_state_condition);
      g_mutex_unlock (core->omx_state_mutex);
      break;
    }
    default:
      break;
  }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
EmptyBufferDone (OMX_HANDLETYPE omx_handle,
    OMX_PTR app_data, OMX_BUFFERHEADERTYPE * omx_buffer)
{
  GOmxCore *core;
  GOmxPort *port;

  core = (GOmxCore *) app_data;
  port = get_port (core, omx_buffer->nInputPortIndex);

  GST_CAT_LOG_OBJECT (gstomx_util_debug, core->object, "omx_buffer=%p",
      omx_buffer);
  got_buffer (core, port, omx_buffer);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
FillBufferDone (OMX_HANDLETYPE omx_handle,
    OMX_PTR app_data, OMX_BUFFERHEADERTYPE * omx_buffer)
{
  GOmxCore *core;
  GOmxPort *port;

  core = (GOmxCore *) app_data;
  port = get_port (core, omx_buffer->nOutputPortIndex);

  GST_CAT_LOG_OBJECT (gstomx_util_debug, core->object, "omx_buffer=%p",
      omx_buffer);
  got_buffer (core, port, omx_buffer);

  return OMX_ErrorNone;
}

static inline const char *
omx_state_to_str (OMX_STATETYPE omx_state)
{
  switch (omx_state) {
    case OMX_StateInvalid:
      return "invalid";
    case OMX_StateLoaded:
      return "loaded";
    case OMX_StateIdle:
      return "idle";
    case OMX_StateExecuting:
      return "executing";
    case OMX_StatePause:
      return "pause";
    case OMX_StateWaitForResources:
      return "wait for resources";
    default:
      return "unknown";
  }
}

static inline const char *
omx_error_to_str (OMX_ERRORTYPE omx_error)
{
  switch (omx_error) {
    case OMX_ErrorNone:
      return "None";

    case OMX_ErrorInsufficientResources:
      return
          "There were insufficient resources to perform the requested operation";

    case OMX_ErrorUndefined:
      return "The cause of the error could not be determined";

    case OMX_ErrorInvalidComponentName:
      return "The component name string was not valid";

    case OMX_ErrorComponentNotFound:
      return "No component with the specified name string was found";

    case OMX_ErrorInvalidComponent:
      return "The component specified did not have an entry point";

    case OMX_ErrorBadParameter:
      return "One or more parameters were not valid";

    case OMX_ErrorNotImplemented:
      return "The requested function is not implemented";

    case OMX_ErrorUnderflow:
      return "The buffer was emptied before the next buffer was ready";

    case OMX_ErrorOverflow:
      return "The buffer was not available when it was needed";

    case OMX_ErrorHardware:
      return "The hardware failed to respond as expected";

    case OMX_ErrorInvalidState:
      return "The component is in invalid state";

    case OMX_ErrorStreamCorrupt:
      return "Stream is found to be corrupt";

    case OMX_ErrorPortsNotCompatible:
      return "Ports being connected are not compatible";

    case OMX_ErrorResourcesLost:
      return "Resources allocated to an idle component have been lost";

    case OMX_ErrorNoMore:
      return "No more indices can be enumerated";

    case OMX_ErrorVersionMismatch:
      return "The component detected a version mismatch";

    case OMX_ErrorNotReady:
      return "The component is not ready to return data at this time";

    case OMX_ErrorTimeout:
      return "There was a timeout that occurred";

    case OMX_ErrorSameState:
      return
          "This error occurs when trying to transition into the state you are already in";

    case OMX_ErrorResourcesPreempted:
      return
          "Resources allocated to an executing or paused component have been preempted";

    case OMX_ErrorPortUnresponsiveDuringAllocation:
      return
          "Waited an unusually long time for the supplier to allocate buffers";

    case OMX_ErrorPortUnresponsiveDuringDeallocation:
      return
          "Waited an unusually long time for the supplier to de-allocate buffers";

    case OMX_ErrorPortUnresponsiveDuringStop:
      return
          "Waited an unusually long time for the non-supplier to return a buffer during stop";

    case OMX_ErrorIncorrectStateTransition:
      return "Attempting a state transition that is not allowed";

    case OMX_ErrorIncorrectStateOperation:
      return
          "Attempting a command that is not allowed during the present state";

    case OMX_ErrorUnsupportedSetting:
      return
          "The values encapsulated in the parameter or config structure are not supported";

    case OMX_ErrorUnsupportedIndex:
      return
          "The parameter or config indicated by the given index is not supported";

    case OMX_ErrorBadPortIndex:
      return "The port index supplied is incorrect";

    case OMX_ErrorPortUnpopulated:
      return
          "The port has lost one or more of its buffers and it thus unpopulated";

    case OMX_ErrorComponentSuspended:
      return "Component suspended due to temporary loss of resources";

    case OMX_ErrorDynamicResourcesUnavailable:
      return
          "Component suspended due to an inability to acquire dynamic resources";

    case OMX_ErrorMbErrorsInFrame:
      return "Frame generated macroblock error";

    case OMX_ErrorFormatNotDetected:
      return "Cannot parse or determine the format of an input stream";

    case OMX_ErrorContentPipeOpenFailed:
      return "The content open operation failed";

    case OMX_ErrorContentPipeCreationFailed:
      return "The content creation operation failed";

    case OMX_ErrorSeperateTablesUsed:
      return "Separate table information is being used";

    case OMX_ErrorTunnelingUnsupported:
      return "Tunneling is unsupported by the component";

    default:
      return "Unknown error";
  }
}
