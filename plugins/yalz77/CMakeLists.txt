include (SquashPlugin)

squash_plugin (
  NAME yalz77
  SOURCES squash-yalz77.cpp
  INCLUDE_DIRS yalz77
  COMPILER_FLAGS
    -Wno-aggregate-return
    -Wno-variadic-macros)
