if ENABLE_DOCS

DOXYGEN_FILES = \
  $(wildcard $(top_srcdir)/squash/*.c) \
  $(wildcard $(top_srcdir)/squash/*.h) \
  index.md \
  user-guide.md \
  @SQUASH_DOXYGEN_PATHS@

stamp: Doxyfile $(DOXYGEN_FILES)
	$(DOXYGEN) Doxyfile && touch $@

all: stamp

endif

man_MANS = \
	squash.1

CLEANFILES = \
	Doxyfile \
  stamp

GITIGNOREFILES = \
	html

EXTRA_DIST = Doxyfile

-include $(top_srcdir)/git.mk
