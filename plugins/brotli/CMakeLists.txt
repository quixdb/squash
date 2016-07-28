include (SquashPlugin)

set(BROTLI_BUNDLED_MODE ON)
add_subdirectory(brotli)

if(CMAKE_VERSION VERSION_LESS 2.8.11)
  include_directories(${BROTLI_INCLUDE_DIRS})
endif()

SQUASH_PLUGIN(
  SOURCES squash-brotli.c
  NAME brotli
  C_STANDARD c99
  EMBED_TARGET ${BROTLI_LIBRARIES}
  INCLUDE_DIRS ${BROTLI_INCLUDE_DIRS})
