SUBDIRS = standalone

TESTS = check_async_queue \
	check_libomxil

TESTS_ENVIRONMENT = \
	LD_LIBRARY_PATH=$(builddir)/standalone

check_PROGRAMS =

check_PROGRAMS += check_async_queue
check_async_queue_SOURCES = check_async_queue.c
check_async_queue_CFLAGS = $(CHECK_CFLAGS) $(GTHREAD_CFLAGS) -I$(top_srcdir)/util
check_async_queue_LDADD = $(CHECK_LIBS) $(GTHREAD_LIBS) $(top_builddir)/util/libutil.la

check_PROGRAMS += check_libomxil
check_libomxil_SOURCES = check_libomxil.c
check_libomxil_CFLAGS = $(CHECK_CFLAGS) -I$(top_srcdir)/omx/headers
check_libomxil_LDADD = $(CHECK_LIBS) -ldl
