enable_testing()

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)

set(SQUASH_TEST_SOURCES
  munit/munit.c
  test.c
  bounds.c
  buffer.c
  file.c
  flush.c
  interop.c
  random-data.c
  splice.c
  stream.c
  threads.c
  version.c
  ../squash/tinycthread/source/tinycthread.c)

set (SQUASH_TESTS
  /buffer/basic
  /buffer/single-byte
  /bounds/decode/exact
  /bounds/decode/small
  /bounds/decode/tiny
  /bounds/encode/exact
  /bounds/encode/small
  /bounds/encode/tiny
  /bounds/decode/truncated
  /file/io
  /file/splice/full
  /file/splice/partial
  /file/printf
  /flush
  /interop/basic
  /random/compress
  /random/decompress
  /splice/custom
  /stream/compress
  /stream/decompress
  /stream/single-byte
  /threads/buffer
  /version)

set_compiler_specific_flags(
  VARIABLE extra_compiler_flags
  INTEL -wd3179)

find_package(ClockGettime)
if(ClockGettime_FOUND)
  add_definitions(-DMUNIT_ALLOW_CLOCK_GETTIME)
  link_libraries(${ClockGettime_LIBRARIES})
endif()

add_executable (test-squash ${SQUASH_TEST_SOURCES})
set_property(TARGET test-squash
  APPEND PROPERTY COMPILE_DEFINITIONS "SQUASH_TEST_PLUGIN_DIR=\"${CMAKE_BINARY_DIR}/plugins\"")
set_property(TARGET test-squash
  APPEND PROPERTY COMPILE_DEFINITIONS "SQUASH_TEST_DATA_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/data\"")
target_add_extra_warning_flags (test-squash)
target_require_c_standard (test-squash "c99")
target_add_compiler_flags (test-squash ${extra_compiler_flags})
target_include_directories (test-squash PRIVATE "${CMAKE_SOURCE_DIR}/squash")

if ($CMAKE_VERSION VERSION_LESS 3.1)
  target_link_libraries (test-squash squash${SQUASH_VERSION_API} ${CMAKE_THREAD_LIBS_INIT})
else()
  target_link_libraries (test-squash squash${SQUASH_VERSION_API} Threads::Threads)
endif()

if (ENABLE_INSTALLED_TESTS)
  add_executable (squash-all ${SQUASH_TEST_SOURCES})
  target_add_extra_warning_flags (squash-all)
  target_link_libraries (squash-all squash${SQUASH_VERSION_API} ${CMAKE_THREAD_LIBS_INIT})
  target_require_c_standard (squash-all "c99")
  target_add_compiler_flags (squash-all ${extra_compiler_flags})
  target_include_directories (squash-all PRIVATE "${CMAKE_SOURCE_DIR}/squash")

  install (TARGETS squash-all
    EXPORT "squash-all"
    RUNTIME DESTINATION "${CMAKE_INSTALL_LIBEXECDIR}/installed-tests/squash")
endif()

if (WIN32)
  add_custom_command(TARGET test-squash POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:squash${SQUASH_VERSION_API}>" "$<TARGET_FILE_DIR:test-squash>")
endif ()

foreach(test_name ${SQUASH_TESTS})
  add_test(NAME ${test_name}
    COMMAND $<TARGET_FILE:test-squash> ${test_name})
endforeach(test_name)
