ACLOCAL_AMFLAGS=-I m4

SUBDIRS = squash docs examples bindings . tests

squash-@SQUASH_API_VERSION@.pc: squash.pc
	$(AM_V_GEN) cat < $^ > $@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = \
	squash-@SQUASH_API_VERSION@.pc \
	$(NULL)

MAINTAINERCLEANFILES = \
  $(GITIGNORE_MAINTAINERCLEANFILES_TOPLEVEL) \
  $(GITIGNORE_MAINTAINERCLEANFILES_MAKEFILE_IN) \
  $(GITIGNORE_MAINTAINERCLEANFILES_M4_LIBTOOL) \
  squash-@SQUASH_API_VERSION@.pc \
  test-driver

GITIGNOREFILES = \
  ChangeLog \
  INSTALL

-include $(top_srcdir)/git.mk
