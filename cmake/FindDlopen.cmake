include (CheckFunctionExists)

set (CMAKE_REQUIRED_LIBRARIES_orig ${CMAKE_REQUIRED_LIBRARIES})
check_function_exists (dlopen res)
if (NOT res EQUAL 1)
  set (CMAKE_REQUIRED_LIBRARIES dl)
  check_function_exists (dlopen DLOPEN_REQUIRES_DL)
endif ()
set (CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES_orig})
