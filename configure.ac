AC_PREREQ([2.52])

AC_INIT([gst-openmax], m4_esyscmd([build-aux/git-version-gen]),
        [http://bugzilla.gnome.org/enter_bug.cgi?product=GStreamer&component=gst-openmax])

AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([-Wall -Wno-portability])
AC_CONFIG_HEADERS([config.h])

dnl versions of GStreamer
GST_MAJORMINOR=0.10
GST_REQUIRED=0.10.0

dnl AM_MAINTAINER_MODE provides the option to enable maintainer mode
AM_MAINTAINER_MODE

dnl check for tools
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_LIBTOOL

PKG_CHECK_MODULES([CHECK], [check], HAVE_CHECK=yes, HAVE_CHECK=no)

dnl Keep correct libtool macros in-tree.
AC_CONFIG_MACRO_DIR([m4])

dnl initialize GStreamer macros
AG_GST_INIT

dnl define an ERROR_CFLAGS Makefile variable
AG_GST_SET_ERROR_CFLAGS($GST_CVS)

dnl add GStreamer arguments
AG_GST_ARG_DEBUG
AG_GST_ARG_PROFILING
AG_GST_ARG_VALGRIND
AG_GST_ARG_GCOV

AG_GST_ARG_WITH_PKG_CONFIG_PATH
AG_GST_ARG_WITH_PACKAGE_NAME
AG_GST_ARG_WITH_PACKAGE_ORIGIN

AC_ARG_ENABLE([experimental], AC_HELP_STRING([--enable-experimental]), [EXPERIMENTAL=$enableval], [EXPERIMENTAL=no])
AM_CONDITIONAL([EXPERIMENTAL], [test x$EXPERIMENTAL = xyes])
if test x$EXPERIMENTAL = xyes; then
        AC_DEFINE([EXPERIMENTAL], , [Enable experimental elements])
fi

dnl ** checks **

dnl Check for GLib
PKG_CHECK_MODULES([GTHREAD], [gthread-2.0])

dnl Check for GStreamer
AG_GST_CHECK_GST($GST_MAJORMINOR, [$GST_REQUIRED])
AG_GST_CHECK_GST_BASE($GST_MAJORMINOR, [$GST_REQUIRED])
AG_GST_CHECK_GST_CHECK($GST_MAJORMINOR, [$GST_REQUIRED], [no])

dnl ** finalize ***

dnl set license and copyright notice
GST_LICENSE="LGPL"
AC_DEFINE_UNQUOTED(GST_LICENSE, "$GST_LICENSE", [GStreamer license])
AC_SUBST(GST_LICENSE)

dnl set GStreamer plug-in dir
AG_GST_SET_PLUGINDIR

dnl GST_OPTION_CFLAGS
if test "x$USE_DEBUG" = xyes; then
   PROFILE_CFLAGS="-g"
fi
AC_SUBST(PROFILE_CFLAGS)

DEPRECATED_CFLAGS="-DGST_DISABLE_DEPRECATED"
AC_SUBST(DEPRECATED_CFLAGS)

dnl every flag in GST_OPTION_CFLAGS can be overridden at make time
GST_OPTION_CFLAGS="\$(ERROR_CFLAGS) \$(DEBUG_CFLAGS) \$(PROFILE_CFLAGS) \$(GCOV_CFLAGS) \$(OPT_CFLAGS) \$(DEPRECATED_CFLAGS)"
AC_SUBST(GST_OPTION_CFLAGS)

dnl our libraries need to be versioned correctly
AC_SUBST(GST_LT_LDFLAGS)

dnl FIXME: do we want to rename to GST_ALL_* ?
dnl prefer internal headers to already installed ones
dnl also add builddir include for enumtypes and marshal
dnl add GST_OPTION_CFLAGS, but overridable
GST_CFLAGS="-I\$(top_srcdir)/gst-libs -I\$(top_builddir)/gst-libs $GST_CFLAGS \$(GST_OPTION_CFLAGS)"
AC_SUBST(GST_CFLAGS)
AC_SUBST(GST_LIBS)

dnl LDFLAGS really should only contain flags, not libs - they get added before
dnl whatevertarget_LIBS and -L flags here affect the rest of the linking
GST_ALL_LDFLAGS="-no-undefined"
AC_SUBST(GST_ALL_LDFLAGS)

dnl this really should only contain flags, not libs - they get added before
dnl whatevertarget_LIBS and -L flags here affect the rest of the linking
GST_PLUGIN_LDFLAGS="-module -avoid-version -export-symbols-regex '^[_]*gst_plugin_desc\$\$' $GST_ALL_LDFLAGS"
AC_SUBST(GST_PLUGIN_LDFLAGS)

SHAVE_INIT
AC_CONFIG_FILES([shave shave-libtool])

AC_CONFIG_FILES([Makefile \
		 omx/Makefile \
		 util/Makefile \
		 tests/Makefile \
		 tests/standalone/Makefile \
		 m4/Makefile])

AC_OUTPUT
