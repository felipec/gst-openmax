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
#include "gstomx_base.h"
#include "gstomx_dummy.h"
#include "gstomx_vorbisdec.h"
#include "gstomx_mp3dec.h"

#include <stdbool.h>

GST_DEBUG_CATEGORY (gstomx_debug);

static gboolean
plugin_init (GstPlugin *plugin)
{
    GST_DEBUG_CATEGORY_INIT (gstomx_debug, "omx", 0, "OpenMAX");

    if (!gst_element_register (plugin, "omx_dummy", GST_RANK_NONE, GST_OMX_DUMMY_TYPE))
    {
        return false;
    }

    if (!gst_element_register (plugin, "omx_vorbisdec", GST_RANK_NONE, GST_OMX_VORBISDEC_TYPE))
    {
        return false;
    }

    if (!gst_element_register (plugin, "omx_mp3dec", GST_RANK_NONE, GST_OMX_MP3DEC_TYPE))
    {
        return false;
    }

    return true;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
                   GST_VERSION_MINOR,
                   "omx",
                   "OpenMAX elements",
                   plugin_init,
                   VERSION,
                   "LGPL",
                   "OpenMAX",
                   "http://gstreamer.freedesktop.org/")
