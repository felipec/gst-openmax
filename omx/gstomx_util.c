/*
 * Copyright (C) 2006-2007 Texas Instruments, Incorporated
 * Copyright (C) 2007-2008 Nokia Corporation.
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

/* #define USE_ALLOCATE_BUFFER */

/*
 * Forward declarations
 */

static inline void
change_state (GOmxCore *core,
              OMX_STATETYPE state);

static inline void
wait_for_state (GOmxCore *core,
                OMX_STATETYPE state);

static inline void
in_port_cb (GOmxPort *port,
            OMX_BUFFERHEADERTYPE *omx_buffer);

static inline void
out_port_cb (GOmxPort *port,
             OMX_BUFFERHEADERTYPE *omx_buffer);

static inline void
got_buffer (GOmxCore *core,
            GOmxPort *port,
            OMX_BUFFERHEADERTYPE *omx_buffer);

static OMX_ERRORTYPE
EventHandler (OMX_HANDLETYPE omx_handle,
              OMX_PTR app_data,
              OMX_EVENTTYPE eEvent,
              OMX_U32 nData1,
              OMX_U32 nData2,
              OMX_PTR pEventData);

static OMX_ERRORTYPE
EmptyBufferDone (OMX_HANDLETYPE omx_handle,
                 OMX_PTR app_data,
                 OMX_BUFFERHEADERTYPE *omx_buffer);

static OMX_ERRORTYPE
FillBufferDone (OMX_HANDLETYPE omx_handle,
                OMX_PTR app_data,
                OMX_BUFFERHEADERTYPE *omx_buffer);

static inline GOmxPort *
g_omx_core_get_port (GOmxCore *core,
                     guint index);

static inline void
port_free_buffers (GOmxPort *port);

static inline void
port_allocate_buffers (GOmxPort *port);

static inline void
port_start_buffers (GOmxPort *port);

static OMX_CALLBACKTYPE callbacks = { EventHandler, EmptyBufferDone, FillBufferDone };

static GHashTable *implementations;
static gboolean initialized;

static void
g_ptr_array_clear (GPtrArray *array)
{
    guint index;
    for (index = 0; index < array->len; index++)
        array->pdata[index] = NULL;
}

static void
g_ptr_array_insert (GPtrArray *array,
                    guint index,
                    gpointer data)
{
    if (index + 1 > array->len)
    {
        g_ptr_array_set_size (array, index + 1);
    }

    array->pdata[index] = data;
}

/*
 * Main
 */

static GOmxImp *imp_new (const gchar *name);
static void imp_free (GOmxImp *imp);

static GOmxImp *
imp_new (const gchar *name)
{
    GOmxImp *imp;

    imp = g_new0 (GOmxImp, 1);

    /* Load the OpenMAX IL symbols */
    {
        void *handle;

        imp->dl_handle = handle = dlopen (name, RTLD_LAZY);
        if (!handle)
        {
            g_warning ("%s\n", dlerror ());
            imp_free (imp);
            return NULL;
        }

        imp->sym_table.init = dlsym (handle, "OMX_Init");
        imp->sym_table.deinit = dlsym (handle, "OMX_Deinit");
        imp->sym_table.get_handle = dlsym (handle, "OMX_GetHandle");
        imp->sym_table.free_handle = dlsym (handle, "OMX_FreeHandle");
    }

    return imp;
}

static void
imp_free (GOmxImp *imp)
{
    if (imp->dl_handle)
    {
        dlclose (imp->dl_handle);
    }
    g_free (imp);
}

static inline GOmxImp *
request_imp (const gchar *name)
{
    GOmxImp *imp;
    imp = g_hash_table_lookup (implementations, name);
    if (!imp)
    {
        imp = imp_new (name);
        if (!imp)
            return NULL;
        g_hash_table_insert (implementations, g_strdup (name), imp);
    }
    if (imp->client_count == 0)
    {
        OMX_ERRORTYPE omx_error;
        omx_error = imp->sym_table.init ();
        if (omx_error)
            return NULL;
    }
    imp->client_count++;
    return imp;
}

static inline void
release_imp (GOmxImp *imp)
{
    imp->client_count--;
    if (imp->client_count == 0)
    {
        imp->sym_table.deinit ();
    }
}

void
g_omx_init (void)
{
    if (!initialized)
    {
        implementations = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify) imp_free);
        initialized = true;
    }
}

void
g_omx_deinit (void)
{
    if (initialized)
    {
        g_hash_table_destroy (implementations);
        initialized = false;
    }
}

/*
 * Core
 */

