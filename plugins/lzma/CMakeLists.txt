include (SquashPlugin)

foreach (header
    "inttypes.h"
    "stdint.h"
    "limits.h"
    "stdbool.h"
    "string.h"
    "strings.h"
    "memory.h"
    "byteswap.h"
    "sys/endian.h"
    "sys/byteorder.h"
    "CommonCrypto/CommonDigest.h"
    "sha256.h"
    "sha2.h"
    "minix/sha2.h"
    "dlfcn.h"
    "fcntl.h"
    "getopt.h"
    "immintrin.h"
    "sys/param.h"
    "sys/stat.h"
    "sys/time.h"
    "sys/types.h"
    "unistd.h")
  string (TOUPPER "${header}" headeruc)
  string (REGEX REPLACE "[\\./]" "_" test_name "HAVE_${headeruc}")

  CHECK_INCLUDE_FILE("${header}" ${test_name})
  if (${test_name})
    list (APPEND lzma_embed_defines ${test_name})
  endif ()
endforeach (header)

include(CheckPrototypeExists)
CHECK_PROTOTYPE_EXISTS ("bswap_16" "byteswap.h" "HAVE_BSWAP_16")
CHECK_PROTOTYPE_EXISTS ("bswap_32" "byteswap.h" "HAVE_BSWAP_32")
CHECK_PROTOTYPE_EXISTS ("bswap_64" "byteswap.h" "HAVE_BSWAP_64")
CHECK_PROTOTYPE_EXISTS ("dcgettext" "libintl.h" "HAVE_DCGETTEXT")
CHECK_PROTOTYPE_EXISTS ("futimens" "fcntl.h;sys/stat.h" "HAVE_FUTIMENS")
CHECK_PROTOTYPE_EXISTS ("futimes" "sys/time.h" "HAVE_FUTIMES")
CHECK_PROTOTYPE_EXISTS ("futimesat" "fcnctl.h;sys/time.h" "HAVE_FUTIMESAT")
CHECK_PROTOTYPE_EXISTS ("getopt_long" "getopt.h" "HAVE_GETOPT_LONG")
CHECK_PROTOTYPE_EXISTS ("gettext" "libintl.h" "HAVE_GETTEXT")
CHECK_PROTOTYPE_EXISTS ("iconv" "iconv.h" "HAVE_ICONV")
CHECK_PROTOTYPE_EXISTS ("mbrtowc" "wchar.h" "HAVE_MBRTOWC")
CHECK_PROTOTYPE_EXISTS ("pipe2" "fcntl.h;unistd.h" "HAVE_PIPE2")
CHECK_PROTOTYPE_EXISTS ("posix_fadvise" "fcntl.h" "HAVE_POSIX_FADVISE")
CHECK_PROTOTYPE_EXISTS ("pthread_condattr_setclock" "pthread.h" "HAVE_PTHREAD_CONDATTR_SETCLOCK")
CHECK_PROTOTYPE_EXISTS ("utime" "sys/types.h;utime.h" "HAVE_UTIME")
CHECK_PROTOTYPE_EXISTS ("utimes" "sys/types.h;utime.h" "HAVE_UTIMES")
CHECK_PROTOTYPE_EXISTS ("wcwidth" "wchar.h" "HAVE_WCWIDTH")
CHECK_PROTOTYPE_EXISTS ("_mm_movemask_epi8" "emmintrin.h" "HAVE__MM_MOVEMASK_EPI8")

include(CheckLibraryExists)
CHECK_LIBRARY_EXISTS(rt clock_gettime "time.h" HAVE_CLOCK_GETTIME)

include(CheckSymbolExists)
CHECK_SYMBOL_EXISTS ("program_invocation_name" "errno.h" "HAVE_DECL_PROGRAM_INVOCATION_NAME")
CHECK_SYMBOL_EXISTS ("CLOCK_MONOTONIC" "time.h" "HAVE_DECL_CLOCK_MONOTONIC")
CHECK_SYMBOL_EXISTS ("optreset" "getopt.h" "HAVE_OPTRESET")
CHECK_SYMBOL_EXISTS ("PTHREAD_PRIO_INHERIT" "pthread.h" "HAVE_PTHREAD_PRIO_INHERIT")
CHECK_SYMBOL_EXISTS ("_SC_NPROCESSORS_ONLN" "unistd.h" "HAVE__SC_NPROCESSORS_ONLN")
CHECK_SYMBOL_EXISTS ("_SC_NPROC_ONLN" "unistd.h" "HAVE__SC_NPROC_ONLN")
CHECK_SYMBOL_EXISTS ("_SC_PHYS_PAGES" "unistd.h" "HAVE__SC_PHYS_PAGES")

include(CheckTypeSize)
set(ORIGINAL_INCLUDE_FILES ${CMAKE_EXTRA_INCLUDE_FILES})

