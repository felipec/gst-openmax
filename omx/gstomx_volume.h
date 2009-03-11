/*
 * Copyright (C) 2007-2009 Nokia Corporation.
 * Copyright (C) 2008 NXP.
 *
 * Author: Frederik Vernelen <frederik.vernelen@nxp.com>
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

#ifndef GSTOMX_VOLUME_H
#define GSTOMX_VOLUME_H

#include <gst/gst.h>

#include <config.h>

G_BEGIN_DECLS

#define GST_OMX_VOLUME(obj) (GstOmxVolume *) (obj)
#define GST_OMX_VOLUME_TYPE (gst_omx_volume_get_type ())

typedef struct GstOmxVolume GstOmxVolume;
typedef struct GstOmxVolumeClass GstOmxVolumeClass;

#include "gstomx_base_filter.h"

struct GstOmxVolume
{
    GstOmxBaseFilter omx_base;
};

struct GstOmxVolumeClass
{
    GstOmxBaseFilterClass parent_class;
};

GType gst_omx_volume_get_type (void);

G_END_DECLS

#endif /* GSTOMX_VOLUME_H */
