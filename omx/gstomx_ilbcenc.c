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

#include "gstomx_ilbcenc.h"
#include "gstomx_base_filter.h"
#include "gstomx.h"

#include <string.h>

#include <stdbool.h>

#define OMX_COMPONENT_NAME "OMX.st.audio_encoder.ilbc"

static GstOmxBaseFilterClass *parent_class = NULL;

static GstCaps *
generate_src_template (void)
{
    GstCaps *caps;
    GstStructure *struc;

    caps = gst_caps_new_empty ();

    struc = gst_structure_new ("audio/x-iLBC",
                               "mode", GST_TYPE_INT_RANGE, 20, 30,
                               NULL);

    gst_caps_append_structure (caps, struc);

    return caps;
}

static GstCaps *
generate_sink_template (void)
{
    GstCaps *caps;

    caps = gst_caps_new_simple ("audio/x-raw-int",
                                "endianness", G_TYPE_INT, G_BYTE_ORDER,
                                "width", GST_TYPE_INT_RANGE, 8, 32,
                                "depth", GST_TYPE_INT_RANGE, 8, 32,
                                "rate", G_TYPE_INT, 8000,
                                "signed", G_TYPE_BOOLEAN, TRUE,
                                "channels", G_TYPE_INT, 1,
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

        details.longname = "OpenMAX IL iLBC audio encoder";
        details.klass = "Codec/Encoder/Audio";
        details.description = "Encodes audio in iLBC format with OpenMAX IL";
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
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS (g_class);

    parent_class = g_type_class_ref (GST_OMX_BASE_FILTER_TYPE);
}

static gboolean
sink_setcaps (GstPad *pad,
              GstCaps *caps)
{
    GstStructure *structure;
    GstOmxBaseFilter *omx_base;
    GOmxCore *gomx;
    const gchar *mode;

    omx_base = GST_OMX_BASE_FILTER (GST_PAD_PARENT (pad));
    gomx = (GOmxCore *) omx_base->gomx;

    GST_INFO_OBJECT (omx_base, "setcaps (sink): %" GST_PTR_FORMAT, caps);

    g_return_val_if_fail (gst_caps_get_size (caps) == 1, FALSE);

    structure = gst_caps_get_structure (caps, 0);

    mode = gst_structure_get_name (structure);

    return gst_pad_set_caps (pad, caps);
}

static void
omx_setup (GstOmxBaseFilter *omx_base)
{
    GstOmxIlbcEnc *self;
    GOmxCore *gomx;

    self = GST_OMX_ILBCENC (omx_base);
    gomx = (GOmxCore *) omx_base->gomx;

    GST_INFO_OBJECT (omx_base, "begin");

    GST_INFO_OBJECT (omx_base, "end");
}

static void
type_instance_init (GTypeInstance *instance,
                    gpointer g_class)
{
    GstOmxBaseFilter *omx_base;
    GstOmxIlbcEnc *self;

    omx_base = GST_OMX_BASE_FILTER (instance);
    self = GST_OMX_ILBCENC (instance);

    omx_base->omx_component = g_strdup (OMX_COMPONENT_NAME);
    omx_base->omx_setup = omx_setup;

    gst_pad_set_setcaps_function (omx_base->sinkpad, sink_setcaps);
}

GType
gst_omx_ilbcenc_get_type (void)
{
    static GType type = 0;

    if (type == 0)
    {
        GTypeInfo *type_info;

        type_info = g_new0 (GTypeInfo, 1);
        type_info->class_size = sizeof (GstOmxIlbcEncClass);
        type_info->base_init = type_base_init;
        type_info->class_init = type_class_init;
        type_info->instance_size = sizeof (GstOmxIlbcEnc);
        type_info->instance_init = type_instance_init;

        type = g_type_register_static (GST_OMX_BASE_FILTER_TYPE, "GstOmxIlbcEnc", type_info, 0);

        g_free (type_info);
    }

    return type;
}
