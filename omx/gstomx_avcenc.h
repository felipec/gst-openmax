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

#ifndef GSTOMX_AVCENC_H
#define GSTOMX_AVCENC_H

#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_OMX_AVCENC(obj) (GstOmxAvcEnc *) (obj)
#define GST_OMX_AVCENC_TYPE (gst_omx_avcenc_get_type ())

typedef struct GstOmxAvcEnc GstOmxAvcEnc;
typedef struct GstOmxAvcEncClass GstOmxAvcEncClass;

#include "gstomx_base_videoenc.h"

struct GstOmxAvcEnc
{
    GstOmxBaseVideoEnc omx_base;
};

struct GstOmxAvcEncClass
{
    GstOmxBaseVideoEncClass parent_class;
};

GType gst_omx_avcenc_get_type (void);

G_END_DECLS

#endif /* GSTOMX_AVCENC_H */
