include (SquashPlugin)
include (FindSnappy)

CHECK_INCLUDE_FILE ("stdint.h"   HAVE_STDINT_H)
CHECK_INCLUDE_FILE ("stddef.h"   HAVE_STDDEF_H)
CHECK_INCLUDE_FILE ("sys/uio.h"  HAVE_SYS_UIO_H)
configure_file (snappy-stubs-public.h.in snappy-stubs-public.h)

squash_plugin (
  NAME snappy
  SOURCES squash-snappy.c crc32.c
  EXTERNAL_PKG_PREFIX "SNAPPY"
  EMBED_SOURCES
    snappy/snappy-c.cc
    snappy/snappy-sinksource.cc
    snappy/snappy-stubs-internal.cc
    snappy/snappy.cc
  INCLUDE_DIRS
    ${CMAKE_CURRENT_BINARY_DIR})
