# This adds -Wl,--no-undefined (if it works).  It should help identify
# issues, especially when updating submodules.

include (CheckCCompilerFlag)

set (OLD_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
set (CMAKE_REQUIRED_FLAGS "-Wl,--no-undefined")
check_c_compiler_flag ("" CFLAG_Wl___no_undefined)
if (CFLAG_Wl___no_undefined)
  set (CMAKE_SHARED_LINKER_FLAGS "-Wl,--no-undefined ${CMAKE_SHARED_LINKER_FLAGS}")
  set (CMAKE_MODULE_LINKER_FLAGS "-Wl,--no-undefined ${CMAKE_MODULE_LINKER_FLAGS}")
  set (CMAKE_EXE_LINKER_FLAGS "-Wl,--no-undefined ${CMAKE_EXE_LINKER_FLAGS}")
endif ()
set (CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")