/*
 * Copyright (C) 2007-2009 Nokia Corporation.
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

#include "gstomx_h264enc.h"
#include "gstomx.h"

#include <string.h> /* for memset */

static GstOmxBaseFilterClass *parent_class;

enum
{
    ARG_0,
    ARG_BYTESTREAM,
};

#define DEFAULT_BYTESTREAM FALSE

static GstCaps *
generate_src_template (void)
{
    GstCaps *caps;

    caps = gst_caps_new_simple ("video/x-h264",
                                "width", GST_TYPE_INT_RANGE, 16, 4096,
                                "height", GST_TYPE_INT_RANGE, 16, 4096,
                                "framerate", GST_TYPE_FRACTION_RANGE, 0, 1, G_MAXINT, 1,
                                NULL);

    return caps;
}

static void
type_base_init (gpointer g_class)
{
    GstElementClass *element_class;

    element_class = GST_ELEMENT_CLASS (g_class);

    {
        GstElementDetails details;

        details.longname = "OpenMAX IL H.264/AVC video encoder";
        details.klass = "Codec/Encoder/Video";
        details.description = "Encodes video in H.264/AVC format with OpenMAX IL";
        details.author = "Felipe Contreras";

        gst_element_class_set_details (element_class, &details);
    }

    {
        GstPadTemplate *template;

        template = gst_pad_template_new ("src", GST_PAD_SRC,
                                         GST_PAD_ALWAYS,
                                         generate_src_template ());

        gst_element_class_add_pad_template (element_class, template);
    }
}

static void
set_property (GObject *obj,
              guint prop_id,
              const GValue *value,
              GParamSpec *pspec)
{
    GstOmxBaseFilter *omx_base;
    GstOmxH264Enc *self;
    GOmxCore *gomx;

    omx_base = GST_OMX_BASE_FILTER (obj);
    self = GST_OMX_H264ENC (obj);
    gomx = (GOmxCore*) omx_base->gomx;

    switch (prop_id)
    {
        case ARG_BYTESTREAM:
            self->bytestream = g_value_get_boolean (value);
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
    GstOmxH264Enc *self;

    self = GST_OMX_H264ENC (obj);

    switch (prop_id)
    {
        case ARG_BYTESTREAM:
            g_value_set_boolean (value, self->bytestream);
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

    gobject_class = G_OBJECT_CLASS (g_class);

    parent_class = g_type_class_ref (GST_OMX_BASE_FILTER_TYPE);

    /* Properties stuff */
    {
        gobject_class->set_property = set_property;
        gobject_class->get_property = get_property;

        g_object_class_install_property (gobject_class, ARG_BYTESTREAM,
                                         g_param_spec_boolean ("bytestream", "BYTESTREAM", "bytestream",
                                                               DEFAULT_BYTESTREAM, G_PARAM_READWRITE));
    }
}

static void
omx_setup (GstOmxBaseFilter *omx_base)
{
    GstOmxBaseVideoEnc *self;
    GOmxCore *gomx;

    self = GST_OMX_BASE_VIDEOENC (omx_base);
    gomx = (GOmxCore *) omx_base->gomx;

    GST_INFO_OBJECT (omx_base, "begin");

    {
        OMX_PARAM_PORTDEFINITIONTYPE param;

        memset (&param, 0, sizeof (param));
        param.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
        param.nVersion.s.nVersionMajor = 1;
        param.nVersion.s.nVersionMinor = 1;

        /* some workarounds required for TI components. */
        /* the component should do this instead */
        {
            param.nPortIndex = 1;
            OMX_GetParameter (gomx->omx_handle, OMX_IndexParamPortDefinition, &param);

            /* this is against the standard; nBufferSize is read-only. */
            param.nBufferSize = 300000;

            OMX_SetParameter (gomx->omx_handle, OMX_IndexParamPortDefinition, &param);
        }
    }

    {
        OMX_INDEXTYPE index;

        if (OMX_GetExtensionIndex (gomx->omx_handle, "OMX.TI.VideoEncode.Config.NALFormat", &index) == OMX_ErrorNone)
        {
            OMX_U32 nal_format;
            GstOmxH264Enc *h264enc;

            h264enc = GST_OMX_H264ENC (omx_base);
            nal_format = h264enc->bytestream ? 0 : 1;
            GST_DEBUG_OBJECT (omx_base, "setting 'OMX.TI.VideoEncode.Config.NALFormat' to %u", nal_format);

            OMX_SetParameter (gomx->omx_handle, index, &nal_format);
        }
        else
            GST_WARNING_OBJECT (omx_base, "'OMX.TI.VideoEncode.Config.NALFormat' unsupported");
    }

    GST_INFO_OBJECT (omx_base, "end");
}

static void
settings_changed_cb (GOmxCore *core)
{
    GstOmxBaseVideoEnc *omx_base;
    GstOmxBaseFilter *omx_base_filter;
    guint width;
    guint height;

    omx_base_filter = core->object;
    omx_base = GST_OMX_BASE_VIDEOENC (omx_base_filter);

    GST_DEBUG_OBJECT (omx_base, "settings changed");

    {
        OMX_PARAM_PORTDEFINITIONTYPE param;

        memset (&param, 0, sizeof (param));
        param.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
        param.nVersion.s.nVersionMajor = 1;
        param.nVersion.s.nVersionMinor = 1;

        param.nPortIndex = 1;
        OMX_GetParameter (core->omx_handle, OMX_IndexParamPortDefinition, &param);

        width = param.format.video.nFrameWidth;
        height = param.format.video.nFrameHeight;
    }

    {
        GstCaps *new_caps;

        new_caps = gst_caps_new_simple ("video/x-h264",
                                        "width", G_TYPE_INT, width,
                                        "height", G_TYPE_INT, height,
                                        "framerate", GST_TYPE_FRACTION,
                                        omx_base->framerate_num, omx_base->framerate_denom,
                                        NULL);

        GST_INFO_OBJECT (omx_base, "caps are: %" GST_PTR_FORMAT, new_caps);
        gst_pad_set_caps (omx_base_filter->srcpad, new_caps);
    }
}

static void
type_instance_init (GTypeInstance *instance,
                    gpointer g_class)
{
    GstOmxBaseFilter *omx_base_filter;
    GstOmxBaseVideoEnc *omx_base;

    omx_base_filter = GST_OMX_BASE_FILTER (instance);
    omx_base = GST_OMX_BASE_VIDEOENC (instance);

    omx_base->omx_setup = omx_setup;

    omx_base->compression_format = OMX_VIDEO_CodingAVC;

    omx_base_filter->gomx->settings_changed_cb = settings_changed_cb;
}

GType
gst_omx_h264enc_get_type (void)
{
    static GType type = 0;

    if (G_UNLIKELY (type == 0))
    {
        GTypeInfo *type_info;

        type_info = g_new0 (GTypeInfo, 1);
        type_info->class_size = sizeof (GstOmxH264EncClass);
        type_info->base_init = type_base_init;
        type_info->class_init = type_class_init;
        type_info->instance_size = sizeof (GstOmxH264Enc);
        type_info->instance_init = type_instance_init;

        type = g_type_register_static (GST_OMX_BASE_VIDEOENC_TYPE, "GstOmxH264Enc", type_info, 0);

        g_free (type_info);
    }

    return type;
}
