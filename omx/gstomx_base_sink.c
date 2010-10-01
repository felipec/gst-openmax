/*
 * Copyright (C) 2007-2009 Nokia Corporation.
 * Copyright (C) 2008 NXP.
 *
 * Authors:
 * Felipe Contreras <felipe.contreras@nokia.com>
 * Frederik Vernelen <frederik.vernelen@tass.be>
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

#include "gstomx_base_sink.h"
#include "gstomx.h"
#include "gstomx_interface.h"

#include <string.h>             /* for memcpy */

enum
{
  ARG_NUM_INPUT_BUFFERS = GSTOMX_NUM_COMMON_PROP,
};

static gboolean share_input_buffer;

static inline gboolean omx_init (GstOmxBaseSink * self);

static void init_interfaces (GType type);
GSTOMX_BOILERPLATE_FULL (GstOmxBaseSink, gst_omx_base_sink, GstBaseSink,
    GST_TYPE_BASE_SINK, init_interfaces);

static void
setup_ports (GstOmxBaseSink * self)
{
  /* Input port configuration. */
  g_omx_port_setup (self->in_port);
  gst_pad_set_element_private (self->sinkpad, self->in_port);
}

static GstStateChangeReturn
change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
  GstOmxBaseSink *self;

  self = GST_OMX_BASE_SINK (element);

  GST_LOG_OBJECT (self, "begin");

  GST_INFO_OBJECT (self, "changing state %s - %s",
      gst_element_state_get_name (GST_STATE_TRANSITION_CURRENT (transition)),
      gst_element_state_get_name (GST_STATE_TRANSITION_NEXT (transition)));

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      if (!self->initialized) {
        if (!omx_init (self))
          return GST_PAD_LINK_REFUSED;

        self->initialized = TRUE;
      }

      g_omx_core_prepare (self->gomx);
      break;

    case GST_STATE_CHANGE_READY_TO_PAUSED:
      g_omx_core_start (self->gomx);
      break;

    case GST_STATE_CHANGE_PAUSED_TO_READY:
      g_omx_port_finish (self->in_port);
      break;

    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  if (ret == GST_STATE_CHANGE_FAILURE)
    goto leave;

  switch (transition) {
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      g_omx_port_pause (self->in_port);
      break;

    case GST_STATE_CHANGE_PAUSED_TO_READY:
      g_omx_core_stop (self->gomx);
      break;

    case GST_STATE_CHANGE_READY_TO_NULL:
      g_omx_core_unload (self->gomx);
      break;

    default:
      break;
  }

leave:
  GST_LOG_OBJECT (self, "end");

  return ret;
}

