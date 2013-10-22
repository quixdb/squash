include (CheckFunctionExists)

set (CMAKE_REQUIRED_LIBRARIES_orig ${CMAKE_REQUIRED_LIBRARIES})
check_function_exists (clock_gettime res)
if (NOT res EQUAL 1)
  set (CMAKE_REQUIRED_LIBRARIES rt)
  check_function_exists (clock_gettime CLOCK_GETTIME_REQUIRES_RT)
endif ()
set (CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES_orig})
