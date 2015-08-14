set (wflz_sources
  squash-wflz.c
  wflz/wfLZ.c)

if ("${ENABLE_WFLZ}" STREQUAL "yes")
  squash_plugin_add (wflz wflz_sources)
endif ()

foreach (file ${wflz_sources})
  squash_source_file_add_compile_flag ("${file}" "-Wno-strict-prototypes")
  squash_source_file_add_compile_flag ("${file}" "-Wno-undef")
endforeach ()