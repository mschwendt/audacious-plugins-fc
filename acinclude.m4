dnl -------------------------------------------------------------------------
dnl Try to find a file (or one of more files in a list of dirs).
dnl -------------------------------------------------------------------------

AC_DEFUN([FC_FIND_FILE],
[
    $3=""
    for i in $2;
    do
        for j in $1;
        do
	        if test -r "$i/$j"; then
		        $3=$i
                break 2
            fi
        done
    done
])

dnl -------------------------------------------------------------------------
dnl Check whether compiler has a working ``bool'' type.
dnl -------------------------------------------------------------------------

AC_DEFUN([FC_CHECK_BOOL],
[
    AC_MSG_CHECKING([for bool])
    AC_CACHE_VAL(fc_cv_have_bool,
    [
        AC_TRY_COMPILE(
            [],
            [bool aBool = true;],
            [fc_cv_have_bool=yes],
            [fc_cv_have_bool=no]
        )
    ])
    AC_MSG_RESULT($fc_cv_have_bool)
    if test "$fc_cv_have_bool" = yes; then
	AC_DEFINE([FC_HAVE_BOOL], 1, [Define if your compiler supports type ``bool''. If not, a user-defined signed integral type will be used.])
    fi
])

dnl -------------------------------------------------------------------------
dnl Check whether C++ library has member ios::bin instead of ios::binary.
dnl -------------------------------------------------------------------------

AC_DEFUN([FC_CHECK_IOS_BINARY],
[
    AC_MSG_CHECKING(whether standard member ios::binary is available)
    AC_CACHE_VAL(fc_cv_have_ios_binary,
    [
        AC_TRY_COMPILE(
            [
#include <fstream.h>
#include <iostream.h>
            ],
		    [ifstream myTest("testfile",ios::in|ios::binary);],
		    [fc_cv_have_ios_binary=yes],
		    [fc_cv_have_ios_binary=no]
	    )
    ])
    AC_MSG_RESULT($fc_cv_have_ios_binary)
    if test "$fc_cv_have_ios_binary" = yes; then
	AC_DEFINE([FC_HAVE_IOS_BINARY], 1, [Define if standard member ``ios::binary'' is available.])
    fi
])

dnl -------------------------------------------------------------------------
dnl Check whether C++ compiler supports the "nothrow allocator".
dnl -------------------------------------------------------------------------

AC_DEFUN([FC_CHECK_NOTHROW],
[
    AC_MSG_CHECKING(whether nothrow allocator is available)
    AC_CACHE_VAL(fc_cv_have_nothrow,
    [
        AC_TRY_COMPILE(
            [#include <new>],
		    [char* buf = new(std::nothrow) char[1024];],
		    [fc_cv_have_nothrow=yes],
		    [fc_cv_have_nothrow=no]
	    )
    ])
    AC_MSG_RESULT($fc_cv_have_nothrow)
    if test "$fc_cv_have_nothrow" = yes; then
	AC_DEFINE([FC_HAVE_NOTHROW], 1, [Define if ``nothrow allocator'' is available.])
    fi
])

dnl -------------------------------------------------------------------------
dnl Pass C++ compiler options to libtool which supports C only.
dnl -------------------------------------------------------------------------

AC_DEFUN([FC_PROG_LIBTOOL],
[
    fc_save_cc="$CC"
    fc_save_cflags="$CFLAGS"
    CC="$CXX"
    CFLAGS="$CXXFLAGS"
    AM_PROG_LIBTOOL
    CFLAGS="$fc_save_cflags"
    CC="$fc_save_cc"
])

dnl -------------------------------------------------------------------------
dnl PKG_CHECK_MODULES(EXAMPLE, gtk+-2.0 >= 2.4.0 glib = 2.4.0, action-if, action-not)
dnl defines EXAMPLE_LIBS, EXAMPLE_CFLAGS, see pkg-config man page
dnl also defines EXAMPLE_PKG_ERRORS on error
dnl -------------------------------------------------------------------------

AC_DEFUN([PKG_CHECK_MODULES], [
  succeeded=no

  if test -z "$PKG_CONFIG"; then
    AC_PATH_PROG(PKG_CONFIG, pkg-config, "")
  fi

  if test "$PKG_CONFIG" = "no"; then
     echo "*** The pkg-config script could not be found. Make sure it is"
     echo "*** in your path, or set the PKG_CONFIG environment variable"
     echo "*** to the full path to pkg-config."
     echo "*** Or see http://www.freedesktop.org/software/pkgconfig to get pkg-config."
  else
     PKG_CONFIG_MIN_VERSION=0.9.0
     if $PKG_CONFIG --atleast-pkgconfig-version $PKG_CONFIG_MIN_VERSION; then
        AC_MSG_CHECKING(for $2)

        if $PKG_CONFIG --exists "$2" ; then
            AC_MSG_RESULT(yes)
            succeeded=yes

            AC_MSG_CHECKING($1_CFLAGS)
            $1_CFLAGS=`$PKG_CONFIG --cflags "$2"`
            AC_MSG_RESULT($$1_CFLAGS)

            AC_MSG_CHECKING($1_LIBS)
            $1_LIBS=`$PKG_CONFIG --libs "$2"`
            AC_MSG_RESULT($$1_LIBS)
        else
            $1_CFLAGS=""
            $1_LIBS=""
            ## If we have a custom action on failure, don't print errors, but 
            ## do set a variable so people can do so.
            $1_PKG_ERRORS=`$PKG_CONFIG --errors-to-stdout --print-errors "$2"`
            ifelse([$4], ,echo $$1_PKG_ERRORS,)
        fi

        AC_SUBST($1_CFLAGS)
        AC_SUBST($1_LIBS)
     else
        echo "*** Your version of pkg-config is too old. You need version $PKG_CONFIG_MIN_VERSION or newer."
        echo "*** See http://www.freedesktop.org/software/pkgconfig"
     fi
  fi

  if test $succeeded = yes; then
     ifelse([$3], , :, [$3])
  else
     ifelse([$4], , AC_MSG_ERROR([Library requirements ($2) not met; consider adjusting the PKG_CONFIG_PATH environment variable if your libraries are in a nonstandard prefix so pkg-config can find them.]), [$4])
  fi
])

