set (sharc_sources
  squash-sharc.c
  squash-sharc-stream.c
  sharc/src/byte_buffer.c
  sharc/src/dictionary.c
  sharc/src/direct_hash_transform.c
  sharc/src/globals.c
  sharc/src/hash_transform.c
  sharc/src/header.c
  sharc/src/sharc.c
  sharc/src/sharc_transform.c
  sharc/src/xor_hash_transform.c)

squash_source_file_set_c99 (sharc_sources)

if ("${ENABLE_SHARC}" STREQUAL "yes")
  squash_plugin_add (sharc sharc_sources)
endif ()
