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

#ifndef GSTOMX_BASE_FILTER_H
#define GSTOMX_BASE_FILTER_H

#include <gst/gst.h>

G_BEGIN_DECLS
#define GST_OMX_BASE_FILTER(obj) (GstOmxBaseFilter *) (obj)
#define GST_OMX_BASE_FILTER_TYPE (gst_omx_base_filter_get_type ())
#define GST_OMX_BASE_FILTER_CLASS(obj) (GstOmxBaseFilterClass *) (obj)
typedef struct GstOmxBaseFilter GstOmxBaseFilter;
typedef struct GstOmxBaseFilterClass GstOmxBaseFilterClass;
typedef void (*GstOmxBaseFilterCb) (GstOmxBaseFilter * self);

#include "gstomx_util.h"
#include <async_queue.h>

struct GstOmxBaseFilter
{
  GstElement element;

  GstPad *sinkpad;
  GstPad *srcpad;

  GOmxCore *gomx;
  GOmxPort *in_port;
  GOmxPort *out_port;

  gboolean use_timestamps;   /** @todo remove; timestamps should always be used */
  gboolean ready;
  GMutex *ready_lock;

  GstOmxBaseFilterCb omx_setup;
  GstFlowReturn last_pad_push_return;
  GstBuffer *codec_data;

    /** @todo these are hacks, OpenMAX IL spec should be revised. */
  gboolean share_input_buffer;
  gboolean share_output_buffer;
};

struct GstOmxBaseFilterClass
{
  GstElementClass parent_class;
};

GType gst_omx_base_filter_get_type (void);

G_END_DECLS
#endif /* GSTOMX_BASE_FILTER_H */
