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

#ifndef __GST_OMX_VORBISDEC_H__
#define __GST_OMX_VORBISDEC_H__

#include <gst/gst.h>

#include <config.h>

G_BEGIN_DECLS

#define GST_OMX_VORBISDEC(obj) (GstOmxVorbisDec *) (obj)
#define GST_OMX_VORBISDEC_TYPE (gst_omx_vorbisdec_get_type ())

typedef struct GstOmxVorbisDec GstOmxVorbisDec;
typedef struct GstOmxVorbisDecClass GstOmxVorbisDecClass;

#include "gstomx_base.h"

struct GstOmxVorbisDec
{
    GstOmxBase omx_base;
};

struct GstOmxVorbisDecClass 
{
    GstOmxBaseClass parent_class;
};

GType gst_omx_vorbisdec_get_type (void);

G_END_DECLS

#endif /* __GST_OMX_VORBISDEC_H__ */
