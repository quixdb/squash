include (SquashPlugin)

find_package (ZLIB)

squash_plugin (
  NAME zlib
  SOURCES squash-zlib.c
  EXTERNAL_PKG_PREFIX ZLIB
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
    -Wno-shift-negative-value
    -Wno-implicit-function-declaration)
