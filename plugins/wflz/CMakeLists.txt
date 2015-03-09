set (wflz_sources
  squash-wflz.c
  wflz/wfLZ.c)

if ("${ENABLE_WFLZ}" STREQUAL "yes")
  squash_plugin_add (wflz wflz_sources)
endif ()
