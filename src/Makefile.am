EXTRA_DIST = config.h.in

pkglibdir = `${PKG_CONFIG} audacious --variable=input_plugin_dir`

pkglib_LTLIBRARIES = fcdecoder.la

fcdecoder_la_LDFLAGS = -module -avoid-version
fcdecoder_la_CFLAGS = $(audacious_CFLAGS)
fcdecoder_la_LIBADD = $(audacious_LIBS)

fcdecoder_la_SOURCES = \
about.c \
about.h \
configure.c \
configure.h \
main.c \
plugin.c
