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

#include "gstomx_vorbisdec.h"
#include "gstomx_base_filter.h"
#include "gstomx.h"

#include <string.h> /* for memset */

GSTOMX_BOILERPLATE (GstOmxVorbisDec, gst_omx_vorbisdec, GstOmxBaseFilter, GST_OMX_BASE_FILTER_TYPE);

static GstCaps *
generate_src_template (void)
{
    GstCaps *caps = NULL;

    caps = gst_caps_new_simple ("audio/x-raw-int",
                                "rate", GST_TYPE_INT_RANGE, 8000, 96000,
                                "signed", G_TYPE_BOOLEAN, TRUE,
                                "endianness", G_TYPE_INT, G_BYTE_ORDER,
                                "width", G_TYPE_INT, 16,
                                "depth", G_TYPE_INT, 16,
                                "channels", GST_TYPE_INT_RANGE, 1, 256,
                                NULL);

    return caps;
}

static GstCaps *
generate_sink_template (void)
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
}

static void
settings_changed_cb (GOmxCore *core)
{
    GstOmxBaseFilter *omx_base;
    guint rate;
    guint channels;

    omx_base = core->object;

    GST_DEBUG_OBJECT (omx_base, "settings changed");

    {
        OMX_AUDIO_PARAM_PCMMODETYPE param;

        memset (&param, 0, sizeof (param));
        param.nSize = sizeof (OMX_AUDIO_PARAM_PCMMODETYPE);
        param.nVersion.s.nVersionMajor = 1;
        param.nVersion.s.nVersionMinor = 1;

        param.nPortIndex = 1;
        OMX_GetParameter (omx_base->gomx->omx_handle, OMX_IndexParamAudioPcm, &param);

        rate = param.nSamplingRate;
        channels = param.nChannels;
    }

    {
        GstCaps *new_caps;

        new_caps = gst_caps_new_simple ("audio/x-raw-int",
                                        "rate", G_TYPE_INT, rate,
                                        "signed", G_TYPE_BOOLEAN, TRUE,
                                        "channels", G_TYPE_INT, channels,
                                        "endianness", G_TYPE_INT, G_BYTE_ORDER,
                                        "width", G_TYPE_INT, 16,
                                        "depth", G_TYPE_INT, 16,
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

    GST_DEBUG_OBJECT (omx_base, "start");

    omx_base->use_timestamps = FALSE;

    omx_base->gomx->settings_changed_cb = settings_changed_cb;
}
