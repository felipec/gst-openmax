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

#include <string.h> /* for memset, memcpy */

static gboolean share_input_buffer;

static inline gboolean omx_init (GstOmxBaseSink *self);

enum
{
    ARG_0,
    ARG_COMPONENT_NAME,
    ARG_LIBRARY_NAME,
};

static GstElementClass *parent_class;

static void
setup_ports (GstOmxBaseSink *self)
{
    GOmxCore *core;
    OMX_PARAM_PORTDEFINITIONTYPE param;

    core = self->gomx;

    memset (&param, 0, sizeof (param));
    param.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
    param.nVersion.s.nVersionMajor = 1;
    param.nVersion.s.nVersionMinor = 1;

    /* Input port configuration. */

    param.nPortIndex = 0;
    OMX_GetParameter (core->omx_handle, OMX_IndexParamPortDefinition, &param);
    self->in_port = g_omx_core_setup_port (core, &param);
    gst_pad_set_element_private (self->sinkpad, self->in_port);
}

static GstStateChangeReturn
change_state (GstElement *element,
              GstStateChange transition)
{
    GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
    GstOmxBaseSink *self;

    self = GST_OMX_BASE_SINK (element);

    GST_LOG_OBJECT (self, "begin");

    GST_INFO_OBJECT (self, "changing state %s - %s",
                     gst_element_state_get_name (GST_STATE_TRANSITION_CURRENT (transition)),
                     gst_element_state_get_name (GST_STATE_TRANSITION_NEXT (transition)));

    switch (transition)
    {
        case GST_STATE_CHANGE_NULL_TO_READY:
            if (!self->initialized)
            {
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

    switch (transition)
    {
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
finalize (GObject *obj)
{
    GstOmxBaseSink *self;

    self = GST_OMX_BASE_SINK (obj);

    g_omx_core_free (self->gomx);

    G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static GstFlowReturn
render (GstBaseSink *gst_base,
        GstBuffer *buf)
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

    if (G_LIKELY (in_port->enabled))
    {
        guint buffer_offset = 0;

        while (G_LIKELY (buffer_offset < GST_BUFFER_SIZE (buf)))
        {
            OMX_BUFFERHEADERTYPE *omx_buffer;

            GST_LOG_OBJECT (self, "request_buffer");
            omx_buffer = g_omx_port_request_buffer (in_port);

            if (G_LIKELY (omx_buffer))
            {
                GST_DEBUG_OBJECT (self, "omx_buffer: size=%lu, len=%lu, flags=%lu, offset=%lu, timestamp=%lld",
                                  omx_buffer->nAllocLen, omx_buffer->nFilledLen, omx_buffer->nFlags,
                                  omx_buffer->nOffset, omx_buffer->nTimeStamp);

                if (omx_buffer->nOffset == 0 &&
                    share_input_buffer)
                {
                    {
                        GstBuffer *old_buf;
                        old_buf = omx_buffer->pAppPrivate;

                        if (old_buf)
                        {
                            gst_buffer_unref (old_buf);
                        }
                        else if (omx_buffer->pBuffer)
                        {
                            g_free (omx_buffer->pBuffer);
                        }
                    }

                    /* We are going to use this. */
                    gst_buffer_ref (buf);

                    omx_buffer->pBuffer = GST_BUFFER_DATA (buf);
                    omx_buffer->nAllocLen = GST_BUFFER_SIZE (buf);
                    omx_buffer->nFilledLen = GST_BUFFER_SIZE (buf);
                    omx_buffer->pAppPrivate = buf;
                }
                else
                {
                    omx_buffer->nFilledLen = MIN (GST_BUFFER_SIZE (buf) - buffer_offset,
                                                  omx_buffer->nAllocLen - omx_buffer->nOffset);
                    memcpy (omx_buffer->pBuffer + omx_buffer->nOffset, GST_BUFFER_DATA (buf) + buffer_offset, omx_buffer->nFilledLen);
                }

                GST_LOG_OBJECT (self, "release_buffer");
                g_omx_port_release_buffer (in_port, omx_buffer);

                buffer_offset += omx_buffer->nFilledLen;
            }
            else
            {
                GST_WARNING_OBJECT (self, "null buffer");
                ret = GST_FLOW_UNEXPECTED;
                break;
            }
        }
    }
    else
    {
        GST_WARNING_OBJECT (self, "done");
        ret = GST_FLOW_UNEXPECTED;
    }

    GST_LOG_OBJECT (self, "end");

    return ret;
}

static gboolean
handle_event (GstBaseSink *gst_base,
              GstEvent *event)
{
    GstOmxBaseSink *self;
    GOmxCore *gomx;
    GOmxPort *in_port;

    self = GST_OMX_BASE_SINK (gst_base);
    gomx = self->gomx;
    in_port = self->in_port;

    GST_LOG_OBJECT (self, "begin");

    GST_DEBUG_OBJECT (self, "event: %s", GST_EVENT_TYPE_NAME (event));

    switch (GST_EVENT_TYPE (event))
    {
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
get_property (GObject *obj,
              guint prop_id,
              GValue *value,
              GParamSpec *pspec)
{
    GstOmxBaseSink *self;

    self = GST_OMX_BASE_SINK (obj);

    switch (prop_id)
    {
        case ARG_COMPONENT_NAME:
            g_value_set_string (value, self->gomx->component_name);
            break;
        case ARG_LIBRARY_NAME:
            g_value_set_string (value, self->gomx->library_name);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
            break;
    }
}

static void
type_class_init (gpointer g_class,
                 gpointer class_data)
{
    GObjectClass *gobject_class;
    GstBaseSinkClass *gst_base_sink_class;
    GstElementClass *gstelement_class;

    gobject_class = G_OBJECT_CLASS (g_class);
    gst_base_sink_class = GST_BASE_SINK_CLASS (g_class);
    gstelement_class = GST_ELEMENT_CLASS (g_class);

    parent_class = g_type_class_ref (GST_TYPE_ELEMENT);

    gobject_class->finalize = finalize;

    gstelement_class->change_state = change_state;

    gst_base_sink_class->event = handle_event;
    gst_base_sink_class->preroll = render;
    gst_base_sink_class->render = render;

    /* Properties stuff */
    {
        gobject_class->get_property = get_property;

        g_object_class_install_property (gobject_class, ARG_COMPONENT_NAME,
                                         g_param_spec_string ("component-name", "Component name",
                                                              "Name of the OpenMAX IL component to use",
                                                              NULL, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

        g_object_class_install_property (gobject_class, ARG_LIBRARY_NAME,
                                         g_param_spec_string ("library-name", "Library name",
                                                              "Name of the OpenMAX IL implementation library to use",
                                                              NULL, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
    }
}

static gboolean
activate_push (GstPad *pad,
               gboolean active)
{
    gboolean result = TRUE;
    GstOmxBaseSink *self;

    self = GST_OMX_BASE_SINK (gst_pad_get_parent (pad));

    if (active)
    {
        GST_DEBUG_OBJECT (self, "activate");

        /* we do not start the task yet if the pad is not connected */
        if (gst_pad_is_linked (pad))
        {
            /** @todo link callback function also needed */
            g_omx_port_resume (self->in_port);
        }
    }
    else
    {
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
omx_init (GstOmxBaseSink *self)
{
    g_omx_core_init (self->gomx);

    if (self->gomx->omx_error)
        return FALSE;

    setup_ports (self);

    return TRUE;
}

static GstPadLinkReturn
pad_sink_link (GstPad *pad,
               GstPad *peer)
{
    GstOmxBaseSink *self;

    self = GST_OMX_BASE_SINK (GST_OBJECT_PARENT (pad));

    GST_INFO_OBJECT (self, "link");

    if (!self->initialized)
    {
        if (!omx_init (self))
            return GST_PAD_LINK_REFUSED;
        self->initialized = TRUE;
    }

    return GST_PAD_LINK_OK;
}

static void
type_instance_init (GTypeInstance *instance,
                    gpointer g_class)
{
    GstOmxBaseSink *self;

    self = GST_OMX_BASE_SINK (instance);

    GST_LOG_OBJECT (self, "begin");

    self->gomx = g_omx_core_new (self);
    gstomx_get_component_info (self->gomx, G_TYPE_FROM_CLASS (g_class));

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
omx_interface_init (GstImplementsInterfaceClass *klass)
{
}

static gboolean
interface_supported (GstImplementsInterface *iface,
                     GType type)
{
    g_assert (type == GST_TYPE_OMX);
    return TRUE;
}

static void
interface_init (GstImplementsInterfaceClass *klass)
{
    klass->supported = interface_supported;
}

GType
gst_omx_base_sink_get_type (void)
{
    static GType type = 0;

    if (G_UNLIKELY (type == 0))
    {
        GTypeInfo *type_info;
        GInterfaceInfo *iface_info;
        GInterfaceInfo *omx_info;

        type_info = g_new0 (GTypeInfo, 1);
        type_info->class_size = sizeof (GstOmxBaseSinkClass);
        type_info->class_init = type_class_init;
        type_info->instance_size = sizeof (GstOmxBaseSink);
        type_info->instance_init = type_instance_init;

        type = g_type_register_static (GST_TYPE_BASE_SINK, "GstOmxBaseSink", type_info, 0);
        g_free (type_info);

        iface_info = g_new0 (GInterfaceInfo, 1);
        iface_info->interface_init = (GInterfaceInitFunc) interface_init;

        g_type_add_interface_static (type, GST_TYPE_IMPLEMENTS_INTERFACE, iface_info);
        g_free (iface_info);

        omx_info = g_new0 (GInterfaceInfo, 1);
        omx_info->interface_init = (GInterfaceInitFunc) omx_interface_init;

        g_type_add_interface_static (type, GST_TYPE_OMX, omx_info);
        g_free (omx_info);
    }

    return type;
}
