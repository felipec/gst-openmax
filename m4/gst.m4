dnl AG_GST_INIT
dnl sets up use of GStreamer configure.ac macros
dnl all GStreamer autoconf macros are prefixed
dnl with AG_GST_ for public macros
dnl with _AG_GST_ for private macros

AC_DEFUN([AG_GST_INIT],
[
  m4_pattern_forbid(^_?AG_GST_)
])