GOmxCore *
g_omx_core_new (void)
{
    GOmxCore *core;

    core = g_new0 (GOmxCore, 1);

    core->ports = g_ptr_array_new ();

    core->omx_state_condition = g_cond_new ();
    core->omx_state_mutex = g_mutex_new ();

    core->done_sem = g_omx_sem_new ();
    core->flush_sem = g_omx_sem_new ();
    core->port_sem = g_omx_sem_new ();

    core->omx_state = OMX_StateInvalid;

    return core;
}

void
g_omx_core_free (GOmxCore *core)
{
    g_omx_sem_free (core->port_sem);
    g_omx_sem_free (core->flush_sem);
    g_omx_sem_free (core->done_sem);

    g_mutex_free (core->omx_state_mutex);
    g_cond_free (core->omx_state_condition);

    g_ptr_array_free (core->ports, TRUE);

    g_free (core);
}

void
g_omx_core_init (GOmxCore *core,
                 const gchar *library_name,
                 const gchar *component_name)
{
    core->imp = request_imp (library_name);

    if (!core->imp)
    {
        core->omx_error = OMX_ErrorUndefined;
        return;
    }

    core->omx_error = core->imp->sym_table.get_handle (&core->omx_handle, (gchar *) component_name, core, &callbacks);
    core->omx_state = OMX_StateLoaded;
}

void
g_omx_core_deinit (GOmxCore *core)
{
    if (!core->imp)
        return;

    core->omx_error = core->imp->sym_table.free_handle (core->omx_handle);

    if (core->omx_error)
        return;

    release_imp (core->imp);
    core->imp = NULL;
}

typedef void (*GOmxPortFunc) (GOmxPort *port);

static void inline
core_for_each_port (GOmxCore *core,
                    GOmxPortFunc func)
{
    guint index;

    for (index = 0; index < core->ports->len; index++)
    {
        GOmxPort *port;

        port = g_omx_core_get_port (core, index);

        if (port)
            func (port);
    }
}

void
g_omx_core_prepare (GOmxCore *core)
{
    change_state (core, OMX_StateIdle);

    /* Allocate buffers. */
    core_for_each_port (core, port_allocate_buffers);

    wait_for_state (core, OMX_StateIdle);
}

void
g_omx_core_start (GOmxCore *core)
{
    change_state (core, OMX_StateExecuting);
    wait_for_state (core, OMX_StateExecuting);

    core_for_each_port (core, port_start_buffers);
}

void
g_omx_core_pause (GOmxCore *core)
{
    change_state (core, OMX_StatePause);
    wait_for_state (core, OMX_StatePause);
}

void
g_omx_core_finish (GOmxCore *core)
{
    change_state (core, OMX_StateIdle);
    wait_for_state (core, OMX_StateIdle);

    change_state (core, OMX_StateLoaded);
    core_for_each_port (core, port_free_buffers);
    wait_for_state (core, OMX_StateLoaded);

    core_for_each_port (core, g_omx_port_free);
    g_ptr_array_clear (core->ports);
}

GOmxPort *
g_omx_core_setup_port (GOmxCore *core,
                       OMX_PARAM_PORTDEFINITIONTYPE *omx_port)
{
    GOmxPort *port;
    guint index;

    index = omx_port->nPortIndex;
    port = g_omx_core_get_port (core, index);

    if (!port)
    {
        port = g_omx_port_new (core);
        g_ptr_array_insert (core->ports, index, port);
    }

    g_omx_port_setup (port, omx_port);

    return port;
}

static inline GOmxPort *
g_omx_core_get_port (GOmxCore *core,
                     guint index)
{
    if (G_LIKELY (index < core->ports->len))
    {
        return g_ptr_array_index (core->ports, index);
    }

    return NULL;
}

void
g_omx_core_set_done (GOmxCore *core)
{
    g_omx_sem_up (core->done_sem);
}

void
g_omx_core_wait_for_done (GOmxCore *core)
{
    g_omx_sem_down (core->done_sem);
}

void
g_omx_core_flush_start (GOmxCore *core)
{
    core_for_each_port (core, g_omx_port_pause);
}

void
g_omx_core_flush (GOmxCore *core)
{
    OMX_SendCommand (core->omx_handle, OMX_CommandFlush, OMX_ALL, NULL);
}

void
g_omx_core_flush_stop (GOmxCore *core)
{
    g_omx_sem_down (core->flush_sem);
    core_for_each_port (core, g_omx_port_flush);
    core_for_each_port (core, g_omx_port_resume);
    core_for_each_port (core, port_start_buffers);
}

/*
 * Port
 */

GOmxPort *
g_omx_port_new (GOmxCore *core)
{
    GOmxPort *port;
    port = g_new0 (GOmxPort, 1);

    port->core = core;
    port->num_buffers = 0;
    port->buffer_size = 0;
    port->buffers = NULL;

    port->enabled = TRUE;
    port->queue = async_queue_new ();
    port->mutex = g_mutex_new ();

    return port;
}