static void
finalize (GObject * obj)
{
  GstOmxBaseSink *self;

  self = GST_OMX_BASE_SINK (obj);

  g_omx_core_free (self->gomx);

  G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static GstFlowReturn
render (GstBaseSink * gst_base, GstBuffer * buf)
{
  GOmxCore *gomx;
  GOmxPort *in_port;
  GstOmxBaseSink *self;
  GstFlowReturn ret = GST_FLOW_OK;

  self = GST_OMX_BASE_SINK (gst_base);

  gomx = self->gomx;

  GST_LOG_OBJECT (self, "begin");
  GST_LOG_OBJECT (self, "gst_buffer: size=%u", GST_BUFFER_SIZE (buf));

  GST_LOG_OBJECT (self, "state: %d", gomx->omx_state);

  in_port = self->in_port;

  if (G_LIKELY (in_port->enabled)) {
    guint buffer_offset = 0;

    while (G_LIKELY (buffer_offset < GST_BUFFER_SIZE (buf))) {
      OMX_BUFFERHEADERTYPE *omx_buffer;

      GST_LOG_OBJECT (self, "request_buffer");
      omx_buffer = g_omx_port_request_buffer (in_port);

      if (G_LIKELY (omx_buffer)) {
        GST_DEBUG_OBJECT (self,
            "omx_buffer: size=%lu, len=%lu, flags=%lu, offset=%lu, timestamp=%lld",
            omx_buffer->nAllocLen, omx_buffer->nFilledLen, omx_buffer->nFlags,
            omx_buffer->nOffset, omx_buffer->nTimeStamp);

        if (omx_buffer->nOffset == 0 && share_input_buffer) {
          {
            GstBuffer *old_buf;
            old_buf = omx_buffer->pAppPrivate;

            if (old_buf) {
              gst_buffer_unref (old_buf);
            } else if (omx_buffer->pBuffer) {
              g_free (omx_buffer->pBuffer);
            }
          }

          /* We are going to use this. */
          gst_buffer_ref (buf);

          omx_buffer->pBuffer = GST_BUFFER_DATA (buf);
          omx_buffer->nAllocLen = GST_BUFFER_SIZE (buf);
          omx_buffer->nFilledLen = GST_BUFFER_SIZE (buf);
          omx_buffer->pAppPrivate = buf;
        } else {
          omx_buffer->nFilledLen = MIN (GST_BUFFER_SIZE (buf) - buffer_offset,
              omx_buffer->nAllocLen - omx_buffer->nOffset);
          memcpy (omx_buffer->pBuffer + omx_buffer->nOffset,
              GST_BUFFER_DATA (buf) + buffer_offset, omx_buffer->nFilledLen);
        }

        GST_LOG_OBJECT (self, "release_buffer");
        g_omx_port_release_buffer (in_port, omx_buffer);

        buffer_offset += omx_buffer->nFilledLen;
      } else {
        GST_WARNING_OBJECT (self, "null buffer");
        ret = GST_FLOW_UNEXPECTED;
        break;
      }
    }
  } else {
    GST_WARNING_OBJECT (self, "done");
    ret = GST_FLOW_UNEXPECTED;
  }

  GST_LOG_OBJECT (self, "end");

  return ret;
}

static gboolean
handle_event (GstBaseSink * gst_base, GstEvent * event)
{
  GstOmxBaseSink *self;
  GOmxCore *gomx;
  GOmxPort *in_port;

  self = GST_OMX_BASE_SINK (gst_base);
  gomx = self->gomx;
  in_port = self->in_port;

  GST_LOG_OBJECT (self, "begin");

  GST_DEBUG_OBJECT (self, "event: %s", GST_EVENT_TYPE_NAME (event));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_EOS:
      /* Close the inpurt port. */
      g_omx_core_set_done (gomx);
      break;

    case GST_EVENT_FLUSH_START:
      /* unlock loops */
      g_omx_port_pause (in_port);

      /* flush all buffers */
      OMX_SendCommand (gomx->omx_handle, OMX_CommandFlush, OMX_ALL, NULL);
      break;

    case GST_EVENT_FLUSH_STOP:
      g_sem_down (gomx->flush_sem);

      g_omx_port_resume (in_port);
      break;

    default:
      break;
  }

  GST_LOG_OBJECT (self, "end");

  return TRUE;
}

static void
set_property (GObject * obj,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  GstOmxBaseSink *self;

  self = GST_OMX_BASE_SINK (obj);

  switch (prop_id) {
    case ARG_NUM_INPUT_BUFFERS:
    {
      OMX_PARAM_PORTDEFINITIONTYPE param;
      OMX_HANDLETYPE omx_handle = self->gomx->omx_handle;
      OMX_U32 nBufferCountActual;

      if (G_UNLIKELY (!omx_handle)) {
        GST_WARNING_OBJECT (self, "no component");
        break;
      }

      nBufferCountActual = g_value_get_uint (value);

      G_OMX_INIT_PARAM (param);

      param.nPortIndex = self->in_port->port_index;
      OMX_GetParameter (omx_handle, OMX_IndexParamPortDefinition, &param);

      if (nBufferCountActual < param.nBufferCountMin) {
        GST_ERROR_OBJECT (self, "buffer count %lu is less than minimum %lu",
            nBufferCountActual, param.nBufferCountMin);
        return;
      }

      param.nBufferCountActual = nBufferCountActual;

      OMX_SetParameter (omx_handle, OMX_IndexParamPortDefinition, &param);
    }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
      break;
  }
}

static void
get_property (GObject * obj, guint prop_id, GValue * value, GParamSpec * pspec)
{
  GstOmxBaseSink *self;

  self = GST_OMX_BASE_SINK (obj);

  if (gstomx_get_property_helper (self->gomx, prop_id, value))
    return;

  switch (prop_id) {
    case ARG_NUM_INPUT_BUFFERS:
    {
      OMX_PARAM_PORTDEFINITIONTYPE param;
      OMX_HANDLETYPE omx_handle = self->gomx->omx_handle;

      if (G_UNLIKELY (!omx_handle)) {
        GST_WARNING_OBJECT (self, "no component");
        g_value_set_uint (value, 0);
        break;
      }

      G_OMX_INIT_PARAM (param);

      param.nPortIndex = self->in_port->port_index;
      OMX_GetParameter (omx_handle, OMX_IndexParamPortDefinition, &param);

      g_value_set_uint (value, param.nBufferCountActual);
    }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
      break;
  }
}

