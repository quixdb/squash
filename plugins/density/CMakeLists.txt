set (density_sources
  squash-density.c
  density/src/block_decode.c
  density/src/block_encode.c
  density/src/block_footer.c
  density/src/block_header.c
  density/src/block_mode_marker.c
  density/src/buffer.c
  density/src/globals.c
  density/src/kernel_chameleon_decode.c
  density/src/kernel_chameleon_dictionary.c
  density/src/kernel_chameleon_encode.c
  density/src/kernel_mandala_decode.c
  density/src/kernel_mandala_dictionary.c
  density/src/kernel_mandala_encode.c
  density/src/main_decode.c
  density/src/main_encode.c
  density/src/main_footer.c
  density/src/main_header.c
  density/src/memory_location.c
  density/src/memory_teleport.c
  density/src/spookyhash/src/context.c
  density/src/spookyhash/src/spookyhash.c
  density/src/stream.c)

squash_plugin_add (density density_sources)
squash_source_file_set_c99 (density_sources)
