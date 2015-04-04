set (lzo_sources
  squash-lzo.c)

include (FindLZO)

if (NOT "${LZO_FOUND}" STREQUAL "LZO_FOUND" OR "${FORCE_IN_TREE_DEPENDENCIES}" STREQUAL "yes")
  list (APPEND lzo_sources
    lzo/src/lzo1.c
    lzo/src/lzo1_99.c
    lzo/src/lzo1a.c
    lzo/src/lzo1a_99.c
    lzo/src/lzo1b_1.c
    lzo/src/lzo1b_2.c
    lzo/src/lzo1b_3.c
    lzo/src/lzo1b_4.c
    lzo/src/lzo1b_5.c
    lzo/src/lzo1b_6.c
    lzo/src/lzo1b_7.c
    lzo/src/lzo1b_8.c
    lzo/src/lzo1b_9.c
    lzo/src/lzo1b_99.c
    lzo/src/lzo1b_9x.c
    lzo/src/lzo1b_cc.c
    lzo/src/lzo1b_d1.c
    lzo/src/lzo1b_d2.c
    lzo/src/lzo1b_rr.c
    lzo/src/lzo1b_xx.c
    lzo/src/lzo1c_1.c
    lzo/src/lzo1c_2.c
    lzo/src/lzo1c_3.c
    lzo/src/lzo1c_4.c
    lzo/src/lzo1c_5.c
    lzo/src/lzo1c_6.c
    lzo/src/lzo1c_7.c
    lzo/src/lzo1c_8.c
    lzo/src/lzo1c_9.c
    lzo/src/lzo1c_99.c
    lzo/src/lzo1c_9x.c
    lzo/src/lzo1c_cc.c
    lzo/src/lzo1c_d1.c
    lzo/src/lzo1c_d2.c
    lzo/src/lzo1c_rr.c
    lzo/src/lzo1c_xx.c
    lzo/src/lzo1f_1.c
    lzo/src/lzo1f_9x.c
    lzo/src/lzo1f_d1.c
    lzo/src/lzo1f_d2.c
    lzo/src/lzo1x_1.c
    lzo/src/lzo1x_1k.c
    lzo/src/lzo1x_1l.c
    lzo/src/lzo1x_1o.c
    lzo/src/lzo1x_9x.c
    lzo/src/lzo1x_d1.c
    lzo/src/lzo1x_d2.c
    lzo/src/lzo1x_d3.c
    lzo/src/lzo1x_o.c
    lzo/src/lzo1y_1.c
    lzo/src/lzo1y_9x.c
    lzo/src/lzo1y_d1.c
    lzo/src/lzo1y_d2.c
    lzo/src/lzo1y_d3.c
    lzo/src/lzo1y_o.c
    lzo/src/lzo1z_9x.c
    lzo/src/lzo1z_d1.c
    lzo/src/lzo1z_d2.c
    lzo/src/lzo1z_d3.c
    lzo/src/lzo2a_9x.c
    lzo/src/lzo2a_d1.c
    lzo/src/lzo2a_d2.c
    lzo/src/lzo_crc.c
    lzo/src/lzo_init.c
    lzo/src/lzo_ptr.c
    lzo/src/lzo_str.c
    lzo/src/lzo_util.c)

  squash_plugin_add (lzo lzo_sources)
  squash_plugin_add_include_directories (lzo lzo/include)

  # The rest of this was stolen from lzo/CMakeLists.txt

  include(CheckFunctionExists)
  include(CheckIncludeFile)
  include(CheckLibraryExists)
  include(CheckSymbolExists)
  include(CheckTypeSize)
  include(TestBigEndian)

  # Checks for header files
  macro(mfx_check_include_file f var)
    check_include_file("${f}" "mfx_${var}")
    if(NOT ",${mfx_${var}}," STREQUAL ",,")
      add_definitions(-D${var}=1)
      set(mfx_${var} 1)
    else()
      set(mfx_${var} 0)
    endif()
  endmacro()
  # mfx_ACC_CHECK_HEADERS
  set(l assert.h ctype.h dirent.h errno.h fcntl.h float.h limits.h malloc.h memory.h setjmp.h signal.h stdarg.h stddef.h stdint.h stdio.h stdlib.h string.h strings.h time.h unistd.h utime.h sys/mman.h sys/resource.h sys/stat.h sys/time.h sys/types.h sys/wait.h)
  foreach(f ${l})
    string(TOUPPER "${f}" var)
    string(REGEX REPLACE "[^0-9A-Z_]" "_" var "${var}")
    mfx_check_include_file("${f}" "HAVE_${var}")
  endforeach()

  # Checks for typedefs and structures
  macro(mfx_check_type_size type var)
    check_type_size("${type}" "mfx_${var}")
    if("${mfx_${var}}" MATCHES "^[1-9][0-9]*$")
      add_definitions(-D${var}=${mfx_${var}})
    else()
      set(mfx_${var} 0)
    endif()
  endmacro()
  # mfx_ACC_CHECK_SIZEOF + mfx_CHECK_SIZEOF
  set(l short int long "long long" __int16 __int32 __int64 "void *" size_t ptrdiff_t intmax_t uintmax_t intptr_t uintptr_t float double "long double" dev_t fpos_t mode_t off_t ssize_t time_t)
  foreach(f ${l})
    string(TOUPPER "${f}" var)
    string(REGEX REPLACE " \\*" "_P" var "${var}")
    string(REGEX REPLACE "[^0-9A-Z_]" "_" var "${var}")
    mfx_check_type_size("${f}" "SIZEOF_${var}")
  endforeach()

  # Checks for library functions
  macro(mfx_check_function_exists func var)
    check_function_exists("${func}" "mfx_${var}")
    if(NOT ",${mfx_${var}}," STREQUAL ",,")
      add_definitions(-D${var}=1)
      set(mfx_${var} 1)
    else()
      set(mfx_${var} 0)
    endif()
  endmacro()
  # mfx_ACC_CHECK_FUNCS
  set(l access alloca atexit atoi atol chmod chown clock_getcpuclockid clock_getres clock_gettime ctime difftime fstat getenv getpagesize getrusage gettimeofday gmtime isatty localtime longjmp lstat memcmp memcpy memmove memset mkdir mktime mmap mprotect munmap qsort raise rmdir setjmp signal snprintf strcasecmp strchr strdup strerror strftime stricmp strncasecmp strnicmp strrchr strstr time umask utime vsnprintf)
  foreach(f ${l})
    string(TOUPPER "${f}" var)
    string(REGEX REPLACE "[^0-9A-Z_]" "_" var "${var}")
    mfx_check_function_exists("${f}" "HAVE_${var}")
  endforeach()

  # mfx_LZO_CHECK_ENDIAN
  TEST_BIG_ENDIAN(big_endian)
  if ("${big_endian}" MATCHES "^1$")
    add_definitions(-DLZO_ABI_BIG_ENDIAN=1)
  elseif ("${big_endian}" MATCHES "^0$")
    add_definitions(-DLZO_ABI_LITTLE_ENDIAN=1)
  else()
    message(FATAL_ERROR "ERROR: TEST_BIG_ENDIAN failed with result '${big_endian}'.")
  endif()

  # LZO_HAVE_CONFIG_H
  add_definitions(-DLZO_CFG_NO_CONFIG_HEADER=1)
else ()
  squash_plugin_add_for_package(lzo LZO lzo_sources)
endif ()
