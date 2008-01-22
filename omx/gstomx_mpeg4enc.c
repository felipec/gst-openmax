/*
 * Copyright (C) 2007-2008 Nokia Corporation. All rights reserved.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include "gstomx_mpeg4enc.h"
#include "gstomx_base_filter.h"
#include "gstomx.h"

#include <string.h>

#include <stdbool.h>

#define OMX_COMPONENT_NAME "OMX.st.video_encoder.mpeg4"

static GstOmxBaseFilterClass *parent_class = NULL;

static GstCaps *
generate_src_template ()
{
    GstCaps *caps;

    caps = gst_caps_new_simple ("video/mpeg",
                                "width", GST_TYPE_INT_RANGE, 16, 4096,
                                "height", GST_TYPE_INT_RANGE, 16, 4096,
                                "framerate", GST_TYPE_FRACTION_RANGE, 0, 1, 30, 1,
                                "mpegversion", G_TYPE_INT, 4,
                                "systemstream", G_TYPE_BOOLEAN, FALSE,
                                NULL);

    return caps;
}

static GstCaps *
generate_sink_template ()
{
    GstCaps *caps;
    GstStructure *struc;

    caps = gst_caps_new_empty ();

    struc = gst_structure_new ("video/x-raw-yuv",
                               "width", GST_TYPE_INT_RANGE, 16, 4096,
                               "height", GST_TYPE_INT_RANGE, 16, 4096,
                               "framerate", GST_TYPE_FRACTION_RANGE, 0, 1, 30, 1,
                               NULL);

    {
        GValue list = { 0 };
        GValue val = { 0 };

        g_value_init (&list, GST_TYPE_LIST);
        g_value_init (&val, GST_TYPE_FOURCC);

        gst_value_set_fourcc (&val, GST_MAKE_FOURCC ('I', '4', '2', '0'));
        gst_value_list_append_value (&list, &val);

        gst_value_set_fourcc (&val, GST_MAKE_FOURCC ('Y', 'U', 'Y', '2'));
        gst_value_list_append_value (&list, &val);

        gst_value_set_fourcc (&val, GST_MAKE_FOURCC ('U', 'Y', 'V', 'Y'));
        gst_value_list_append_value (&list, &val);

        gst_structure_set_value (struc, "format", &list);

        g_value_unset (&val);
        g_value_unset (&list);
    }

    gst_caps_append_structure (caps, struc);

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

        details.longname = "OpenMAX IL MPEG-4 video encoder";
        details.klass = "Codec/Encoder/Video";
        details.description = "Encodes video in MPEG-4 format with OpenMAX IL";
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

    {
        GstPadTemplate *template;

        template = gst_pad_template_new ("sink", GST_PAD_SINK,
                                         GST_PAD_ALWAYS,
                                         generate_sink_template ());

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

        new_caps = gst_caps_new_simple ("video/mpeg",
                                        "mpegversion", G_TYPE_INT, 4,
                                        "width", G_TYPE_INT, width,
                                        "height", G_TYPE_INT, height,
                                        "framerate", GST_TYPE_FRACTION, framerate, 1,
                                        "systemstream", G_TYPE_BOOLEAN, FALSE,
                                        NULL);

        GST_INFO_OBJECT (omx_base, "caps are: %" GST_PTR_FORMAT, new_caps);
        gst_pad_set_caps (omx_base->srcpad, new_caps);
    }
}

static gboolean
sink_setcaps (GstPad *pad,
              GstCaps *caps)
{
    GstStructure *structure;
    GstOmxBaseFilter *omx_base;
    GOmxCore *gomx;
    OMX_PARAM_PORTDEFINITIONTYPE *param;
    gint width = 0;
    gint height = 0;
    gint framerate = 0;

    omx_base = GST_OMX_BASE_FILTER (GST_PAD_PARENT (pad));
    gomx = (GOmxCore *) omx_base->gomx;

    GST_INFO_OBJECT (omx_base, "setcaps (sink): %" GST_PTR_FORMAT, caps);

    g_return_val_if_fail (gst_caps_get_size (caps) == 1, FALSE);

    structure = gst_caps_get_structure (caps, 0);

    gst_structure_get_int (structure, "width", &width);
    gst_structure_get_int (structure, "height", &height);

    {
        const GValue *tmp;
        tmp = gst_structure_get_value (structure, "framerate");

        if (tmp != NULL)
        {
            framerate = gst_value_get_fraction_numerator (tmp) / gst_value_get_fraction_denominator (tmp);
        }
    }

    param = calloc (1, sizeof (OMX_PARAM_PORTDEFINITIONTYPE));
    param->nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
    param->nVersion.s.nVersionMajor = 1;
    param->nVersion.s.nVersionMinor = 1;

    /* Input port configuration. */
    {
		param->nPortIndex = 0;
		OMX_GetParameter (gomx->omx_handle, OMX_IndexParamPortDefinition, param);

		param->format.video.nBitrate = 512000;
		param->format.video.nBitrate = 64000 * 4;

		param->format.video.nFrameWidth = width;
		param->format.video.nFrameHeight = height;
		param->format.video.xFramerate = framerate;

        {
            OMX_COLOR_FORMATTYPE color_format;

            color_format = OMX_COLOR_FormatYUV420Planar;

            switch (color_format)
            {
                case OMX_COLOR_FormatYCbYCr:
                case OMX_COLOR_FormatCbYCrY:
                    param->nBufferSize = (width * height) * 2;
                    break;
                case OMX_COLOR_FormatYUV420Planar:
                    param->nBufferSize = (width * height) * 1.5;
                    break;
                default:
                    break;
            }

            param->format.video.eColorFormat = color_format;
        }

		OMX_SetParameter (gomx->omx_handle, OMX_IndexParamPortDefinition, param);
	}

    /* Output port configuration. */
	{
		param->nPortIndex = 1;
		OMX_GetParameter (gomx->omx_handle, OMX_IndexParamPortDefinition, param);

        {
            OMX_VIDEO_CODINGTYPE compression_format;

            compression_format = OMX_VIDEO_CodingMPEG4;

			param->nBufferSize = (width * height / 2); /** @todo keep an eye on that */

            param->format.video.eCompressionFormat = compression_format;
        }

		param->format.video.nFrameWidth = width;
		param->format.video.nFrameHeight = height;
		param->format.video.xFramerate = framerate;

		OMX_SetParameter (gomx->omx_handle, OMX_IndexParamPortDefinition, param);
	}

    free (param);

    /* Control Rate */
    {
		OMX_VIDEO_PARAM_BITRATETYPE *param;
		param = (OMX_VIDEO_PARAM_BITRATETYPE *) calloc (1, sizeof (OMX_VIDEO_PARAM_BITRATETYPE));

		param->nSize = sizeof (OMX_VIDEO_PARAM_BITRATETYPE);
		param->nPortIndex = 1;

		OMX_GetParameter (gomx->omx_handle, OMX_IndexParamVideoBitrate, param);

		param->eControlRate = OMX_Video_ControlRateVariable;

		OMX_SetParameter (gomx->omx_handle, OMX_IndexParamVideoBitrate, param);

		free (param);
	}

    /* MPEG4 Level */
	{
		OMX_VIDEO_PARAM_MPEG4TYPE *param;
		param = (OMX_VIDEO_PARAM_MPEG4TYPE *) calloc (1, sizeof (OMX_VIDEO_PARAM_MPEG4TYPE));

		param->nSize = sizeof (OMX_VIDEO_PARAM_MPEG4TYPE);

        param->nPortIndex = 1;
		OMX_GetParameter (gomx->omx_handle, OMX_IndexParamVideoMpeg4, param);

		param->eLevel = OMX_VIDEO_MPEG4Level4; /** @todo calculate this automatically */

		OMX_SetParameter (gomx->omx_handle, OMX_IndexParamVideoMpeg4, param);

		free (param);
	}

    return gst_pad_set_caps (pad, caps);
}

static void
type_instance_init (GTypeInstance *instance,
                    gpointer g_class)
{
    GstOmxBaseFilter *omx_base;

    omx_base = GST_OMX_BASE_FILTER (instance);

    omx_base->omx_component = g_strdup (OMX_COMPONENT_NAME);

    omx_base->gomx->settings_changed_cb = settings_changed_cb;

    gst_pad_set_setcaps_function (omx_base->sinkpad, sink_setcaps);
}

GType
gst_omx_mpeg4enc_get_type (void)
{
    static GType type = 0;

    if (type == 0)
    {
        GTypeInfo *type_info;

        type_info = g_new0 (GTypeInfo, 1);
        type_info->class_size = sizeof (GstOmxMpeg4EncClass);
        type_info->base_init = type_base_init;
        type_info->class_init = type_class_init;
        type_info->instance_size = sizeof (GstOmxMpeg4Enc);
        type_info->instance_init = type_instance_init;

        type = g_type_register_static (GST_OMX_BASE_FILTER_TYPE, "GstOmxMpeg4Enc", type_info, 0);

        g_free (type_info);
    }

    return type;
}
