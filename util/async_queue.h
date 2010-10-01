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

#ifndef ASYNC_QUEUE_H
#define ASYNC_QUEUE_H

#include <glib.h>

typedef struct AsyncQueue AsyncQueue;

struct AsyncQueue
{
  GMutex *mutex;
  GCond *condition;
  GList *head;
  GList *tail;
  guint length;
  gboolean enabled;
};

AsyncQueue *async_queue_new (void);
void async_queue_free (AsyncQueue * queue);
void async_queue_push (AsyncQueue * queue, gpointer data);
gpointer async_queue_pop (AsyncQueue * queue);
gpointer async_queue_pop_forced (AsyncQueue * queue);
void async_queue_disable (AsyncQueue * queue);
void async_queue_enable (AsyncQueue * queue);
void async_queue_flush (AsyncQueue * queue);

#endif /* ASYNC_QUEUE_H */
