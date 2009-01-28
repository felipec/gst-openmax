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

#include "gstomx_wmvdec.h"
#include "gstomx.h"

static GstOmxBaseVideoDecClass *parent_class;

static GstCaps *
generate_sink_template (void)
{
    GstCaps *caps;
    GstStructure *struc;

    caps = gst_caps_new_empty ();

    struc = gst_structure_new ("video/x-wmv",
                               "width", GST_TYPE_INT_RANGE, 16, 4096,
                               "height", GST_TYPE_INT_RANGE, 16, 4096,
                               "framerate", GST_TYPE_FRACTION_RANGE, 0, 1, G_MAXINT, 1,
                               "wmvversion", G_TYPE_INT, 3,
                               NULL);

    gst_caps_append_structure (caps, struc);

    return caps;
}

static gboolean
sink_setcaps (GstPad *pad,
              GstCaps *caps)
{
    GstStructure *structure;
    GstOmxBaseVideoDec *omx_base;
    GstOmxBaseFilter *omx_base_filter;
    GOmxCore *gomx;
    gboolean is_vc1 = FALSE;

    omx_base = GST_OMX_BASE_VIDEODEC (GST_PAD_PARENT (pad));
    omx_base_filter = GST_OMX_BASE_FILTER (omx_base);

    gomx = (GOmxCore *) omx_base_filter->gomx;

    structure = gst_caps_get_structure (caps, 0);

    {
        guint32 fourcc;

        /** @todo which one should it be? Is this a demuxer bug? */
        if (gst_structure_get_fourcc (structure, "fourcc", &fourcc) ||
            gst_structure_get_fourcc (structure, "format", &fourcc))
        {
            if (fourcc == GST_MAKE_FOURCC ('W', 'V', 'C', '1'))
                is_vc1 = TRUE;
        }
    }

    /* This is specific for TI. */
    {
        OMX_INDEXTYPE index;
        OMX_U32 file_type = is_vc1 ? 0 : 1; /* 0 = wvc1, 1 = wmv3 */
        OMX_GetExtensionIndex (gomx->omx_handle, "OMX.TI.VideoDecode.Param.WMVFileType", &index);
        OMX_SetParameter (gomx->omx_handle, index, &file_type);

        GST_DEBUG_OBJECT (omx_base,
                          "OMX_SetParameter OMX.TI.VideoDecode.Param.WMVFileType %" G_GUINT32_FORMAT,
                          file_type);
    }

    return TRUE;
}

static void
type_base_init (gpointer g_class)
{
    GstElementClass *element_class;

    element_class = GST_ELEMENT_CLASS (g_class);

    {
        GstElementDetails details;

        details.longname = "OpenMAX IL WMV video decoder";
        details.klass = "Codec/Decoder/Video";
        details.description = "Decodes video in WMV format with OpenMAX IL";
        details.author = "Felipe Contreras";

        gst_element_class_set_details (element_class, &details);
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
    parent_class = g_type_class_ref (GST_OMX_BASE_VIDEODEC_TYPE);
}

static void
type_instance_init (GTypeInstance *instance,
                    gpointer g_class)
{
    GstOmxBaseVideoDec *omx_base;

    omx_base = GST_OMX_BASE_VIDEODEC (instance);

    omx_base->compression_format = OMX_VIDEO_CodingWMV;

    omx_base->sink_setcaps = sink_setcaps;
}

GType
gst_omx_wmvdec_get_type (void)
{
    static GType type = 0;

    if (G_UNLIKELY (type == 0))
    {
        GTypeInfo *type_info;

        type_info = g_new0 (GTypeInfo, 1);
        type_info->class_size = sizeof (GstOmxWmvDecClass);
        type_info->base_init = type_base_init;
        type_info->class_init = type_class_init;
        type_info->instance_size = sizeof (GstOmxWmvDec);
        type_info->instance_init = type_instance_init;

        type = g_type_register_static (GST_OMX_BASE_VIDEODEC_TYPE, "GstOmxWmvDec", type_info, 0);

        g_free (type_info);
    }

    return type;
}
