include (SquashPlugin)

squash_plugin (
  NAME zpaq
  SOURCES squash-zpaq.cpp
  EMBED_SOURCES zpaq/libzpaq.cpp
  COMPILER_FLAGS "-include \"${CMAKE_CURRENT_SOURCE_DIR}/zpaq-config.h\""
  INCLUDE_DIRS zpaq)
