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

#include "gstomx_g729dec.h"
#include "gstomx.h"

GSTOMX_BOILERPLATE (GstOmxG729Dec, gst_omx_g729dec, GstOmxBaseAudioDec, GST_OMX_BASE_AUDIODEC_TYPE);

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

    struc = gst_structure_new ("audio/G729",
                               "rate", G_TYPE_INT, 8000,
                               "channels", G_TYPE_INT, 1,
                               NULL);

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

        details.longname = "OpenMAX IL G.729 audio decoder";
        details.klass = "Codec/Decoder/Audio";
        details.description = "Decodes audio in G.729 format with OpenMAX IL";
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

/* should we be overriding the settings_changed_cb from parent class like this?? */
static void
settings_changed_cb (GOmxCore *core)
{
    GstOmxBaseFilter *omx_base;

    omx_base = core->object;

    GST_DEBUG_OBJECT (omx_base, "settings changed");

    {
        GstCaps *new_caps;

        new_caps = gst_caps_new_simple ("audio/x-raw-int",
                                        "endianness", G_TYPE_INT, G_BYTE_ORDER,
                                        "width", G_TYPE_INT, 16,
                                        "depth", G_TYPE_INT, 16,
                                        "rate", G_TYPE_INT, 8000,
                                        "signed", G_TYPE_BOOLEAN, TRUE,
                                        "channels", G_TYPE_INT, 1,
                                        NULL);

        GST_INFO_OBJECT (omx_base, "caps are: %" GST_PTR_FORMAT, new_caps);
        gst_pad_set_caps (omx_base->srcpad, new_caps);
    }
}

static void
type_instance_init (GTypeInstance *instance,
                    gpointer g_class)
{
    GstOmxBaseFilter *omx_base;

    omx_base = GST_OMX_BASE_FILTER (instance);

    omx_base->gomx->settings_changed_cb = settings_changed_cb;
}
