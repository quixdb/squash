include (SquashPlugin)

squash_plugin (
  NAME zling
  SOURCES squash-zling.cpp
  EMBED_SOURCES
    libzling/src/libzling.cpp
    libzling/src/libzling_huffman.cpp
    libzling/src/libzling_lz.cpp
    libzling/src/libzling_utils.cpp
  INCLUDE_DIRS libzling/src
  COMPILER_FLAGS
    -Wno-undef
    -Wno-variadic-macros)
