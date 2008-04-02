plugin_LTLIBRARIES = libgstomx.la

libgstomx_la_SOURCES = gstomx.c gstomx.h \
		       gstomx_base_filter.c gstomx_base_filter.h \
		       gstomx_base_sink.c gstomx_base_sink.h \
		       gstomx_base_src.c gstomx_base_src.h \
		       gstomx_base_videodec.c gstomx_base_videodec.h \
		       gstomx_base_videoenc.c gstomx_base_videoenc.h \
		       gstomx_util.c gstomx_util.h \
		       gstomx_dummy.c gstomx_dummy.h \
		       gstomx_mpeg4dec.c gstomx_mpeg4dec.h \
		       gstomx_h263dec.c gstomx_h263dec.h \
		       gstomx_h264dec.c gstomx_h264dec.h \
		       gstomx_wmvdec.c gstomx_wmvdec.h \
		       gstomx_mpeg4enc.c gstomx_mpeg4enc.h \
		       gstomx_avcenc.c gstomx_avcenc.h \
		       gstomx_h263enc.c gstomx_h263enc.h \
		       gstomx_vorbisdec.c gstomx_vorbisdec.h \
		       gstomx_amrnbdec.c gstomx_amrnbdec.h \
		       gstomx_amrwbdec.c gstomx_amrwbdec.h \
		       gstomx_aacdec.c gstomx_aacdec.h \
		       gstomx_aacenc.c gstomx_aacenc.h \
		       gstomx_mp3dec.c gstomx_mp3dec.h \
		       gstomx_audiosink.c gstomx_audiosink.h \
		       gstomx_videosink.c gstomx_videosink.h \
		       gstomx_filereadersrc.c gstomx_filereadersrc.h

libgstomx_la_CFLAGS = -I$(srcdir)/headers $(GST_CFLAGS) $(GST_BASE_CFLAGS)
libgstomx_la_LIBADD = $(GST_LIBS) $(GST_BASE_LIBS)
libgstomx_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)

EXTRA_DIST = headers
