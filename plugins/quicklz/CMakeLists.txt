include (SquashPlugin)

squash_plugin (
  NAME quicklz
  SOURCES squash-quicklz.c
  EMBED_SOURCES quicklz.c
  EMBED_DEFINES QLZ_MEMORY_SAFE)
