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

#include "gstomx_g711enc.h"
#include "gstomx_base_filter.h"
#include "gstomx.h"

#include <string.h> /* for memset, strcmp */

GSTOMX_BOILERPLATE (GstOmxG711Enc, gst_omx_g711enc, GstOmxBaseFilter, GST_OMX_BASE_FILTER_TYPE);

static GstCaps *
generate_src_template (void)
{
    GstCaps *caps;
    GstStructure *struc;

    caps = gst_caps_new_empty ();

    struc = gst_structure_new ("audio/x-alaw",
                               "rate", G_TYPE_INT, 8000,
                               "channels", G_TYPE_INT, 1,
                               NULL);

    gst_caps_append_structure (caps, struc);

    struc = gst_structure_new ("audio/x-mulaw",
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

        details.longname = "OpenMAX IL G.711 audio encoder";
        details.klass = "Codec/Encoder/Audio";
        details.description = "Encodes audio in G.711 format with OpenMAX IL";
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
    GstCaps *peer_caps;
    GstStructure *structure;
    GstOmxBaseFilter *omx_base;
    GOmxCore *gomx;
    const gchar *mode;
    gboolean ret = TRUE;

    omx_base = GST_OMX_BASE_FILTER (GST_PAD_PARENT (pad));
    gomx = (GOmxCore *) omx_base->gomx;

    GST_INFO_OBJECT (omx_base, "setcaps (sink): %" GST_PTR_FORMAT, caps);

    peer_caps = gst_pad_peer_get_caps (omx_base->srcpad);

    g_return_val_if_fail (peer_caps, FALSE);

    GST_INFO_OBJECT (omx_base, "setcaps (sink): peercaps: %" GST_PTR_FORMAT, peer_caps);

    if (gst_caps_get_size (peer_caps) == 0)
        goto leave;

    structure = gst_caps_get_structure (peer_caps, 0);

    mode = gst_structure_get_name (structure);

    /* Output port configuration. */
    {
        OMX_AUDIO_PARAM_PCMMODETYPE param;

        memset (&param, 0, sizeof (param));
        param.nSize = sizeof (OMX_AUDIO_PARAM_PCMMODETYPE);
        param.nVersion.s.nVersionMajor = 1;
        param.nVersion.s.nVersionMinor = 1;

        param.nPortIndex = 1;
        OMX_GetParameter (gomx->omx_handle, OMX_IndexParamAudioPcm, &param);

        if (strcmp (mode, "audio/x-alaw") == 0)
            param.ePCMMode = OMX_AUDIO_PCMModeALaw;
        else if (strcmp (mode, "audio/x-mulaw") == 0)
            param.ePCMMode = OMX_AUDIO_PCMModeMULaw;

        OMX_SetParameter (gomx->omx_handle, OMX_IndexParamAudioPcm, &param);
    }

leave:

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

    ret = gst_pad_set_caps (pad, caps);

    gst_caps_unref (peer_caps);

    return ret;
}

static void
type_instance_init (GTypeInstance *instance,
                    gpointer g_class)
{
    GstOmxBaseFilter *omx_base;

    omx_base = GST_OMX_BASE_FILTER (instance);

    gst_pad_set_setcaps_function (omx_base->sinkpad, sink_setcaps);
}
