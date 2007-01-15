EXTRA_DIST = Config.h.in FC.README

pkglibdir = `${PKG_CONFIG} bmp --variable=input_plugin_dir`

pkglib_LTLIBRARIES = libfc.la

libfc_la_LDFLAGS = -module -avoid-version

libfc_la_SOURCES = \
about.c \
about.h \
configure.c \
configure.h \
FC.cpp \
FC_Defs.h \
FC.h \
LamePaula.cpp \
LamePaula.h \
Main.cpp \
MyEndian.h \
MyTypes.h \
SmartPtr.h
