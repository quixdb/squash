add_definitions (-DSQUASH_COMPILATION)

set (squash_SOURCES
  buffer.c
  codec.c
  ini.c
  options.c
  status.c
  buffer-stream.c
  context.c
  object.c
  plugin.c
  stream.c
  tinycthread/source/tinycthread.c)

set (squash_PUBLIC_HEADERS
  context.h
  codec.h
  object.h
  options.h
  plugin.h
  squash.h
  status.h
  stream.h
  types.h
  version.h)

add_library (squash${SQUASH_VERSION_API} SHARED ${squash_SOURCES})
add_extra_warning_flags (TARGET squash${SQUASH_VERSION_API})

if (CFLAG__fvisibility_hidden)
  squash_target_add_cflag (squash${SQUASH_VERSION_API} -fvisibility=hidden)
  squash_target_add_ldflag (squash${SQUASH_VERSION_API} -fvisibility=hidden)
endif ()

find_package (Threads)
target_link_libraries (squash${SQUASH_VERSION_API} ${CMAKE_THREAD_LIBS_INIT})

if (WIN32)
else ()
  include (FindClockGettime)
  if (${CLOCK_GETTIME_REQUIRES_RT})
    target_link_libraries (squash${SQUASH_VERSION_API} rt)
  endif ()
  target_link_libraries (squash${SQUASH_VERSION_API} dl)
endif ()

install (TARGETS squash${SQUASH_VERSION_API}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${squash_PUBLIC_HEADERS}
  DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}/squash-${SQUASH_VERSION_API}/squash)
