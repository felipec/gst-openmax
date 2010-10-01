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

#include <gst/check/gstcheck.h>

#define BUFFER_SIZE 0x1000
#define BUFFER_COUNT 0x100
#define FLUSH_AT 0x10

static GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

/* some global vars, makes it easy as for the ones above */
static GMutex *eos_mutex;
static GCond *eos_cond;
static gboolean eos_arrived;

static gboolean
test_sink_event (GstPad * pad, GstEvent * event)
{

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_EOS:
      g_mutex_lock (eos_mutex);
      eos_arrived = TRUE;
      g_cond_signal (eos_cond);
      g_mutex_unlock (eos_mutex);
      break;
    default:
      break;
  }

  return gst_pad_event_default (pad, event);
}

static void
helper (gboolean flush)
{
  GstElement *filter;
  GstBus *bus;
  GstPad *mysrcpad;
  GstPad *mysinkpad;

  /* init */
  filter = gst_check_setup_element ("omx_dummy");
  mysrcpad = gst_check_setup_src_pad (filter, &srctemplate, NULL);
  mysinkpad = gst_check_setup_sink_pad (filter, &sinktemplate, NULL);

  gst_pad_set_active (mysrcpad, TRUE);
  gst_pad_set_active (mysinkpad, TRUE);

  /* need to know when we are eos */
  gst_pad_set_event_function (mysinkpad, test_sink_event);

  /* and notify the test run */
  eos_mutex = g_mutex_new ();
  eos_cond = g_cond_new ();
  eos_arrived = FALSE;

  /* start */

  fail_unless_equals_int (gst_element_set_state (filter, GST_STATE_PLAYING),
      GST_STATE_CHANGE_SUCCESS);

  bus = gst_bus_new ();

  gst_element_set_bus (filter, bus);

  /* send buffers in order */
  {
    guint i;
    for (i = 0; i < BUFFER_COUNT; i++) {
      GstBuffer *inbuffer;
      inbuffer = gst_buffer_new_and_alloc (BUFFER_SIZE);
      GST_BUFFER_DATA (inbuffer)[0] = i;
      ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

      fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);

      if (flush && i % FLUSH_AT == 0) {
        gst_pad_push_event (mysrcpad, gst_event_new_flush_start ());
        gst_pad_push_event (mysrcpad, gst_event_new_flush_stop ());
        i += FLUSH_AT;
      }
    }
  }

  {
    GstMessage *message;

    /* make sure there's no error on the bus */
    message = gst_bus_poll (bus, GST_MESSAGE_ERROR, 0);
    fail_if (message);
  }

  gst_pad_push_event (mysrcpad, gst_event_new_eos ());
  /* need to wait a bit to make sure src pad task digested all and sent eos */
  g_mutex_lock (eos_mutex);
  while (!eos_arrived)
    g_cond_wait (eos_cond, eos_mutex);
  g_mutex_unlock (eos_mutex);

  /* check the order of the buffers */
  if (!flush) {
    GList *cur;
    guint i;
    for (cur = buffers, i = 0; cur; cur = g_list_next (cur), i++) {
      GstBuffer *buffer;
      buffer = cur->data;
      fail_unless (GST_BUFFER_DATA (buffer)[0] == i);
    }
    fail_unless (i == BUFFER_COUNT);
  }

  /* cleanup */
  gst_bus_set_flushing (bus, TRUE);
  gst_element_set_bus (filter, NULL);
  gst_object_unref (GST_OBJECT (bus));
  gst_check_drop_buffers ();

  /* deinit */
  gst_element_set_state (filter, GST_STATE_NULL);

  gst_pad_set_active (mysrcpad, FALSE);
  gst_pad_set_active (mysinkpad, FALSE);
  gst_check_teardown_src_pad (filter);
  gst_check_teardown_sink_pad (filter);
  gst_check_teardown_element (filter);

  g_mutex_free (eos_mutex);
  g_cond_free (eos_cond);
}

GST_START_TEST (test_flush)
{
  helper (TRUE);
}

GST_END_TEST
GST_START_TEST (test_basic)
{
  helper (FALSE);
}

GST_END_TEST static Suite *
gstomx_suite (void)
{
  Suite *s = suite_create ("gstomx");
  TCase *tc_chain = tcase_create ("general");

  tcase_set_timeout (tc_chain, 10);
  tcase_add_test (tc_chain, test_basic);
  tcase_add_test (tc_chain, test_flush);
  suite_add_tcase (s, tc_chain);

  return s;
}

GST_CHECK_MAIN (gstomx);
