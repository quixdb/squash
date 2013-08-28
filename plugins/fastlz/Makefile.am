COMMON_CFLAGS = \
   -O3 \
   -I$(top_srcdir) \
   @WARN_CFLAGS@ \
  -DSQUASH_IS_UNSTABLE \
  -DSQUASH_COMPILATION

COMMON_LIBS = $(top_builddir)/squash/libsquash@SQUASH_API_VERSION@.la

if ENABLE_FASTLZ

plugindir = $(libdir)/squash/@SQUASH_API_VERSION@/plugins/fastlz
plugin_DATA = fastlz.codecs

plugin_LTLIBRARIES = \
	libsquash@SQUASH_API_VERSION@-plugin-fastlz.la

endif

libsquash@SQUASH_API_VERSION@_plugin_fastlz_la_CFLAGS = \
	$(COMMON_CFLAGS) \
  $(FASTLZ_CFLAGS)

libsquash@SQUASH_API_VERSION@_plugin_fastlz_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)

libsquash@SQUASH_API_VERSION@_plugin_fastlz_la_LIBADD = \
	$(COMMON_LIBS)

libsquash@SQUASH_API_VERSION@_plugin_fastlz_la_SOURCES = \
  squash-fastlz.c \
  fastlz.c \
  $(FASTLZ_FILES)

EXTRA_DIST = \
	$(plugin_DATA) \
	fastlz.md

include $(top_srcdir)/git.mk
