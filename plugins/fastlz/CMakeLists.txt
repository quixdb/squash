include (SquashPlugin)

squash_plugin (
  NAME fastlz
  SOURCES squash-fastlz.c
  EMBED_SOURCES fastlz/fastlz.c)
