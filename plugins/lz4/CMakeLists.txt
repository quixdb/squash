include (SquashPlugin)

squash_plugin (
  NAME lz4
  SOURCES
    squash-lz4.c
    squash-lz4f.c
  EMBED_SOURCES
    lz4/lib/lz4.c
    lz4/lib/lz4hc.c
    lz4/lib/lz4frame.c
    lz4/lib/xxhash.c
  INCLUDE_DIRS
    lz4/lib)
