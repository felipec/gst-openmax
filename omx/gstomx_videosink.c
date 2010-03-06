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

#include "gstomx_videosink.h"
#include "gstomx_base_sink.h"
#include "gstomx.h"

#include <string.h> /* for memset, strcmp */

static GstOmxBaseSinkClass *parent_class;

enum
{
    ARG_0,
    ARG_X_SCALE,
    ARG_Y_SCALE,
    ARG_ROTATION,
};

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
        GstElementDetails details;

        details.longname = "OpenMAX IL videosink element";
        details.klass = "Video/Sink";
        details.description = "Renders video";
        details.author = "Felipe Contreras";

        gst_element_class_set_details (element_class, &details);
    }

    {
        GstPadTemplate *template;

        template = gst_pad_template_new ("sink", GST_PAD_SINK,
                                         GST_PAD_ALWAYS,
                                         generate_sink_template ());

        gst_element_class_add_pad_template (element_class, template);
    }
}

static gboolean
setcaps (GstBaseSink *gst_sink,
         GstCaps *caps)
{
    GstOmxBaseSink *omx_base;
    GstOmxVideoSink *self;
    GOmxCore *gomx;

    omx_base = GST_OMX_BASE_SINK (gst_sink);
    self = GST_OMX_VIDEOSINK (gst_sink);
    gomx = (GOmxCore *) omx_base->gomx;

    GST_INFO_OBJECT (omx_base, "setcaps (sink): %" GST_PTR_FORMAT, caps);

    g_return_val_if_fail (gst_caps_get_size (caps) == 1, FALSE);

    {
        GstStructure *structure;
        const GValue *framerate = NULL;
        gint width;
        gint height;
        OMX_COLOR_FORMATTYPE color_format = OMX_COLOR_FormatUnused;

        structure = gst_caps_get_structure (caps, 0);

        gst_structure_get_int (structure, "width", &width);
        gst_structure_get_int (structure, "height", &height);

        if (strcmp (gst_structure_get_name (structure), "video/x-raw-yuv") == 0)
        {
            guint32 fourcc;

            framerate = gst_structure_get_value (structure, "framerate");

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

            param.nPortIndex = 0;
            OMX_GetParameter (gomx->omx_handle, OMX_IndexParamPortDefinition, &param);

            switch (color_format)
            {
                case OMX_COLOR_FormatYUV420PackedPlanar:
                    param.nBufferSize = (width * height * 1.5);
                    break;
                case OMX_COLOR_FormatYCbYCr:
                case OMX_COLOR_FormatCbYCrY:
                    param.nBufferSize = (width * height * 2);
                    break;
                default:
                  break;
            }

            param.format.video.nFrameWidth = width;
            param.format.video.nFrameHeight = height;
            param.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
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

        {
            OMX_CONFIG_ROTATIONTYPE config;

            memset (&config, 0, sizeof (config));
            config.nSize = sizeof (OMX_CONFIG_ROTATIONTYPE);
            config.nVersion.s.nVersionMajor = 1;
            config.nVersion.s.nVersionMinor = 1;

            config.nPortIndex = 0;
            OMX_GetConfig (gomx->omx_handle, OMX_IndexConfigCommonScale, &config);

            config.nRotation = self->rotation;

            OMX_SetConfig (gomx->omx_handle, OMX_IndexConfigCommonRotate, &config);
        }

        {
            OMX_CONFIG_SCALEFACTORTYPE config;

            memset (&config, 0, sizeof (config));
            config.nSize = sizeof (OMX_CONFIG_SCALEFACTORTYPE);
            config.nVersion.s.nVersionMajor = 1;
            config.nVersion.s.nVersionMinor = 1;

            config.nPortIndex = 0;
            OMX_GetConfig (gomx->omx_handle, OMX_IndexConfigCommonScale, &config);

            config.xWidth = self->x_scale;
            config.xHeight = self->y_scale;

            OMX_SetConfig (gomx->omx_handle, OMX_IndexConfigCommonScale, &config);
        }
    }

    return TRUE;
}

static void
set_property (GObject *object,
              guint prop_id,
              const GValue *value,
              GParamSpec *pspec)
{
    GstOmxVideoSink *self;

    self = GST_OMX_VIDEOSINK (object);

    switch (prop_id)
    {
        case ARG_X_SCALE:
            self->x_scale = g_value_get_uint (value);
            break;
        case ARG_Y_SCALE:
            self->y_scale = g_value_get_uint (value);
            break;
        case ARG_ROTATION:
            self->rotation = g_value_get_uint (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
get_property (GObject *object,
              guint prop_id,
              GValue *value,
              GParamSpec *pspec)
{
    GstOmxVideoSink *self;

    self = GST_OMX_VIDEOSINK (object);

    switch (prop_id)
    {
        case ARG_X_SCALE:
            g_value_set_uint (value, self->x_scale);
            break;
        case ARG_Y_SCALE:
            g_value_set_uint (value, self->y_scale);
            break;
        case ARG_ROTATION:
            g_value_set_uint (value, self->rotation);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
type_class_init (gpointer g_class,
                 gpointer class_data)
{
    GObjectClass *gobject_class;
    GstBaseSinkClass *gst_base_sink_class;

    gobject_class = (GObjectClass *) g_class;
    gst_base_sink_class = GST_BASE_SINK_CLASS (g_class);

    parent_class = g_type_class_ref (GST_OMX_BASE_SINK_TYPE);

    gst_base_sink_class->set_caps = setcaps;

    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    g_object_class_install_property (gobject_class, ARG_X_SCALE,
                                     g_param_spec_uint ("x-scale", "X Scale",
                                                        "How much to scale the image in the X axis (100 means nothing)",
                                                        0, G_MAXUINT, 100, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, ARG_Y_SCALE,
                                     g_param_spec_uint ("y-scale", "Y Scale",
                                                        "How much to scale the image in the Y axis (100 means nothing)",
                                                        0, G_MAXUINT, 100, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, ARG_ROTATION,
                                     g_param_spec_uint ("rotation", "Rotation",
                                                        "Rotation angle",
                                                        0, G_MAXUINT, 360, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
type_instance_init (GTypeInstance *instance,
                    gpointer g_class)
{
    GstOmxBaseSink *omx_base;

    omx_base = GST_OMX_BASE_SINK (instance);

    GST_DEBUG_OBJECT (omx_base, "start");
}

GType
gst_omx_videosink_get_type (void)
{
    static GType type = 0;

    if (G_UNLIKELY (type == 0))
    {
        GTypeInfo *type_info;

        type_info = g_new0 (GTypeInfo, 1);
        type_info->class_size = sizeof (GstOmxVideoSinkClass);
        type_info->base_init = type_base_init;
        type_info->class_init = type_class_init;
        type_info->instance_size = sizeof (GstOmxVideoSink);
        type_info->instance_init = type_instance_init;

        type = g_type_register_static (GST_OMX_BASE_SINK_TYPE, "GstOmxVideoSink", type_info, 0);

        g_free (type_info);
    }

    return type;
}
