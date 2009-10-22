EXTRA_DIST = Config.h.in FC.README

pkglibdir = `${PKG_CONFIG} audacious --variable=input_plugin_dir`

pkglib_LTLIBRARIES = libfc.la

libfc_la_LDFLAGS = -module -avoid-version

libfc_la_SOURCES = \
about.c \
about.h \
configure.c \
configure.h \
Main.cpp \
plugin.c \
 \
FC.cpp \
FC_Defs.h \
FC.h \
LamePaula.cpp \
LamePaula.h \
MyEndian.h \
MyTypes.h \
SmartPtr.h
