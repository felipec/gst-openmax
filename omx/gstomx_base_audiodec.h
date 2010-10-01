/*
 * Copyright (C) 2009 Texas Instruments, Inc.
 *
 * Author: Rob Clark <rob@ti.com>
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

#ifndef GSTOMX_BASE_AUDIODEC_H
#define GSTOMX_BASE_AUDIODEC_H

#include <gst/gst.h>

G_BEGIN_DECLS
#define GST_OMX_BASE_AUDIODEC(obj) (GstOmxBaseAudioDec *) (obj)
#define GST_OMX_BASE_AUDIODEC_TYPE (gst_omx_base_audiodec_get_type ())
typedef struct GstOmxBaseAudioDec GstOmxBaseAudioDec;
typedef struct GstOmxBaseAudioDecClass GstOmxBaseAudioDecClass;

#include "gstomx_base_filter.h"

struct GstOmxBaseAudioDec
{
  GstOmxBaseFilter omx_base;
};

struct GstOmxBaseAudioDecClass
{
  GstOmxBaseFilterClass parent_class;
};

GType gst_omx_base_audiodec_get_type (void);

G_END_DECLS
#endif /* GSTOMX_BASE_AUDIODEC_H */
