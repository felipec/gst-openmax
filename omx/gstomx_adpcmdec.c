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

#include "gstomx_adpcmdec.h"
#include "gstomx_base_filter.h"
#include "gstomx.h"

#include <string.h>

#include <stdbool.h>

#define OMX_COMPONENT_NAME "OMX.st.audio_decoder.adpcm"

static GstOmxBaseFilterClass *parent_class = NULL;

static GstCaps *
generate_src_template (void)
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

static GstCaps *
generate_sink_template (void)
{
    GstCaps *caps;

    caps = gst_caps_new_simple ("audio/x-adpcm",
                                "rate", GST_TYPE_INT_RANGE, 8000, 48000,
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

        details.longname = "OpenMAX IL ADPCM audio decoder";
        details.klass = "Codec/Decoder/Audio";
        details.description = "Decodes audio in ADPCM format with OpenMAX IL";
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
    guint rate;
    guint channels;

    omx_base = core->client_data;

    GST_DEBUG_OBJECT (omx_base, "settings changed");

    {
        OMX_AUDIO_PARAM_PCMMODETYPE *param;

        param = calloc (1, sizeof (OMX_AUDIO_PARAM_PCMMODETYPE));
        param->nSize = sizeof (OMX_AUDIO_PARAM_PCMMODETYPE);
        param->nVersion.s.nVersionMajor = 1;
        param->nVersion.s.nVersionMinor = 1;

        param->nPortIndex = 1;
        OMX_GetParameter (omx_base->gomx->omx_handle, OMX_IndexParamAudioPcm, param);

        rate = param->nSamplingRate;
        channels = param->nChannels;

        free (param);
    }

    {
        GstCaps *new_caps;

        new_caps = gst_caps_new_simple ("audio/x-raw-int",
                                        "width", G_TYPE_INT, 16,
                                        "depth", G_TYPE_INT, 16,
                                        "rate", G_TYPE_INT, rate,
                                        "signed", G_TYPE_BOOLEAN, TRUE,
                                        "endianness", G_TYPE_INT, G_BYTE_ORDER ? 1234 : 4321,
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
    GstOmxBaseFilter *omx_base;
    GOmxCore *gomx;

    omx_base = GST_OMX_BASE_FILTER (GST_PAD_PARENT (pad));
    gomx = (GOmxCore *) omx_base->gomx;

    GST_INFO_OBJECT (omx_base, "setcaps (sink): %" GST_PTR_FORMAT, caps);

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
gst_omx_adpcmdec_get_type (void)
{
    static GType type = 0;

    if (type == 0)
    {
        GTypeInfo *type_info;

        type_info = g_new0 (GTypeInfo, 1);
        type_info->class_size = sizeof (GstOmxAdpcmDecClass);
        type_info->base_init = type_base_init;
        type_info->class_init = type_class_init;
        type_info->instance_size = sizeof (GstOmxAdpcmDec);
        type_info->instance_init = type_instance_init;

        type = g_type_register_static (GST_OMX_BASE_FILTER_TYPE, "GstOmxAdpcmDec", type_info, 0);

        g_free (type_info);
    }

    return type;
}
