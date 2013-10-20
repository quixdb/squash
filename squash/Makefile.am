lib_LTLIBRARIES = \
  libsquash@SQUASH_API_VERSION@.la

squash_headers = \
  squash.h \
  codec.h \
  context.h \
  object.h \
  options.h \
  plugin.h \
  status.h \
  stream.h \
  types.h \
  version.h

squashincludedir = $(includedir)/squash-$(SQUASH_API_VERSION)/squash
squashinclude_HEADERS = \
  $(squash_headers)

TINYCTHREAD_FILES = \
  tinycthread/source/tinycthread.c

libsquash@SQUASH_API_VERSION@_la_SOURCES = \
  buffer.c \
	buffer-stream.c \
  codec.c \
  context.c \
  ini.c \
  object.c \
  options.c \
  plugin.c \
  status.c \
  stream.c \
  $(TINYCTHREAD_FILES)

libsquash@SQUASH_API_VERSION@_la_CFLAGS = \
  @WARN_CFLAGS@ \
  @HIDDEN_VISIBILITY_CFLAGS@ \
  -DSQUASH_IS_UNSTABLE \
  -DSQUASH_COMPILATION \
  -DSQUASH_PLUGIN_DIR="\"$(libdir)/squash/@SQUASH_API_VERSION@/plugins\"" \
  -DSQUASH_VERSION_API="\"@SQUASH_API_VERSION@\""

libsquash@SQUASH_API_VERSION@_la_LIBADD = \
  -lltdl

if HAVE_PTHREAD
libsquash@SQUASH_API_VERSION@_la_CFLAGS += @PTHREAD_CFLAGS@ -DHAVE_PTHREAD
libsquash@SQUASH_API_VERSION@_la_LIBADD += @PTHREAD_LIBS@
endif

version.h: version.h.in
	$(AM_V_GEN) cat version.h.in | sed \
		  -e "s/@SQUASH_VERSION_MAJOR@/$(SQUASH_MAJOR_VERSION)/" \
		  -e "s/@SQUASH_VERSION_MINOR@/$(SQUASH_MINOR_VERSION)/" \
		  -e "s/@SQUASH_VERSION_MICRO@/$(SQUASH_MICRO_VERSION)/" \
		> $@

MAINTAINERCLEANFILES = \
  config.h.in

BUILT_SOURCES = \
  config.h \
  version.h

CLEANFILES = \
  stamp-h1

include $(top_srcdir)/git.mk
