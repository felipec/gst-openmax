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

#include <OMX_Core.h>
#include <OMX_Component.h>

#include <glib.h>

#include <stdlib.h>             /* For calloc, free */
#include <string.h>             /* For memcpy */

#include "async_queue.h"

static void *foo_thread (void *cb_data);

OMX_ERRORTYPE
OMX_Init (void)
{
  if (!g_thread_supported ()) {
    g_thread_init (NULL);
  }

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
OMX_Deinit (void)
{
  return OMX_ErrorNone;
}

typedef struct CompPrivate CompPrivate;
typedef struct CompPrivatePort CompPrivatePort;

struct CompPrivate
{
  OMX_STATETYPE state;
  OMX_CALLBACKTYPE *callbacks;
  OMX_PTR app_data;
  CompPrivatePort *ports;
  gboolean done;
  GMutex *flush_mutex;
};

struct CompPrivatePort
{
  OMX_PARAM_PORTDEFINITIONTYPE port_def;
  AsyncQueue *queue;
};

static OMX_ERRORTYPE
comp_GetState (OMX_HANDLETYPE handle, OMX_STATETYPE * state)
{
  OMX_COMPONENTTYPE *comp;
  CompPrivate *private;

  /* printf ("GetState\n"); */

  comp = handle;
  private = comp->pComponentPrivate;

  *state = private->state;

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
comp_GetParameter (OMX_HANDLETYPE handle, OMX_INDEXTYPE index, OMX_PTR param)
{
  OMX_COMPONENTTYPE *comp;
  CompPrivate *private;

  /* printf ("GetParameter\n"); */

  comp = handle;
  private = comp->pComponentPrivate;

  switch (index) {
    case OMX_IndexParamPortDefinition:
    {
      OMX_PARAM_PORTDEFINITIONTYPE *port_def;
      port_def = param;
      memcpy (port_def, &private->ports[port_def->nPortIndex].port_def,
          port_def->nSize);
      break;
    }
    default:
      break;
  }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
comp_SetParameter (OMX_HANDLETYPE handle, OMX_INDEXTYPE index, OMX_PTR param)
{
  OMX_COMPONENTTYPE *comp;
  CompPrivate *private;

  /* printf ("SetParameter\n"); */

  comp = handle;
  private = comp->pComponentPrivate;

  switch (index) {
    case OMX_IndexParamPortDefinition:
    {
      OMX_PARAM_PORTDEFINITIONTYPE *port_def;
      port_def = param;
      memcpy (&private->ports[port_def->nPortIndex].port_def, port_def,
          port_def->nSize);
      break;
    }
    default:
      break;
  }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
comp_SendCommand (OMX_HANDLETYPE handle,
    OMX_COMMANDTYPE command, OMX_U32 param_1, OMX_PTR data)
{
  OMX_COMPONENTTYPE *comp;
  CompPrivate *private;

  /* printf ("SendCommand\n"); */

  comp = handle;
  private = comp->pComponentPrivate;

  switch (command) {
    case OMX_CommandStateSet:
    {
      if (private->state == OMX_StateLoaded && param_1 == OMX_StateIdle) {
        g_thread_create (foo_thread, comp, TRUE, NULL);
      }
      private->state = param_1;
      private->callbacks->EventHandler (handle,
          private->app_data, OMX_EventCmdComplete,
          OMX_CommandStateSet, private->state, data);
    }
      break;
    case OMX_CommandFlush:
    {
      g_mutex_lock (private->flush_mutex);
      {
        OMX_BUFFERHEADERTYPE *buffer;

        while ((buffer = async_queue_pop_forced (private->ports[0].queue))) {
          private->callbacks->EmptyBufferDone (comp, private->app_data, buffer);
        }

        while ((buffer = async_queue_pop_forced (private->ports[1].queue))) {
          private->callbacks->FillBufferDone (comp, private->app_data, buffer);
        }
      }
      g_mutex_unlock (private->flush_mutex);

      private->callbacks->EventHandler (handle,
          private->app_data, OMX_EventCmdComplete,
          OMX_CommandFlush, param_1, data);
    }
      break;
    default:
      /* printf ("command: %d\n", command); */
      break;
  }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
comp_UseBuffer (OMX_HANDLETYPE handle,
    OMX_BUFFERHEADERTYPE ** buffer_header,
    OMX_U32 index, OMX_PTR data, OMX_U32 size, OMX_U8 * buffer)
{
  OMX_BUFFERHEADERTYPE *new;

  new = calloc (1, sizeof (OMX_BUFFERHEADERTYPE));
  new->nSize = sizeof (OMX_BUFFERHEADERTYPE);
  new->nVersion.nVersion = 1;
  new->pBuffer = buffer;
  new->nAllocLen = size;

  switch (index) {
    case 0:
      new->nInputPortIndex = 0;
      break;
    case 1:
      new->nOutputPortIndex = 1;
      break;
    default:
      break;
  }

  *buffer_header = new;

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
comp_FreeBuffer (OMX_HANDLETYPE handle,
    OMX_U32 index, OMX_BUFFERHEADERTYPE * buffer_header)
{
  free (buffer_header);

  return OMX_ErrorNone;
}

static gpointer
foo_thread (gpointer cb_data)
{
  OMX_COMPONENTTYPE *comp;
  CompPrivate *private;

  comp = cb_data;
  private = comp->pComponentPrivate;

  while (!private->done) {
    OMX_BUFFERHEADERTYPE *in_buffer;
    OMX_BUFFERHEADERTYPE *out_buffer;

    in_buffer = async_queue_pop (private->ports[0].queue);
    if (!in_buffer)
      continue;

    out_buffer = async_queue_pop (private->ports[1].queue);
    if (!out_buffer)
      continue;

    /* process buffers */
    {
      unsigned long size;
      size = MIN (in_buffer->nFilledLen, out_buffer->nAllocLen);
      memcpy (out_buffer->pBuffer, in_buffer->pBuffer, size);
      out_buffer->nFilledLen = size;
      in_buffer->nFilledLen -= size;
      out_buffer->nTimeStamp = in_buffer->nTimeStamp;
      out_buffer->nFlags = in_buffer->nFlags;
    }

    g_mutex_lock (private->flush_mutex);

    private->callbacks->FillBufferDone (comp, private->app_data, out_buffer);
    if (in_buffer->nFilledLen == 0) {
      private->callbacks->EmptyBufferDone (comp, private->app_data, in_buffer);
    }

    g_mutex_unlock (private->flush_mutex);
  }

  return NULL;
}

static OMX_ERRORTYPE
comp_EmptyThisBuffer (OMX_HANDLETYPE handle,
    OMX_BUFFERHEADERTYPE * buffer_header)
{
  OMX_COMPONENTTYPE *comp;
  CompPrivate *private;

  /* printf ("EmptyThisBuffer\n"); */

  comp = handle;
  private = comp->pComponentPrivate;

  async_queue_push (private->ports[0].queue, buffer_header);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
comp_FillThisBuffer (OMX_HANDLETYPE handle,
    OMX_BUFFERHEADERTYPE * buffer_header)
{
  OMX_COMPONENTTYPE *comp;
  CompPrivate *private;

  /* printf ("FillThisBuffer\n"); */

  comp = handle;
  private = comp->pComponentPrivate;

  async_queue_push (private->ports[1].queue, buffer_header);

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
OMX_GetHandle (OMX_HANDLETYPE * handle,
    OMX_STRING component_name, OMX_PTR data, OMX_CALLBACKTYPE * callbacks)
{
  OMX_COMPONENTTYPE *comp;

  comp = calloc (1, sizeof (OMX_COMPONENTTYPE));
  comp->nSize = sizeof (OMX_COMPONENTTYPE);
  comp->nVersion.nVersion = 1;

  comp->GetState = comp_GetState;
  comp->GetParameter = comp_GetParameter;
  comp->SetParameter = comp_SetParameter;
  comp->SendCommand = comp_SendCommand;
  comp->UseBuffer = comp_UseBuffer;
  comp->FreeBuffer = comp_FreeBuffer;
  comp->EmptyThisBuffer = comp_EmptyThisBuffer;
  comp->FillThisBuffer = comp_FillThisBuffer;

  {
    CompPrivate *private;

    private = calloc (1, sizeof (CompPrivate));
    private->state = OMX_StateLoaded;
    private->callbacks = callbacks;
    private->app_data = data;
    private->ports = calloc (2, sizeof (CompPrivatePort));
    private->flush_mutex = g_mutex_new ();

    private->ports[0].queue = async_queue_new ();
    private->ports[1].queue = async_queue_new ();

    {
      OMX_PARAM_PORTDEFINITIONTYPE *port_def;

      port_def = &private->ports[0].port_def;
      port_def->nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
      port_def->nVersion.nVersion = 1;
      port_def->nPortIndex = 0;
      port_def->eDir = OMX_DirInput;
      port_def->nBufferCountActual = 1;
      port_def->nBufferCountMin = 1;
      port_def->nBufferSize = 0x1000;
      port_def->eDomain = OMX_PortDomainAudio;

    }

    {
      OMX_PARAM_PORTDEFINITIONTYPE *port_def;

      port_def = &private->ports[1].port_def;
      port_def->nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
      port_def->nVersion.nVersion = 1;
      port_def->nPortIndex = 1;
      port_def->eDir = OMX_DirOutput;
      port_def->nBufferCountActual = 1;
      port_def->nBufferCountMin = 1;
      port_def->nBufferSize = 0x1000;
      port_def->eDomain = OMX_PortDomainAudio;
    }

    comp->pComponentPrivate = private;
  }

  *handle = comp;

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
OMX_FreeHandle (OMX_HANDLETYPE handle)
{
    /** @todo Free private structure? */
  return OMX_ErrorNone;
}
