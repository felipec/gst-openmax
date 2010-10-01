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

#include "gstomx_h264enc.h"
#include "gstomx.h"

GSTOMX_BOILERPLATE (GstOmxH264Enc, gst_omx_h264enc, GstOmxBaseVideoEnc,
    GST_OMX_BASE_VIDEOENC_TYPE);

static GstCaps *
generate_src_template (void)
{
  GstCaps *caps;

  caps = gst_caps_new_simple ("video/x-h264",
      "width", GST_TYPE_INT_RANGE, 16, 4096,
      "height", GST_TYPE_INT_RANGE, 16, 4096,
      "framerate", GST_TYPE_FRACTION_RANGE, 0, 1, G_MAXINT, 1, NULL);

  return caps;
}

static void
type_base_init (gpointer g_class)
{
  GstElementClass *element_class;

  element_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_set_details_simple (element_class,
      "OpenMAX IL H.264/AVC video encoder",
      "Codec/Encoder/Video",
      "Encodes video in H.264/AVC format with OpenMAX IL", "Felipe Contreras");

  {
    GstPadTemplate *template;

    template = gst_pad_template_new ("src", GST_PAD_SRC,
        GST_PAD_ALWAYS, generate_src_template ());

    gst_element_class_add_pad_template (element_class, template);
  }
}

static void
type_class_init (gpointer g_class, gpointer class_data)
{
}

static void
settings_changed_cb (GOmxCore * core)
{
  GstOmxBaseVideoEnc *omx_base;
  GstOmxBaseFilter *omx_base_filter;
  guint width;
  guint height;

  omx_base_filter = core->object;
  omx_base = GST_OMX_BASE_VIDEOENC (omx_base_filter);

  GST_DEBUG_OBJECT (omx_base, "settings changed");

  {
    OMX_PARAM_PORTDEFINITIONTYPE param;

    G_OMX_INIT_PARAM (param);

    param.nPortIndex = omx_base_filter->out_port->port_index;
    OMX_GetParameter (core->omx_handle, OMX_IndexParamPortDefinition, &param);

    width = param.format.video.nFrameWidth;
    height = param.format.video.nFrameHeight;
  }

  {
    GstCaps *new_caps;

    new_caps = gst_caps_new_simple ("video/x-h264",
        "width", G_TYPE_INT, width,
        "height", G_TYPE_INT, height,
        "framerate", GST_TYPE_FRACTION,
        omx_base->framerate_num, omx_base->framerate_denom, NULL);

    GST_INFO_OBJECT (omx_base, "caps are: %" GST_PTR_FORMAT, new_caps);
    gst_pad_set_caps (omx_base_filter->srcpad, new_caps);
  }
}

static void
type_instance_init (GTypeInstance * instance, gpointer g_class)
{
  GstOmxBaseFilter *omx_base_filter;
  GstOmxBaseVideoEnc *omx_base;

  omx_base_filter = GST_OMX_BASE_FILTER (instance);
  omx_base = GST_OMX_BASE_VIDEOENC (instance);

  omx_base->compression_format = OMX_VIDEO_CodingAVC;

  omx_base_filter->gomx->settings_changed_cb = settings_changed_cb;
}
