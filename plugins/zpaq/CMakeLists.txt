include (SquashPlugin)

squash_plugin (
  NAME zpaq
  SOURCES squash-zpaq.cpp
  EMBED_SOURCES zpaq/libzpaq.cpp
  COMPILER_FLAGS
    "-include \"${CMAKE_CURRENT_SOURCE_DIR}/zpaq-config.h\""
    -Wno-cast-align
  INCLUDE_DIRS zpaq
  EMBED_COMPILER_FLAGS
    -Wno-shift-negative-value
    /wd4244)
