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

#include "gstomx_amrnbenc.h"
#include "gstomx_base_filter.h"
#include "gstomx.h"

#include <string.h> /* for memset */

enum
{
    ARG_0,
    ARG_BITRATE,
};

#define DEFAULT_BITRATE 64000

static GstOmxBaseFilterClass *parent_class;

static GstCaps *
generate_src_template (void)
{
    GstCaps *caps;

    caps = gst_caps_new_simple ("audio/AMR",
                                "channels", G_TYPE_INT, 1,
                                "rate", G_TYPE_INT, 8000,
                                NULL);

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

        details.longname = "OpenMAX IL AMR-NB audio encoder";
        details.klass = "Codec/Encoder/Audio";
        details.description = "Encodes audio in AMR-NB format with OpenMAX IL";
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
    GstOmxAmrNbEnc *self;

    self = GST_OMX_AMRNBENC (obj);

    switch (prop_id)
    {
        case ARG_BITRATE:
            self->bitrate = g_value_get_uint (value);
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
    GstOmxAmrNbEnc *self;

    self = GST_OMX_AMRNBENC (obj);

    switch (prop_id)
    {
        case ARG_BITRATE:
            /** @todo propagate this to OpenMAX when processing. */
            g_value_set_uint (value, self->bitrate);
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

        g_object_class_install_property (gobject_class, ARG_BITRATE,
                                         g_param_spec_uint ("bitrate", "Bit-rate",
                                                            "Encoding bit-rate",
                                                            0, G_MAXUINT, DEFAULT_BITRATE,
                                                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
    }
}

static void
settings_changed_cb (GOmxCore *core)
{
    GstOmxBaseFilter *omx_base;
    guint channels;

    omx_base = core->object;

    GST_DEBUG_OBJECT (omx_base, "settings changed");

    {
        OMX_AUDIO_PARAM_AMRTYPE param;

        memset (&param, 0, sizeof (param));
        param.nSize = sizeof (OMX_AUDIO_PARAM_AMRTYPE);
        param.nVersion.s.nVersionMajor = 1;
        param.nVersion.s.nVersionMinor = 1;

        param.nPortIndex = 1;
        OMX_GetParameter (omx_base->gomx->omx_handle, OMX_IndexParamAudioAmr, &param);

        channels = param.nChannels;
    }

    {
        GstCaps *new_caps;

        new_caps = gst_caps_new_simple ("audio/AMR",
                                        "channels", G_TYPE_INT, channels,
                                        "rate", G_TYPE_INT, 8000,
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
    gint rate = 0;
    gint channels = 0;

    omx_base = GST_OMX_BASE_FILTER (GST_PAD_PARENT (pad));
    gomx = (GOmxCore *) omx_base->gomx;

    GST_INFO_OBJECT (omx_base, "setcaps (sink): %" GST_PTR_FORMAT, caps);

    g_return_val_if_fail (gst_caps_get_size (caps) == 1, FALSE);

    structure = gst_caps_get_structure (caps, 0);

    gst_structure_get_int (structure, "rate", &rate);
    gst_structure_get_int (structure, "channels", &channels);

    /* Input port configuration. */
    {
        OMX_AUDIO_PARAM_PCMMODETYPE param;

        memset (&param, 0, sizeof (param));
        param.nSize = sizeof (OMX_AUDIO_PARAM_PCMMODETYPE);
        param.nVersion.s.nVersionMajor = 1;
        param.nVersion.s.nVersionMinor = 1;

        param.nPortIndex = 0;
        OMX_GetParameter (gomx->omx_handle, OMX_IndexParamAudioPcm, &param);

        param.nSamplingRate = rate;
        param.nChannels = channels;

        OMX_SetParameter (gomx->omx_handle, OMX_IndexParamAudioPcm, &param);
    }

    return gst_pad_set_caps (pad, caps);
}

static void
type_instance_init (GTypeInstance *instance,
                    gpointer g_class)
{
    GstOmxBaseFilter *omx_base;
    GstOmxAmrNbEnc *self;

    omx_base = GST_OMX_BASE_FILTER (instance);
    self = GST_OMX_AMRNBENC (instance);

    omx_base->gomx->settings_changed_cb = settings_changed_cb;

    gst_pad_set_setcaps_function (omx_base->sinkpad, sink_setcaps);

    self->bitrate = DEFAULT_BITRATE;
}

GType
gst_omx_amrnbenc_get_type (void)
{
    static GType type = 0;

    if (G_UNLIKELY (type == 0))
    {
        GTypeInfo *type_info;

        type_info = g_new0 (GTypeInfo, 1);
        type_info->class_size = sizeof (GstOmxAmrNbEncClass);
        type_info->base_init = type_base_init;
        type_info->class_init = type_class_init;
        type_info->instance_size = sizeof (GstOmxAmrNbEnc);
        type_info->instance_init = type_instance_init;

        type = g_type_register_static (GST_OMX_BASE_FILTER_TYPE, "GstOmxAmrNbEnc", type_info, 0);

        g_free (type_info);
    }

    return type;
}
