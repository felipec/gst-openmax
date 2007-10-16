plugin_LTLIBRARIES = libgstomx.la

libgstomx_la_SOURCES = gstomx.c gstomx.h \
		       gstomx_base_filter.c gstomx_base_filter.h \
		       gstomx_base_sink.c gstomx_base_sink.h \
		       gstomx_util.c gstomx_util.h \
		       gstomx_dummy.c gstomx_dummy.h \
		       gstomx_mpeg4dec.c gstomx_mpeg4dec.h \
		       gstomx_h263dec.c gstomx_h263dec.h \
		       gstomx_vorbisdec.c gstomx_vorbisdec.h \
		       gstomx_mp3dec.c gstomx_mp3dec.h \
		       gstomx_alsasink.c gstomx_alsasink.h \
		       gstomx_videosink.c gstomx_videosink.h

libgstomx_la_CFLAGS = $(GST_CFLAGS) $(GST_BASE_CFLAGS) $(OMXIL_CFLAGS)
libgstomx_la_LIBADD = $(GST_LIBS) $(GST_BASE_LIBS) $(OMXIL_LIBS)
libgstomx_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
