COMMON_CFLAGS = \
   -I$(top_srcdir) \
   @WARN_CFLAGS@ \
  -DSQUASH_IS_UNSTABLE \
  -DSQUASH_COMPILATION

COMMON_LIBS = $(top_builddir)/squash/libsquash@SQUASH_API_VERSION@.la

IPP_LIBS = \
  -lippdc

if ENABLE_IPP

plugindir = $(libdir)/squash/@SQUASH_API_VERSION@/plugins/ipp
plugin_DATA = ipp.codecs

plugin_LTLIBRARIES = \
	libsquash@SQUASH_API_VERSION@-plugin-ipp.la

endif

libsquash@SQUASH_API_VERSION@_plugin_ipp_la_CFLAGS = \
	$(COMMON_CFLAGS) \
  $(IPP_CFLAGS)

libsquash@SQUASH_API_VERSION@_plugin_ipp_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)

libsquash@SQUASH_API_VERSION@_plugin_ipp_la_LIBADD = \
	$(COMMON_LIBS) \
  $(IPP_LIBS)

libsquash@SQUASH_API_VERSION@_plugin_ipp_la_SOURCES = squash-ipp.c

EXTRA_DIST = \
	$(plugin_DATA) \
	ipp.md

include $(top_srcdir)/git.mk
