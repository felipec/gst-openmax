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

#include "gstomx_base_videoenc.h"
#include "gstomx.h"

#include <string.h> /* for memset, strcmp */

enum
{
    ARG_0,
    ARG_BITRATE,
};

#define DEFAULT_BITRATE 500000

static GstOmxBaseFilterClass *parent_class;

static GstCaps *
generate_sink_template (void)
{
    GstCaps *caps;
    GstStructure *struc;

    caps = gst_caps_new_empty ();

    struc = gst_structure_new ("video/x-raw-yuv",
                               "width", GST_TYPE_INT_RANGE, 16, 4096,
                               "height", GST_TYPE_INT_RANGE, 16, 4096,
                               "framerate", GST_TYPE_FRACTION_RANGE, 0, 1, G_MAXINT, 1,
                               NULL);

    {
        GValue list;
        GValue val;

        list.g_type = val.g_type = 0;

        g_value_init (&list, GST_TYPE_LIST);
        g_value_init (&val, GST_TYPE_FOURCC);

        gst_value_set_fourcc (&val, GST_MAKE_FOURCC ('I', '4', '2', '0'));
        gst_value_list_append_value (&list, &val);

        gst_value_set_fourcc (&val, GST_MAKE_FOURCC ('Y', 'U', 'Y', '2'));
        gst_value_list_append_value (&list, &val);

        gst_value_set_fourcc (&val, GST_MAKE_FOURCC ('U', 'Y', 'V', 'Y'));
        gst_value_list_append_value (&list, &val);

        gst_structure_set_value (struc, "format", &list);

        g_value_unset (&val);
        g_value_unset (&list);
    }

    gst_caps_append_structure (caps, struc);

    return caps;
}

static void
type_base_init (gpointer g_class)
{
    GstElementClass *element_class;

    element_class = GST_ELEMENT_CLASS (g_class);

    {
        GstPadTemplate *template;

        template = gst_pad_template_new ("sink", GST_PAD_SINK,
                                         GST_PAD_ALWAYS,
                                         generate_sink_template ());

        gst_element_class_add_pad_template (element_class, template);
    }
}

static void
set_property (GObject *obj,
              guint prop_id,
              const GValue *value,
              GParamSpec *pspec)
{
    GstOmxBaseVideoEnc *self;

    self = GST_OMX_BASE_VIDEOENC (obj);

    switch (prop_id)
    {
        case ARG_BITRATE:
            self->bitrate = g_value_get_uint (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
            break;
    }
}

static void
get_property (GObject *obj,
              guint prop_id,
              GValue *value,
              GParamSpec *pspec)
{
    GstOmxBaseVideoEnc *self;

    self = GST_OMX_BASE_VIDEOENC (obj);

    switch (prop_id)
    {
        case ARG_BITRATE:
            /** @todo propagate this to OpenMAX when processing. */
            g_value_set_uint (value, self->bitrate);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
            break;
    }
}

static void
type_class_init (gpointer g_class,
                 gpointer class_data)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS (g_class);

    parent_class = g_type_class_ref (GST_OMX_BASE_FILTER_TYPE);

    /* Properties stuff */
    {
        gobject_class->set_property = set_property;
        gobject_class->get_property = get_property;

        g_object_class_install_property (gobject_class, ARG_BITRATE,
                                         g_param_spec_uint ("bitrate", "Bit-rate",
                                                            "Encoding bit-rate",
                                                            0, G_MAXUINT, DEFAULT_BITRATE, G_PARAM_READWRITE));
    }
}

static gboolean
sink_setcaps (GstPad *pad,
              GstCaps *caps)
{
    GstStructure *structure;
    GstOmxBaseVideoEnc *self;
    GstOmxBaseFilter *omx_base;
    GOmxCore *gomx;
    OMX_COLOR_FORMATTYPE color_format = OMX_COLOR_FormatUnused;
    gint width = 0;
    gint height = 0;
    const GValue *framerate = NULL;

    self = GST_OMX_BASE_VIDEOENC (GST_PAD_PARENT (pad));
    omx_base = GST_OMX_BASE_FILTER (self);
    gomx = (GOmxCore *) omx_base->gomx;

    GST_INFO_OBJECT (self, "setcaps (sink): %" GST_PTR_FORMAT, caps);

    g_return_val_if_fail (gst_caps_get_size (caps) == 1, FALSE);

    structure = gst_caps_get_structure (caps, 0);

    gst_structure_get_int (structure, "width", &width);
    gst_structure_get_int (structure, "height", &height);

    if (strcmp (gst_structure_get_name (structure), "video/x-raw-yuv") == 0)
    {
        guint32 fourcc;

        framerate = gst_structure_get_value (structure, "framerate");
        if (framerate)
        {
            self->framerate_num = gst_value_get_fraction_numerator (framerate);
            self->framerate_denom = gst_value_get_fraction_denominator (framerate);
        }

        if (gst_structure_get_fourcc (structure, "format", &fourcc))
        {
            switch (fourcc)
            {
                case GST_MAKE_FOURCC ('I', '4', '2', '0'):
                    color_format = OMX_COLOR_FormatYUV420PackedPlanar;
                    break;
                case GST_MAKE_FOURCC ('Y', 'U', 'Y', '2'):
                    color_format = OMX_COLOR_FormatYCbYCr;
                    break;
                case GST_MAKE_FOURCC ('U', 'Y', 'V', 'Y'):
                    color_format = OMX_COLOR_FormatCbYCrY;
                    break;
            }
        }
    }

    {
        OMX_PARAM_PORTDEFINITIONTYPE param;
        memset (&param, 0, sizeof (param));
        param.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
        param.nVersion.s.nVersionMajor = 1;
        param.nVersion.s.nVersionMinor = 1;

        /* Input port configuration. */
        {
            param.nPortIndex = 0;
            OMX_GetParameter (gomx->omx_handle, OMX_IndexParamPortDefinition, &param);

            param.format.video.nFrameWidth = width;
            param.format.video.nFrameHeight = height;
            param.format.video.eColorFormat = color_format;
            if (framerate)
            {
                /* convert to Q.16 */
                param.format.video.xFramerate =
                    (gst_value_get_fraction_numerator (framerate) << 16) /
                    gst_value_get_fraction_denominator (framerate);
            }

            OMX_SetParameter (gomx->omx_handle, OMX_IndexParamPortDefinition, &param);
        }
    }

    return gst_pad_set_caps (pad, caps);
}

