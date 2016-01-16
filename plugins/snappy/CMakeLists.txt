include (SquashPlugin)
include (FindSnappy)
include (CheckTypeSize)

CHECK_INCLUDE_FILE ("stdint.h"   HAVE_STDINT_H)
CHECK_INCLUDE_FILE ("stddef.h"   HAVE_STDDEF_H)
CHECK_INCLUDE_FILE ("sys/uio.h"  HAVE_SYS_UIO_H)
CHECK_TYPE_SIZE    ("ssize_t"    HAVE_SSIZE_T)

configure_file (snappy-stubs-public.h.in snappy-stubs-public.h)
configure_file (config.h.in config.h)

set_compiler_specific_flags(
  VARIABLE embed_compiler_flags
  MSVC
    /wd4267
    /wd4018)

squash_plugin (
  NAME snappy
  SOURCES squash-snappy.c
  EXTERNAL_PKG_PREFIX "SNAPPY"
  EMBED_SOURCES
    snappy/snappy-c.cc
    snappy/snappy-sinksource.cc
    snappy/snappy-stubs-internal.cc
    snappy/snappy.cc
  EMBED_INCLUDE_DIRS
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/snappy
  EMBED_DEFINES
    HAVE_CONFIG_H
  EMBED_COMPILER_FLAGS
    ${embed_compiler_flags})
