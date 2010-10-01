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

#ifndef GSTOMX_BASE_VIDEOENC_H
#define GSTOMX_BASE_VIDEOENC_H

#include <gst/gst.h>

G_BEGIN_DECLS
#define GST_OMX_BASE_VIDEOENC(obj) (GstOmxBaseVideoEnc *) (obj)
#define GST_OMX_BASE_VIDEOENC_TYPE (gst_omx_base_videoenc_get_type ())
typedef struct GstOmxBaseVideoEnc GstOmxBaseVideoEnc;
typedef struct GstOmxBaseVideoEncClass GstOmxBaseVideoEncClass;

#include "gstomx_base_filter.h"

struct GstOmxBaseVideoEnc
{
  GstOmxBaseFilter omx_base;

  OMX_VIDEO_CODINGTYPE compression_format;
  guint bitrate;
  gint framerate_num;
  gint framerate_denom;
};

struct GstOmxBaseVideoEncClass
{
  GstOmxBaseFilterClass parent_class;
};

GType gst_omx_base_videoenc_get_type (void);

G_END_DECLS
#endif /* GSTOMX_BASE_VIDEOENC_H */