void
g_omx_port_free (GOmxPort *port)
{
    g_mutex_free (port->mutex);
    async_queue_free (port->queue);

    g_free (port->buffers);
    g_free (port);
}

void
g_omx_port_setup (GOmxPort *port,
                  OMX_PARAM_PORTDEFINITIONTYPE *omx_port)
{
    GOmxPortType type = -1;

    switch (omx_port->eDir)
    {
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
    port->num_buffers = omx_port->nBufferCountActual;
    port->buffer_size = omx_port->nBufferSize;
    port->port_index = omx_port->nPortIndex;

    g_free (port->buffers);
    port->buffers = g_new0 (OMX_BUFFERHEADERTYPE *, port->num_buffers);
}

static void
port_allocate_buffers (GOmxPort *port)
{
    guint i;

    for (i = 0; i < port->num_buffers; i++)
    {
        gpointer buffer_data;
        guint size;

        size = port->buffer_size;
        buffer_data = g_malloc (size);

#ifdef USE_ALLOCATE_BUFFER
        OMX_AllocateBuffer (port->core->omx_handle,
                            &port->buffers[i],
                            port->port_index,
                            NULL,
                            size);
#else
        OMX_UseBuffer (port->core->omx_handle,
                       &port->buffers[i],
                       port->port_index,
                       NULL,
                       size,
                       buffer_data);
#endif /* USE_ALLOCATE_BUFFER */
    }
}

static void
port_free_buffers (GOmxPort *port)
{
    guint i;

    for (i = 0; i < port->num_buffers; i++)
    {
        OMX_BUFFERHEADERTYPE *omx_buffer;

        omx_buffer = port->buffers[i];

        if (omx_buffer)
        {
#ifdef USE_ALLOCATE_BUFFER
            g_free (omx_buffer->pBuffer);
            omx_buffer->pBuffer = NULL;
#endif /* USE_ALLOCATE_BUFFER */

            OMX_FreeBuffer (port->core->omx_handle, port->port_index, omx_buffer);
            port->buffers[i] = NULL;
        }
    }
}

static void
port_start_buffers (GOmxPort *port)
{
    guint i;

    for (i = 0; i < port->num_buffers; i++)
    {
        OMX_BUFFERHEADERTYPE *omx_buffer;

        omx_buffer = port->buffers[i];

        got_buffer (port->core, port, omx_buffer);
    }
}

void
g_omx_port_push_buffer (GOmxPort *port,
                        OMX_BUFFERHEADERTYPE *omx_buffer)
{
    async_queue_push (port->queue, omx_buffer);
}

OMX_BUFFERHEADERTYPE *
g_omx_port_request_buffer (GOmxPort *port)
{
    return async_queue_pop (port->queue);
}

void
g_omx_port_release_buffer (GOmxPort *port,
                           OMX_BUFFERHEADERTYPE *omx_buffer)
{
    switch (port->type)
    {
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
g_omx_port_resume (GOmxPort *port)
{
    async_queue_enable (port->queue);
}

void
g_omx_port_pause (GOmxPort *port)
{
    async_queue_disable (port->queue);
}

void
g_omx_port_flush (GOmxPort *port)
{
    async_queue_flush (port->queue);
}

void
g_omx_port_enable (GOmxPort *port)
{
    GOmxCore *core;

    core = port->core;

    OMX_SendCommand (core->omx_handle, OMX_CommandPortEnable, port->port_index, NULL);
    port_allocate_buffers (port);
    if (core->omx_state != OMX_StateLoaded)
        port_start_buffers (port);
    g_omx_port_resume (port);

    g_omx_sem_down (core->port_sem);
}

void
g_omx_port_disable (GOmxPort *port)
{
    GOmxCore *core;

    core = port->core;

    OMX_SendCommand (core->omx_handle, OMX_CommandPortDisable, port->port_index, NULL);
    g_omx_port_pause (port);
    g_omx_port_flush (port);
    port_free_buffers (port);

    g_omx_sem_down (core->port_sem);
}

void
g_omx_port_finish (GOmxPort *port)
{
    port->enabled = FALSE;
    async_queue_disable (port->queue);
}

/*
 * Semaphore
 */

GOmxSem *
g_omx_sem_new (void)
{
    GOmxSem *sem;

    sem = g_new (GOmxSem, 1);
    sem->condition = g_cond_new ();
    sem->mutex = g_mutex_new ();
    sem->counter = 0;

    return sem;
}

void
g_omx_sem_free (GOmxSem *sem)
{
    g_cond_free (sem->condition);
    g_mutex_free (sem->mutex);
    g_free (sem);
}

void
g_omx_sem_down (GOmxSem *sem)
{
    g_mutex_lock (sem->mutex);

    while (sem->counter == 0)
    {
        g_cond_wait (sem->condition, sem->mutex);
    }

    sem->counter--;

    g_mutex_unlock (sem->mutex);
}

void
g_omx_sem_up (GOmxSem *sem)
{
    g_mutex_lock (sem->mutex);

    sem->counter++;
    g_cond_signal (sem->condition);

    g_mutex_unlock (sem->mutex);
}

/*
 * Helper functions.
 */

static inline void
change_state (GOmxCore *core,
              OMX_STATETYPE state)
{
    OMX_SendCommand (core->omx_handle, OMX_CommandStateSet, state, NULL);
}

static inline void
complete_change_state (GOmxCore *core,
                       OMX_STATETYPE state)
{
    g_mutex_lock (core->omx_state_mutex);

    core->omx_state = state;
    g_cond_signal (core->omx_state_condition);

    g_mutex_unlock (core->omx_state_mutex);
}

static inline void
wait_for_state (GOmxCore *core,
                OMX_STATETYPE state)
{
    g_mutex_lock (core->omx_state_mutex);

    while (core->omx_state != state)
    {
        g_cond_wait (core->omx_state_condition, core->omx_state_mutex);
    }

    g_mutex_unlock (core->omx_state_mutex);
}

/*
 * Callbacks
 */

static inline void
in_port_cb (GOmxPort *port,
            OMX_BUFFERHEADERTYPE *omx_buffer)
{
    /** @todo remove this */

    if (!port->enabled)
        return;
}

static inline void
out_port_cb (GOmxPort *port,
             OMX_BUFFERHEADERTYPE *omx_buffer)
{
    /** @todo remove this */

    if (!port->enabled)
        return;

#if 0
    if (omx_buffer->nFlags & OMX_BUFFERFLAG_EOS)
    {
        g_omx_port_set_done (port);
        return;
    }
#endif
}

static inline void
got_buffer (GOmxCore *core,
            GOmxPort *port,
            OMX_BUFFERHEADERTYPE *omx_buffer)
{
    if (G_UNLIKELY (!omx_buffer))
    {
        return;
    }

    if (G_LIKELY (port))
    {
        g_omx_port_push_buffer (port, omx_buffer);

        switch (port->type)
        {
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
              OMX_EVENTTYPE eEvent,
              OMX_U32 nData1,
              OMX_U32 nData2,
              OMX_PTR pEventData)
{
    GOmxCore *core;

    core = (GOmxCore *) app_data;

    switch (eEvent)
    {
        case OMX_EventCmdComplete:
            {
                OMX_COMMANDTYPE cmd;

                cmd = (OMX_COMMANDTYPE) nData1;

                switch (cmd)
                {
                    case OMX_CommandStateSet:
                        complete_change_state (core, nData2);
                        break;
                    case OMX_CommandFlush:
                        g_omx_sem_up (core->flush_sem);
                        break;
                    case OMX_CommandPortDisable:
                    case OMX_CommandPortEnable:
                        g_omx_sem_up (core->port_sem);
                    default:
                        break;
                }
                break;
            }
        case OMX_EventBufferFlag:
            {
                if (nData2 & OMX_BUFFERFLAG_EOS)
                {
                    g_omx_core_set_done (core);
                }
                break;
            }
        case OMX_EventPortSettingsChanged:
            {
                /** @todo only on the relevant port. */
                if (core->settings_changed_cb)
                {
                    core->settings_changed_cb (core);
                }
            }
        default:
            break;
    }

    return OMX_ErrorNone;
}

static OMX_ERRORTYPE
EmptyBufferDone (OMX_HANDLETYPE omx_handle,
                 OMX_PTR app_data,
                 OMX_BUFFERHEADERTYPE *omx_buffer)
{
    GOmxCore *core;
    GOmxPort *port;

    core = (GOmxCore*) app_data;
    port = g_omx_core_get_port (core, omx_buffer->nInputPortIndex);

    got_buffer (core, port, omx_buffer);

    return OMX_ErrorNone;
}

static OMX_ERRORTYPE
FillBufferDone (OMX_HANDLETYPE omx_handle,
                OMX_PTR app_data,
                OMX_BUFFERHEADERTYPE *omx_buffer)
{
    GOmxCore *core;
    GOmxPort *port;

    core = (GOmxCore *) app_data;
    port = g_omx_core_get_port (core, omx_buffer->nOutputPortIndex);

    got_buffer (core, port, omx_buffer);

    return OMX_ErrorNone;
}
