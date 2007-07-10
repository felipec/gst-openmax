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

#ifndef __GST_OMX_MP3DEC_H__
#define __GST_OMX_MP3DEC_H__

#include <gst/gst.h>

#include <config.h>

G_BEGIN_DECLS

#define GST_OMX_MP3DEC(obj) (GstOmxMp3Dec *) (obj)
#define GST_OMX_MP3DEC_TYPE (gst_omx_mp3dec_get_type ())

typedef struct GstOmxMp3Dec GstOmxMp3Dec;
typedef struct GstOmxMp3DecClass GstOmxMp3DecClass;

#include "gstomx_base_filter.h"

struct GstOmxMp3Dec
{
    GstOmxBaseFilter omx_base;
};

struct GstOmxMp3DecClass 
{
    GstOmxBaseFilterClass parent_class;
};

GType gst_omx_mp3dec_get_type (void);

G_END_DECLS

#endif /* __GST_OMX_MP3DEC_H__ */
