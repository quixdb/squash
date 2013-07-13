if ENABLE_DOCS

DOXYGEN_SOURCES = \
  $(wildcard $(top_srcdir)/squash/*.c) \
  $(wildcard $(top_srcdir)/squash/*.h) \
  index.md \
  user-guide.md \
  @SQUASH_DOXYGEN_PATHS@

stamp: Doxyfile $(DOXYGEN_SOURCES)
	$(DOXYGEN) Doxyfile && touch $@

all: stamp

endif

CLEANFILES = \
	Doxyfile \
  stamp

GITIGNOREFILES = \
	html

EXTRA_DIST = Doxyfile

-include $(top_srcdir)/git.mk
