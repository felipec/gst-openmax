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

#include "gstomx.h"
#include "gstomx_dummy.h"
#include "gstomx_mpeg4dec.h"
#include "gstomx_h263dec.h"
#include "gstomx_h264dec.h"
#include "gstomx_wmvdec.h"
#include "gstomx_mpeg4enc.h"
#include "gstomx_h264enc.h"
#include "gstomx_h263enc.h"
#include "gstomx_vorbisdec.h"
#include "gstomx_mp3dec.h"
#include "gstomx_mp2dec.h"
#include "gstomx_aacdec.h"
#include "gstomx_aacenc.h"
#include "gstomx_amrnbdec.h"
#include "gstomx_amrnbenc.h"
#include "gstomx_amrwbdec.h"
#include "gstomx_amrwbenc.h"
#include "gstomx_adpcmdec.h"
#include "gstomx_adpcmenc.h"
#include "gstomx_g711dec.h"
#include "gstomx_g711enc.h"
#include "gstomx_g729dec.h"
#include "gstomx_g729enc.h"
#include "gstomx_ilbcdec.h"
#include "gstomx_ilbcenc.h"
#include "gstomx_jpegenc.h"
#include "gstomx_audiosink.h"
#include "gstomx_videosink.h"
#include "gstomx_filereadersrc.h"
#include "gstomx_volume.h"

#include "config.h"

GST_DEBUG_CATEGORY (gstomx_debug);

typedef struct TableItem
{
    const gchar *name;
    const gchar *library_name;
    const gchar *component_name;
    guint rank;
    GType (*get_type) (void);
} TableItem;

