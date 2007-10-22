/*
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

#include "gstomx_base_filter.h"
#include "gstomx.h"

#include <string.h>

#include <stdbool.h>

#define OMX_LIBRARY_NAME "libomxil.so"

enum
{
    ARG_0,
    ARG_COMPONENT_NAME,
    ARG_LIBRARY_NAME,
    ARG_USE_TIMESTAMPS
};

static GstElementClass *parent_class = NULL;

/** Finishes the processing. */
static void
out_port_done_cb (GOmxPort *port)
{
    g_omx_sem_up (port->core->done_sem);
}

static void
setup_ports (GstOmxBaseFilter *self)
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
    self->in_port->enable_queue = true;

    /* Output port configuration. */

    param->nPortIndex = 1;
    OMX_GetParameter (core->omx_handle, OMX_IndexParamPortDefinition, param);
    self->out_port = g_omx_core_setup_port (core, param);
    self->out_port->enable_queue = true;
    self->out_port->done_cb = out_port_done_cb;

    free (param);
}

static GstStateChangeReturn
change_state (GstElement *element,
              GstStateChange transition)
{
    GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
    GstOmxBaseFilter *self;

    self = GST_OMX_BASE_FILTER (element);

    GST_LOG_OBJECT (self, "begin");

    GST_INFO_OBJECT (self, "changing state %s - %s",
                     gst_element_state_get_name (GST_STATE_TRANSITION_CURRENT (transition)),
                     gst_element_state_get_name (GST_STATE_TRANSITION_NEXT (transition)));

    switch (transition)
    {
        case GST_STATE_CHANGE_NULL_TO_READY:
            g_omx_core_init (self->gomx, self->omx_library, self->omx_component);
            if (self->gomx->omx_error)
                return GST_STATE_CHANGE_FAILURE;
            break;

        case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
            break;

        case GST_STATE_CHANGE_PAUSED_TO_READY:
            if (self->initialized)
            {
                g_omx_port_set_done (self->in_port);
                g_omx_port_set_done (self->out_port);
            }
            break;

        default:
            break;
    }

    ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

    if (ret == GST_STATE_CHANGE_FAILURE)
        return ret;

    switch (transition)
    {
        case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
            break;

        case GST_STATE_CHANGE_PAUSED_TO_READY:
            if (self->initialized)
            {
                g_thread_join (self->thread);
                g_omx_core_finish (self->gomx);
            }
            break;

        case GST_STATE_CHANGE_READY_TO_NULL:
            g_omx_core_deinit (self->gomx);
            if (self->gomx->omx_error)
                return GST_STATE_CHANGE_FAILURE;
            break;

        default:
            break;
    }

    GST_LOG_OBJECT (self, "end");

    return ret;
}

