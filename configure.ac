AC_INIT([gst-openmax], [0.1.0])
AC_CONFIG_AUX_DIR([config])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AM_CONFIG_HEADER([config.h])
AC_CONFIG_FILES([Makefile omx/Makefile])

dnl versions of OpenMAX IL
OMXIL_REQUIRED=0.3

dnl versions of GStreamer
GST_MAJORMINOR=0.10
GST_REQUIRED=0.10.0

dnl AM_MAINTAINER_MODE provides the option to enable maintainer mode
AM_MAINTAINER_MODE

dnl check for tools
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

# GST_ARG_DEBUG
# GST_ARG_VALGRIND

dnl Check for OpenMAX IL
PKG_CHECK_MODULES([OMXIL], [libomxil >= $OMXIL_REQUIRED])

dnl Check for GStreamer
PKG_CHECK_MODULES([GST], [gstreamer-$GST_MAJORMINOR >= $GST_REQUIRED])

dnl Check for GStreamer Base
PKG_CHECK_MODULES([GST_BASE], [gstreamer-base-$GST_MAJORMINOR >= $GST_REQUIRED])

dnl append GST_ERROR cflags to GST_CFLAGS
dnl GST_CFLAGS="$GST_CFLAGS $GST_ERROR"

dnl set the plugindir where plugins should be installed
if test "x${prefix}" = "x$HOME"; then
  plugindir="$HOME/.gstreamer-$GST_MAJORMINOR/plugins"
else
  plugindir="\$(libdir)/gstreamer-$GST_MAJORMINOR"
fi
AC_SUBST([plugindir])

dnl set proper LDFLAGS for plugins
GST_PLUGIN_LDFLAGS='-module -avoid-version -export-symbols-regex [_]*\(gst_\|Gst\|GST_\).*'
AC_SUBST([GST_PLUGIN_LDFLAGS])

AC_OUTPUT
