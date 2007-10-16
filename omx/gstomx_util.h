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

#ifndef __GST_OMX_UTIL_H__
#define __GST_OMX_UTIL_H__

#include <stdbool.h>
#include <glib.h>
#include <OMX_Core.h>
#include <OMX_Component.h>

/* Typedefs. */

typedef struct GOmxCore GOmxCore;
typedef struct GOmxPort GOmxPort;
typedef struct GOmxSem GOmxSem;
typedef enum GOmxPortType GOmxPortType;

typedef void (*GOmxCb) (GOmxCore *core);
typedef void (*GOmxPortCb) (GOmxPort *port);
typedef void (*GOmxBufferCb) (GOmxPort *port, OMX_BUFFERHEADERTYPE *omx_buffer);

/* Enums. */

enum GOmxPortType
{
    GOMX_PORT_INPUT,
    GOMX_PORT_OUTPUT
};

/* Structures. */

struct GOmxCore
{
    OMX_HANDLETYPE omx_handle;
    OMX_STATETYPE omx_state;
    OMX_ERRORTYPE omx_error;

    GPtrArray *ports;

    gpointer client_data; /**< Placeholder for the client data. */

    GOmxSem *state_sem;
    GOmxSem *done_sem;

    gboolean eos_sent; /**< Determines if the EOS flag has been sent. */
    GOmxCb settings_changed_cb;
};

struct GOmxPort
{
    GOmxCore *core;
    GOmxPortType type;

    gint num_buffers;
    gint buffer_size;
    GOmxPortCb done_cb;
    GOmxBufferCb callback;
    GOmxBufferCb user_cb;
    OMX_BUFFERHEADERTYPE **buffers;

    gboolean enable_queue;
    GOmxSem *sem;
    GQueue *queue;
    GMutex *mutex;

    gboolean done;
};

struct GOmxSem
{
    GCond *condition;
    GMutex *mutex;
    gint counter;
};

/* Functions. */

GOmxCore *g_omx_core_new ();
void g_omx_core_free (GOmxCore *core);
void g_omx_core_init (GOmxCore *core, const gchar *name);
void g_omx_core_deinit (GOmxCore *core);
void g_omx_core_prepare (GOmxCore *core);
void g_omx_core_start (GOmxCore *core);
void g_omx_core_finish (GOmxCore *core);
void g_omx_core_wait_for_done (GOmxCore *core);
void g_omx_core_set_port_cb (GOmxCore *core, guint index, GOmxBufferCb cb);
GOmxPort *g_omx_core_get_port (GOmxCore *core, guint index);
GOmxPort *g_omx_core_setup_port (GOmxCore *core, OMX_PARAM_PORTDEFINITIONTYPE *omx_port);

GOmxPort *g_omx_port_new (GOmxCore *core);
void g_omx_port_free (GOmxPort *port);
void g_omx_port_setup (GOmxPort *port, OMX_PARAM_PORTDEFINITIONTYPE *omx_port);
void g_omx_port_push_buffer (GOmxPort *port, OMX_BUFFERHEADERTYPE *omx_buffer);
OMX_BUFFERHEADERTYPE *g_omx_port_request_buffer (GOmxPort *port);
void g_omx_port_release_buffer (GOmxPort *port, OMX_BUFFERHEADERTYPE *omx_buffer);
void g_omx_port_set_done (GOmxPort *port);

GOmxSem *g_omx_sem_new (void);
void g_omx_sem_free (GOmxSem *sem);
void g_omx_sem_down (GOmxSem *sem);
void g_omx_sem_up (GOmxSem *sem);

#endif /* __GST_OMX_UTIL_H__ */
