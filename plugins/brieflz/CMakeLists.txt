include (SquashPlugin)

SQUASH_PLUGIN(
  SOURCES squash-brieflz.c
  NAME brieflz
  EMBED_SOURCES brieflz/brieflz.c brieflz/depacks.c
  INCLUDE_DIRS brieflz)
