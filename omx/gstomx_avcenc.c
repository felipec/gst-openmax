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

#include "gstomx_avcenc.h"
#include "gstomx.h"

#define OMX_COMPONENT_NAME "OMX.st.video_encoder.avc"

static GstOmxBaseFilterClass *parent_class = NULL;

static GstCaps *
generate_src_template (void)
{
    GstCaps *caps;

    caps = gst_caps_new_simple ("video/x-h264",
                                "width", GST_TYPE_INT_RANGE, 16, 4096,
                                "height", GST_TYPE_INT_RANGE, 16, 4096,
                                "framerate", GST_TYPE_FRACTION_RANGE, 0, 1, 30, 1,
                                NULL);

    return caps;
}

static void
type_base_init (gpointer g_class)
{
    GstElementClass *element_class;
    GstOmxBaseFilterClass *omx_base_class;

    omx_base_class = GST_OMX_BASE_FILTER_CLASS (g_class);
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
type_class_init (gpointer g_class,
                 gpointer class_data)
{
    parent_class = g_type_class_ref (GST_OMX_BASE_FILTER_TYPE);
}

static void
settings_changed_cb (GOmxCore *core)
{
    GstOmxBaseFilter *omx_base;
    guint width;
    guint height;
    guint framerate;

    omx_base = core->client_data;

    GST_DEBUG_OBJECT (omx_base, "settings changed");

    {
        OMX_PARAM_PORTDEFINITIONTYPE *param;

        param = calloc (1, sizeof (OMX_PARAM_PORTDEFINITIONTYPE));

        param->nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
        param->nVersion.s.nVersionMajor = 1;
        param->nVersion.s.nVersionMinor = 1;

        param->nPortIndex = 1;
        OMX_GetParameter (omx_base->gomx->omx_handle, OMX_IndexParamPortDefinition, param);

        width = param->format.video.nFrameWidth;
        height = param->format.video.nFrameHeight;
        framerate = param->format.video.xFramerate;

        free (param);
    }

    {
        GstCaps *new_caps;

        new_caps = gst_caps_new_simple ("video/x-h264",
                                        "width", G_TYPE_INT, width,
                                        "height", G_TYPE_INT, height,
                                        "framerate", GST_TYPE_FRACTION, framerate, 1,
                                        NULL);

        GST_INFO_OBJECT (omx_base, "caps are: %" GST_PTR_FORMAT, new_caps);
        gst_pad_set_caps (omx_base->srcpad, new_caps);
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

    omx_base_filter->omx_component = g_strdup (OMX_COMPONENT_NAME);
    omx_base->compression_format = OMX_VIDEO_CodingAVC;

    omx_base_filter->gomx->settings_changed_cb = settings_changed_cb;
}

GType
gst_omx_avcenc_get_type (void)
{
    static GType type = 0;

    if (type == 0)
    {
        GTypeInfo *type_info;

        type_info = g_new0 (GTypeInfo, 1);
        type_info->class_size = sizeof (GstOmxAvcEncClass);
        type_info->base_init = type_base_init;
        type_info->class_init = type_class_init;
        type_info->instance_size = sizeof (GstOmxAvcEnc);
        type_info->instance_init = type_instance_init;

        type = g_type_register_static (GST_OMX_BASE_VIDEOENC_TYPE, "GstOmxAvcEnc", type_info, 0);

        g_free (type_info);
    }

    return type;
}
