/*
 * Copyright (C) 2007-2008 Nokia Corporation.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <glib.h>
#include <gst/gst.h>

typedef struct Core Core;

struct Core
{
    GstElement *pipeline;
    GMainLoop *loop;
};

static gboolean
bus_cb (GstBus *bus,
        GstMessage *msg,
        gpointer data)
{
    Core *core;

    core = data;

    switch (GST_MESSAGE_TYPE (msg))
    {
        case GST_MESSAGE_EOS:
            g_debug ("end-of-stream");
            g_main_loop_quit (core->loop);
            break;
        case GST_MESSAGE_ERROR:
            {
                gchar *debug;
                GError *err;

                gst_message_parse_error (msg, &err, &debug);
                g_free (debug);

                g_warning ("Error: %s", err->message);
                g_error_free (err);
                break;
            }
        default:
            /* g_debug ("message-type: %s", GST_MESSAGE_TYPE_NAME (msg)); */
            break;
    }

    return TRUE;
}

Core *
core_new (void)
{
    Core *core;
    core = g_new0 (Core, 1);

    core->loop = g_main_loop_new (NULL, FALSE);

    return core;
}

void
core_free (Core *core)
{
    gst_element_set_state (core->pipeline, GST_STATE_NULL);
    gst_object_unref (GST_OBJECT (core->pipeline));

    g_main_loop_unref (core->loop);

    g_free (core);
}

int
main (int argc,
      char *argv[])
{
    Core *core;

    gst_init (&argc, &argv);

    core = core_new ();

    core->pipeline = gst_pipeline_new ("audio-player");

    {
        GstElement *src;
        GstElement *filter;
        GstElement *sink;

        src = gst_element_factory_make ("filesrc", "src");
        g_object_set (G_OBJECT (src), "location", "/tmp/test.mp3", NULL);
        gst_bin_add (GST_BIN (core->pipeline), src);

        filter = gst_element_factory_make ("omx_dummy", "dummy");
        g_object_set (G_OBJECT (filter), "library-name", "libomxil-foo.so", NULL);
        gst_bin_add (GST_BIN (core->pipeline), filter);
        gst_element_link (src, filter);

        sink = gst_element_factory_make ("filesink", "sink");
        g_object_set (G_OBJECT (sink), "location", "/tmp/test_out.mp3", NULL);
        gst_bin_add (GST_BIN (core->pipeline), sink);
        gst_element_link (filter, sink);
    }

    {
        GstBus *bus;
        bus = gst_pipeline_get_bus (GST_PIPELINE (core->pipeline));
        gst_bus_add_watch (bus, bus_cb, core);
        gst_object_unref (bus);
    }

    gst_element_set_state (core->pipeline, GST_STATE_PLAYING);

    g_main_loop_run (core->loop);

    core_free (core);

    return 0;
}