static void
dispose (GObject *obj)
{
    GstOmxBaseFilter *self;

    self = GST_OMX_BASE_FILTER (obj);

    g_omx_core_free (self->gomx);

    g_free (self->omx_component);
    g_free (self->omx_library);

    G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
set_property (GObject *obj,
              guint prop_id,
              const GValue *value,
              GParamSpec *pspec)
{
    GstOmxBaseFilter *self;

    self = GST_OMX_BASE_FILTER (obj);

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
        case ARG_USE_TIMESTAMPS:
            self->use_timestamps = g_value_get_boolean (value);
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
    GstOmxBaseFilter *self;

    self = GST_OMX_BASE_FILTER (obj);

    switch (prop_id)
    {
        case ARG_COMPONENT_NAME:
            g_value_set_string (value, self->omx_component);
            break;
        case ARG_LIBRARY_NAME:
            g_value_set_string (value, self->omx_library);
            break;
        case ARG_USE_TIMESTAMPS:
            g_value_set_boolean (value, self->use_timestamps);
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

    gobject_class = G_OBJECT_CLASS (g_class);
    gstelement_class = GST_ELEMENT_CLASS (g_class);

    parent_class = g_type_class_ref (GST_TYPE_ELEMENT);

    gobject_class->dispose = dispose;
    gstelement_class->change_state = change_state;

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

        g_object_class_install_property (gobject_class, ARG_USE_TIMESTAMPS,
                                         g_param_spec_boolean ("use-timestamps", "Use timestamps",
                                                               "Whether or not to use timestamps",
                                                               TRUE, G_PARAM_READWRITE));
    }
}

static void
push_buffer (GstOmxBaseFilter *self,
             GstBuffer *buf)
{
    GST_LOG_OBJECT (self, "pad push begin");
    gst_pad_push (self->srcpad, buf);
    GST_LOG_OBJECT (self, "pad push end");
}

static gpointer
output_thread (gpointer cb_data)
{
    GOmxCore *gomx;
    GOmxPort *out_port;
    GstOmxBaseFilter *self;

    gomx = (GOmxCore *) cb_data;
    self = GST_OMX_BASE_FILTER (gomx->client_data);

    GST_LOG_OBJECT (self, "begin");

    out_port = self->out_port;

    while (!out_port->done)
    {
        OMX_BUFFERHEADERTYPE *omx_buffer;

        GST_LOG_OBJECT (self, "request buffer");
        omx_buffer = g_omx_port_request_buffer (out_port);

        GST_LOG_OBJECT (self, "omx_buffer: %p", omx_buffer);

        if (!omx_buffer)
        {
            GST_WARNING_OBJECT (self, "null buffer");
            continue;
        }

        GST_LOG_OBJECT (self, "buffer: size=%lu, len=%lu, flags=%lu",
                        omx_buffer->nAllocLen, omx_buffer->nFilledLen, omx_buffer->nFlags);

        if (omx_buffer->nFilledLen > 0)
        {
            GstBuffer *buf;

            if (!self->in_port->done)
            {
                GstCaps *caps = NULL;

                caps = gst_pad_get_negotiated_caps (self->srcpad);

                if (!caps)
                {
                    /** @todo We shouldn't be doing this. */
                    GST_WARNING_OBJECT (self, "somebody didn't do his work");
                    gomx->settings_changed_cb (gomx);
                }
                else
                {
                    GST_LOG_OBJECT (self, "caps already fixed");
                    gst_caps_unref (caps);
                }
            }

            buf = omx_buffer->pAppPrivate;

            if (buf && !(omx_buffer->nFlags & OMX_BUFFERFLAG_EOS))
            {
                GST_BUFFER_SIZE (buf) = omx_buffer->nFilledLen;
                if (self->use_timestamps)
                {
                    GST_BUFFER_TIMESTAMP (buf) = omx_buffer->nTimeStamp * (GST_SECOND / OMX_TICKS_PER_SECOND);
                }

                omx_buffer->pAppPrivate = NULL;
                omx_buffer->pBuffer = NULL;
                omx_buffer->nFilledLen = 0;

                push_buffer (self, buf);

                gst_buffer_unref (buf);
            }
            else
            {
                /* This is only meant for the first OpenMAX buffers,
                 * which need to be pre-allocated. */
                /* Also for the very last one. */
                gst_pad_alloc_buffer_and_set_caps (self->srcpad,
                                                   GST_BUFFER_OFFSET_NONE,
                                                   omx_buffer->nFilledLen,
                                                   GST_PAD_CAPS (self->srcpad),
                                                   &buf);

                if (buf)
                {
                    GST_WARNING_OBJECT (self, "couldn't zero-copy");
                    memcpy (GST_BUFFER_DATA (buf), omx_buffer->pBuffer + omx_buffer->nOffset, omx_buffer->nFilledLen);
                    if (self->use_timestamps)
                    {
                        GST_BUFFER_TIMESTAMP (buf) = omx_buffer->nTimeStamp * (GST_SECOND / OMX_TICKS_PER_SECOND);
                    }

                    omx_buffer->nFilledLen = 0;
                    g_free (omx_buffer->pBuffer);
                    omx_buffer->pBuffer = NULL;

                    push_buffer (self, buf);
                }
            }
        }

        if (omx_buffer->nFlags & OMX_BUFFERFLAG_EOS)
        {
            GST_INFO_OBJECT (self, "set_done: out_port");
            g_omx_port_set_done (out_port);
            break;
        }

        if (!omx_buffer->pBuffer)
        {
            GstBuffer *buf;
            GstFlowReturn result;

            GST_LOG_OBJECT (self, "allocate buffer");
            result = gst_pad_alloc_buffer_and_set_caps (self->srcpad,
                                                        GST_BUFFER_OFFSET_NONE,
                                                        omx_buffer->nAllocLen,
                                                        GST_PAD_CAPS (self->srcpad),
                                                        &buf);

            if (result == GST_FLOW_OK)
            {
                gst_buffer_ref (buf);
                omx_buffer->pAppPrivate = buf;

                omx_buffer->pBuffer = GST_BUFFER_DATA (buf);
                omx_buffer->nAllocLen = GST_BUFFER_SIZE (buf);
            }
            else
            {
                GST_WARNING_OBJECT (self, "could not allocate buffer");
                omx_buffer->pBuffer = g_malloc (omx_buffer->nAllocLen);
            }
        }

        GST_LOG_OBJECT (self, "release_buffer");
        g_omx_port_release_buffer (out_port, omx_buffer);
    }

    GST_DEBUG_OBJECT (self, "finishing output thread");

    GST_LOG_OBJECT (self, "end");

    return NULL;
}

static GstFlowReturn
pad_chain (GstPad *pad,
           GstBuffer *buf)
{
    GOmxCore *gomx;
    GOmxPort *in_port;
    GstOmxBaseFilter *self;
    GstFlowReturn ret = GST_FLOW_OK;

    self = GST_OMX_BASE_FILTER (GST_OBJECT_PARENT (pad));

    gomx = self->gomx;

    GST_LOG_OBJECT (self, "begin");
    GST_LOG_OBJECT (self, "gst_buffer: size=%lu", GST_BUFFER_SIZE (buf));

    GST_LOG_OBJECT (self, "state: %d", gomx->omx_state);

    if (gomx->omx_state == OMX_StateLoaded)
    {
        GST_INFO_OBJECT (self, "omx: prepare");

        setup_ports (self);
        g_omx_core_prepare (self->gomx);

        self->thread = g_thread_create (output_thread, gomx, TRUE, NULL);

        self->initialized = true;
    }

    in_port = self->in_port;

    if (!in_port->done)
    {
        switch (gomx->omx_state)
        {
            case OMX_StateIdle:
                {
                    GST_INFO_OBJECT (self, "omx: play");
                    g_omx_core_start (gomx);
                }
                break;
            default:
                break;
        }

        switch (gomx->omx_state)
        {
            case OMX_StateExecuting:
                /* OK */
                break;
            default:
                GST_ERROR_OBJECT (self, "Whoa! very wrong");
                break;
        }

        {
            OMX_BUFFERHEADERTYPE *omx_buffer;

            GST_LOG_OBJECT (self, "request buffer");
            omx_buffer = g_omx_port_request_buffer (in_port);

            if (omx_buffer)
            {
                GST_LOG_OBJECT (self, "omx_buffer: size=%lu, len=%lu, offset=%lu",
                                omx_buffer->nAllocLen, omx_buffer->nFilledLen, omx_buffer->nOffset);

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

                omx_buffer->nOffset = 0;
                omx_buffer->pBuffer = GST_BUFFER_DATA (buf);
                omx_buffer->nAllocLen = GST_BUFFER_SIZE (buf);
                omx_buffer->nFilledLen = GST_BUFFER_SIZE (buf);
                omx_buffer->pAppPrivate = buf;

                if (self->use_timestamps)
                {
                    omx_buffer->nTimeStamp = GST_BUFFER_TIMESTAMP (buf) / (GST_SECOND / OMX_TICKS_PER_SECOND);
                }

                GST_LOG_OBJECT (self, "release_buffer");
                g_omx_port_release_buffer (in_port, omx_buffer);
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
pad_event (GstPad *pad,
           GstEvent *event)
{
    GstOmxBaseFilter *self;
    gboolean ret;

    self = GST_OMX_BASE_FILTER (GST_OBJECT_PARENT (pad));

    GST_LOG_OBJECT (self, "begin");

    GST_DEBUG_OBJECT (self, "event: %s", GST_EVENT_TYPE_NAME (event));

    switch (GST_EVENT_TYPE (event))
    {
        case GST_EVENT_EOS:
            /* Close the inpurt port. */
            g_omx_port_set_done (self->in_port);
            /* Wait for the output port to get the EOS. */
            g_omx_core_wait_for_done (self->gomx);
            ret = gst_pad_push_event (self->srcpad, event);
            break;

        case GST_EVENT_FLUSH_START:
            g_omx_sem_up (self->in_port->sem);
            OMX_SendCommand (self->gomx->omx_handle, OMX_CommandFlush, 0, NULL);
            ret = gst_pad_push_event (self->srcpad, event);
            break;

        default:
            ret = gst_pad_push_event (self->srcpad, event);
            break;
    }

    GST_LOG_OBJECT (self, "end");

    return ret;
}

static void
type_instance_init (GTypeInstance *instance,
                    gpointer g_class)
{
    GstOmxBaseFilter *self;
    GstElementClass *element_class;

    element_class = GST_ELEMENT_CLASS (g_class);

    self = GST_OMX_BASE_FILTER (instance);

    GST_LOG_OBJECT (self, "begin");

    self->use_timestamps = true;

    /* GOmx */
    {
        GOmxCore *gomx;
        self->gomx = gomx = g_omx_core_new ();
        gomx->client_data = self;
    }

    self->sinkpad =
        gst_pad_new_from_template (gst_element_class_get_pad_template (element_class, "sink"), "sink");

    gst_pad_set_chain_function (self->sinkpad, pad_chain);
    gst_pad_set_event_function (self->sinkpad, pad_event);

    self->srcpad =
        gst_pad_new_from_template (gst_element_class_get_pad_template (element_class, "src"), "src");

    gst_pad_use_fixed_caps (self->srcpad);

    gst_element_add_pad (GST_ELEMENT (self), self->sinkpad);
    gst_element_add_pad (GST_ELEMENT (self), self->srcpad);

    self->omx_library = g_strdup (OMX_LIBRARY_NAME);

    GST_LOG_OBJECT (self, "end");
}

GType
gst_omx_base_filter_get_type (void)
{
    static GType type = 0;

    if (type == 0)
    {
        GTypeInfo *type_info;

        type_info = g_new0 (GTypeInfo, 1);
        type_info->class_size = sizeof (GstOmxBaseFilterClass);
        type_info->class_init = type_class_init;
        type_info->instance_size = sizeof (GstOmxBaseFilter);
        type_info->instance_init = type_instance_init;

        type = g_type_register_static (GST_TYPE_ELEMENT, "GstOmxBaseFilter", type_info, 0);

        g_free (type_info);
    }

    return type;
}
