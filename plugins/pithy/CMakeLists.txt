set (pithy_sources
  squash-pithy.c
  pithy/pithy.c)

squash_plugin_add (pithy pithy_sources)
squash_plugin_add_include_directories (pithy pithy)
