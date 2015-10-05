add_definitions (-DSQUASH_COMPILATION)

configure_file (version.h.in version.h)

find_package(RAGEL 6.6 REQUIRED)

RAGEL_TARGET(ini ini.rl ${CMAKE_CURRENT_BINARY_DIR}/ini.c)

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
  mapped-file.c
  plugin.c
  splice.c
  stream.c
  util.c
  version.c
  tinycthread/source/tinycthread.c)

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

squash_source_file_set_c99 (squash_SOURCES)

add_library (squash${SQUASH_VERSION_API} SHARED ${squash_SOURCES})
add_extra_warning_cflags (TARGET squash${SQUASH_VERSION_API})

if (CFLAG__fvisibility_hidden)
  squash_target_add_cflag (squash${SQUASH_VERSION_API} -fvisibility=hidden)
  squash_target_add_ldflag (squash${SQUASH_VERSION_API} -fvisibility=hidden)
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
