compress_stream_SOURCES = \
  compress-stream.vala

compress_stream_VALAFLAGS = \
  --target-glib=$(GLIB_TARGET_VERSION) \
  --vapidir $(top_srcdir)/bindings/vala \
  --pkg squash-@SQUASH_API_VERSION@

compress_stream_CFLAGS = \
  $(GLIB_CFLAGS) \
  $(GOBJECT_LIBS) \
  $(GOBJECT_CFLAGS) \
  -w \
  -I$(top_srcdir) \
  -DSQUASH_IS_UNSTABLE

compress_stream_LDADD = \
  $(top_builddir)/squash/libsquash@SQUASH_API_VERSION@.la \
  $(GLIB_LIBS) \
  $(GOBJECT_LIBS) \
  $(GOBJECT_LIBS)

noinst_PROGRAMS =

if HAVE_GLIB
if HAVE_VALAC
noinst_PROGRAMS += compress-stream
endif
endif

GITIGNOREFILES = \
  $(compress_stream_SOURCES:.vala=.c) \
  compress_stream_vala.stamp

include $(top_srcdir)/git.mk
