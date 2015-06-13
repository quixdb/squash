set (wflz_sources
  squash-wflz.c
  wflz/wfLZ.c)

squash_plugin_add (wflz wflz_sources)

foreach (file ${wflz_sources})
  squash_source_file_add_cflag ("${file}" "-Wno-strict-prototypes")
  squash_source_file_add_cflag ("${file}" "-Wno-undef")
endforeach ()