enable_testing()

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package (Threads REQUIRED)

add_executable (test-squash
  munit/munit.c
  test.c
  bounds.c
  buffer.c
  file.c
  flush.c
  random-data.c
  splice.c
  stream.c
  threads.c
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
  /file/io
  /file/splice/full
  /file/splice/partial
  /file/printf
  /flush
  /random/compress
  /random/decompress
  /splice/custom
  /stream/compress
  /stream/decompress
  /stream/single-byte
  /threads/buffer)

add_definitions(-DSQUASH_TEST_PLUGIN_DIR="${CMAKE_BINARY_DIR}/plugins")

set_compiler_specific_flags(
  VARIABLE extra_compiler_flags
  INTEL -wd3179)

target_add_extra_warning_flags (test-squash)
target_link_libraries (test-squash squash${SQUASH_VERSION_API})
target_link_libraries(test-squash ${CMAKE_THREAD_LIBS_INIT})
target_require_c_standard (test-squash "c99")
target_add_compiler_flags (test-squash ${extra_compiler_flags})

foreach(test_name ${SQUASH_TESTS})
  add_test(NAME ${test_name}
    COMMAND $<TARGET_FILE:test-squash> test-squash ${test_name})
endforeach(test_name)
