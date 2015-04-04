set (plugin_sources squash-bzip2.c)

if ("${FORCE_IN_TREE_DEPENDENCIES}" STREQUAL "yes")
  list (APPEND bzip2_sources
    bzip2/blocksort.c
    bzip2/bzlib.c
    bzip2/compress.c
    bzip2/crctable.c
    bzip2/decompress.c
    bzip2/huffman.c)
  squash_plugin_add (bzip2 bzip2_sources)
else ()
  squash_plugin_add_for_package (bzip2 BZip2 bzip2_sources)
endif ()
