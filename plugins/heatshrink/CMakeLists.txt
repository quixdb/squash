include (SquashPlugin)

squash_plugin(
  NAME heatshrink
  SOURCES squash-heatshrink.c
  C_STANDARD c99
  EMBED_SOURCES
    heatshrink/heatshrink_encoder.c
    heatshrink/heatshrink_decoder.c)