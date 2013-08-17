COMMON_CFLAGS = \
   -O3 \
   -I$(top_srcdir) \
   @WARN_CFLAGS@ \
  -DSQUASH_IS_UNSTABLE \
  -DSQUASH_COMPILATION

COMMON_LIBS = $(top_builddir)/squash/libsquash@SQUASH_API_VERSION@.la

if ENABLE_QUICKLZ

plugindir = $(libdir)/squash/@SQUASH_API_VERSION@/plugins/quicklz
plugin_DATA = quicklz.codecs

plugin_LTLIBRARIES = \
	libsquash@SQUASH_API_VERSION@-plugin-quicklz.la

endif

libsquash@SQUASH_API_VERSION@_plugin_quicklz_la_CFLAGS = \
	$(COMMON_CFLAGS) \
  -Wno-error=missing-prototypes

libsquash@SQUASH_API_VERSION@_plugin_quicklz_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)

libsquash@SQUASH_API_VERSION@_plugin_quicklz_la_LIBADD = \
	$(COMMON_LIBS)

libsquash@SQUASH_API_VERSION@_plugin_quicklz_la_SOURCES = \
  squash-quicklz.c \
  quicklz.c \
  $(QUICKLZ_FILES)

EXTRA_DIST = \
	$(plugin_DATA) \
	quicklz.md

-include $(top_srcdir)/git.mk
