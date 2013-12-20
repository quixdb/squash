set (fari_sources
  squash-fari.c
  FastARI/FastAri.c)

if ("${ENABLE_FARI}" STREQUAL "yes")
  add_definitions(-DFA_USE_EOF)

  squash_plugin_add (fari fari_sources)
endif ()