set(CMAKE_EXTRA_INCLUDE_FILES "stdint.h")
CHECK_TYPE_SIZE ("uintptr_t" UINTPTR_T)

set(CMAKE_EXTRA_INCLUDE_FILES "stdbool.h")
CHECK_TYPE_SIZE ("_Bool" _BOOL)

set(CMAKE_EXTRA_INCLUDE_FILES "stddef.h")
CHECK_TYPE_SIZE("size_t" SIZEOF_SIZE_T)

set(CMAKE_EXTRA_INCLUDE_FILES ${ORIGINAL_INCLUDE_FILES})

include(CheckStructHasMember)
CHECK_STRUCT_HAS_MEMBER ("struct stat" "st_atimensec" "sys/types.h;sys/stat.h;unistd.h" "HAVE_STRUCT_STAT_ST_ATIMENSEC")
CHECK_STRUCT_HAS_MEMBER ("struct stat" "st_atimespec.tv_nsec" "sys/types.h;sys/stat.h;unistd.h" "HAVE_STRUCT_STAT_ST_ATIMESPEC_TV_NSEC")
CHECK_STRUCT_HAS_MEMBER ("struct stat" "st_atim.st__tim.tv_nsec" "sys/types.h;sys/stat.h;unistd.h" "HAVE_STRUCT_STAT_ST_ATIM_ST__TIM_TV_NSEC")
CHECK_STRUCT_HAS_MEMBER ("struct stat" "st_atim.tv_nsec" "sys/types.h;sys/stat.h;unistd.h" "HAVE_STRUCT_STAT_ST_ATIM_TV_NSEC")
CHECK_STRUCT_HAS_MEMBER ("struct stat" "st_uatime" "sys/types.h;sys/stat.h;unistd.h" "HAVE_STRUCT_STAT_ST_UATIME")

