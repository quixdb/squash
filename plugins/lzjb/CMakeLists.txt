include (SquashPlugin)

squash_plugin (
  NAME lzjb
  SOURCES squash-lzjb.c
  EMBED_SOURCES lzjb/lzjb.c
  INCLUDE_DIRS lzjb)