static void
type_base_init (gpointer g_class)
{
}

static void
type_class_init (gpointer g_class, gpointer class_data)
{
  GObjectClass *gobject_class;
  GstBaseSinkClass *gst_base_sink_class;
  GstElementClass *gstelement_class;

  gobject_class = G_OBJECT_CLASS (g_class);
  gst_base_sink_class = GST_BASE_SINK_CLASS (g_class);
  gstelement_class = GST_ELEMENT_CLASS (g_class);

  gobject_class->finalize = finalize;

  gstelement_class->change_state = change_state;

  gst_base_sink_class->event = handle_event;
  gst_base_sink_class->preroll = render;
  gst_base_sink_class->render = render;

  /* Properties stuff */
  {
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    gstomx_install_property_helper (gobject_class);

    g_object_class_install_property (gobject_class, ARG_NUM_INPUT_BUFFERS,
        g_param_spec_uint ("input-buffers", "Input buffers",
            "The number of OMX input buffers",
            1, 10, 4, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  }
}

static gboolean
activate_push (GstPad * pad, gboolean active)
{
  gboolean result = TRUE;
  GstOmxBaseSink *self;

  self = GST_OMX_BASE_SINK (gst_pad_get_parent (pad));

  if (active) {
    GST_DEBUG_OBJECT (self, "activate");

    /* we do not start the task yet if the pad is not connected */
    if (gst_pad_is_linked (pad)) {
            /** @todo link callback function also needed */
      g_omx_port_resume (self->in_port);
    }
  } else {
    GST_DEBUG_OBJECT (self, "deactivate");

        /** @todo disable this until we properly reinitialize the buffers. */
#if 0
    /* flush all buffers */
    OMX_SendCommand (self->gomx->omx_handle, OMX_CommandFlush, OMX_ALL, NULL);
#endif

    /* unlock loops */
    g_omx_port_pause (self->in_port);
  }

  gst_object_unref (self);

  if (result)
    result = self->base_activatepush (pad, active);

  return result;
}

static inline gboolean
omx_init (GstOmxBaseSink * self)
{
  if (self->gomx->omx_error)
    return FALSE;

  setup_ports (self);

  return TRUE;
}

static GstPadLinkReturn
pad_sink_link (GstPad * pad, GstPad * peer)
{
  GstOmxBaseSink *self;

  self = GST_OMX_BASE_SINK (GST_OBJECT_PARENT (pad));

  GST_INFO_OBJECT (self, "link");

  if (!self->initialized) {
    if (!omx_init (self))
      return GST_PAD_LINK_REFUSED;
    self->initialized = TRUE;
  }

  return GST_PAD_LINK_OK;
}

static void
type_instance_init (GTypeInstance * instance, gpointer g_class)
{
  GstOmxBaseSink *self;

  self = GST_OMX_BASE_SINK (instance);

  GST_LOG_OBJECT (self, "begin");

  self->gomx = gstomx_core_new (self, G_TYPE_FROM_CLASS (g_class));
  self->in_port = g_omx_core_new_port (self->gomx, 0);

  {
    GstPad *sinkpad;
    self->sinkpad = sinkpad = GST_BASE_SINK_PAD (self);
    self->base_activatepush = GST_PAD_ACTIVATEPUSHFUNC (sinkpad);
    gst_pad_set_activatepush_function (sinkpad, activate_push);
    gst_pad_set_link_function (sinkpad, pad_sink_link);
  }

  GST_LOG_OBJECT (self, "end");
}

static void
omx_interface_init (GstImplementsInterfaceClass * klass)
{
}

static gboolean
interface_supported (GstImplementsInterface * iface, GType type)
{
  g_assert (type == GST_TYPE_OMX);
  return TRUE;
}

static void
interface_init (GstImplementsInterfaceClass * klass)
{
  klass->supported = interface_supported;
}

static void
init_interfaces (GType type)
{
  GInterfaceInfo *iface_info;
  GInterfaceInfo *omx_info;

  iface_info = g_new0 (GInterfaceInfo, 1);
  iface_info->interface_init = (GInterfaceInitFunc) interface_init;

  g_type_add_interface_static (type, GST_TYPE_IMPLEMENTS_INTERFACE, iface_info);
  g_free (iface_info);

  omx_info = g_new0 (GInterfaceInfo, 1);
  omx_info->interface_init = (GInterfaceInitFunc) omx_interface_init;

  g_type_add_interface_static (type, GST_TYPE_OMX, omx_info);
  g_free (omx_info);
}
