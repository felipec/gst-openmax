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

#ifndef GSTOMX_H264DEC_H
#define GSTOMX_H264DEC_H

#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_OMX_H264DEC(obj) (GstOmxH264Dec *) (obj)
#define GST_OMX_H264DEC_TYPE (gst_omx_h264dec_get_type ())

typedef struct GstOmxH264Dec GstOmxH264Dec;
typedef struct GstOmxH264DecClass GstOmxH264DecClass;

#include "gstomx_base_videodec.h"

struct GstOmxH264Dec
{
    GstOmxBaseVideoDec omx_base;
};

struct GstOmxH264DecClass
{
    GstOmxBaseVideoDecClass parent_class;
};

GType gst_omx_h264dec_get_type (void);

G_END_DECLS

#endif /* GSTOMX_H264DEC_H */
