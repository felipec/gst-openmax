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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <check.h>
#include "async_queue.h"

#define PROCESS_COUNT 0x1000
#define DISABLE_AT PROCESS_COUNT / 2

START_TEST (test_async_queue_create)
{
    AsyncQueue *queue;
    queue = async_queue_new ();
    fail_if (!queue,
             "Construction failed");
    async_queue_free (queue);
}
END_TEST

START_TEST (test_async_queue_pop)
{
    AsyncQueue *queue;
    gpointer foo;
    gpointer tmp;
    queue = async_queue_new ();
    fail_if (!queue,
             "Construction failed");
    foo = GINT_TO_POINTER (1);
    async_queue_push (queue, foo);
    tmp = async_queue_pop (queue);
    fail_if (tmp != foo,
             "Pop failed");
    async_queue_free (queue);
}
END_TEST

START_TEST (test_async_queue_process)
{
    AsyncQueue *queue;
    gpointer foo;
    guint i;

    queue = async_queue_new ();
    fail_if (!queue,
             "Construction failed");

    foo = GINT_TO_POINTER (1);
    for (i = 0; i < PROCESS_COUNT; i++, foo++)
    {
        async_queue_push (queue, foo);
    }
    foo = GINT_TO_POINTER (1);
    for (i = 0; i < PROCESS_COUNT; i++, foo++)
    {
        gpointer tmp;
        tmp = async_queue_pop (queue);
        fail_if (tmp != foo,
                 "Pop failed");
    }

    async_queue_free (queue);
}
END_TEST

static gpointer
push_func (gpointer data)
{
    AsyncQueue *queue;
    gpointer foo;
    guint i;

    queue = data;
    foo = GINT_TO_POINTER (1);
    for (i = 0; i < PROCESS_COUNT; i++, foo++)
    {
        async_queue_push (queue, foo);
    }

    return NULL;
}

static gpointer
pop_func (gpointer data)
{
    AsyncQueue *queue;
    gpointer foo;
    guint i;

    queue = data;
    foo = GINT_TO_POINTER (1);
    for (i = 0; i < PROCESS_COUNT; i++, foo++)
    {
        gpointer tmp;
        tmp = async_queue_pop (queue);
        fail_if (tmp != foo,
                 "Pop failed");
    }

    return NULL;
}

START_TEST (test_async_queue_threads)
{
    AsyncQueue *queue;
    GThread *push_thread;
    GThread *pop_thread;

    queue = async_queue_new ();
    fail_if (!queue,
             "Construction failed");

    pop_thread = g_thread_create (pop_func, queue, TRUE, NULL);
    push_thread = g_thread_create (push_func, queue, TRUE, NULL);

    g_thread_join (pop_thread);
    g_thread_join (push_thread);

    async_queue_free (queue);
}
END_TEST

static gpointer
push_and_disable_func (gpointer data)
{
    AsyncQueue *queue;
    gpointer foo;
    guint i;

    queue = data;
    foo = GINT_TO_POINTER (1);
    for (i = 0; i < DISABLE_AT; i++, foo++)
    {
        async_queue_push (queue, foo);
    }

    async_queue_disable (queue);

    return NULL;
}

static gpointer
pop_with_disable_func (gpointer data)
{
    AsyncQueue *queue;
    gpointer foo;
    guint i;
    guint count = 0;

    queue = data;
    foo = GINT_TO_POINTER (1);
    for (i = 0; i < PROCESS_COUNT; i++, foo++)
    {
        gpointer tmp;
        tmp = async_queue_pop (queue);
        if (!tmp)
            continue;
        count++;
        fail_if (tmp != foo,
                 "Pop failed");
    }

    return GINT_TO_POINTER (count);
}

START_TEST (test_async_queue_disable_simple)
{
    AsyncQueue *queue;
    GThread *pop_thread;
    guint count;

    queue = async_queue_new ();
    fail_if (!queue,
             "Construction failed");

    pop_thread = g_thread_create (pop_with_disable_func, queue, TRUE, NULL);

    async_queue_disable (queue);

    count = GPOINTER_TO_INT (g_thread_join (pop_thread));

    fail_if (count != 0,
             "Disable failed");

    async_queue_free (queue);
}
END_TEST

START_TEST (test_async_queue_disable)
{
    AsyncQueue *queue;
    GThread *push_thread;
    GThread *pop_thread;
    guint count;

    queue = async_queue_new ();
    fail_if (!queue,
             "Construction failed");

    pop_thread = g_thread_create (pop_with_disable_func, queue, TRUE, NULL);
    push_thread = g_thread_create (push_and_disable_func, queue, TRUE, NULL);

    count = GPOINTER_TO_INT (g_thread_join (pop_thread));
    g_thread_join (push_thread);

    fail_if (count > DISABLE_AT,
             "Disable failed");

    async_queue_free (queue);
}
END_TEST

START_TEST (test_async_queue_enable)
{
    AsyncQueue *queue;
    GThread *push_thread;
    GThread *pop_thread;
    guint count;

    queue = async_queue_new ();
    fail_if (!queue,
             "Construction failed");

    pop_thread = g_thread_create (pop_with_disable_func, queue, TRUE, NULL);

    async_queue_disable (queue);

    count = GPOINTER_TO_INT (g_thread_join (pop_thread));

    fail_if (count != 0,
             "Disable failed");

    async_queue_enable (queue);

    pop_thread = g_thread_create (pop_with_disable_func, queue, TRUE, NULL);
    push_thread = g_thread_create (push_and_disable_func, queue, TRUE, NULL);

    count = GPOINTER_TO_INT (g_thread_join (pop_thread));
    g_thread_join (push_thread);

    fail_if (count > DISABLE_AT,
             "Disable failed");

    async_queue_free (queue);
}
END_TEST

Suite *
util_suite (void)
{
    Suite *s = suite_create ("util");

    if (!g_thread_supported ())
    {
        g_thread_init (NULL);
    }

    /* Core test case */
    TCase *tc_core = tcase_create ("Core");
    tcase_add_test (tc_core, test_async_queue_create);
    tcase_add_test (tc_core, test_async_queue_pop);
    tcase_add_test (tc_core, test_async_queue_process);
    tcase_add_test (tc_core, test_async_queue_threads);
    tcase_add_test (tc_core, test_async_queue_disable_simple);
    tcase_add_test (tc_core, test_async_queue_disable);
    tcase_add_test (tc_core, test_async_queue_enable);
    suite_add_tcase (s, tc_core);

    return s;
}

int
main (void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = util_suite ();
    sr = srunner_create (s);
    srunner_run_all (sr, CK_NORMAL);
    number_failed = srunner_ntests_failed (sr);
    srunner_free (sr);

    return (number_failed == 0) ? 0 : 1;
}
