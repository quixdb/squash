include (SquashPlugin)

squash_plugin (
  NAME gipfeli
  SOURCES squash-gipfeli.cpp
  EMBED_SOURCES
    gipfeli/decompress.cc
    gipfeli/entropy.cc
    gipfeli/entropy_code_builder.cc
    gipfeli/lz77.cc
    gipfeli/gipfeli-internal.cc
  EMBED_INCLUDE_DIRS
    gipfeli)
