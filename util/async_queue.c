/*
 * Copyright (C) 2008-2009 Nokia Corporation.
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

#include <glib.h>

#include "async_queue.h"

AsyncQueue *
async_queue_new (void)
{
  AsyncQueue *queue;

  queue = g_slice_new0 (AsyncQueue);

  queue->condition = g_cond_new ();
  queue->mutex = g_mutex_new ();
  queue->enabled = TRUE;

  return queue;
}

void
async_queue_free (AsyncQueue * queue)
{
  g_cond_free (queue->condition);
  g_mutex_free (queue->mutex);

  g_list_free (queue->head);
  g_slice_free (AsyncQueue, queue);
}

void
async_queue_push (AsyncQueue * queue, gpointer data)
{
  g_mutex_lock (queue->mutex);

  queue->head = g_list_prepend (queue->head, data);
  if (!queue->tail)
    queue->tail = queue->head;
  queue->length++;

  g_cond_signal (queue->condition);

  g_mutex_unlock (queue->mutex);
}

gpointer
async_queue_pop (AsyncQueue * queue)
{
  gpointer data = NULL;

  g_mutex_lock (queue->mutex);

  if (!queue->enabled) {
    /* g_warning ("not enabled!"); */
    goto leave;
  }

  if (!queue->tail) {
    g_cond_wait (queue->condition, queue->mutex);
  }

  if (queue->tail) {
    GList *node = queue->tail;
    data = node->data;

    queue->tail = node->prev;
    if (queue->tail)
      queue->tail->next = NULL;
    else
      queue->head = NULL;
    queue->length--;
    g_list_free_1 (node);
  }

leave:
  g_mutex_unlock (queue->mutex);

  return data;
}

gpointer
async_queue_pop_forced (AsyncQueue * queue)
{
  gpointer data = NULL;

  g_mutex_lock (queue->mutex);

  if (queue->tail) {
    GList *node = queue->tail;
    data = node->data;

    queue->tail = node->prev;
    if (queue->tail)
      queue->tail->next = NULL;
    else
      queue->head = NULL;
    queue->length--;
    g_list_free_1 (node);
  }

  g_mutex_unlock (queue->mutex);

  return data;
}

void
async_queue_disable (AsyncQueue * queue)
{
  g_mutex_lock (queue->mutex);
  queue->enabled = FALSE;
  g_cond_broadcast (queue->condition);
  g_mutex_unlock (queue->mutex);
}

void
async_queue_enable (AsyncQueue * queue)
{
  g_mutex_lock (queue->mutex);
  queue->enabled = TRUE;
  g_mutex_unlock (queue->mutex);
}

void
async_queue_flush (AsyncQueue * queue)
{
  g_mutex_lock (queue->mutex);
  g_list_free (queue->head);
  queue->head = queue->tail = NULL;
  queue->length = 0;
  g_mutex_unlock (queue->mutex);
}
