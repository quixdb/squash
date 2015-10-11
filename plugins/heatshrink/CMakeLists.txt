include (SquashPlugin)

squash_plugin(
  NAME heatshrink
  SOURCES squash-heatshrink.c
  EMBED_SOURCES
    heatshrink/heatshrink_encoder.c
    heatshrink/heatshrink_decoder.c)