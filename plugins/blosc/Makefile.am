COMMON_CFLAGS = \
   -I$(top_srcdir) \
   @WARN_CFLAGS@ \
  -DSQUASH_IS_UNSTABLE

BLOSC_FILES = \
  blosc/blosc/blosc.c \
  blosc/blosc/blosclz.c \
  blosc/blosc/shuffle.c

COMMON_LIBS = $(top_builddir)/squash/libsquash@SQUASH_API_VERSION@.la

if ENABLE_BLOSC

plugindir = $(libdir)/squash/@SQUASH_API_VERSION@/plugins/blosc
plugin_DATA = blosc.codecs

plugin_LTLIBRARIES = \
	libsquash@SQUASH_API_VERSION@-plugin-blosc.la

endif

libsquash@SQUASH_API_VERSION@_plugin_blosc_la_CFLAGS = \
	$(COMMON_CFLAGS)

libsquash@SQUASH_API_VERSION@_plugin_blosc_la_LDFLAGS = \
  $(PLUGIN_LIBTOOL_FLAGS)

libsquash@SQUASH_API_VERSION@_plugin_blosc_la_LIBADD = \
	$(COMMON_LIBS)

libsquash@SQUASH_API_VERSION@_plugin_blosc_la_SOURCES = \
  squash-blosc.c

if USE_SYSTEM_BLOSC
libsquash@SQUASH_API_VERSION@_plugin_blosc_la_LIBADD += -lblosc
else
libsquash@SQUASH_API_VERSION@_plugin_blosc_la_SOURCES += \
  $(BLOSC_FILES)

libsquash@SQUASH_API_VERSION@_plugin_blosc_la_CFLAGS += -O3 -Iblosc/blosc
endif

EXTRA_DIST = \
	$(plugin_DATA) \
	blosc.md

-include $(top_srcdir)/git.mk