static void
omx_setup (GstOmxBaseFilter *omx_base)
{
    GstOmxBaseVideoEnc *self;
    GOmxCore *gomx;

    self = GST_OMX_BASE_VIDEOENC (omx_base);
    gomx = (GOmxCore *) omx_base->gomx;

    GST_INFO_OBJECT (omx_base, "begin");

    {
        OMX_PARAM_PORTDEFINITIONTYPE param;

        memset (&param, 0, sizeof (param));
        param.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
        param.nVersion.s.nVersionMajor = 1;
        param.nVersion.s.nVersionMinor = 1;

        /* Output port configuration. */
        {
            param.nPortIndex = 1;
            OMX_GetParameter (gomx->omx_handle, OMX_IndexParamPortDefinition, &param);

            param.format.video.eCompressionFormat = self->compression_format;

            /** @todo this should be set with a property */
            param.format.video.nBitrate = self->bitrate;

            OMX_SetParameter (gomx->omx_handle, OMX_IndexParamPortDefinition, &param);
        }

        /* some workarounds required for TI components. */
        {
            OMX_COLOR_FORMATTYPE color_format;
            gint width, height;
            gulong framerate;

            /* the component should do this instead */
            {
                param.nPortIndex = 0;
                OMX_GetParameter (gomx->omx_handle, OMX_IndexParamPortDefinition, &param);

                width = param.format.video.nFrameWidth;
                height = param.format.video.nFrameHeight;
                framerate = param.format.video.xFramerate;
                color_format = param.format.video.eColorFormat;

                /* this is against the standard; nBufferSize is read-only. */
                switch (color_format)
                {
                    case OMX_COLOR_FormatYCbYCr:
                    case OMX_COLOR_FormatCbYCrY:
                        param.nBufferSize = (width * height) * 2;
                        break;
                    case OMX_COLOR_FormatYUV420PackedPlanar:
                        param.nBufferSize = (width * height) * 3 / 2;
                        break;
                    default:
                        break;
                }

                OMX_SetParameter (gomx->omx_handle, OMX_IndexParamPortDefinition, &param);
            }

            /* the component should do this instead */
            {
                param.nPortIndex = 1;
                OMX_GetParameter (gomx->omx_handle, OMX_IndexParamPortDefinition, &param);

                /* this is against the standard; nBufferSize is read-only. */
                param.nBufferSize = width * height;

                param.format.video.nFrameWidth = width;
                param.format.video.nFrameHeight = height;
                param.format.video.xFramerate = framerate;

                OMX_SetParameter (gomx->omx_handle, OMX_IndexParamPortDefinition, &param);
            }
        }
    }

    if (self->omx_setup)
        self->omx_setup (GST_OMX_BASE_FILTER (self));

    GST_INFO_OBJECT (omx_base, "end");
}

static void
type_instance_init (GTypeInstance *instance,
                    gpointer g_class)
{
    GstOmxBaseFilter *omx_base;
    GstOmxBaseVideoEnc *self;

    omx_base = GST_OMX_BASE_FILTER (instance);
    self = GST_OMX_BASE_VIDEOENC (instance);

    omx_base->omx_setup = omx_setup;

    gst_pad_set_setcaps_function (omx_base->sinkpad, sink_setcaps);

    self->bitrate = DEFAULT_BITRATE;
}

GType
gst_omx_base_videoenc_get_type (void)
{
    static GType type = 0;

    if (G_UNLIKELY (type == 0))
    {
        GTypeInfo *type_info;

        type_info = g_new0 (GTypeInfo, 1);
        type_info->class_size = sizeof (GstOmxBaseVideoEncClass);
        type_info->base_init = type_base_init;
        type_info->class_init = type_class_init;
        type_info->instance_size = sizeof (GstOmxBaseVideoEnc);
        type_info->instance_init = type_instance_init;

        type = g_type_register_static (GST_OMX_BASE_FILTER_TYPE, "GstOmxBaseVideoEnc", type_info, 0);

        g_free (type_info);
    }

    return type;
}
