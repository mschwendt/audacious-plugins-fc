AC_PREREQ([2.72])
AC_INIT([audacious-plugin-fc], [0.9])
AC_CONFIG_SRCDIR([src/audfc.h])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_MACRO_DIR([m4])

dnl AC_CANONICAL_HOST

AC_PROG_CXX
AC_PROG_AR
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PATH_PROG(RM,rm,rm)

AC_LANG([C])
AC_C_BIGENDIAN

CXXFLAGS="$CXXFLAGS -std=gnu++11 -fvisibility=hidden"
EXPORT="__attribute__((visibility(\"default\")))"
AC_DEFINE_UNQUOTED([EXPORT], [$EXPORT], [Define to compiler syntax for public symbols])

dnl AC_HEADER_STDC
AC_CHECK_HEADER_STDBOOL
AC_TYPE_SIZE_T

AC_FUNC_MALLOC

PKG_CHECK_MODULES([LIBFC14AUDIODECODER], [libfc14audiodecoder >= 2.0.0], [],
    AC_MSG_ERROR([Could not find libfc14audiodecoder >= 2.0.0 API pkgconfig file.]))

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
