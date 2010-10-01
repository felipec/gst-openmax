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

#include <string.h>

#include <gst/gststructure.h>

#include "gstomx.h"
#include "gstomx_dummy.h"
#include "gstomx_mpeg4dec.h"
#include "gstomx_h263dec.h"
#include "gstomx_h264dec.h"
#include "gstomx_wmvdec.h"
#include "gstomx_mpeg4enc.h"
#include "gstomx_h264enc.h"
#include "gstomx_h263enc.h"
#include "gstomx_vorbisdec.h"
#include "gstomx_mp3dec.h"
#ifdef EXPERIMENTAL
#include "gstomx_mp2dec.h"
#include "gstomx_aacdec.h"
#include "gstomx_aacenc.h"
#include "gstomx_amrnbdec.h"
#include "gstomx_amrnbenc.h"
#include "gstomx_amrwbdec.h"
#include "gstomx_amrwbenc.h"
#include "gstomx_adpcmdec.h"
#include "gstomx_adpcmenc.h"
#include "gstomx_g711dec.h"
#include "gstomx_g711enc.h"
#include "gstomx_g729dec.h"
#include "gstomx_g729enc.h"
#include "gstomx_ilbcdec.h"
#include "gstomx_ilbcenc.h"
#include "gstomx_jpegenc.h"
#endif /* EXPERIMENTAL */
#include "gstomx_audiosink.h"
#ifdef EXPERIMENTAL
#include "gstomx_videosink.h"
#include "gstomx_filereadersrc.h"
#endif /* EXPERIMENTAL */
#include "gstomx_volume.h"

GST_DEBUG_CATEGORY (gstomx_debug);

static const GstStructure *element_table;
static GQuark element_name_quark;

extern const gchar *default_config;

static GType (*get_type[]) (void) = {
  gst_omx_dummy_get_type,
      gst_omx_mpeg4dec_get_type,
      gst_omx_h264dec_get_type,
      gst_omx_h263dec_get_type,
      gst_omx_wmvdec_get_type,
      gst_omx_mpeg4enc_get_type,
      gst_omx_h264enc_get_type,
      gst_omx_h263enc_get_type,
      gst_omx_vorbisdec_get_type, gst_omx_mp3dec_get_type,
#ifdef EXPERIMENTAL
      gst_omx_mp2dec_get_type,
      gst_omx_amrnbdec_get_type,
      gst_omx_amrnbenc_get_type,
      gst_omx_amrwbdec_get_type,
      gst_omx_amrwbenc_get_type,
      gst_omx_aacdec_get_type,
      gst_omx_aacenc_get_type,
      gst_omx_adpcmdec_get_type,
      gst_omx_adpcmenc_get_type,
      gst_omx_g711dec_get_type,
      gst_omx_g711enc_get_type,
      gst_omx_g729dec_get_type,
      gst_omx_g729enc_get_type,
      gst_omx_ilbcdec_get_type,
      gst_omx_ilbcenc_get_type, gst_omx_jpegenc_get_type,
#endif /* EXPERIMENTAL */
      gst_omx_audiosink_get_type,
#ifdef EXPERIMENTAL
      gst_omx_videosink_get_type, gst_omx_filereadersrc_get_type,
#endif /* EXPERIMENTAL */
gst_omx_volume_get_type,};

static gchar *
get_config_path (void)
{
  gchar *path;
  const gchar *const *dirs;
  int i;

  path = g_strdup (g_getenv ("OMX_CONFIG"));

  if (path)
    return path;

  dirs = g_get_system_config_dirs ();
  for (i = 0; dirs[i]; i++) {
    path =
        g_build_filename (dirs[i], "gstreamer-0.10", "gst-openmax.conf", NULL);
    if (g_file_test (path, G_FILE_TEST_IS_REGULAR))
      return path;
    g_free (path);
  }

  return g_build_filename (g_get_user_config_dir (), "gst-openmax.conf", NULL);
}

static void
fetch_element_table (GstPlugin * plugin)
{
  gchar *path;
  gchar *config, *s;
  GstStructure *tmp, *element;

  element_table = gst_plugin_get_cache_data (plugin);

  if (element_table)
    return;

  path = get_config_path ();

  if (!g_file_get_contents (path, &config, NULL, NULL)) {
    g_warning ("could not find config file '%s'.. using defaults!", path);
    config = (gchar *) default_config;
  }

  gst_plugin_add_dependency_simple (plugin, "ONX_CONFIG", path, NULL,
      GST_PLUGIN_DEPENDENCY_FLAG_NONE);

  g_free (path);

  GST_DEBUG ("parsing config:\n%s", config);

  tmp = gst_structure_empty_new ("element_table");

  s = config;

  while ((element = gst_structure_from_string (s, &s))) {
    const gchar *element_name = gst_structure_get_name (element);
    gst_structure_set (tmp, element_name, GST_TYPE_STRUCTURE, element, NULL);
  }

  if (config != default_config)
    g_free (config);

  GST_DEBUG ("element_table=%" GST_PTR_FORMAT, tmp);

  gst_plugin_set_cache_data (plugin, tmp);

  element_table = tmp;
}

