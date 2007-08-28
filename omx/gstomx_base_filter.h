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

#ifndef __GST_OMX_BASE_FILTER_H__
#define __GST_OMX_BASE_FILTER_H__

#include <gst/gst.h>

#include <config.h>

G_BEGIN_DECLS

#define GST_OMX_BASE_FILTER(obj) (GstOmxBaseFilter *) (obj)
#define GST_OMX_BASE_FILTER_TYPE (gst_omx_base_filter_get_type ())
#define GST_OMX_BASE_FILTER_CLASS(obj) (GstOmxBaseFilterClass *) (obj)

typedef struct GstOmxBaseFilter GstOmxBaseFilter;
typedef struct GstOmxBaseFilterClass GstOmxBaseFilterClass;
typedef void (*GstOmxBaseFilterCb) (GstOmxBaseFilter *self);

#include <gstomx_util.h>

struct GstOmxBaseFilter
{
    GstElement element;

    GstPad *sinkpad;
    GstPad *srcpad;

    GOmxCore *gomx;
    GOmxPort *in_port;
    GOmxPort *out_port;

    GThread *thread;

    const char *omx_component;
    gboolean use_timestamps;
};

struct GstOmxBaseFilterClass
{
    GstElementClass parent_class;
};

GType gst_omx_base_filter_get_type (void);

G_END_DECLS

#endif /* __GST_OMX_BASE_FILTER_H__ */
