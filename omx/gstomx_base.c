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

#include "gstomx_base.h"
#include "gstomx.h"

#include <string.h>

#include <stdbool.h>

static GstElementClass *parent_class = NULL;

static void 
setup_ports (GOmxCore *core)
{
    OMX_PARAM_PORTDEFINITIONTYPE *param;

    param = calloc (1, sizeof (OMX_PARAM_PORTDEFINITIONTYPE));
    param->nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
    param->nVersion.s.nVersionMajor = 1;
    param->nVersion.s.nVersionMinor = 1;

    /* Input port configuration. */

    param->nPortIndex = 0;
    OMX_GetParameter (core->omx_handle, OMX_IndexParamPortDefinition, param); 
    g_omx_port_setup (core->ports[param->nPortIndex], param->nBufferCountActual, param->nBufferSize);

    /* Output port configuration. */

    param->nPortIndex = 1;
    OMX_GetParameter (core->omx_handle, OMX_IndexParamPortDefinition, param); 
    g_omx_port_setup (core->ports[param->nPortIndex], param->nBufferCountActual, param->nBufferSize);

    free (param);
}

static GstStateChangeReturn
change_state (GstElement *element,
              GstStateChange transition)
{
    GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
    GstOmxBase *self;

    self = GST_OMX_BASE (element);

    GST_DEBUG_OBJECT (self, "changing state %s - %s",
                      gst_element_state_get_name (GST_STATE_TRANSITION_CURRENT (transition)),
                      gst_element_state_get_name (GST_STATE_TRANSITION_NEXT (transition)));

    switch (transition)
    {
        case GST_STATE_CHANGE_NULL_TO_READY:
            g_omx_core_init (self->gomx, self->omx_component);
            if (self->gomx->omx_error)
                return GST_STATE_CHANGE_FAILURE;
            setup_ports (self->gomx);
            g_omx_core_prepare (self->gomx);
            break;

        case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
            g_omx_port_set_done (self->gomx->ports[0]);
            g_omx_port_set_done (self->gomx->ports[1]);
            break;

        case GST_STATE_CHANGE_PAUSED_TO_READY:
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
            g_thread_join (self->thread);
            g_omx_core_finish (self->gomx);
            self->started = false;
            break;

        case GST_STATE_CHANGE_READY_TO_NULL:
            g_omx_core_deinit (self->gomx);
            if (self->gomx->omx_error)
                return GST_STATE_CHANGE_FAILURE;
            break;

        default:
            break;
    }

    return ret;
}

static void
dispose (GObject *obj)
{
    GstOmxBase *self;

    self = GST_OMX_BASE (obj);

    g_omx_core_free (self->gomx);

    G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
type_class_init (gpointer g_class,
                 gpointer class_data)
{
    GObjectClass *gobject_class;
    GstElementClass *gstelement_class;
    GstOmxBaseClass *gstomx_base_class;

    gobject_class = G_OBJECT_CLASS (g_class);
    gstelement_class = GST_ELEMENT_CLASS (g_class);
    gstomx_base_class = GST_OMX_BASE_CLASS (g_class);

    parent_class = g_type_class_ref (GST_TYPE_ELEMENT);

    gobject_class->dispose = dispose;
    gstelement_class->change_state = change_state;
}

static void
set_caps (GstOmxBase *self)
{
    if (self->set_caps)
    {
        self->set_caps (self);
    }
}

static void
settings_changed_cb (GOmxCore *core)
{
    GstOmxBase *self;
    self = core->client_data;
    set_caps (self);
}

gpointer
output_thread (gpointer cb_data)
{
    GOmxCore *gomx;
    GstOmxBase *self;

    gomx = (GOmxCore *) cb_data;
    self = GST_OMX_BASE (gomx->client_data);

    GST_LOG_OBJECT (self, "start");

    while (!gomx->ports[1]->done)
    {
        OMX_BUFFERHEADERTYPE *omx_buffer;

        omx_buffer = g_omx_port_request_buffer (gomx->ports[1]);

        GST_LOG_OBJECT (self, "omx_buffer: %p", omx_buffer);

        if (!omx_buffer)
        {
            GST_WARNING_OBJECT (self, "Bad buffer");
            continue;
        }

        GST_LOG_OBJECT (self, "buffer: size=%lu, len=%lu, flags=%lu",
                        omx_buffer->nAllocLen, omx_buffer->nFilledLen, omx_buffer->nFlags);

        if (omx_buffer->nFilledLen > 0)
        {
            GstBuffer *buf;

            {
                GstCaps *caps = NULL;

                caps = gst_pad_get_negotiated_caps (self->srcpad);

                if (!caps)
                {
                    set_caps (self);
                }
                else
                {
                    GST_DEBUG_OBJECT (self, "caps already fixed");
                    gst_caps_unref (caps);
                }
            }

            buf = omx_buffer->pAppPrivate;

            if (buf && !(omx_buffer->nFlags & OMX_BUFFERFLAG_EOS))
            {
                GST_BUFFER_SIZE (buf) = omx_buffer->nFilledLen;
                omx_buffer->pAppPrivate = NULL;
                omx_buffer->pBuffer = NULL;
                omx_buffer->nFilledLen = 0;
                gst_pad_push (self->srcpad, buf);
                gst_buffer_unref (buf);
            }
            else
            {
                /* This is only menat for the first OpenMAX buffers,
                 * which need to be pre-allocated. */
                /* Also for the very last one. */
                gst_pad_alloc_buffer_and_set_caps (self->srcpad,
                                                   GST_BUFFER_OFFSET_NONE,
                                                   omx_buffer->nFilledLen,
                                                   GST_PAD_CAPS (self->srcpad),
                                                   &buf);

                if (buf)
                {
                    memcpy (GST_BUFFER_DATA (buf), omx_buffer->pBuffer + omx_buffer->nOffset, omx_buffer->nFilledLen);
                    gst_pad_push (self->srcpad, buf);
                    omx_buffer->nFilledLen = 0;
                }
            }
        }

        if (omx_buffer->nFlags & OMX_BUFFERFLAG_EOS)
        {
            g_omx_port_set_done (gomx->ports[1]);
            break;
        }

        if (!omx_buffer->pBuffer)
        {
            GstBuffer *buf;
            GstFlowReturn result;

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
                GST_ERROR_OBJECT (self, "Could not allocate buffer");
                break;
            }
        }

        g_omx_port_release_buffer (gomx->ports[1], omx_buffer);
    }

    GST_LOG_OBJECT (self, "end");

    return NULL;
}

