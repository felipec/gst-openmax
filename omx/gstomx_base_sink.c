/*
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

#include "gstomx_base_sink.h"
#include "gstomx.h"

#include <string.h> /* For memcpy */

static gboolean share_input_buffer = FALSE;

enum
{
    ARG_0,
    ARG_COMPONENT_NAME,
    ARG_LIBRARY_NAME
};

static GstElementClass *parent_class = NULL;

static void
setup_ports (GstOmxBaseSink *self)
{
    GOmxCore *core;
    OMX_PARAM_PORTDEFINITIONTYPE *param;

    core = self->gomx;

    param = calloc (1, sizeof (OMX_PARAM_PORTDEFINITIONTYPE));
    param->nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
    param->nVersion.s.nVersionMajor = 1;
    param->nVersion.s.nVersionMinor = 1;

    /* Input port configuration. */

    param->nPortIndex = 0;
    OMX_GetParameter (core->omx_handle, OMX_IndexParamPortDefinition, param);
    self->in_port = g_omx_core_setup_port (core, param);

    free (param);
}

static gboolean
start (GstBaseSink *gst_base)
{
    GstOmxBaseSink *self;

    self = GST_OMX_BASE_SINK (gst_base);

    GST_LOG_OBJECT (self, "begin");

    g_omx_core_init (self->gomx, self->omx_library, self->omx_component);
    if (self->gomx->omx_error)
        return GST_STATE_CHANGE_FAILURE;

    GST_LOG_OBJECT (self, "end");

    return TRUE;
}

static gboolean
stop (GstBaseSink *gst_base)
{
    GstOmxBaseSink *self;

    self = GST_OMX_BASE_SINK (gst_base);

    GST_LOG_OBJECT (self, "begin");

    g_omx_core_finish (self->gomx);

    g_omx_core_deinit (self->gomx);
    if (self->gomx->omx_error)
        return GST_STATE_CHANGE_FAILURE;

    GST_LOG_OBJECT (self, "end");

    return TRUE;
}

static void
dispose (GObject *obj)
{
    GstOmxBaseSink *self;

    self = GST_OMX_BASE_SINK (obj);

    g_omx_core_free (self->gomx);

    g_free (self->omx_component);
    g_free (self->omx_library);

    G_OBJECT_CLASS (parent_class)->dispose (obj);
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
    GST_LOG_OBJECT (self, "gst_buffer: size=%lu", GST_BUFFER_SIZE (buf));

    GST_LOG_OBJECT (self, "state: %d", gomx->omx_state);

    if (G_UNLIKELY (gomx->omx_state == OMX_StateLoaded))
    {
        GST_INFO_OBJECT (self, "omx: prepare");

        setup_ports (self);
        g_omx_core_prepare (self->gomx);
    }

    in_port = self->in_port;

    if (G_LIKELY (in_port->enabled))
    {
        guint buffer_offset = 0;

        if (G_UNLIKELY (gomx->omx_state == OMX_StateIdle))
        {
            GST_INFO_OBJECT (self, "omx: play");
            g_omx_core_start (gomx);
        }

        if (G_UNLIKELY (gomx->omx_state != OMX_StateExecuting))
        {
            GST_ERROR_OBJECT (self, "Whoa! very wrong");
        }

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
                /* ret = GST_FLOW_ERROR; */
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

    self = GST_OMX_BASE_SINK (gst_base);

    GST_LOG_OBJECT (self, "begin");

    GST_DEBUG_OBJECT (self, "event: %s", GST_EVENT_TYPE_NAME (event));

    switch (GST_EVENT_TYPE (event))
    {
        case GST_EVENT_EOS:
            /* Close the inpurt port. */
            g_omx_core_set_done (self->gomx);
            break;

        case GST_EVENT_FLUSH_START:
            /* unlock loops */
            g_omx_port_disable (self->in_port);

            /* flush all buffers */
            OMX_SendCommand (self->gomx->omx_handle, OMX_CommandFlush, OMX_ALL, NULL);
            break;

        case GST_EVENT_FLUSH_STOP:
            g_omx_sem_down (self->gomx->flush_sem);

            g_omx_port_enable (self->in_port);
            break;

        default:
            break;
    }

    GST_LOG_OBJECT (self, "end");

    return TRUE;
}

static void
set_property (GObject *obj,
              guint prop_id,
              const GValue *value,
              GParamSpec *pspec)
{
    GstOmxBaseSink *self;

    self = GST_OMX_BASE_SINK (obj);

    switch (prop_id)
    {
        case ARG_COMPONENT_NAME:
            if (self->omx_component)
            {
                g_free (self->omx_component);
            }
            self->omx_component = g_value_dup_string (value);
            break;
        case ARG_LIBRARY_NAME:
            if (self->omx_library)
            {
                g_free (self->omx_library);
            }
            self->omx_library = g_value_dup_string (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
            break;
    }
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
            g_value_set_string (value, self->omx_component);
            break;
        case ARG_LIBRARY_NAME:
            g_value_set_string (value, self->omx_library);
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
    GstElementClass *gstelement_class;
    GstBaseSinkClass *gst_base_sink_class;

    gobject_class = G_OBJECT_CLASS (g_class);
    gstelement_class = GST_ELEMENT_CLASS (g_class);
    gst_base_sink_class = GST_BASE_SINK_CLASS (g_class);

    parent_class = g_type_class_ref (GST_TYPE_ELEMENT);

    gobject_class->dispose = dispose;

    gst_base_sink_class->start = start;
    gst_base_sink_class->stop = stop;
    gst_base_sink_class->event = handle_event;
    gst_base_sink_class->preroll = render;
    gst_base_sink_class->render = render;

    /* Properties stuff */
    {
        gobject_class->set_property = set_property;
        gobject_class->get_property = get_property;

        g_object_class_install_property (gobject_class, ARG_COMPONENT_NAME,
                                         g_param_spec_string ("component-name", "Component name",
                                                              "Name of the OpenMAX IL component to use",
                                                              NULL, G_PARAM_READWRITE));

        g_object_class_install_property (gobject_class, ARG_LIBRARY_NAME,
                                         g_param_spec_string ("library-name", "Library name",
                                                              "Name of the OpenMAX IL implementation library to use",
                                                              NULL, G_PARAM_READWRITE));
    }
}

static void
type_instance_init (GTypeInstance *instance,
                    gpointer g_class)
{
    GstOmxBaseSink *self;
    GstElementClass *element_class;

    element_class = GST_ELEMENT_CLASS (g_class);

    self = GST_OMX_BASE_SINK (instance);

    GST_LOG_OBJECT (self, "begin");

    /* GOmx */
    {
        GOmxCore *gomx;
        self->gomx = gomx = g_omx_core_new ();
        gomx->client_data = self;
    }

    self->omx_library = g_strdup (DEFAULT_LIBRARY_NAME);

    GST_LOG_OBJECT (self, "end");
}

GType
gst_omx_base_sink_get_type (void)
{
    static GType type = 0;

    if (type == 0)
    {
        GTypeInfo *type_info;

        type_info = g_new0 (GTypeInfo, 1);
        type_info->class_size = sizeof (GstOmxBaseSinkClass);
        type_info->class_init = type_class_init;
        type_info->instance_size = sizeof (GstOmxBaseSink);
        type_info->instance_init = type_instance_init;

        type = g_type_register_static (GST_TYPE_BASE_SINK, "GstOmxBaseSink", type_info, 0);

        g_free (type_info);
    }

    return type;
}
