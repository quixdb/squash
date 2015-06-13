set (zlib_sources squash-zlib.c)

include (FindZLIB)

if (NOT ZLIB_FOUND OR "${FORCE_IN_TREE_DEPENDENCIES}" STREQUAL "yes")
  set (libzlib_sources
    zlib/adler32.c
    zlib/compress.c
    zlib/crc32.c
    zlib/deflate.c
    zlib/gzclose.c
    zlib/gzlib.c
    zlib/gzread.c
    zlib/gzwrite.c
    zlib/infback.c
    zlib/inffast.c
    zlib/inflate.c
    zlib/inftrees.c
    zlib/trees.c
    zlib/uncompr.c
    zlib/zutil.c)
  list (APPEND zlib_sources ${libzlib_sources})
  squash_plugin_add (zlib zlib_sources)
  squash_plugin_add_include_directories (zlib zlib)

  foreach (file ${libzlib_sources})
    squash_source_file_add_cflag ("${file}" "-Wno-implicit-function-declaration")
  endforeach ()
else ()
  squash_plugin_add_for_package(zlib ZLIB zlib_sources)
endif ()
