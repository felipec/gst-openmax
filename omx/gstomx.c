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

#include "config.h"

#include "gstomx.h"
#include "gstomx_dummy.h"
#include "gstomx_mpeg4dec.h"
#include "gstomx_h263dec.h"
#include "gstomx_h264dec.h"
#include "gstomx_mp3dec.h"
#include "gstomx_mp2dec.h"
#include "gstomx_aacdec.h"
#include "gstomx_amrnbdec.h"
#include "gstomx_amrwbdec.h"

GST_DEBUG_CATEGORY (gstomx_debug);

typedef struct TableItem
{
    const gchar *name;
    const gchar *library_name;
    const gchar *component_name;
    guint rank;
    GType (*get_type) (void);
} TableItem;

static TableItem element_table[] =
{
    { "omx_dummy", "libomxil-bellagio.so.0", "OMX.st.dummy", GST_RANK_NONE, gst_omx_dummy_get_type },
    { "omx_mpeg4dec", "libomxil-bellagio.so.0", "OMX.PV.mpeg4dec", GST_RANK_PRIMARY, gst_omx_mpeg4dec_get_type },
    { "omx_h264dec", "libomxil-bellagio.so.0", "OMX.PV.avcdec", GST_RANK_PRIMARY, gst_omx_h264dec_get_type },
    { "omx_h263dec", "libomxil-bellagio.so.0", "OMX.PV.h263dec", GST_RANK_PRIMARY, gst_omx_h263dec_get_type },
    { "omx_mp3dec", "libomxil-bellagio.so.0", "OMX.PV.mp3dec", GST_RANK_PRIMARY, gst_omx_mp3dec_get_type },
    { "omx_mp2dec", "libomxil-bellagio.so.0", "OMX.PV.mp3dec", GST_RANK_PRIMARY, gst_omx_mp2dec_get_type },
    { "omx_amrnbdec", "libomxil-bellagio.so.0", "OMX.PV.amrdec", GST_RANK_PRIMARY, gst_omx_amrnbdec_get_type },
    { "omx_amrwbdec", "libomxil-bellagio.so.0", "OMX.PV.amrdec", GST_RANK_PRIMARY, gst_omx_amrwbdec_get_type },
    { "omx_aacdec", "libomxil-bellagio.so.0", "OMX.PV.aacdec", GST_RANK_PRIMARY, gst_omx_aacdec_get_type },
    { NULL, NULL, NULL, 0, NULL },
};

static gboolean
plugin_init (GstPlugin *plugin)
{
    GQuark library_name_quark;
    GQuark component_name_quark;
    GST_DEBUG_CATEGORY_INIT (gstomx_debug, "omx", 0, "gst-openmax");
    GST_DEBUG_CATEGORY_INIT (gstomx_util_debug, "omx_util", 0, "gst-openmax utility");

    library_name_quark = g_quark_from_static_string ("library-name");
    component_name_quark = g_quark_from_static_string ("component-name");

    g_omx_init ();

    {
        guint i;
        for (i = 0; element_table[i].name; i++)
        {
            TableItem *element;
            GType type;

            element = &element_table[i];
            type = element->get_type ();
            g_type_set_qdata (type, library_name_quark, (gpointer) element->library_name);
            g_type_set_qdata (type, component_name_quark, (gpointer) element->component_name);

            if (!gst_element_register (plugin, element->name, element->rank, type))
            {
                g_warning ("failed registering '%s'", element->name);
                return FALSE;
            }
        }
    }

    return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
                   GST_VERSION_MINOR,
                   "omx",
                   "OpenMAX IL",
                   plugin_init,
                   PACKAGE_VERSION,
                   GST_LICENSE,
                   GST_PACKAGE_NAME,
                   GST_PACKAGE_ORIGIN)
