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

#include "gstomx_g729enc.h"
#include "gstomx_base_filter.h"
#include "gstomx.h"

#include <string.h> /* for memset */

#define DEFAULT_DTX TRUE

enum
{
    ARG_0,
    ARG_DTX,
};

static GstOmxBaseFilterClass *parent_class;

static GstCaps *
generate_src_template (void)
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

static GstCaps *
generate_sink_template (void)
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

static void
type_base_init (gpointer g_class)
{
    GstElementClass *element_class;

    element_class = GST_ELEMENT_CLASS (g_class);

    {
        GstElementDetails details;

        details.longname = "OpenMAX IL G.729 audio encoder";
        details.klass = "Codec/Encoder/Audio";
        details.description = "Encodes audio in G.729 format with OpenMAX IL";
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
set_property (GObject *obj,
              guint prop_id,
              const GValue *value,
              GParamSpec *pspec)
{
    GstOmxG729Enc *self;

    self = GST_OMX_G729ENC (obj);

    switch (prop_id)
    {
        case ARG_DTX:
            self->dtx = g_value_get_boolean (value);
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
    GstOmxG729Enc *self;

    self = GST_OMX_G729ENC (obj);

    switch (prop_id)
    {
        case ARG_DTX:
            /** @todo propagate this to OpenMAX when processing. */
            g_value_set_boolean (value, self->dtx);
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

        g_object_class_install_property (gobject_class, ARG_DTX,
                                         g_param_spec_boolean ("dtx", "DTX",
                                                               "Enable DTX",
                                                               DEFAULT_DTX, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
    }
}

static gboolean
sink_setcaps (GstPad *pad,
              GstCaps *caps)
{
    GstOmxBaseFilter *omx_base;
    gboolean ret = TRUE;

    omx_base = GST_OMX_BASE_FILTER (GST_PAD_PARENT (pad));

    GST_INFO_OBJECT (omx_base, "setcaps (sink): %" GST_PTR_FORMAT, caps);

    if (!caps || gst_caps_get_size (caps) == 0)
        goto refuse_caps;

    /* some extreme checking */
    if (!gst_pad_accept_caps (pad, caps))
        goto refuse_caps;

    /* set caps on the srcpad */
    {
        GstCaps *tmp_caps;

        /* src template are fixed caps */
        tmp_caps = generate_src_template ();

        ret = gst_pad_set_caps (omx_base->srcpad, tmp_caps);
        gst_caps_unref (tmp_caps);
    }

    return ret;

    /* ERRORS */
refuse_caps:
    {
        GST_WARNING_OBJECT (omx_base, "refused caps %" GST_PTR_FORMAT, caps);
        return FALSE;
    }
}

static void
omx_setup (GstOmxBaseFilter *omx_base)
{
    GstOmxG729Enc *self;
    GOmxCore *gomx;

    self = GST_OMX_G729ENC (omx_base);
    gomx = omx_base->gomx;

    GST_INFO_OBJECT (omx_base, "begin");

    {
        OMX_AUDIO_PARAM_G729TYPE param;

        memset (&param, 0, sizeof (param));
        param.nSize = sizeof (OMX_AUDIO_PARAM_G729TYPE);
        param.nVersion.s.nVersionMajor = 1;
        param.nVersion.s.nVersionMinor = 1;

        param.nPortIndex = 1;
        OMX_GetParameter (gomx->omx_handle, OMX_IndexParamAudioG729, &param);

        param.bDTX = self->dtx;

        OMX_SetParameter (gomx->omx_handle, OMX_IndexParamAudioG729, &param);
    }

    GST_INFO_OBJECT (omx_base, "end");
}

static void
type_instance_init (GTypeInstance *instance,
                    gpointer g_class)
{
    GstOmxBaseFilter *omx_base;
    GstOmxG729Enc *self;

    omx_base = GST_OMX_BASE_FILTER (instance);
    self = GST_OMX_G729ENC (instance);

    omx_base->omx_setup = omx_setup;

    gst_pad_set_setcaps_function (omx_base->sinkpad, sink_setcaps);

    self->dtx = DEFAULT_DTX;
}

GType
gst_omx_g729enc_get_type (void)
{
    static GType type = 0;

    if (G_UNLIKELY (type == 0))
    {
        GTypeInfo *type_info;

        type_info = g_new0 (GTypeInfo, 1);
        type_info->class_size = sizeof (GstOmxG729EncClass);
        type_info->base_init = type_base_init;
        type_info->class_init = type_class_init;
        type_info->instance_size = sizeof (GstOmxG729Enc);
        type_info->instance_init = type_instance_init;

        type = g_type_register_static (GST_OMX_BASE_FILTER_TYPE, "GstOmxG729Enc", type_info, 0);

        g_free (type_info);
    }

    return type;
}
