/*
 * Copyright (C) 2008 Nokia Corporation.
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

#include "gstomx_interface.h"

#include "gstomx_util.h"

gboolean
gst_omx_setup_tunnel (GstPad *src_pad,
                      GstPad *sink_pad)
{
    GOmxPort *src_port;
    GOmxPort *sink_port;

    {
        GstElement *element;
        gboolean enabled;

        enabled = FALSE;
        element = gst_pad_get_parent_element (src_pad);
        if (GST_IS_OMX (element))
        {
            g_object_get (G_OBJECT (element), "tunneling", &enabled, NULL);
            gst_object_unref (element);
        }

        if (!enabled)
            return FALSE;

        enabled = FALSE;
        element = gst_pad_get_parent_element (sink_pad);
        if (GST_IS_OMX (element))
        {
            g_object_get (G_OBJECT (element), "tunneling", &enabled, NULL);
            gst_object_unref (element);
        }

        if (!enabled)
            return FALSE;
    }

    src_port = gst_pad_get_element_private (src_pad);
    sink_port = gst_pad_get_element_private (sink_pad);

    return g_omx_core_setup_tunnel (src_port, sink_port);
}

GType
gst_omx_get_type (void)
{
    static GType type = 0;

    if (G_UNLIKELY (type == 0))
    {
        GTypeInfo *type_info;

        type_info = g_new0 (GTypeInfo, 1);

        type_info->class_size = sizeof (GstOmxClass);

        type = g_type_register_static (G_TYPE_INTERFACE, "GstOmx", type_info, 0);
        g_type_interface_add_prerequisite (type, GST_TYPE_IMPLEMENTS_INTERFACE);
    }

    return type;
}
