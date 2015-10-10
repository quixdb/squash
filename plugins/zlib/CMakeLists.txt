include (SquashPlugin)

squash_plugin (
  NAME zlib
  SOURCES squash-zlib.c
  EXTERNAL_PKG zlib
  EMBED_SOURCES
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
    zlib/zutil.c
  INCLUDE_DIRS
    zlib
  COMPILER_FLAGS
    -Wno-implicit-function-declaration)

#   foreach (file ${libzlib_sources})
#     squash_source_file_add_compile_flag ("${file}" "-Wno-implicit-function-declaration")
#   endforeach ()
# else ()
#   squash_plugin_add_for_package(zlib ZLIB zlib_sources)
# endif ()
