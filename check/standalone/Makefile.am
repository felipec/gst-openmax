noinst_LTLIBRARIES = libomxil-foo.la

libomxil_foo_la_SOURCES = core.c

libomxil_foo_la_CFLAGS = -I$(top_srcdir)/omx/headers $(GTHREAD_CFLAGS) -I$(top_srcdir)/util
libomxil_foo_la_LIBADD = $(GTHREAD_LIBS) $(top_srcdir)/util/libutil.la
