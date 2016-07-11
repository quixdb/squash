include (RequireStandard)

add_definitions (-DSQUASH_COMPILATION)

find_package(RAGEL 6.6)

# To make lcov happy
if (NOT "${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
  ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/squash-ini.rl
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/squash-ini.rl ${CMAKE_CURRENT_BINARY_DIR}/squash-ini.rl
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/squash-ini.rl)
endif ()

if (RAGEL_FOUND)
  RAGEL_TARGET(squash_ini ${CMAKE_CURRENT_BINARY_DIR}/squash-ini.rl ${CMAKE_CURRENT_BINARY_DIR}/squash-ini.c COMPILE_FLAGS -L)
  set (SQUASH_INI "${CMAKE_CURRENT_BINARY_DIR}/squash-ini.c")
else ()
  set (SQUASH_INI squash-ini.c)
endif ()

set (squash_SOURCES
  ${SQUASH_INI}
  squash-buffer.c
  squash-charset.c
  squash-codec.c
  squash-file.c
  squash-license.c
  squash-memory.c
  squash-options.c
  squash-status.c
  squash-buffer-stream.c
  squash-context.c
  squash-object.c
  squash-plugin.c
  squash-splice.c
  squash-stream.c
  squash-util.c
  squash-version.c
  tinycthread/source/tinycthread.c)

if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  list (APPEND squash_SOURCES
    squash-mapped-file.c)
else ()
  list (APPEND squash_SOURCES
    win-iconv/win_iconv.c)
endif ()

add_library (squash${SQUASH_VERSION_API} SHARED ${squash_SOURCES})
target_add_extra_warning_flags (squash${SQUASH_VERSION_API})
squash_set_target_visibility (squash${SQUASH_VERSION_API} hidden)
target_require_c_standard (squash${SQUASH_VERSION_API} "c99")

set_target_properties ("squash${SQUASH_VERSION_API}" PROPERTIES
  SOVERSION "${SQUASH_SOVERSION_CURRENT}.${SQUASH_SOVERSION_REVISION}.${SQUASH_SOVERSION_AGE}"
  VERSION   "${SQUASH_SOVERSION_CURRENT}.${SQUASH_SOVERSION_REVISION}")

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  set_property (TARGET "squash${SQUASH_VERSION_API}"
    APPEND PROPERTY INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/win-iconv")
  source_file_add_compiler_flags ("win-iconv/win_iconv.c" "-Wno-sign-compare" "-Wno-maybe-uninitialized")
endif ()

if (NOT CMAKE_BUILD_TYPE MATCHES "Release")
  CHECK_CXX_COMPILER_FLAG ("-Wno-tautological-pointer-compare" "CFLAG__Wno_tautological_pointer_compare")

  if (CFLAG__Wno_tautological_pointer_compare)
    target_add_compiler_flags (squash${SQUASH_VERSION_API} "-Wno-tautological-pointer-compare")
  endif ()
endif ()

if ($CMAKE_VERSION VERSION_LESS 3.1)
  target_link_libraries (squash${SQUASH_VERSION_API} ${CMAKE_THREAD_LIBS_INIT})
else()
  target_link_libraries (squash${SQUASH_VERSION_API} Threads::Threads)
endif()

# For TinyCThread
find_package(ClockGettime)
if(ClockGettime_FOUND)
  target_link_libraries(squash${SQUASH_VERSION_API} ${ClockGettime_LIBRARIES})
endif()

include_directories("${CMAKE_SOURCE_DIR}")
include_directories("${CMAKE_BINARY_DIR}")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

squash_target_add_coverage (squash${SQUASH_VERSION_API})

include(CheckPrototypeExists)
set (orig_required_definitions ${CMAKE_REQUIRED_DEFINITIONS})

list (APPEND CMAKE_REQUIRED_DEFINITIONS -D_BSD_SOURCE)
check_prototype_exists ("fread_unlocked" "stdio.h" "HAVE_FREAD_UNLOCKED")
check_prototype_exists ("fwrite_unlocked" "stdio.h" "HAVE_FWRITE_UNLOCKED")
check_prototype_exists ("fflush_unlocked" "stdio.h" "HAVE_FFLUSH_UNLOCKED")
check_prototype_exists ("flockfile" "stdio.h" "HAVE_FLOCKFILE")
set (CMAKE_REQUIRED_DEFINITIONS ${orig_required_definitions})

list (APPEND CMAKE_REQUIRED_DEFINITIONS -D_ISOC11_SOURCE -D_POSIX_C_SOURCE=200112L)
check_prototype_exists ("aligned_alloc" "stdlib.h" "HAVE_ALIGNED_ALLOC")
check_prototype_exists ("posix_memalign" "stdlib.h" "HAVE_POSIX_MEMALIGN")
check_prototype_exists ("_aligned_malloc" "malloc.h" "HAVE__ALIGNED_MALLOC")
check_prototype_exists ("__mingw_aligned_malloc" "malloc.h" "HAVE___MINGW_ALIGNED_MALLOC")
set (CMAKE_REQUIRED_DEFINITIONS ${orig_required_definitions})

list (APPEND CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
check_prototype_exists ("secure_getenv" "stdlib.h" "HAVE_SECURE_GETENV")
set (CMAKE_REQUIRED_DEFINITIONS ${orig_required_definitions})

check_prototype_exists ("_vscwprintf" "wchar.h;stdio.h" "HAVE__VSCWPRINTF")

if (NOT WIN32)
  target_link_libraries (squash${SQUASH_VERSION_API} ${CMAKE_DL_LIBS})

  include (FindIconv)
  target_link_libraries (squash${SQUASH_VERSION_API} ${ICONV_LIBRARIES})
  set_property (TARGET squash${SQUASH_VERSION_API}
    APPEND PROPERTY INCLUDE_DIRECTORIES ${ICONV_INCLUDE_DIR})
endif ()

cppcheck(FORCE TARGET squash${SQUASH_VERSION_API} ENABLE warning style performance portability)

install (TARGETS squash${SQUASH_VERSION_API}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES squash.h
  DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}/squash-${SQUASH_VERSION_API})
install(FILES
    squash-context.h
    squash-codec.h
    squash-file.h
    squash-license.h
    squash-memory.h
    squash-object.h
    squash-options.h
    squash-plugin.h
    squash-splice.h
    squash-status.h
    squash-stream.h
    squash-types.h
    "${CMAKE_CURRENT_BINARY_DIR}/squash-version.h"
  DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}/squash-${SQUASH_VERSION_API}/squash)

configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/SquashConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/SquashConfig.cmake")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/SquashConfig.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Squash-${SQUASH_VERSION_MAJOR}.${SQUASH_VERSION_MINOR}.${SQUASH_VERSION_REVISION}")

configure_file (squash-version.h.in squash-version.h)
configure_file (squash-config.h.in squash-config.h)