configure_file (config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  list (APPEND lzma_embed_defines MYTHREAD_WIN95)
else ()
  list (APPEND lzma_embed_defines MYTHREAD_POSIX)
endif ()

test_big_endian(LZMA_IS_BIG_ENDIAN)
if(LZMA_IS_BIG_ENDIAN)
  list (APPEND lzma_embed_defines "WORDS_BIGENDIAN")
endif(LZMA_IS_BIG_ENDIAN)

find_package(LZMA)

squash_plugin (
  NAME lzma
  SOURCES squash-lzma.c
  C_STANDARD c99
  EXTERNAL_PKG_PREFIX LZMA
  EMBED_SOURCES
    xz/src/common/tuklib_physmem.c
    xz/src/common/tuklib_cpucores.c
    xz/src/liblzma/common/common.c
    xz/src/liblzma/common/common.h
    xz/src/liblzma/common/memcmplen.h
    xz/src/liblzma/common/block_util.c
    xz/src/liblzma/common/easy_preset.c
    xz/src/liblzma/common/easy_preset.h
    xz/src/liblzma/common/filter_common.c
    xz/src/liblzma/common/filter_common.h
    xz/src/liblzma/common/hardware_physmem.c
    xz/src/liblzma/common/index.c
    xz/src/liblzma/common/index.h
    xz/src/liblzma/common/stream_flags_common.c
    xz/src/liblzma/common/stream_flags_common.h
    xz/src/liblzma/common/vli_size.c
    xz/src/liblzma/common/alone_encoder.c
    xz/src/liblzma/common/block_buffer_encoder.c
    xz/src/liblzma/common/block_buffer_encoder.h
    xz/src/liblzma/common/block_encoder.c
    xz/src/liblzma/common/block_encoder.h
    xz/src/liblzma/common/block_header_encoder.c
    xz/src/liblzma/common/easy_buffer_encoder.c
    xz/src/liblzma/common/easy_encoder.c
    xz/src/liblzma/common/easy_encoder_memusage.c
    xz/src/liblzma/common/filter_buffer_encoder.c
    xz/src/liblzma/common/filter_encoder.c
    xz/src/liblzma/common/filter_encoder.h
    xz/src/liblzma/common/filter_flags_encoder.c
    xz/src/liblzma/common/index_encoder.c
    xz/src/liblzma/common/index_encoder.h
    xz/src/liblzma/common/stream_buffer_encoder.c
    xz/src/liblzma/common/stream_encoder.c
    xz/src/liblzma/common/stream_flags_encoder.c
    xz/src/liblzma/common/vli_encoder.c
    xz/src/liblzma/common/hardware_cputhreads.c
    xz/src/liblzma/common/outqueue.c
    xz/src/liblzma/common/outqueue.h
    xz/src/liblzma/common/stream_encoder_mt.c
    xz/src/liblzma/common/alone_decoder.c
    xz/src/liblzma/common/alone_decoder.h
    xz/src/liblzma/common/auto_decoder.c
    xz/src/liblzma/common/block_buffer_decoder.c
    xz/src/liblzma/common/block_decoder.c
    xz/src/liblzma/common/block_decoder.h
    xz/src/liblzma/common/block_header_decoder.c
    xz/src/liblzma/common/easy_decoder_memusage.c
    xz/src/liblzma/common/filter_buffer_decoder.c
    xz/src/liblzma/common/filter_decoder.c
    xz/src/liblzma/common/filter_decoder.h
    xz/src/liblzma/common/filter_flags_decoder.c
    xz/src/liblzma/common/index_decoder.c
    xz/src/liblzma/common/index_hash.c
    xz/src/liblzma/common/stream_buffer_decoder.c
    xz/src/liblzma/common/stream_decoder.c
    xz/src/liblzma/common/stream_decoder.h
    xz/src/liblzma/common/stream_flags_decoder.c
    xz/src/liblzma/common/vli_decoder.c
    xz/src/liblzma/check/check.c
    xz/src/liblzma/check/check.h
    xz/src/liblzma/check/crc_macros.h
    xz/src/liblzma/check/crc32_table.c
    xz/src/liblzma/check/crc32_table_le.h
    xz/src/liblzma/check/crc32_table_be.h
    xz/src/liblzma/check/crc32_fast.c
    xz/src/liblzma/check/crc64_table.c
    xz/src/liblzma/check/crc64_table_le.h
    xz/src/liblzma/check/crc64_table_be.h
    xz/src/liblzma/check/crc64_fast.c
    xz/src/liblzma/check/sha256.c
    xz/src/liblzma/lz/lz_encoder.c
    xz/src/liblzma/lz/lz_encoder.h
    xz/src/liblzma/lz/lz_encoder_hash.h
    xz/src/liblzma/lz/lz_encoder_hash_table.h
    xz/src/liblzma/lz/lz_encoder_mf.c
    xz/src/liblzma/lz/lz_decoder.c
    xz/src/liblzma/lz/lz_decoder.h
    xz/src/liblzma/lzma/lzma_common.h
    xz/src/liblzma/lzma/fastpos.h
    xz/src/liblzma/lzma/lzma_encoder.h
    xz/src/liblzma/lzma/lzma_encoder.c
    xz/src/liblzma/lzma/lzma_encoder_presets.c
    xz/src/liblzma/lzma/lzma_encoder_private.h
    xz/src/liblzma/lzma/lzma_encoder_optimum_fast.c
    xz/src/liblzma/lzma/lzma_encoder_optimum_normal.c
    xz/src/liblzma/lzma/fastpos_table.c
    xz/src/liblzma/lzma/lzma_decoder.c
    xz/src/liblzma/lzma/lzma_decoder.h
    xz/src/liblzma/lzma/lzma2_encoder.c
    xz/src/liblzma/lzma/lzma2_encoder.h
    xz/src/liblzma/lzma/lzma2_decoder.c
    xz/src/liblzma/lzma/lzma2_decoder.h
    xz/src/liblzma/rangecoder/range_common.h
    xz/src/liblzma/rangecoder/range_encoder.h
    xz/src/liblzma/rangecoder/price.h
    xz/src/liblzma/rangecoder/price_table.c
    xz/src/liblzma/rangecoder/range_decoder.h
    xz/src/liblzma/delta/delta_common.c
    xz/src/liblzma/delta/delta_common.h
    xz/src/liblzma/delta/delta_private.h
    xz/src/liblzma/delta/delta_encoder.c
    xz/src/liblzma/delta/delta_encoder.h
    xz/src/liblzma/delta/delta_decoder.c
    xz/src/liblzma/delta/delta_decoder.h
    xz/src/liblzma/simple/simple_coder.c
    xz/src/liblzma/simple/simple_coder.h
    xz/src/liblzma/simple/simple_private.h
    xz/src/liblzma/simple/simple_encoder.c
    xz/src/liblzma/simple/simple_encoder.h
    xz/src/liblzma/simple/simple_decoder.c
    xz/src/liblzma/simple/simple_decoder.h
    xz/src/liblzma/simple/x86.c
    xz/src/liblzma/simple/powerpc.c
    xz/src/liblzma/simple/ia64.c
    xz/src/liblzma/simple/arm.c
    xz/src/liblzma/simple/armthumb.c
    xz/src/liblzma/simple/sparc.c
  EMBED_INCLUDE_DIRS
    ${CMAKE_CURRENT_BINARY_DIR}
    xz/src/liblzma/api
    xz/src/liblzma/common
    xz/src/liblzma/check
    xz/src/liblzma/lz
    xz/src/liblzma/rangecoder
    xz/src/liblzma/lzma
    xz/src/liblzma/delta
    xz/src/liblzma/simple
    xz/src/common
    xz/src/common
  EMBED_DEFINES
    ${lzma_embed_defines}
    HAVE_CONFIG_H
  EMBED_COMPILER_FLAGS
    -w)
