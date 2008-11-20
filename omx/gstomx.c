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

#include "gstomx.h"
#include "gstomx_dummy.h"
#include "gstomx_mpeg4dec.h"
#include "gstomx_h263dec.h"
#include "gstomx_h264dec.h"
#include "gstomx_wmvdec.h"
#include "gstomx_mpeg4enc.h"
#include "gstomx_avcenc.h"
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
#include "gstomx_audiosink.h"
#include "gstomx_videosink.h"
#include "gstomx_filereadersrc.h"
#include "gstomx_volume.h"

#include "config.h"

#include <stdbool.h>

GST_DEBUG_CATEGORY (gstomx_debug);

#define DEFAULT_RANK GST_RANK_PRIMARY

static gboolean
plugin_init (GstPlugin *plugin)
{
    GST_DEBUG_CATEGORY_INIT (gstomx_debug, "omx", 0, "OpenMAX");

    g_omx_init ();

    if (!gst_element_register (plugin, "omx_dummy", GST_RANK_NONE, GST_OMX_DUMMY_TYPE))
    {
        return false;
    }

    if (!gst_element_register (plugin, "omx_mpeg4dec", DEFAULT_RANK, GST_OMX_MPEG4DEC_TYPE))
    {
        return false;
    }

    if (!gst_element_register (plugin, "omx_h263dec", DEFAULT_RANK, GST_OMX_H263DEC_TYPE))
    {
        return false;
    }

    if (!gst_element_register (plugin, "omx_h264dec", DEFAULT_RANK, GST_OMX_H264DEC_TYPE))
    {
        return false;
    }

    if (!gst_element_register (plugin, "omx_wmvdec", DEFAULT_RANK, GST_OMX_WMVDEC_TYPE))
    {
        return false;
    }

    if (!gst_element_register (plugin, "omx_mpeg4enc", DEFAULT_RANK, GST_OMX_MPEG4ENC_TYPE))
    {
        return false;
    }

    if (!gst_element_register (plugin, "omx_avcenc", DEFAULT_RANK, GST_OMX_AVCENC_TYPE))
    {
        return false;
    }

    if (!gst_element_register (plugin, "omx_h263enc", DEFAULT_RANK, GST_OMX_H263ENC_TYPE))
    {
        return false;
    }

    if (!gst_element_register (plugin, "omx_vorbisdec", DEFAULT_RANK, GST_OMX_VORBISDEC_TYPE))
    {
        return false;
    }

    if (!gst_element_register (plugin, "omx_mp3dec", DEFAULT_RANK, GST_OMX_MP3DEC_TYPE))
    {
        return false;
    }

    if (!gst_element_register (plugin, "omx_mp2dec", DEFAULT_RANK, GST_OMX_MP2DEC_TYPE))
    {
        return false;
    }

    if (!gst_element_register (plugin, "omx_amrnbdec", DEFAULT_RANK, GST_OMX_AMRNBDEC_TYPE))
    {
        return false;
    }

    if (!gst_element_register (plugin, "omx_amrnbenc", DEFAULT_RANK, GST_OMX_AMRNBENC_TYPE))
    {
        return false;
    }

    if (!gst_element_register (plugin, "omx_amrwbdec", DEFAULT_RANK, GST_OMX_AMRWBDEC_TYPE))
    {
        return false;
    }

    if (!gst_element_register (plugin, "omx_amrwbenc", DEFAULT_RANK, GST_OMX_AMRWBENC_TYPE))
    {
        return false;
    }

    if (!gst_element_register (plugin, "omx_aacdec", DEFAULT_RANK, GST_OMX_AACDEC_TYPE))
    {
        return false;
    }

    if (!gst_element_register (plugin, "omx_aacenc", DEFAULT_RANK, GST_OMX_AACENC_TYPE))
    {
        return false;
    }

    if (!gst_element_register (plugin, "omx_adpcmdec", DEFAULT_RANK, GST_OMX_ADPCMDEC_TYPE))
    {
        return false;
    }

    if (!gst_element_register (plugin, "omx_adpcmenc", DEFAULT_RANK, GST_OMX_ADPCMENC_TYPE))
    {
        return false;
    }

    if (!gst_element_register (plugin, "omx_g711dec", DEFAULT_RANK, GST_OMX_G711DEC_TYPE))
    {
        return false;
    }

    if (!gst_element_register (plugin, "omx_g711enc", DEFAULT_RANK, GST_OMX_G711ENC_TYPE))
    {
        return false;
    }

    if (!gst_element_register (plugin, "omx_g729dec", DEFAULT_RANK, GST_OMX_G729DEC_TYPE))
    {
        return false;
    }

    if (!gst_element_register (plugin, "omx_g729enc", DEFAULT_RANK, GST_OMX_G729ENC_TYPE))
    {
        return false;
    }

    if (!gst_element_register (plugin, "omx_ilbcdec", DEFAULT_RANK, GST_OMX_ILBCDEC_TYPE))
    {
        return false;
    }

    if (!gst_element_register (plugin, "omx_ilbcenc", DEFAULT_RANK, GST_OMX_ILBCENC_TYPE))
    {
        return false;
    }

    if (!gst_element_register (plugin, "omx_audiosink", GST_RANK_NONE, GST_OMX_AUDIOSINK_TYPE))
    {
        return false;
    }

    if (!gst_element_register (plugin, "omx_videosink", GST_RANK_NONE, GST_OMX_VIDEOSINK_TYPE))
    {
        return false;
    }

    if (!gst_element_register (plugin, "omx_filereadersrc", GST_RANK_NONE, GST_OMX_FILEREADERSRC_TYPE))
    {
        return false;
    }

    if (!gst_element_register (plugin, "omx_volume", GST_RANK_NONE, GST_OMX_VOLUME_TYPE))
    {
        return false;
    }

    return true;
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
