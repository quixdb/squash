include (SquashPlugin)

squash_plugin (
  NAME zstd
  SOURCES squash-zstd.c
  EMBED_SOURCES
    zstd/lib/zstd.c
    zstd/lib/zstdhc.c
    zstd/lib/fse.c
    zstd/lib/huff0.c
    zstd/lib/legacy/zstd_v01.c
    EMBED_INCLUDE_DIRS
      zstd/lib
      zstd/lib/legacy)
