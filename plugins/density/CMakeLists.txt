include (SquashPlugin)

set_compiler_specific_flags(
  VARIABLE compiler_flags
  ICC -wd858)

squash_plugin (
  NAME density
  SOURCES squash-density
  C_STANDARD c99
  EMBED_SOURCES
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
    density/src/kernel_cheetah_decode.c
    density/src/kernel_cheetah_dictionary.c
    density/src/kernel_cheetah_encode.c
    density/src/kernel_lion_decode.c
    density/src/kernel_lion_dictionary.c
    density/src/kernel_lion_encode.c
    density/src/kernel_lion_form_model.c
    density/src/main_decode.c
    density/src/main_encode.c
    density/src/main_footer.c
    density/src/main_header.c
    density/src/memory_location.c
    density/src/memory_teleport.c
    density/src/stream.c
    density/src/spookyhash/src/context.c
    density/src/spookyhash/src/spookyhash.c
  EMBED_COMPILER_FLAGS
    ${compiler_flags})
