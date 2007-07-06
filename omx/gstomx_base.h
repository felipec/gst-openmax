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

#ifndef __GST_OMX_BASE_H__
#define __GST_OMX_BASE_H__

#include <gst/gst.h>

#include <config.h>

G_BEGIN_DECLS

#define GST_OMX_BASE(obj) (GstOmxBase *) (obj)
#define GST_OMX_BASE_TYPE (gst_omx_base_get_type ())
#define GST_OMX_BASE_CLASS(obj) (GstOmxBaseClass *) (obj)

typedef struct GstOmxBase GstOmxBase;
typedef struct GstOmxBaseClass GstOmxBaseClass;
typedef void (*GstOmxBaseCb) (GstOmxBase *self);

#include <gstomx_util.h>

struct GstOmxBase
{
    GstElement element;

    GstPad *sinkpad;
    GstPad *srcpad;

    GOmxCore *gomx;
    GOmxPort *in_port;
    GOmxPort *out_port;

    gboolean started;
    GThread *thread;

    const char *omx_component;
};

struct GstOmxBaseClass
{
    GstElementClass parent_class;
};

GType gst_omx_base_get_type (void);

G_END_DECLS

#endif /* __GST_OMX_BASE_H__ */
