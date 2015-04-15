set (snappy_sources
  squash-snappy.c
  crc32.c)

include (FindSnappy)
find_package(Snappy)

if (NOT SNAPPY_FOUND OR "${FORCE_IN_TREE_DEPENDENCIES}" STREQUAL "yes")
  list (APPEND snappy_sources
    snappy/snappy-c.cc
    snappy/snappy-sinksource.cc
    snappy/snappy-stubs-internal.cc
    snappy/snappy.cc)
  CHECK_INCLUDE_FILE ("stdint.h"   HAVE_STDINT_H)
  CHECK_INCLUDE_FILE ("stddef.h"   HAVE_STDDEF_H)
  CHECK_INCLUDE_FILE ("sys/uio.h"  HAVE_SYS_UIO_H)
  squash_plugin_add (snappy snappy_sources)
  configure_file (snappy-stubs-public.h.in ${CMAKE_CURRENT_BINARY_DIR}/snappy-stubs-public.h)
  squash_plugin_add_include_directories (snappy
    snappy
    ${CMAKE_CURRENT_BINARY_DIR})
else ()
  squash_plugin_add_for_package(snappy Snappy snappy_sources)
endif ()