static GstFlowReturn
pad_chain (GstPad *pad,
           GstBuffer *buf)
{
    GstOmxBase *self;
    GOmxCore *gomx;
    GstFlowReturn ret = GST_FLOW_OK;

    self = GST_OMX_BASE (GST_OBJECT_PARENT (pad));

    gomx = self->gomx;

    GST_LOG_OBJECT (self, "start");
    GST_LOG_OBJECT (self, "gst_buffer: size=%lu", GST_BUFFER_SIZE (buf));

    /* Start OpenMAX IL */
    if (self->started == false)
    {
        GST_DEBUG_OBJECT (self, "start_omx");

        g_omx_core_start (gomx);

        self->thread = g_thread_create (output_thread, gomx, TRUE, NULL);
        self->started = true;
    }

    if (!gomx->ports[0]->done)
    {
        OMX_BUFFERHEADERTYPE *omx_buffer;

        omx_buffer = g_omx_port_request_buffer (gomx->ports[0]);

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
            }

            omx_buffer->nOffset = 0;
            omx_buffer->pBuffer = GST_BUFFER_DATA (buf);
            omx_buffer->nAllocLen = GST_BUFFER_SIZE (buf);
            omx_buffer->nFilledLen = GST_BUFFER_SIZE (buf);
            omx_buffer->pAppPrivate = buf;

            GST_LOG_OBJECT (self, "release_buffer");
            g_omx_port_release_buffer (gomx->ports[0], omx_buffer);
        }
        else
        {
            GST_WARNING_OBJECT (self, "bad buffer");
            /* ret = GST_FLOW_ERROR; */
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
    GstOmxBase *self;

    self = GST_OMX_BASE (GST_OBJECT_PARENT (pad));

    GST_DEBUG_OBJECT (self, "start");

    GST_DEBUG_OBJECT (self, "event: %s", GST_EVENT_TYPE_NAME (event));

    switch (GST_EVENT_TYPE (event))
    {
        case GST_EVENT_EOS:
            /* Close the inpurt port. */
            g_omx_port_set_done (self->gomx->ports[0]);
            /* Wait for the output port to get the EOS. */
            g_omx_core_wait_for_done (self->gomx);
            break;

        case GST_EVENT_NEWSEGMENT:
            break;

        default:
            break;
    }

    GST_DEBUG_OBJECT (self, "stop");

    return gst_pad_event_default (pad, event);
}

static void
type_instance_init (GTypeInstance *instance,
                    gpointer g_class)
{
    GstOmxBase *self;
    GstElementClass *element_class;
    GstOmxBaseClass *omx_base_class;

    element_class = GST_ELEMENT_CLASS (g_class);
    omx_base_class = GST_OMX_BASE_CLASS (g_class);

    self = GST_OMX_BASE (instance);

    GST_DEBUG_OBJECT (self, "start");

    /* GOmx */
    {
        GOmxCore *gomx;
        self->gomx = gomx = g_omx_core_new ();
        gomx->client_data = self;
        gomx->settings_changed_cb = settings_changed_cb;
        gomx->ports[0]->enable_queue = true;
        gomx->ports[1]->enable_queue = true;
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
}

GType
gst_omx_base_get_type (void)
{
    static GType type = 0;

    if (type == 0) 
    {
        GTypeInfo *type_info;

        type_info = g_new0 (GTypeInfo, 1);
        type_info->class_size = sizeof (GstOmxBaseClass);
        type_info->class_init = type_class_init;
        type_info->instance_size = sizeof (GstOmxBase);
        type_info->instance_init = type_instance_init;

        type = g_type_register_static (GST_TYPE_ELEMENT, "GstOmxBase", type_info, 0);

        g_free (type_info);
    }

    return type;
}
