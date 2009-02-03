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

#include "gstomx_base_videodec.h"
#include "gstomx.h"

#include <string.h> /* for memset */

static GstOmxBaseFilterClass *parent_class;

static GstCaps *
generate_src_template (void)
{
    GstCaps *caps;
    GstStructure *struc;

    caps = gst_caps_new_empty ();

    struc = gst_structure_new ("video/x-raw-yuv",
                               "width", GST_TYPE_INT_RANGE, 16, 4096,
                               "height", GST_TYPE_INT_RANGE, 16, 4096,
                               "framerate", GST_TYPE_FRACTION_RANGE, 0, 1, G_MAXINT, 1,
                               NULL);

    {
        GValue list;
        GValue val;

        list.g_type = val.g_type = 0;

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

    element_class = GST_ELEMENT_CLASS (g_class);

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
    GstOmxBaseVideoDec *self;
    guint width;
    guint height;
    guint32 format = 0;

    omx_base = core->object;
    self = GST_OMX_BASE_VIDEODEC (omx_base);

    GST_DEBUG_OBJECT (omx_base, "settings changed");

    {
        OMX_PARAM_PORTDEFINITIONTYPE param;

        memset (&param, 0, sizeof (param));

        param.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
        param.nVersion.s.nVersionMajor = 1;
        param.nVersion.s.nVersionMinor = 1;

        param.nPortIndex = 1;
        OMX_GetParameter (omx_base->gomx->omx_handle, OMX_IndexParamPortDefinition, &param);

        width = param.format.video.nFrameWidth;
        height = param.format.video.nFrameHeight;
        switch (param.format.video.eColorFormat)
        {
            case OMX_COLOR_FormatYUV420Planar:
                format = GST_MAKE_FOURCC ('I', '4', '2', '0'); break;
            case OMX_COLOR_FormatYCbYCr:
                format = GST_MAKE_FOURCC ('Y', 'U', 'Y', '2'); break;
            case OMX_COLOR_FormatCbYCrY:
                format = GST_MAKE_FOURCC ('U', 'Y', 'V', 'Y'); break;
            default:
                break;
        }
    }

    {
        GstCaps *new_caps;

        new_caps = gst_caps_new_simple ("video/x-raw-yuv",
                                        "width", G_TYPE_INT, width,
                                        "height", G_TYPE_INT, height,
                                        "framerate", GST_TYPE_FRACTION,
                                        self->framerate_num, self->framerate_denom,
                                        "format", GST_TYPE_FOURCC, format,
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
    GstOmxBaseVideoDec *self;
    GstOmxBaseFilter *omx_base;
    GOmxCore *gomx;
    OMX_PARAM_PORTDEFINITIONTYPE param;
    gint width = 0;
    gint height = 0;

    self = GST_OMX_BASE_VIDEODEC (GST_PAD_PARENT (pad));
    omx_base = GST_OMX_BASE_FILTER (self);

    gomx = (GOmxCore *) omx_base->gomx;

    GST_INFO_OBJECT (self, "setcaps (sink): %" GST_PTR_FORMAT, caps);

    g_return_val_if_fail (gst_caps_get_size (caps) == 1, FALSE);

    structure = gst_caps_get_structure (caps, 0);

    gst_structure_get_int (structure, "width", &width);
    gst_structure_get_int (structure, "height", &height);

    {
        const GValue *framerate = NULL;
        framerate = gst_structure_get_value (structure, "framerate");
        if (framerate)
        {
            self->framerate_num = gst_value_get_fraction_numerator (framerate);
            self->framerate_denom = gst_value_get_fraction_denominator (framerate);
        }
    }

    memset (&param, 0, sizeof (param));
    param.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
    param.nVersion.s.nVersionMajor = 1;
    param.nVersion.s.nVersionMinor = 1;

    {
        const GValue *codec_data;
        GstBuffer *buffer;

        codec_data = gst_structure_get_value (structure, "codec_data");
        if (codec_data)
        {
            buffer = gst_value_get_buffer (codec_data);
            omx_base->codec_data = buffer;
            gst_buffer_ref (buffer);
        }
    }

    /* Input port configuration. */
    {
        param.nPortIndex = 0;
        OMX_GetParameter (gomx->omx_handle, OMX_IndexParamPortDefinition, &param);

        param.format.video.nFrameWidth = width;
        param.format.video.nFrameHeight = height;

        OMX_SetParameter (gomx->omx_handle, OMX_IndexParamPortDefinition, &param);
    }

    return gst_pad_set_caps (pad, caps);
}

static void
omx_setup (GstOmxBaseFilter *omx_base)
{
    GstOmxBaseVideoDec *self;
    GOmxCore *gomx;

    self = GST_OMX_BASE_VIDEODEC (omx_base);
    gomx = (GOmxCore *) omx_base->gomx;

    GST_INFO_OBJECT (omx_base, "begin");

    {
        OMX_PARAM_PORTDEFINITIONTYPE param;

        memset (&param, 0, sizeof (param));
        param.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
        param.nVersion.s.nVersionMajor = 1;
        param.nVersion.s.nVersionMinor = 1;

        /* Input port configuration. */
        {
            param.nPortIndex = 0;
            OMX_GetParameter (gomx->omx_handle, OMX_IndexParamPortDefinition, &param);

            param.format.video.eCompressionFormat = self->compression_format;

            OMX_SetParameter (gomx->omx_handle, OMX_IndexParamPortDefinition, &param);
        }

        /* some workarounds required for TI components. */
        {
            OMX_COLOR_FORMATTYPE color_format;
            gint width, height;

            {
                param.nPortIndex = 0;
                OMX_GetParameter (gomx->omx_handle, OMX_IndexParamPortDefinition, &param);

                width = param.format.video.nFrameWidth;
                height = param.format.video.nFrameHeight;

                /* this is against the standard; nBufferSize is read-only. */
                param.nBufferSize = (width * height) / 2;

                OMX_SetParameter (gomx->omx_handle, OMX_IndexParamPortDefinition, &param);
            }

            /* the component should do this instead */
            {
                param.nPortIndex = 1;
                OMX_GetParameter (gomx->omx_handle, OMX_IndexParamPortDefinition, &param);

                param.format.video.nFrameWidth = width;
                param.format.video.nFrameHeight = height;

                /** @todo get this from the srcpad. */
                param.format.video.eColorFormat = OMX_COLOR_FormatCbYCrY;

                color_format = param.format.video.eColorFormat;

                /* this is against the standard; nBufferSize is read-only. */
                switch (color_format)
                {
                    case OMX_COLOR_FormatYCbYCr:
                    case OMX_COLOR_FormatCbYCrY:
                        param.nBufferSize = (width * height) * 2;
                        break;
                    case OMX_COLOR_FormatYUV420Planar:
                        param.nBufferSize = (width * height) * 3 / 2;
                        break;
                    default:
                        break;
                }

                OMX_SetParameter (gomx->omx_handle, OMX_IndexParamPortDefinition, &param);
            }
        }
    }

    GST_INFO_OBJECT (omx_base, "end");
}

static void
type_instance_init (GTypeInstance *instance,
                    gpointer g_class)
{
    GstOmxBaseFilter *omx_base;

    omx_base = GST_OMX_BASE_FILTER (instance);

    omx_base->omx_setup = omx_setup;

    omx_base->gomx->settings_changed_cb = settings_changed_cb;

    gst_pad_set_setcaps_function (omx_base->sinkpad, sink_setcaps);
}

GType
gst_omx_base_videodec_get_type (void)
{
    static GType type = 0;

    if (G_UNLIKELY (type == 0))
    {
        GTypeInfo *type_info;

        type_info = g_new0 (GTypeInfo, 1);
        type_info->class_size = sizeof (GstOmxBaseVideoDecClass);
        type_info->base_init = type_base_init;
        type_info->class_init = type_class_init;
        type_info->instance_size = sizeof (GstOmxBaseVideoDec);
        type_info->instance_init = type_instance_init;

        type = g_type_register_static (GST_OMX_BASE_FILTER_TYPE, "GstOmxBaseVideoDec", type_info, 0);

        g_free (type_info);
    }

    return type;
}
