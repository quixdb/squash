include (SquashPlugin)

set(BRIEFLZ_BUNDLED_MODE ON)
set(BRIEFLZ_TEST_ENABLE TRUE)
add_subdirectory (brieflz)

if(CMAKE_VERSION VERSION_LESS 2.8.11)
  include_directories(${BRIEFLZ_INCLUDE_DIRS})
  add_definitions(${BRIEFLZ_DEFINITIONS})
endif()

SQUASH_PLUGIN(
  SOURCES squash-brieflz.c
  NAME brieflz
  EMBED_TARGET brieflz
  INCLUDE_DIRS ${BRIEFLZ_INCLUDE_DIRS})
