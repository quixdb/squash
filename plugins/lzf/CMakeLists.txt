set (lzf_sources
  squash-lzf.c
  liblzf/lzf_c.c
  liblzf/lzf_d.c)

squash_plugin_add (lzf lzf_sources)
squash_plugin_add_include_directories (lzf liblzf)

squash_source_file_add_compile_flag ("squash-lzf.c" "-Wno-undef")
squash_source_file_add_compile_flag ("squash-lzf.c" "-Wno-missing-prototypes")
squash_source_file_add_compile_flag ("squash-lzf.c" "-Wno-missing-declarations")
squash_source_file_add_compile_flag ("squash-lzf.c" "-Wno-shadow")
