SUBDIRS = . examples

vapidir = $(libdir)/pkgconfig
vapi_DATA = squash-@SQUASH_API_VERSION@.vapi

squash-@SQUASH_API_VERSION@.vapi: squash.vapi
	$(AM_V_GEN) cat < $^ > $@

CLEANFILES = squash-@SQUASH_API_VERSION@.vapi

-include $(top_srcdir)/git.mk
