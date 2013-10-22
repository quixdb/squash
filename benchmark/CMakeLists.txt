add_executable (benchmark
  benchmark.c
  timer.c)
target_link_libraries (benchmark squash${SQUASH_VERSION_API})
