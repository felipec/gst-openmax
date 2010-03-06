SUBDIRS = standalone

TESTS = check_async_queue \
	check_libomxil \
	check_gstomx

CHECK_REGISTRY = $(top_builddir)/tests/test-registry.reg

TESTS_ENVIRONMENT = GST_REGISTRY=$(CHECK_REGISTRY) \
		    LD_LIBRARY_PATH=$(builddir)/standalone \
		    GST_PLUGIN_PATH=$(top_builddir)/omx \
		    OMX_CONFIG=$(srcdir)/gst-openmax.conf

check_PROGRAMS =

check_PROGRAMS += check_async_queue
check_async_queue_SOURCES = check_async_queue.c
check_async_queue_CFLAGS = $(CHECK_CFLAGS) $(GTHREAD_CFLAGS) -I$(top_srcdir)/util
check_async_queue_LDADD = $(CHECK_LIBS) $(GTHREAD_LIBS) $(top_builddir)/util/libutil.la

check_PROGRAMS += check_libomxil
check_libomxil_SOURCES = check_libomxil.c
check_libomxil_CFLAGS = $(CHECK_CFLAGS) $(GTHREAD_CFLAGS) -I$(top_srcdir)/omx/headers
check_libomxil_LDADD = $(CHECK_LIBS) $(GTHREAD_LIBS) -ldl

check_PROGRAMS += check_gstomx
check_gstomx_SOURCES = check_gstomx.c
check_gstomx_CFLAGS = $(GST_CHECK_CFLAGS)
check_gstomx_LDADD = $(GST_CHECK_LIBS)
