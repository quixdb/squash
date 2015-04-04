set (snappy_sources
  squash-snappy.c
  crc32.c)

include (FindSnappy)

if ("${SNAPPY_FOUND}" STREQUAL "" OR "${FORCE_IN_TREE_DEPENDENCIES}" STREQUAL "yes")
  list (APPEND snappy_sources
    snappy/snappy-c.cc
    snappy/snappy-sinksource.cc
    snappy/snappy-stubs-internal.cc
    snappy/snappy.cc)
  squash_plugin_add (snappy snappy_sources)
  squash_plugin_add_include_directories (snappy snappy)
else ()
  squash_plugin_add_for_package(snappy Snappy snappy_sources)
endif ()