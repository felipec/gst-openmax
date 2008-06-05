TESTS = check_async_queue
check_PROGRAMS = check_async_queue
check_async_queue_SOURCES = check_async_queue.c
check_async_queue_CFLAGS = $(CHECK_CFLAGS) $(GTHREAD_CFLAGS) -I$(top_srcdir)/util
check_async_queue_LDADD = $(CHECK_LIBS) $(GTHREAD_LIBS) $(top_builddir)/util/libutil.la
