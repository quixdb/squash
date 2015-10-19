include (RequireStandard)

add_definitions (-DSQUASH_COMPILATION)

find_package(RAGEL 6.6 REQUIRED)

# To make lcov happy
if (NOT "${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
  ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ini.rl
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/ini.rl ${CMAKE_CURRENT_BINARY_DIR}/ini.rl
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ini.rl)
endif ()

RAGEL_TARGET(ini ${CMAKE_CURRENT_BINARY_DIR}/ini.rl ${CMAKE_CURRENT_BINARY_DIR}/ini.c)

set (squash_SOURCES
  ${RAGEL_ini_OUTPUTS}
  buffer.c
  codec.c
  file.c
  license.c
  options.c
  status.c
  buffer-stream.c
  context.c
  object.c
  plugin.c
  splice.c
  stream.c
  util.c
  version.c
  tinycthread/source/tinycthread.c)

if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  list (APPEND squash_SOURCES
    mapped-file.c)
endif ()

set (squash_PUBLIC_HEADERS
  context.h
  codec.h
  file.h
  license.h
  object.h
  options.h
  plugin.h
  squash.h
  splice.h
  status.h
  stream.h
  types.h)

add_library (squash${SQUASH_VERSION_API} SHARED ${squash_SOURCES})
target_add_extra_warning_flags (squash${SQUASH_VERSION_API})
squash_set_target_visibility (squash${SQUASH_VERSION_API} hidden)
target_require_c_standard (squash${SQUASH_VERSION_API} "c99")

if (NOT CMAKE_BUILD_TYPE MATCHES "Release")
  CHECK_CXX_COMPILER_FLAG ("-Wno-tautological-pointer-compare" "CFLAG__Wno_tautological_pointer_compare")

  if (CFLAG__Wno_tautological_pointer_compare)
    target_add_compiler_flags (squash${SQUASH_VERSION_API} "-Wno-tautological-pointer-compare")
  endif ()
endif ()

find_package (Threads)
target_link_libraries (squash${SQUASH_VERSION_API} ${CMAKE_THREAD_LIBS_INIT})

squash_target_add_coverage (squash${SQUASH_VERSION_API})

include(CheckPrototypeExists)
set (orig_required_definitions ${CMAKE_REQUIRED_DEFINITIONS})

list (APPEND CMAKE_REQUIRED_DEFINITIONS -D_BSD_SOURCE)
check_prototype_exists ("fread_unlocked" "stdio.h" "HAVE_FREAD_UNLOCKED")
check_prototype_exists ("fwrite_unlocked" "stdio.h" "HAVE_FWRITE_UNLOCKED")
check_prototype_exists ("fflush_unlocked" "stdio.h" "HAVE_FFLUSH_UNLOCKED")
check_prototype_exists ("flockfile" "stdio.h" "HAVE_FLOCKFILE")
set (CMAKE_REQUIRED_DEFINITIONS ${orig_required_definitions})

list (APPEND CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
check_prototype_exists ("secure_getenv" "stdlib.h" "HAVE_SECURE_GETENV")
set (CMAKE_REQUIRED_DEFINITIONS ${orig_required_definitions})

if (WIN32)
else ()
  include (FindClockGettime)
  if (${CLOCK_GETTIME_REQUIRES_RT})
    target_link_libraries (squash${SQUASH_VERSION_API} rt)
  endif ()

  include (FindDlopen)
  if (${DLOPEN_REQUIRES_DL})
    target_link_libraries (squash${SQUASH_VERSION_API} dl)
  endif ()
endif ()

install (TARGETS squash${SQUASH_VERSION_API}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${squash_PUBLIC_HEADERS} ${CMAKE_CURRENT_BINARY_DIR}/version.h
  DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}/squash-${SQUASH_VERSION_API}/squash)

configure_file (version.h.in version.h)
configure_file (config.h.in config.h)