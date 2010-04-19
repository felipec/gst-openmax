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

#include "gstomx_ilbcdec.h"
#include "gstomx_base_filter.h"
#include "gstomx.h"

/* should this class extend GstOmxBaseAudioDec? */
GSTOMX_BOILERPLATE (GstOmxIlbcDec, gst_omx_ilbcdec, GstOmxBaseFilter, GST_OMX_BASE_FILTER_TYPE);

static GstCaps *
generate_src_template (void)
{
    GstCaps *caps;

    caps = gst_caps_new_simple ("audio/x-raw-int",
                                "endianness", G_TYPE_INT, G_BYTE_ORDER,
                                "width", G_TYPE_INT, 16,
                                "depth", G_TYPE_INT, 16,
                                "rate", G_TYPE_INT, 8000,
                                "signed", G_TYPE_BOOLEAN, TRUE,
                                "channels", G_TYPE_INT, 1,
                                NULL);

    return caps;
}

static GstCaps *
generate_sink_template (void)
{
    GstCaps *caps;
    GstStructure *struc;

    caps = gst_caps_new_empty ();

    struc = gst_structure_new ("audio/x-iLBC",
                               NULL);

    {
        GValue list;
        GValue val;

        list.g_type = val.g_type = 0;

        g_value_init (&list, GST_TYPE_LIST);
        g_value_init (&val, G_TYPE_INT);

        g_value_set_int (&val, 20);
        gst_value_list_append_value (&list, &val);

        g_value_set_int (&val, 30);
        gst_value_list_append_value (&list, &val);

        gst_structure_set_value (struc, "mode", &list);

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
        GstElementDetails details;

        details.longname = "OpenMAX IL iLBC audio decoder";
        details.klass = "Codec/Decoder/Audio";
        details.description = "Decodes audio in iLBC format with OpenMAX IL";
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
}

static gboolean
sink_setcaps (GstPad *pad,
              GstCaps *caps)
{
    GstOmxBaseFilter *omx_base;

    omx_base = GST_OMX_BASE_FILTER (GST_PAD_PARENT (pad));

    GST_INFO_OBJECT (omx_base, "setcaps (sink): %" GST_PTR_FORMAT, caps);

    /* set caps on the srcpad */
    {
        GstCaps *tmp_caps;

        tmp_caps = gst_pad_get_allowed_caps (omx_base->srcpad);
        tmp_caps = gst_caps_make_writable (tmp_caps);
        gst_caps_truncate (tmp_caps);

        gst_pad_fixate_caps (omx_base->srcpad, tmp_caps);

        if (gst_caps_is_fixed (tmp_caps))
        {
            GST_INFO_OBJECT (omx_base, "fixated to: %" GST_PTR_FORMAT, tmp_caps);
            gst_pad_set_caps (omx_base->srcpad, tmp_caps);
        }

        gst_caps_unref (tmp_caps);
    }

    return gst_pad_set_caps (pad, caps);
}

static void
type_instance_init (GTypeInstance *instance,
                    gpointer g_class)
{
    GstOmxBaseFilter *omx_base;

    omx_base = GST_OMX_BASE_FILTER (instance);

    gst_pad_set_setcaps_function (omx_base->sinkpad, sink_setcaps);
}
