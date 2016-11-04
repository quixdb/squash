include (RequireStandard)
include (AddCompilerFlags)


add_executable (squash squash.c parg/parg.c)
target_add_extra_warning_flags (squash)
target_link_libraries (squash squash${SQUASH_VERSION_API})

install (TARGETS squash
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})


if (ENABLE_FUZZER)
  file (GLOB libFuzzer_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/libFuzzer/*.cpp")

  add_executable (squash-fuzzer squash-fuzzer.c ${libFuzzer_SOURCES})
  target_require_cxx_standard (squash-fuzzer "c++11")
  target_link_libraries (squash-fuzzer squash${SQUASH_VERSION_API})
endif (ENABLE_FUZZER)
