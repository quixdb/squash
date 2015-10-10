include (SquashPlugin)

squash_plugin (
  NAME fari
  SOURCES squash-fari.c
  EMBED_SOURCES FastARI/FastAri.c
  EMBED_DEFINES FA_USE_EOF)
