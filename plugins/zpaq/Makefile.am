if ENABLE_ZPAQ
plugindir = $(libdir)/squash/@SQUASH_API_VERSION@/plugins/zpaq
plugin_DATA = zpaq.codecs

plugin_LTLIBRARIES = \
	libsquash@SQUASH_API_VERSION@-plugin-zpaq.la
endif

LIBZPAQ_FILES = \
  zpaq/libzpaq.cpp \
  zpaq/divsufsort.c

libsquash@SQUASH_API_VERSION@_plugin_zpaq_la_CPPFLAGS = \
   -O3 \
   -I$(top_srcdir) \
   -I$(srcdir)/zpaq \
  -DSQUASH_IS_UNSTABLE \
  $(OPENMP_CXXFLAGS) \
  -include $(srcdir)/zpaq-config.h

libsquash@SQUASH_API_VERSION@_plugin_zpaq_la_LDFLAGS = \
  $(PLUGIN_LIBTOOL_FLAGS) \
  $(OPENMP_CXXFLAGS)

libsquash@SQUASH_API_VERSION@_plugin_zpaq_la_LIBADD = \
  $(top_builddir)/squash/libsquash@SQUASH_API_VERSION@.la

libsquash@SQUASH_API_VERSION@_plugin_zpaq_la_SOURCES = \
  $(LIBZPAQ_FILES) \
  squash-zpaq.cpp

EXTRA_DIST = \
	$(plugin_DATA) \
	zpaq.md \
  zpaq-config.h

include $(top_srcdir)/git.mk
