AC_INIT([audacious-plugin-fc], [0.7.3.5])
AC_CONFIG_SRCDIR([src/plugin.c])
AM_INIT_AUTOMAKE([foreign])
AM_DISABLE_STATIC
AM_PROG_LIBTOOL
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_MACRO_DIR([m4])

dnl AC_CANONICAL_HOST

AC_PROG_CC
AC_LANG([C])
AC_C_BIGENDIAN
AC_PATH_PROG(RM,rm,rm)

dnl AC_HEADER_STDC

AC_CHECK_HEADER(fc14audiodecoder.h,
    [],[AC_MSG_ERROR([fc14audiodecoder.h not found])])
AC_CHECK_LIB(fc14audiodecoder,fc14dec_new)
if ! test "$ac_cv_lib_fc14audiodecoder_fc14dec_new" = yes; then
    AC_MSG_ERROR([libfc14audiodecoder not found])
fi

PKG_CHECK_MODULES([AUDACIOUS], [audacious >= 3.5], [],
    AC_MSG_ERROR([Could not find Audacious >= 3.5 API pkgconfig file.]))

AC_MSG_CHECKING([for Audacious plug-in directory])
audfc_plugin_dir=`$PKG_CONFIG --variable=plugin_dir audacious`
test -n "$audfc_plugin_dir" && AC_MSG_RESULT([$audfc_plugin_dir])
test "x$audfc_plugin_dir" == "x" && AC_MSG_ERROR([check failed])
AUDACIOUS_IPLUGIN_DIR="$audfc_plugin_dir/Input"
AC_SUBST([AUDACIOUS_IPLUGIN_DIR])

PKG_CHECK_MODULES([GLIB], [glib-2.0], [], AC_MSG_ERROR([Could not find GLib 2.]))
AC_SUBST([GLIB_CFLAGS])
AC_SUBST([GLIB_LIBS])

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
