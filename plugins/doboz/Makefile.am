COMMON_CFLAGS = \
   -I$(top_srcdir) \
  -DSQUASH_IS_UNSTABLE \
  -DSQUASH_COMPILATION

COMMON_LIBS = $(top_builddir)/squash/libsquash@SQUASH_API_VERSION@.la

DOBOZ_FILES = \
  doboz/Source/Doboz/Compressor.cpp \
  doboz/Source/Doboz/Decompressor.cpp \
  doboz/Source/Doboz/Dictionary.cpp

if ENABLE_DOBOZ

plugindir = $(libdir)/squash/@SQUASH_API_VERSION@/plugins/doboz
plugin_DATA = doboz.codecs

plugin_LTLIBRARIES = \
	libsquash@SQUASH_API_VERSION@-plugin-doboz.la

endif

libsquash@SQUASH_API_VERSION@_plugin_doboz_la_CPPFLAGS = \
	$(COMMON_CFLAGS)

libsquash@SQUASH_API_VERSION@_plugin_doboz_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)

libsquash@SQUASH_API_VERSION@_plugin_doboz_la_LIBADD = \
	$(COMMON_LIBS)

libsquash@SQUASH_API_VERSION@_plugin_doboz_la_SOURCES = \
  squash-doboz.cpp \
  $(DOBOZ_FILES)

EXTRA_DIST = \
	$(plugin_DATA) \
	doboz.md

GITIGNOREFILES = \
  .libs \
  *.lo

-include $(top_srcdir)/git.mk
