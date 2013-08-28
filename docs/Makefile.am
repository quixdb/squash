DOXYGEN_FILES = \
  $(wildcard $(top_srcdir)/squash/*.c) \
  $(wildcard $(top_srcdir)/squash/*.h) \
  index.md \
  user-guide.md \
  @SQUASH_DOXYGEN_PATHS@

stamp: Doxyfile $(DOXYGEN_FILES)
	$(DOXYGEN) Doxyfile && touch $@

if ENABLE_DOCS
STAMP=stamp
else
STAMP=
endif

all: $(STAMP)

man_MANS = \
	squash.1

CLEANFILES = \
	Doxyfile \
  stamp

GITIGNOREFILES = \
	html

EXTRA_DIST = Doxyfile

include $(top_srcdir)/git.mk
