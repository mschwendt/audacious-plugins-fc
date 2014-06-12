AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src

EXTRA_DIST = m4 audacious-plugin-fc.metainfo.xml

appdatadir = $(datadir)/appdata

appdata_DATA = audacious-plugin-fc.metainfo.xml

dist-hook:
		@RM@ -f $(distdir)/src/config.h
