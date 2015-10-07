set (heatshrink_sources
  squash-heatshrink.c
  heatshrink/heatshrink_encoder.c
  heatshrink/heatshrink_decoder.c)

squash_plugin_add (heatshrink heatshrink_sources)
squash_source_file_set_c99 (heatshrink_sources)
