/*
 * Copyright (C) 2007-2008 Nokia Corporation. All rights reserved.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#ifndef GSTOMX_H263ENC_H
#define GSTOMX_H263ENC_H

#include <gst/gst.h>

#include <config.h>

G_BEGIN_DECLS

#define GST_OMX_H263ENC(obj) (GstOmxH263Enc *) (obj)
#define GST_OMX_H263ENC_TYPE (gst_omx_h263enc_get_type ())

typedef struct GstOmxH263Enc GstOmxH263Enc;
typedef struct GstOmxH263EncClass GstOmxH263EncClass;

#include "gstomx_base_videoenc.h"

struct GstOmxH263Enc
{
    GstOmxBaseVideoEnc omx_base;
};

struct GstOmxH263EncClass
{
    GstOmxBaseVideoEncClass parent_class;
};

GType gst_omx_h263enc_get_type (void);

G_END_DECLS

#endif /* GSTOMX_H263ENC_H */
