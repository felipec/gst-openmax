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

#ifndef GSTOMX_H264ENC_H
#define GSTOMX_H264ENC_H

#include <gst/gst.h>

G_BEGIN_DECLS
#define GST_OMX_H264ENC(obj) (GstOmxH264Enc *) (obj)
#define GST_OMX_H264ENC_TYPE (gst_omx_h264enc_get_type ())
typedef struct GstOmxH264Enc GstOmxH264Enc;
typedef struct GstOmxH264EncClass GstOmxH264EncClass;

#include "gstomx_base_videoenc.h"

struct GstOmxH264Enc
{
  GstOmxBaseVideoEnc omx_base;
};

struct GstOmxH264EncClass
{
  GstOmxBaseVideoEncClass parent_class;
};

GType gst_omx_h264enc_get_type (void);

G_END_DECLS
#endif /* GSTOMX_H264ENC_H */
