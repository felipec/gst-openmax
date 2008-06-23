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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include "gstomx_aacenc.h"
#include "gstomx_base_filter.h"
#include "gstomx.h"

#include <string.h>

#include <stdbool.h>

#define OMX_COMPONENT_NAME "OMX.st.audio_encoder.aac"

enum
{
    ARG_0,
    ARG_BITRATE
};

#define DEFAULT_BITRATE 64000

static GstOmxBaseFilterClass *parent_class = NULL;

static GstCaps *
generate_src_template (void)
{
    GstCaps *caps;

    GstStructure *struc;

    caps = gst_caps_new_empty ();

    struc = gst_structure_new ("audio/mpeg",
                               "mpegversion", G_TYPE_INT, 4,
                               "rate", GST_TYPE_INT_RANGE, 8000, 48000,
                               "channels", GST_TYPE_INT_RANGE, 1, 8,
                               NULL);

    {
        GValue list = { 0, 0 };
        GValue val = { 0, 0 };

        g_value_init (&list, GST_TYPE_LIST);
        g_value_init (&val, G_TYPE_INT);

        g_value_set_int (&val, 2);
        gst_value_list_append_value (&list, &val);

        g_value_set_int (&val, 4);
        gst_value_list_append_value (&list, &val);

        gst_structure_set_value (struc, "mpegversion", &list);

        g_value_unset (&val);
        g_value_unset (&list);
    }

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
                                "rate", GST_TYPE_INT_RANGE, 8000, 48000,
                                "signed", G_TYPE_BOOLEAN, TRUE,
                                "channels", GST_TYPE_INT_RANGE, 1, 8,
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

        details.longname = "OpenMAX IL AAC audio encoder";
        details.klass = "Codec/Encoder/Audio";
        details.description = "Encodes audio in AAC format with OpenMAX IL";
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
    GstOmxAacEnc *self;

    self = GST_OMX_AACENC (obj);

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
    GstOmxAacEnc *self;

    self = GST_OMX_AACENC (obj);

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
                                                            0, G_MAXUINT, DEFAULT_BITRATE, G_PARAM_READWRITE));
    }
}

