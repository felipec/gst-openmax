/*
 * Copyright (C) 2006-2007 Texas Instruments, Incorporated
 * Copyright (C) 2007 Nokia Corporation. All rights reserved.
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

static void
out_port_done_cb (GOmxPort *port);

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

static OMX_CALLBACKTYPE callbacks = { EventHandler, EmptyBufferDone, FillBufferDone };

/*
 * Core
 */

GOmxCore *
g_omx_core_new (void)
{
    GOmxCore *core;

    core = g_new0 (GOmxCore, 1);

    core->ports[0] = g_omx_port_new (core);
    core->ports[0]->callback = in_port_cb;
    core->ports[0]->type = GOMX_PORT_INPUT;

    core->ports[1] = g_omx_port_new (core);
    core->ports[1]->callback = out_port_cb;
    core->ports[1]->type = GOMX_PORT_OUTPUT;
    core->ports[1]->done_cb = out_port_done_cb;

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

    g_omx_port_free (core->ports[0]);
    g_omx_port_free (core->ports[1]);

    g_free (core);
}

/* We need this so we don't call OMX_Init twice. */
static int counter = 0;

void
g_omx_core_init (GOmxCore *core, const gchar *name)
{
    if (counter == 0)
    {
        core->omx_error = OMX_Init ();

        if (core->omx_error)
            return;
    }

    counter++;

    /** @todo: Why is it not a const? */
    core->omx_error = OMX_GetHandle (&core->omx_handle, (gchar *) name, core, &callbacks);
}

void
g_omx_core_deinit (GOmxCore *core)
{
    core->omx_error = OMX_FreeHandle (core->omx_handle);

    if (core->omx_error)
        return;

    counter--;

    if (counter == 0)
    {
        core->omx_error = OMX_Deinit ();
    }
}

void
g_omx_core_prepare (GOmxCore *core)
{
    change_state (core, OMX_StateIdle);

    /* Allocate buffers. */
    {
        gint index;
        gint i;

        for (index = 0; index < 2; index++)
        {
            if (core->ports[index])
            {
                for (i = 0; i < core->ports[index]->num_buffers; i++)
                {
                    gpointer buffer_data;
                    guint size;

                    size = core->ports[index]->buffer_size;
                    buffer_data = g_malloc (size);

                    OMX_UseBuffer (core->omx_handle,
                                   &core->ports[index]->buffers[i],
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

        for (index = 0; index < 2; index++)
        {
            for (i = 0; i < core->ports[index]->num_buffers; i++)
            {
                OMX_BUFFERHEADERTYPE *omx_buffer;

                omx_buffer = core->ports[index]->buffers[i];

                got_buffer (core, core->ports[index], omx_buffer);
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

        for (index = 0; index < 2; index++)
        {
            for (i = 0; i < core->ports[index]->num_buffers; i++)
            {
                OMX_BUFFERHEADERTYPE *omx_buffer;

                omx_buffer = core->ports[index]->buffers[i];

                g_free (omx_buffer->pBuffer);

                OMX_FreeBuffer (core->omx_handle, index, omx_buffer);
            }
        }
    }

    wait_for_state (core, OMX_StateLoaded);
}

void
g_omx_core_setup_port (GOmxCore *core,
                       OMX_PARAM_PORTDEFINITIONTYPE *omx_port)
{
    GOmxPort *port;

    port = core->ports[omx_port->nPortIndex];

    g_omx_port_setup (port, omx_port->nBufferCountActual, omx_port->nBufferSize);
}

void
g_omx_core_set_port_cb (GOmxCore *core,
                        guint index,
                        GOmxBufferCb cb)
{
    core->ports[index]->user_cb = cb;
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
                  gint num_buffers,
                  gint buffer_size)
{
    port->num_buffers = num_buffers;
    port->buffer_size = buffer_size;
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

static void
out_port_done_cb (GOmxPort *port)
{
    g_omx_sem_up (port->core->done_sem);
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

    core = (GOmxCore*) app_data;

    got_buffer (core, core->ports[omx_buffer->nInputPortIndex], omx_buffer);

    return OMX_ErrorNone;
}

static OMX_ERRORTYPE
FillBufferDone (OMX_HANDLETYPE omx_handle,
                OMX_PTR app_data,
                OMX_BUFFERHEADERTYPE *omx_buffer)
{
    GOmxCore *core;

    core = (GOmxCore *) app_data;

    got_buffer (core, core->ports[omx_buffer->nOutputPortIndex], omx_buffer);

    return OMX_ErrorNone;
}
