include (SquashPlugin)

squash_plugin (
  NAME ncompress
  NO_BIG_ENDIAN
  SOURCES squash-ncompress.c
  EMBED_SOURCES compress.c)
