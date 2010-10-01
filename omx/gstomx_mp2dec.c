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

#include "gstomx_mp2dec.h"
#include "gstomx_base_filter.h"
#include "gstomx.h"

GSTOMX_BOILERPLATE (GstOmxMp2Dec, gst_omx_mp2dec, GstOmxBaseAudioDec,
    GST_OMX_BASE_AUDIODEC_TYPE);

static GstCaps *
generate_src_template (void)
{
  GstCaps *caps;

  caps = gst_caps_new_simple ("audio/x-raw-int",
      "endianness", G_TYPE_INT, G_BYTE_ORDER,
      "width", G_TYPE_INT, 16,
      "depth", G_TYPE_INT, 16,
      "rate", GST_TYPE_INT_RANGE, 8000, 96000,
      "signed", G_TYPE_BOOLEAN, TRUE,
      "channels", GST_TYPE_INT_RANGE, 1, 2, NULL);

  return caps;
}

static GstCaps *
generate_sink_template (void)
{
  GstCaps *caps;

  caps = gst_caps_new_simple ("audio/mpeg",
      "mpegversion", G_TYPE_INT, 1,
      "layer", G_TYPE_INT, 2,
      "rate", GST_TYPE_INT_RANGE, 8000, 96000,
      "channels", GST_TYPE_INT_RANGE, 1, 2,
      "parsed", G_TYPE_BOOLEAN, TRUE, NULL);

  return caps;
}

static void
type_base_init (gpointer g_class)
{
  GstElementClass *element_class;

  element_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_set_details_simple (element_class,
      "OpenMAX IL MP2 audio decoder",
      "Codec/Decoder/Audio",
      "Decodes audio in MP2 format with OpenMAX IL", "Felipe Contreras");

  {
    GstPadTemplate *template;

    template = gst_pad_template_new ("src", GST_PAD_SRC,
        GST_PAD_ALWAYS, generate_src_template ());

    gst_element_class_add_pad_template (element_class, template);
  }

  {
    GstPadTemplate *template;

    template = gst_pad_template_new ("sink", GST_PAD_SINK,
        GST_PAD_ALWAYS, generate_sink_template ());

    gst_element_class_add_pad_template (element_class, template);
  }
}

static void
type_class_init (gpointer g_class, gpointer class_data)
{
}

static void
type_instance_init (GTypeInstance * instance, gpointer g_class)
{
}
