include (SquashPlugin)

squash_plugin (
  NAME pithy
  DEFAULT_DISABLED
  SOURCES squash-pithy.c
  EMBED_SOURCES pithy/pithy.c
  INCLUDE_DIRS pithy)
