/*
 * Copyright (C) 2006-2007 Texas Instruments, Incorporated
 * Copyright (C) 2007-2008 Nokia Corporation. All rights reserved.
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

#include "gstomx_util.h"
#include <dlfcn.h>

/*
 * Forward declarations
 */

static void
change_state (GOmxCore *core,
              OMX_STATETYPE state);

static void
wait_for_state (GOmxCore *core,
                OMX_STATETYPE state);

static void
send_eos_buffer (GOmxCore *core,
                 OMX_BUFFERHEADERTYPE *omx_buffer);

static void
in_port_cb (GOmxPort *port,
            OMX_BUFFERHEADERTYPE *omx_buffer);

static void
out_port_cb (GOmxPort *port,
             OMX_BUFFERHEADERTYPE *omx_buffer);

static void
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

GOmxImp *
g_omx_imp_new (const gchar *name);

void
g_omx_imp_free (GOmxImp *imp);

static OMX_CALLBACKTYPE callbacks = { EventHandler, EmptyBufferDone, FillBufferDone };

static GHashTable *implementations;
static gboolean initialized;

/*
 * Main
 */

void
g_omx_init (void)
{
    if (!initialized)
    {
        implementations = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_omx_imp_free);
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

GOmxImp *
g_omx_request_imp (const gchar *name)
{
    GOmxImp *imp;
    imp = g_hash_table_lookup (implementations, name);
    if (!imp)
    {
        imp = g_omx_imp_new (name);
        if (!imp)
            return NULL;
        g_hash_table_insert (implementations, name, imp);
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

void
g_omx_release_imp (GOmxImp *imp)
{
    imp->client_count--;
    if (imp->client_count == 0)
    {
        imp->sym_table.deinit ();
    }
}

GOmxImp *
g_omx_imp_new (const gchar *name)
{
    GOmxImp *imp;

    imp = g_new0 (GOmxImp, 1);

    /* Load the OpenMAX IL symbols */
    {
        void *handle;
        char *error;

        imp->dl_handle = handle = dlopen (name, RTLD_LAZY);
        if (!handle)
        {
            g_warning ("%s\n", dlerror ());
            g_omx_imp_free (imp);
            return NULL;
        }

        imp->sym_table.init = dlsym (handle, "OMX_Init");
        imp->sym_table.deinit = dlsym (handle, "OMX_Deinit");
        imp->sym_table.get_handle = dlsym (handle, "OMX_GetHandle");
        imp->sym_table.free_handle = dlsym (handle, "OMX_FreeHandle");
    }

    return imp;
}

void
g_omx_imp_free (GOmxImp *imp)
{
    if (imp->dl_handle)
    {
        dlclose (imp->dl_handle);
    }
    g_free (imp);
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

    core->state_sem = g_omx_sem_new ();
    core->done_sem = g_omx_sem_new ();

    core->omx_state = OMX_StateInvalid;

    return core;
}

void
g_omx_core_free (GOmxCore *core)
{
    g_omx_sem_free (core->done_sem);
    g_omx_sem_free (core->state_sem);

    {
        gint index;
        for (index = 0; index < core->ports->len; index++)
        {
            GOmxPort *port;
            port = g_omx_core_get_port (core, index);
            g_omx_port_free (port);
        }
        g_ptr_array_free (core->ports, TRUE);
    }

    g_free (core);
}

void
g_omx_core_init (GOmxCore *core,
                 const gchar *library_name,
                 const gchar *component_name)
{
    core->imp = g_omx_request_imp (library_name);

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

    g_omx_release_imp (core->imp);
    core->imp = NULL;
}

void
g_omx_core_prepare (GOmxCore *core)
{
    change_state (core, OMX_StateIdle);

    /* Allocate buffers. */
    {
        gint index;
        gint i;

        for (index = 0; index < core->ports->len; index++)
        {
            GOmxPort *port;

            port = g_omx_core_get_port (core, index);

            if (port)
            {
                for (i = 0; i < port->num_buffers; i++)
                {
                    gpointer buffer_data;
                    guint size;

                    size = port->buffer_size;
                    buffer_data = g_malloc (size);

                    OMX_UseBuffer (core->omx_handle,
                                   &port->buffers[i],
                                   index,
                                   NULL,
                                   size,
                                   buffer_data);
                }
            }
        }
    }

    wait_for_state (core, OMX_StateIdle);
}

void
g_omx_core_start (GOmxCore *core)
{
    change_state (core, OMX_StateExecuting);

    wait_for_state (core, OMX_StateExecuting);

    {
        guint index;
        guint i;

        for (index = 0; index < core->ports->len; index++)
        {
            GOmxPort *port;

            port = g_omx_core_get_port (core, index);

            for (i = 0; i < port->num_buffers; i++)
            {
                OMX_BUFFERHEADERTYPE *omx_buffer;

                omx_buffer = port->buffers[i];

                got_buffer (core, port, omx_buffer);
            }
        }
    }
}

void
g_omx_core_finish (GOmxCore *core)
{
    change_state (core, OMX_StateIdle);

    wait_for_state (core, OMX_StateIdle);

    change_state (core, OMX_StateLoaded);

    {
        guint index;
        guint i;

        for (index = 0; index < core->ports->len; index++)
        {
            GOmxPort *port;

            port = g_omx_core_get_port (core, index);

            for (i = 0; i < port->num_buffers; i++)
            {
                OMX_BUFFERHEADERTYPE *omx_buffer;

                omx_buffer = port->buffers[i];

#if 0
                /** @todo GStreamer should notify us when a buffer is
                 * destroyed. */
                g_free (omx_buffer->pBuffer);
#endif

                OMX_FreeBuffer (core->omx_handle, index, omx_buffer);
            }
        }
    }

    wait_for_state (core, OMX_StateLoaded);
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
    }

    g_omx_port_setup (port, omx_port);

    g_ptr_array_insert (core->ports, index, port);

    return port;
}

GOmxPort *
g_omx_core_get_port (GOmxCore *core,
                     guint index)
{
    if (index < core->ports->len)
    {
        return g_ptr_array_index (core->ports, index);
    }

    return NULL;
}

void
g_omx_core_set_port_cb (GOmxCore *core,
                        guint index,
                        GOmxBufferCb cb)
{
    GOmxPort *port;
    port = g_omx_core_get_port (core, index);
    port->user_cb = cb;
}

void
g_omx_core_wait_for_done (GOmxCore *core)
{
    g_omx_sem_down (core->done_sem);
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

    port->mutex = g_mutex_new ();
    port->sem = g_omx_sem_new ();
    port->queue = g_queue_new ();

    return port;
}

void
g_omx_port_free (GOmxPort *port)
{
    g_mutex_free (port->mutex);
    g_queue_free (port->queue);

    g_omx_sem_free (port->sem);

    g_free (port->buffers);
    g_free (port);
}

void
g_omx_port_setup (GOmxPort *port,
                  OMX_PARAM_PORTDEFINITIONTYPE *omx_port)
{
    GOmxPortType type;
    GOmxBufferCb callback;

    switch (omx_port->eDir)
    {
        case OMX_DirInput:
            type = GOMX_PORT_INPUT;
            callback = in_port_cb;
            break;
        case OMX_DirOutput:
            type = GOMX_PORT_OUTPUT;
            callback = out_port_cb;
            break;
        default:
            break;
    }

    port->type = type;
    port->callback = callback;
    port->num_buffers = omx_port->nBufferCountActual;
    port->buffer_size = omx_port->nBufferSize;

    if (port->buffers)
    {
        /** @todo handle this case */
        g_print ("WARNING: unhandled setup\n");
    }
    port->buffers = g_new (OMX_BUFFERHEADERTYPE *, port->num_buffers);
}

void
g_omx_port_push_buffer (GOmxPort *port,
                        OMX_BUFFERHEADERTYPE *omx_buffer)
{
    g_mutex_lock (port->mutex);
    g_queue_push_tail (port->queue, omx_buffer);
    g_mutex_unlock (port->mutex);

    g_omx_sem_up (port->sem);
}

OMX_BUFFERHEADERTYPE *
g_omx_port_request_buffer (GOmxPort *port)
{
    OMX_BUFFERHEADERTYPE *omx_buffer = NULL;

    g_omx_sem_down (port->sem);

    g_mutex_lock (port->mutex);
    omx_buffer = g_queue_pop_head (port->queue);
    g_mutex_unlock (port->mutex);

    return omx_buffer;
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
g_omx_port_set_done (GOmxPort *port)
{
    port->done = true;

    g_omx_sem_up (port->sem);

    if (port->done_cb)
    {
        port->done_cb (port);
    }
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

static void
change_state (GOmxCore *core,
              OMX_STATETYPE state)
{
    OMX_SendCommand (core->omx_handle, OMX_CommandStateSet, state, NULL);
}

static void
wait_for_state (GOmxCore *core,
                OMX_STATETYPE state)
{
    g_omx_sem_down (core->state_sem);
}

static void
send_eos_buffer (GOmxCore *core,
                 OMX_BUFFERHEADERTYPE *omx_buffer)
{
    omx_buffer->nFlags = OMX_BUFFERFLAG_EOS;
    omx_buffer->nFilledLen = 0;

    core->eos_sent = true;

    OMX_EmptyThisBuffer (core->omx_handle, omx_buffer);
}

/*
 * Callbacks
 */

static void
in_port_cb (GOmxPort *port,
            OMX_BUFFERHEADERTYPE *omx_buffer)
{
    if (port->done)
    {
        if (!port->core->eos_sent)
        {
            send_eos_buffer (port->core, omx_buffer);
        }
        return;
    }

    if (port->user_cb)
    {
        port->user_cb (port, omx_buffer);
    }

    if (port->done)
    {
        omx_buffer->nFlags = OMX_BUFFERFLAG_EOS;
        port->core->eos_sent = true;
    }
}

static void
out_port_cb (GOmxPort *port,
             OMX_BUFFERHEADERTYPE *omx_buffer)
{
    if (port->done)
    {
        return;
    }

    if (port->user_cb)
    {
        port->user_cb (port, omx_buffer);
    }

#if 0
    if (omx_buffer->nFlags & OMX_BUFFERFLAG_EOS)
    {
        g_omx_port_set_done (port);
        return;
    }
#endif
}

static void
got_buffer (GOmxCore *core,
            GOmxPort *port,
            OMX_BUFFERHEADERTYPE *omx_buffer)
{
    if (!omx_buffer)
    {
        return;
    }

    if (port)
    {
        if (port->enable_queue)
        {
            g_omx_port_push_buffer (port, omx_buffer);
        }

        if (port->callback)
        {
            port->callback (port, omx_buffer);
        }

        if (!port->enable_queue)
        {
            g_omx_port_release_buffer (port, omx_buffer);
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
                        {
                            core->omx_state = (OMX_STATETYPE) nData2;

                            if (cmd == OMX_CommandStateSet)
                            {
                                g_omx_sem_up (core->state_sem);
                            }
                        }
                    default:
                        break;
                }
                break;
            }
        case OMX_EventBufferFlag:
            {
#if 0
                if (nData2 & OMX_BUFFERFLAG_EOS)
                {
                    g_omx_port_set_done (core->ports[1]);
                }
#endif
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
