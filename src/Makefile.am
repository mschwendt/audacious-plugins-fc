EXTRA_DIST = config.h.in

pkglibdir = $(AUDACIOUS_IPLUGIN_DIR)

pkglib_LTLIBRARIES = @PLUGINNAME@.la

@PLUGINNAME@_la_LDFLAGS = -module -avoid-version -no-undefined
@PLUGINNAME@_la_CXXFLAGS = $(AUDACIOUS_CFLAGS) $(DECODER_CFLAGS)
@PLUGINNAME@_la_LIBADD = $(AUDACIOUS_LIBS) $(DECODER_LIBS)
@PLUGINNAME@_la_SOURCES = \
	audfc.cpp \
	audfc.h \
	configure.cpp \
	configure.h
