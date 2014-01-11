set (blosc_sources
  squash-blosc.c
  blosc/blosc/blosc.c
  blosc/blosc/blosclz.c
  blosc/blosc/shuffle.c)

squash_plugin_add (blosc blosc_sources)
