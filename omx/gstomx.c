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

#include "gstomx.h"
#include "gstomx_dummy.h"
#include "gstomx_mpeg4dec.h"
#include "gstomx_h263dec.h"
#include "gstomx_h264dec.h"
#include "gstomx_mpeg4enc.h"
#include "gstomx_vorbisdec.h"
#include "gstomx_mp3dec.h"
#include "gstomx_aacdec.h"
#include "gstomx_alsasink.h"
#include "gstomx_videosink.h"
#include "gstomx_filereadersrc.h"

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

    if (!gst_element_register (plugin, "omx_h264dec", DEFAULT_RANK, GST_OMX_H263DEC_TYPE))
    {
        return false;
    }

    if (!gst_element_register (plugin, "omx_mpeg4dec", DEFAULT_RANK, GST_OMX_H263DEC_TYPE))
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

    if (!gst_element_register (plugin, "omx_aacdec", GST_RANK_NONE, GST_OMX_AACDEC_TYPE))
    {
        return false;
    }

    if (!gst_element_register (plugin, "omx_alsasink", GST_RANK_NONE, GST_OMX_ALSASINK_TYPE))
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
