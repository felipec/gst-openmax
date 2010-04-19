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

#ifndef GSTOMX_AMRWBDEC_H
#define GSTOMX_AMRWBDEC_H

#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_OMX_AMRWBDEC(obj) (GstOmxAmrWbDec *) (obj)
#define GST_OMX_AMRWBDEC_TYPE (gst_omx_amrwbdec_get_type ())

typedef struct GstOmxAmrWbDec GstOmxAmrWbDec;
typedef struct GstOmxAmrWbDecClass GstOmxAmrWbDecClass;

#include "gstomx_base_audiodec.h"

struct GstOmxAmrWbDec
{
    GstOmxBaseAudioDec omx_base;
};

struct GstOmxAmrWbDecClass
{
    GstOmxBaseAudioDecClass parent_class;
};

GType gst_omx_amrwbdec_get_type (void);

G_END_DECLS

#endif /* GSTOMX_AMRWBDEC_H */
