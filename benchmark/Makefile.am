COMMON_CFLAGS = \
   -I$(top_srcdir) \
   @WARN_CFLAGS@ \
  -DSQUASH_IS_UNSTABLE

COMMON_LIBS = $(top_builddir)/squash/libsquash@SQUASH_API_VERSION@.la

noinst_PROGRAMS = \
  benchmark

benchmark_CFLAGS = \
  $(COMMON_CFLAGS)

benchmark_LDADD = \
  $(COMMON_LIBS) \
  -lrt

benchmark_SOURCES = \
  benchmark.c

iliad:
	wget -O $@ "http://www.gutenberg.org/files/22382/22382-h/22382-h.htm"

enwik8.bz2:
	wget -O $@ "http://cs.fit.edu.nyud.net/~mmahoney/compression/enwik8.bz2"

enwik8: enwik8.bz2
	SQUASH_PLUGINS="$(top_srcdir)/plugins" $(top_builddir)/utils/squash -kdc bzip2 $< $@

enwik9.bz2:
	wget -O $@ "http://cs.fit.edu.nyud.net/~mmahoney/compression/enwik9.bz2"

enwik9: enwik9.bz2
	SQUASH_PLUGINS="$(top_srcdir)/plugins" $(top_builddir)/utils/squash -kdc bzip2 $< $@

data.json: iliad enwik8 | benchmark
	SQUASH_PLUGINS="$(top_srcdir)/plugins" $(builddir)/benchmark -o $@ $^

benchmark.html: benchmark-header.html data.json benchmark.js benchmark-footer.html
	cat $^ > $@

GITIGNOREFILES = \
  iliad \
  enwik8 \
  enwik9 \
  enwik8.bz2 \
  enwik9.bz2 \
  data.json \
  benchmark.html

EXTRA_DIST = \
  data.json \
  benchmark-header.html \
  benchmark-footer.html

-include $(top_srcdir)/git.mk