static GstStructure *
get_element_entry (const gchar * element_name)
{
  GstStructure *element;

  if (!gst_structure_get ((GstStructure *) element_table, element_name,
          GST_TYPE_STRUCTURE, &element, NULL)) {
    element = NULL;
  }

  /* This assert should never fail, because plugin elements are registered
   * based on the entries in this table.  Someone would have to manually
   * override the type qdata for this to fail.
   */
  g_assert (element);

  return element;
}

/* register a new dynamic sub-class with the name 'type_name'.. this gives us
 * a way to use the same (for example) GstOmxMp3Dec element mapping to
 * multiple different element names with different OMX library implementations
 * and/or component names
 */
static GType
create_subtype (GType parent_type, const gchar * type_name)
{
  GTypeQuery q;
  GTypeInfo i = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

  if (!type_name)
    return 0;

  g_type_query (parent_type, &q);

  i.class_size = q.class_size;
  i.instance_size = q.instance_size;

  return g_type_register_static (parent_type, type_name, &i, 0);
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  guint i, cnt;

  GST_DEBUG_CATEGORY_INIT (gstomx_debug, "omx", 0, "gst-openmax");
  GST_DEBUG_CATEGORY_INIT (gstomx_util_debug, "omx_util", 0,
      "gst-openmax utility");

  element_name_quark = g_quark_from_static_string ("element-name");

  /*
   * First, call all the _get_type() functions to ensure the types are
   * registered.
   */
  for (i = 0; i < G_N_ELEMENTS (get_type); i++)
    get_type[i] ();

  fetch_element_table (plugin);

  g_omx_init ();

  cnt = gst_structure_n_fields (element_table);
  for (i = 0; i < cnt; i++) {
    const gchar *element_name = gst_structure_nth_field_name (element_table, i);
    GstStructure *element = get_element_entry (element_name);
    const gchar *type_name, *parent_type_name;
    const gchar *component_name, *component_role, *library_name;
    GType type;
    gint rank;

    GST_DEBUG ("element_name=%s, element=%" GST_PTR_FORMAT, element_name,
        element);

    parent_type_name = gst_structure_get_string (element, "parent-type");
    type_name = gst_structure_get_string (element, "type");
    component_name = gst_structure_get_string (element, "component-name");
    component_role = gst_structure_get_string (element, "component-role");
    library_name = gst_structure_get_string (element, "library-name");

    if (!type_name || !component_name || !library_name) {
      g_warning ("malformed config file: missing required fields for %s",
          element_name);
      return FALSE;
    }

    if (parent_type_name) {
      type = g_type_from_name (parent_type_name);
      if (type) {
        type = create_subtype (type, type_name);
      } else {
        g_warning ("malformed config file: invalid parent-type '%s' for %s",
            parent_type_name, element_name);
        return FALSE;
      }
    } else {
      type = g_type_from_name (type_name);
    }

    if (!type) {
      g_warning ("malformed config file: invalid type '%s' for %s",
          type_name, element_name);
      return FALSE;
    }

    g_type_set_qdata (type, element_name_quark, (gpointer) element_name);

    if (!gst_structure_get_int (element, "rank", &rank)) {
      /* use default rank: */
      rank = GST_RANK_NONE;
    }

    if (!gst_element_register (plugin, element_name, rank, type)) {
      g_warning ("failed registering '%s'", element_name);
      return FALSE;
    }
  }

  return TRUE;
}

gboolean
gstomx_get_component_info (void *core, GType type)
{
  GOmxCore *rcore = core;
  const gchar *element_name;
  GstStructure *element;
  const gchar *str;

  element_name = g_type_get_qdata (type, element_name_quark);
  element = get_element_entry (element_name);

  if (!element)
    return FALSE;

  str = gst_structure_get_string (element, "library-name");
  rcore->library_name = g_strdup (str);

  str = gst_structure_get_string (element, "component-name");
  rcore->component_name = g_strdup (str);

  str = gst_structure_get_string (element, "component-role");
  rcore->component_role = g_strdup (str);

  return TRUE;
}

void *
gstomx_core_new (void *object, GType type)
{
  GOmxCore *core = g_omx_core_new (object);
  gstomx_get_component_info (core, type);
  g_omx_core_init (core);
  return core;
}

void
gstomx_install_property_helper (GObjectClass * gobject_class)
{

  g_object_class_install_property (gobject_class, ARG_COMPONENT_NAME,
      g_param_spec_string ("component-name", "Component name",
          "Name of the OpenMAX IL component to use",
          NULL, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, ARG_COMPONENT_ROLE,
      g_param_spec_string ("component-role", "Component role",
          "Role of the OpenMAX IL component",
          NULL, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, ARG_LIBRARY_NAME,
      g_param_spec_string ("library-name", "Library name",
          "Name of the OpenMAX IL implementation library to use",
          NULL, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}

gboolean
gstomx_get_property_helper (void *core, guint prop_id, GValue * value)
{
  GOmxCore *gomx = core;
  switch (prop_id) {
    case ARG_COMPONENT_NAME:
      g_value_set_string (value, gomx->component_name);
      return TRUE;
    case ARG_COMPONENT_ROLE:
      g_value_set_string (value, gomx->component_role);
      return TRUE;
    case ARG_LIBRARY_NAME:
      g_value_set_string (value, gomx->library_name);
      return TRUE;
    default:
      return FALSE;
  }
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "omx",
    "OpenMAX IL",
    plugin_init,
    PACKAGE_VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
