include (SquashPlugin)

squash_plugin (
  NAME zstd
  SOURCES squash-zstd.c
  EMBED_SOURCES zstd/lib/zstd.c)