static void
settings_changed_cb (GOmxCore *core)
{
    GstOmxBaseFilter *omx_base;
    guint rate;
    guint channels;

    omx_base = core->client_data;

    GST_DEBUG_OBJECT (omx_base, "settings changed");

    {
        OMX_AUDIO_PARAM_AACPROFILETYPE *param;

        param = calloc (1, sizeof (OMX_AUDIO_PARAM_AACPROFILETYPE));
        param->nSize = sizeof (OMX_AUDIO_PARAM_AACPROFILETYPE);
        param->nVersion.s.nVersionMajor = 1;
        param->nVersion.s.nVersionMinor = 1;

        param->nPortIndex = 1;
        OMX_GetParameter (omx_base->gomx->omx_handle, OMX_IndexParamAudioAac, param);

        rate = param->nSampleRate;
        channels = param->nChannels;

        free (param);
    }

    {
        GstCaps *new_caps;

        new_caps = gst_caps_new_simple ("audio/mpeg",
                                        "mpegversion", G_TYPE_INT, 4,
                                        "rate", G_TYPE_INT, rate,
                                        "channels", G_TYPE_INT, channels,
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
        OMX_AUDIO_PARAM_PCMMODETYPE *param;

        param = calloc (1, sizeof (OMX_AUDIO_PARAM_PCMMODETYPE));
        param->nSize = sizeof (OMX_AUDIO_PARAM_PCMMODETYPE);
        param->nVersion.s.nVersionMajor = 1;
        param->nVersion.s.nVersionMinor = 1;

        param->nPortIndex = 0;
        OMX_GetParameter (omx_base->gomx->omx_handle, OMX_IndexParamAudioPcm, param);

        param->nSamplingRate = rate;
        param->nChannels = channels;

        OMX_SetParameter (omx_base->gomx->omx_handle, OMX_IndexParamAudioPcm, param);

        free (param);
    }

    return gst_pad_set_caps (pad, caps);
}

static void
omx_setup (GstOmxBaseFilter *omx_base)
{
    GstOmxAacEnc *self;
    GOmxCore *gomx;

    self = GST_OMX_AACENC (omx_base);
    gomx = (GOmxCore *) omx_base->gomx;

    GST_INFO_OBJECT (omx_base, "begin");

#if 0
    {
        OMX_AUDIO_PARAM_AACPROFILETYPE *param;
        OMX_COLOR_FORMATTYPE color_format;
        gint width, height;
        guint framerate;

        param = calloc (1, sizeof (OMX_AUDIO_PARAM_AACPROFILETYPE));
        param->nSize = sizeof (OMX_AUDIO_PARAM_AACPROFILETYPE);
        param->nVersion.s.nVersionMajor = 1;
        param->nVersion.s.nVersionMinor = 1;

        /* Output port configuration. */
        {
            param->nPortIndex = 1;
            OMX_GetParameter (gomx->omx_handle, OMX_IndexParamAudioAac, param);

            param->nBitRate = self->bitrate;

            OMX_SetParameter (gomx->omx_handle, OMX_IndexParamAudioAac, param);
        }
    }
#endif

    /* some workarounds. */
#if 0
    {
        OMX_AUDIO_PARAM_PCMMODETYPE *param;

        param = calloc (1, sizeof (OMX_AUDIO_PARAM_PCMMODETYPE));
        param->nSize = sizeof (OMX_AUDIO_PARAM_PCMMODETYPE);
        param->nVersion.s.nVersionMajor = 1;
        param->nVersion.s.nVersionMinor = 1;

        param->nPortIndex = 0;
        OMX_GetParameter (omx_base->gomx->omx_handle, OMX_IndexParamAudioPcm, param);

        rate = param->nSamplingRate;
        channels = param->nChannels;

        free (param);
    }

    {
        OMX_AUDIO_PARAM_AACPROFILETYPE *param;

        param = calloc (1, sizeof (OMX_AUDIO_PARAM_AACPROFILETYPE));
        param->nSize = sizeof (OMX_AUDIO_PARAM_AACPROFILETYPE);
        param->nVersion.s.nVersionMajor = 1;
        param->nVersion.s.nVersionMinor = 1;

        param->nPortIndex = 1;
        OMX_GetParameter (omx_base->gomx->omx_handle, OMX_IndexParamAudioAac, param);

        param->nSampleRate = rate;
        param->nChannels = channels;

        OMX_SetParameter (omx_base->gomx->omx_handle, OMX_IndexParamAudioAac, param);

        free (param);
    }
#endif

    GST_INFO_OBJECT (omx_base, "end");
}

static void
type_instance_init (GTypeInstance *instance,
                    gpointer g_class)
{
    GstOmxBaseFilter *omx_base;
    GstOmxAacEnc *self;

    omx_base = GST_OMX_BASE_FILTER (instance);
    self = GST_OMX_AACENC (instance);

    omx_base->omx_component = g_strdup (OMX_COMPONENT_NAME);
    omx_base->omx_setup = omx_setup;

    omx_base->gomx->settings_changed_cb = settings_changed_cb;

    gst_pad_set_setcaps_function (omx_base->sinkpad, sink_setcaps);

    self->bitrate = DEFAULT_BITRATE;
}

GType
gst_omx_aacenc_get_type (void)
{
    static GType type = 0;

    if (type == 0)
    {
        GTypeInfo *type_info;

        type_info = g_new0 (GTypeInfo, 1);
        type_info->class_size = sizeof (GstOmxAacEncClass);
        type_info->base_init = type_base_init;
        type_info->class_init = type_class_init;
        type_info->instance_size = sizeof (GstOmxAacEnc);
        type_info->instance_init = type_instance_init;

        type = g_type_register_static (GST_OMX_BASE_FILTER_TYPE, "GstOmxAacEnc", type_info, 0);

        g_free (type_info);
    }

    return type;
}
