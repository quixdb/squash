set (gipfeli_sources
  squash-gipfeli.cpp
  gipfeli/decompress.cc
  gipfeli/entropy.cc
  gipfeli/entropy_code_builder.cc
  gipfeli/lz77.cc
  gipfeli/gipfeli-internal.cc)

squash_plugin_add (gipfeli gipfeli_sources)
squash_plugin_add_include_directories (gipfeli gipfeli)
