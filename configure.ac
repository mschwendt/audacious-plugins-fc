AC_INIT([audacious-plugins-fc], [0.8.5])
AC_CONFIG_SRCDIR([src/audfc.h])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_MACRO_DIR([m4])

dnl AC_CANONICAL_HOST

AC_PROG_CC
AC_PROG_CXX
AC_LANG([C])
AC_C_BIGENDIAN
AC_PATH_PROG(RM,rm,rm)

CXXFLAGS="$CXXFLAGS -std=gnu++11 -fvisibility=hidden"
EXPORT="__attribute__((visibility(\"default\")))"
AC_DEFINE_UNQUOTED([EXPORT], [$EXPORT], [Define to compiler syntax for public symbols])

dnl AC_HEADER_STDC

AC_CHECK_HEADER(fc14audiodecoder.h,
    [],[AC_MSG_ERROR([fc14audiodecoder.h not found])])
AC_CHECK_LIB(fc14audiodecoder,fc14dec_new)
if ! test "$ac_cv_lib_fc14audiodecoder_fc14dec_new" = yes; then
    AC_MSG_ERROR([libfc14audiodecoder not found])
fi

PKG_CHECK_MODULES([AUDACIOUS], [audacious >= 3.8], [],
    AC_MSG_ERROR([Could not find Audacious >= 3.8 API pkgconfig file.]))

AC_MSG_CHECKING([for Audacious plug-in directory])
audfc_plugin_dir=`$PKG_CONFIG --variable=plugin_dir audacious`
test -n "$audfc_plugin_dir" && AC_MSG_RESULT([$audfc_plugin_dir])
test "x$audfc_plugin_dir" == "x" && AC_MSG_ERROR([check failed])
AUDACIOUS_IPLUGIN_DIR="$audfc_plugin_dir/Input"
AC_SUBST([AUDACIOUS_IPLUGIN_DIR])

LT_INIT([disable-static])

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
