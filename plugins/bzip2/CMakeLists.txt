include (SquashPlugin)
include (FindBZip2)

squash_plugin (
  NAME bzip2
  SOURCES squash-bzip2.c
  EXTERNAL_PKG_PREFIX BZIP2
  EMBED_SOURCES
    bzip2/blocksort.c
    bzip2/bzlib.c
    bzip2/compress.c
    bzip2/crctable.c
    bzip2/decompress.c
    bzip2/huffman.c
    bzip2/randtable.c
  EMBED_INCLUDE_DIRS bzip2)
