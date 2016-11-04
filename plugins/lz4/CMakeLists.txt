include (SquashPlugin)

squash_plugin (
  NAME lz4
  EXTERNAL_PKG liblz4
  SOURCES
    squash-lz4.c
    squash-lz4f.c
  EMBED_SOURCES
    lz4/lib/lz4.c
    lz4/lib/lz4hc.c
    lz4/lib/lz4frame.c
    lz4/lib/xxhash.c
  EMBED_INCLUDE_DIRS
    lz4/lib
  EMBED_DEFINES
    SQUASH_LZ4_EMBED)
