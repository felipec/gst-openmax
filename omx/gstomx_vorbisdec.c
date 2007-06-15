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

#include "gstomx_vorbisdec.h"
#include "gstomx_base.h"
#include "gstomx.h"

#include <string.h>

#include <stdbool.h>

#define OMX_COMPONENT_ID "OMX.st.audio_decoder.ogg.single"

static GstOmxBaseClass *parent_class = NULL;

static GstCaps *
generate_src_template ()
{
    GstCaps *caps = NULL;

    caps = gst_caps_new_simple ("audio/x-raw-int",
                                "width", GST_TYPE_INT_RANGE, 8, 32,
                                "depth", GST_TYPE_INT_RANGE, 8, 32,
                                "rate", GST_TYPE_INT_RANGE, 8000, 48000,
                                "signed", G_TYPE_BOOLEAN, TRUE,
                                "channels", GST_TYPE_INT_RANGE, 1, 8,
                                NULL);

    return caps;
}

static GstCaps *
generate_sink_template ()
{
    GstCaps *caps = NULL;

    caps = gst_caps_new_simple ("application/ogg",
                                NULL);

    return caps;
}

static void
type_base_init (gpointer g_class)
{
    GstElementClass *element_class;
    GstOmxBaseClass *omx_base_class;

    omx_base_class = GST_OMX_BASE_CLASS (g_class);
    element_class = GST_ELEMENT_CLASS (g_class);

    {
        GstElementDetails details;

        details.longname = "OpenMAX IL Vorbis audio decoder";
        details.klass = "Codec/Decoder/Audio";
        details.description = "Decodes audio in Vorbis format with OpenMAX IL";
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
    parent_class = g_type_class_ref (GST_OMX_BASE_TYPE);
}

static void
type_instance_init (GTypeInstance *instance,
                    gpointer g_class)
{
    GstOmxBase *omx_base;
    GstElementClass *element_class;

    omx_base = GST_OMX_BASE (instance);
    element_class = GST_ELEMENT_CLASS (g_class);

    GST_DEBUG_OBJECT (omx_base, "start");

    omx_base->omx_component = OMX_COMPONENT_ID;
}

GType
gst_omx_vorbisdec_get_type (void)
{
    static GType type = 0;

    if (type == 0)
    {
        GTypeInfo *type_info;

        type_info = g_new0 (GTypeInfo, 1);

        type_info->class_size = sizeof (GstOmxVorbisDecClass);
        type_info->base_init = type_base_init;
        type_info->class_init = type_class_init;
        type_info->instance_size = sizeof (GstOmxVorbisDec);
        type_info->instance_init = type_instance_init;

        type = g_type_register_static (GST_OMX_BASE_TYPE, "GstOmxVorbisDec", type_info, 0);

        g_free (type_info);
    }

    return type;
}
