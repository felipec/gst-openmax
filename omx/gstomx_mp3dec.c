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

#include "gstomx_mp3dec.h"
#include "gstomx_base.h"
#include "gstomx.h"

#include <string.h>

#include <stdbool.h>

#define OMX_COMPONENT_ID "OMX.st.audio_decoder.mp3.mad"

static GstOmxBaseClass *parent_class = NULL;

static GstCaps *
generate_src_template ()
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
generate_sink_template ()
{
    GstCaps *caps;

    caps = gst_caps_new_simple ("audio/mpeg",
                                "mpegversion", G_TYPE_INT, 1,
                                "layer", GST_TYPE_INT_RANGE, 1, 3,
                                "rate", GST_TYPE_INT_RANGE, 8000, 48000,
                                "channels", GST_TYPE_INT_RANGE, 1, 8,
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

        details.longname = "OpenMAX IL MP3 audio decoder";
        details.klass = "Codec/Decoder/Audio";
        details.description = "Decodes audio in MP3 format with OpenMAX IL";
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
set_caps (GstOmxBase *omx_base)
{
    guint rate;
    guint channels;

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
        if (rate == 0)
        {
            /** @todo: fetch the right one. */
            GST_WARNING_OBJECT (omx_base, "Bad samplerate");
            rate = 44100;
        }

        free (param);
    }

    GST_DEBUG_OBJECT (omx_base, "fixing caps");

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

static void
type_instance_init (GTypeInstance *instance,
                    gpointer g_class)
{
    GstOmxBase *omx_base;

    omx_base = GST_OMX_BASE (instance);

    GST_DEBUG_OBJECT (omx_base, "start");

    omx_base->omx_component = OMX_COMPONENT_ID;
    omx_base->set_caps = set_caps;
}

GType
gst_omx_mp3dec_get_type (void)
{
    static GType type = 0;

    if (type == 0)
    {
        GTypeInfo *type_info;

        type_info = g_new0 (GTypeInfo, 1);
        type_info->class_size = sizeof (GstOmxMp3DecClass);
        type_info->base_init = type_base_init;
        type_info->class_init = type_class_init;
        type_info->instance_size = sizeof (GstOmxMp3Dec);
        type_info->instance_init = type_instance_init;

        type = g_type_register_static (GST_OMX_BASE_TYPE, "GstOmxMp3Dec", type_info, 0);

        g_free (type_info);
    }

    return type;
}
