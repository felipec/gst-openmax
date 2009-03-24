Dnl handle various error-related things

dnl Thomas Vander Stichele <thomas@apestaart.org>
dnl Tim-Philipp Müller <tim centricular net>

dnl Last modification: 2008-02-18

dnl AG_GST_SET_ERROR_CFLAGS([ADD-WERROR])
dnl AG_GST_SET_ERROR_CXXFLAGS([ADD-WERROR])
dnl AG_GST_SET_LEVEL_DEFAULT([IS-CVS-VERSION])


dnl Sets ERROR_CFLAGS to something the compiler will accept.
dnl AC_SUBST them so they are available in Makefile

dnl -Wall is added if it is supported
dnl -Werror is added if ADD-WERROR is not "no"

dnl These flags can be overridden at make time:
dnl make ERROR_CFLAGS=
AC_DEFUN([AG_GST_SET_ERROR_CFLAGS],
[
  AC_REQUIRE([AC_PROG_CC])
  AC_REQUIRE([AS_COMPILER_FLAG])


  dnl if we support -Wall, set it unconditionally
  AS_COMPILER_FLAG(-Wall,
                   ERROR_CFLAGS="-Wall",
                   ERROR_CFLAGS="")

  dnl Warn if declarations after statements are used (C99 extension)
  AS_COMPILER_FLAG(-Wdeclaration-after-statement,
        ERROR_CFLAGS="$ERROR_CFLAGS -Wdeclaration-after-statement")

  dnl Warn if variable length arrays are used (C99 extension)
  AS_COMPILER_FLAG(-Wvla,
        ERROR_CFLAGS="$ERROR_CFLAGS -Wvla")

  dnl Warn for invalid pointer arithmetic
  AS_COMPILER_FLAG(-Wpointer-arith,
        ERROR_CFLAGS="$ERROR_CFLAGS -Wpointer-arith")

  dnl if asked for, add -Werror if supported
  if test "x$1" != "xno"
  then
    AS_COMPILER_FLAG(-Werror, ERROR_CFLAGS="$ERROR_CFLAGS -Werror")

    dnl if -Werror isn't suported, try -errwarn=%all (Sun Forte case)
    if test "x$ERROR_CFLAGS" == "x"
    then
      AS_COMPILER_FLAG([-errwarn=%all], [
          ERROR_CFLAGS="-errwarn=%all"
          dnl try -errwarn=%all,no%E_EMPTY_DECLARATION,
          dnl no%E_STATEMENT_NOT_REACHED,no%E_ARGUEMENT_MISMATCH,
          dnl no%E_MACRO_REDEFINED (Sun Forte case)
          dnl For Forte we need disable "empty declaration" warning produced by un-needed semicolon
          dnl "statement not reached" disabled because there is g_assert_not_reached () in some places
          dnl "macro redefined" because of gst/gettext.h
          dnl FIXME: is it really supposed to be 'ARGUEMENT' and not 'ARGUMENT'?
          for f in 'no%E_EMPTY_DECLARATION' \
                   'no%E_STATEMENT_NOT_REACHED' \
                   'no%E_ARGUEMENT_MISMATCH' \
                   'no%E_MACRO_REDEFINED' \
                   'no%E_LOOP_NOT_ENTERED_AT_TOP'
          do
            AS_COMPILER_FLAG([-errwarn=%all,$f], [
              ERROR_CFLAGS="$ERROR_CFLAGS,$f"
            ])
          done
      ])
    else
      dnl Add -fno-strict-aliasing for GLib versions before 2.19.8
      dnl as before G_LOCK and friends caused strict aliasing compiler
      dnl warnings.
      PKG_CHECK_EXISTS([glib-2.0 < 2.19.8], [
        AS_COMPILER_FLAG(-fno-strict-aliasing,
            ERROR_CFLAGS="$ERROR_CFLAGS -fno-strict-aliasing")
	])
    fi
  fi

  AC_SUBST(ERROR_CFLAGS)
  AC_MSG_NOTICE([set ERROR_CFLAGS to $ERROR_CFLAGS])
])