static TableItem element_table[] =
{
    { "omx_dummy", "libomxil-bellagio.so.0", "OMX.st.dummy", GST_RANK_NONE, gst_omx_dummy_get_type },
    { "omx_mpeg4dec", "libomxil-bellagio.so.0", "OMX.st.video_decoder.mpeg4", GST_RANK_PRIMARY, gst_omx_mpeg4dec_get_type },
    { "omx_h264dec", "libomxil-bellagio.so.0", "OMX.st.video_decoder.avc", GST_RANK_PRIMARY, gst_omx_h264dec_get_type },
    { "omx_h263dec", "libomxil-bellagio.so.0", "OMX.st.video_decoder.h263", GST_RANK_PRIMARY, gst_omx_h263dec_get_type },
    { "omx_wmvdec", "libomxil-bellagio.so.0", "OMX.st.video_decoder.wmv", GST_RANK_PRIMARY, gst_omx_wmvdec_get_type },
    { "omx_mpeg4enc", "libomxil-bellagio.so.0", "OMX.st.video_encoder.mpeg4", GST_RANK_PRIMARY, gst_omx_mpeg4enc_get_type },
    { "omx_h264enc", "libomxil-bellagio.so.0", "OMX.st.video_encoder.avc", GST_RANK_PRIMARY, gst_omx_h264enc_get_type },
    { "omx_h263enc", "libomxil-bellagio.so.0", "OMX.st.video_encoder.h263", GST_RANK_PRIMARY, gst_omx_h263enc_get_type },
    { "omx_vorbisdec", "libomxil-bellagio.so.0", "OMX.st.audio_decoder.ogg.single", GST_RANK_SECONDARY, gst_omx_vorbisdec_get_type },
    { "omx_mp3dec", "libomxil-bellagio.so.0", "OMX.st.audio_decoder.mp3.mad", GST_RANK_PRIMARY, gst_omx_mp3dec_get_type },
    { "omx_mp2dec", "libomxil-bellagio.so.0", "OMX.st.audio_decoder.mp3.mad", GST_RANK_PRIMARY, gst_omx_mp2dec_get_type },
    { "omx_amrnbdec", "libomxil-bellagio.so.0", "OMX.st.audio_decoder.amrnb", GST_RANK_PRIMARY, gst_omx_amrnbdec_get_type },
    { "omx_amrnbenc", "libomxil-bellagio.so.0", "OMX.st.audio_encoder.amrnb", GST_RANK_PRIMARY, gst_omx_amrnbenc_get_type },
    { "omx_amrwbdec", "libomxil-bellagio.so.0", "OMX.st.audio_decoder.amrwb", GST_RANK_PRIMARY, gst_omx_amrwbdec_get_type },
    { "omx_amrwbenc", "libomxil-bellagio.so.0", "OMX.st.audio_encoder.amrwb", GST_RANK_PRIMARY, gst_omx_amrwbenc_get_type },
    { "omx_aacdec", "libomxil-bellagio.so.0", "OMX.st.audio_decoder.aac", GST_RANK_PRIMARY, gst_omx_aacdec_get_type },
    { "omx_aacenc", "libomxil-bellagio.so.0", "OMX.st.audio_encoder.aac", GST_RANK_PRIMARY, gst_omx_aacenc_get_type },
    { "omx_adpcmdec", "libomxil-bellagio.so.0", "OMX.st.audio_decoder.adpcm", GST_RANK_PRIMARY, gst_omx_adpcmdec_get_type },
    { "omx_adpcmenc", "libomxil-bellagio.so.0", "OMX.st.audio_encoder.adpcm", GST_RANK_PRIMARY, gst_omx_adpcmenc_get_type },
    { "omx_g711dec", "libomxil-bellagio.so.0", "OMX.st.audio_decoder.g711", GST_RANK_PRIMARY, gst_omx_g711dec_get_type },
    { "omx_g711enc", "libomxil-bellagio.so.0", "OMX.st.audio_encoder.g711", GST_RANK_PRIMARY, gst_omx_g711enc_get_type },
    { "omx_g729dec", "libomxil-bellagio.so.0", "OMX.st.audio_decoder.g729", GST_RANK_PRIMARY, gst_omx_g729dec_get_type },
    { "omx_g729enc", "libomxil-bellagio.so.0", "OMX.st.audio_encoder.g729", GST_RANK_PRIMARY, gst_omx_g729enc_get_type },
    { "omx_ilbcdec", "libomxil-bellagio.so.0", "OMX.st.audio_decoder.ilbc", GST_RANK_PRIMARY, gst_omx_ilbcdec_get_type },
    { "omx_ilbcenc", "libomxil-bellagio.so.0", "OMX.st.audio_encoder.ilbc", GST_RANK_PRIMARY, gst_omx_ilbcenc_get_type },
    { "omx_jpegenc", "libomxil-bellagio.so.0", "OMX.st.image_encoder.jpeg", GST_RANK_PRIMARY, gst_omx_jpegenc_get_type },
    { "omx_audiosink", "libomxil-bellagio.so.0", "OMX.st.alsa.alsasink", GST_RANK_NONE, gst_omx_audiosink_get_type },
    { "omx_videosink", "libomxil-bellagio.so.0", "OMX.st.videosink", GST_RANK_NONE, gst_omx_videosink_get_type },
    { "omx_filereadersrc", "libomxil-bellagio.so.0", "OMX.st.audio_filereader", GST_RANK_NONE, gst_omx_filereadersrc_get_type },
    { "omx_volume", "libomxil-bellagio.so.0", "OMX.st.volume.component", GST_RANK_NONE, gst_omx_volume_get_type },
    { NULL, NULL, NULL, 0, NULL },
};

static gboolean
plugin_init (GstPlugin *plugin)
{
    GQuark library_name_quark;
    GQuark component_name_quark;
    GST_DEBUG_CATEGORY_INIT (gstomx_debug, "omx", 0, "gst-openmax");
    GST_DEBUG_CATEGORY_INIT (gstomx_util_debug, "omx_util", 0, "gst-openmax utility");

    library_name_quark = g_quark_from_static_string ("library-name");
    component_name_quark = g_quark_from_static_string ("component-name");

    g_omx_init ();

    {
        guint i;
        for (i = 0; element_table[i].name; i++)
        {
            TableItem *element;
            GType type;

            element = &element_table[i];
            type = element->get_type ();
            g_type_set_qdata (type, library_name_quark, (gpointer) element->library_name);
            g_type_set_qdata (type, component_name_quark, (gpointer) element->component_name);

            if (!gst_element_register (plugin, element->name, element->rank, type))
            {
                g_warning ("failed registering '%s'", element->name);
                return FALSE;
            }
        }
    }

    return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
                   GST_VERSION_MINOR,
                   "omx",
                   "OpenMAX IL",
                   plugin_init,
                   PACKAGE_VERSION,
                   GST_LICENSE,
                   GST_PACKAGE_NAME,
                   GST_PACKAGE_ORIGIN)
