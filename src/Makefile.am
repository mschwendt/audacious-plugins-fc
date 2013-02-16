EXTRA_DIST = config.h.in

pkglibdir = $(AUDACIOUS_IPLUGIN_DIR)

pkglib_LTLIBRARIES = fcdecoder.la

fcdecoder_la_LDFLAGS = -module -avoid-version
fcdecoder_la_CFLAGS = $(AUDACIOUS_CFLAGS) $(GLIB_CFLAGS)
fcdecoder_la_LIBADD = $(AUDACIOUS_LIBS)

fcdecoder_la_SOURCES = \
configure.c \
configure.h \
main.c \
plugin.c
