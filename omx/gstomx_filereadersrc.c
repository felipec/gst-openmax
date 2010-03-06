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

#include "gstomx_filereadersrc.h"
#include "gstomx_base_src.h"
#include "gstomx.h"

enum
{
    ARG_0,
    ARG_FILE_NAME,
};

static GstOmxBaseSrcClass *parent_class;

static GstCaps *
generate_src_template (void)
{
    GstCaps *caps;

    caps = gst_caps_new_any ();

    return caps;
}

static void
type_base_init (gpointer g_class)
{
    GstElementClass *element_class;

    element_class = GST_ELEMENT_CLASS (g_class);

    {
        GstElementDetails details;

        details.longname = "OpenMAX IL filereader src element";
        details.klass = "None";
        details.description = "Does nothing";
        details.author = "Felipe Contreras";

        gst_element_class_set_details (element_class, &details);
    }

    {
        GstPadTemplate *template;

        template = gst_pad_template_new ("src", GST_PAD_SRC,
                                         GST_PAD_ALWAYS,
                                         generate_src_template ());

        gst_element_class_add_pad_template (element_class, template);
    }
}

static gboolean
setcaps (GstBaseSrc *gst_src,
         GstCaps *caps)
{
    GstOmxBaseSrc *self;

    self = GST_OMX_BASE_SRC (gst_src);

    GST_INFO_OBJECT (self, "setcaps (src): %" GST_PTR_FORMAT, caps);

    g_return_val_if_fail (gst_caps_get_size (caps) == 1, FALSE);

    return TRUE;
}

static void
settings_changed_cb (GOmxCore *core)
{
    GstOmxBaseSrc *omx_base;

    omx_base = core->object;

    GST_DEBUG_OBJECT (omx_base, "settings changed");

    /** @todo properly set the capabilities */
}

static void
setup_ports (GstOmxBaseSrc *base_src)
{
    GOmxCore *gomx;
    GstOmxFilereaderSrc *self;

    self = GST_OMX_FILEREADERSRC (base_src);
    gomx = base_src->gomx;

    /* This is specific for Bellagio. */
    {
        OMX_INDEXTYPE index;
        OMX_GetExtensionIndex (gomx->omx_handle, "OMX.ST.index.param.filereader.inputfilename", &index);
        OMX_SetParameter (gomx->omx_handle, index, self->file_name);
    }
}

static void
set_property (GObject *obj,
              guint prop_id,
              const GValue *value,
              GParamSpec *pspec)
{
    GstOmxFilereaderSrc *self;

    self = GST_OMX_FILEREADERSRC (obj);

    switch (prop_id)
    {
        case ARG_FILE_NAME:
            if (self->file_name)
            {
                g_free (self->file_name);
            }
            self->file_name = g_value_dup_string (value);
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
    GstOmxFilereaderSrc *self;

    self = GST_OMX_FILEREADERSRC (obj);

    switch (prop_id)
    {
        case ARG_FILE_NAME:
            g_value_set_string (value, self->file_name);
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
    GstBaseSrcClass *gst_base_src_class;
    GObjectClass *gobject_class;

    parent_class = g_type_class_ref (GST_OMX_BASE_SRC_TYPE);
    gst_base_src_class = GST_BASE_SRC_CLASS (g_class);
    gobject_class = G_OBJECT_CLASS (g_class);

    gst_base_src_class->set_caps = setcaps;

    /* Properties stuff */
    {
        gobject_class->set_property = set_property;
        gobject_class->get_property = get_property;

        g_object_class_install_property (gobject_class, ARG_FILE_NAME,
                                         g_param_spec_string ("file-name", "File name",
                                                              "The input filename to use",
                                                              NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
    }
}

static void
type_instance_init (GTypeInstance *instance,
                    gpointer g_class)
{
    GstOmxBaseSrc *omx_base;

    omx_base = GST_OMX_BASE_SRC (instance);

    GST_DEBUG_OBJECT (omx_base, "begin");

    omx_base->setup_ports = setup_ports;

    omx_base->gomx->settings_changed_cb = settings_changed_cb;

    GST_DEBUG_OBJECT (omx_base, "end");
}

GType
gst_omx_filereadersrc_get_type (void)
{
    static GType type = 0;

    if (G_UNLIKELY (type == 0))
    {
        GTypeInfo *type_info;

        type_info = g_new0 (GTypeInfo, 1);
        type_info->class_size = sizeof (GstOmxFilereaderSrcClass);
        type_info->base_init = type_base_init;
        type_info->class_init = type_class_init;
        type_info->instance_size = sizeof (GstOmxFilereaderSrc);
        type_info->instance_init = type_instance_init;

        type = g_type_register_static (GST_OMX_BASE_SRC_TYPE, "GstOmxFilereaderSrc", type_info, 0);

        g_free (type_info);
    }

    return type;
}
