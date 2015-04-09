set (lzma_sources squash-lzma.c)

find_package (PkgConfig)
pkg_check_modules (LZMA liblzma)

function (define_have_include_file file name)
  CHECK_INCLUDE_FILE("${file}" "HAVE_${name}_H")
  if ("${HAVE_${name}_H}")
    add_definitions (-DHAVE_${name}_H)
  endif ()
endfunction (define_have_include_file)

if (NOT ${LZMA_FOUND} OR "${FORCE_IN_TREE_DEPENDENCIES}" STREQUAL "yes")
  list (APPEND lzma_sources
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
    xz/src/liblzma/simple/sparc.c)

  squash_plugin_add (lzma lzma_sources)
  squash_plugin_add_include_directories (lzma
    xz/src/liblzma/api
    xz/src/liblzma/common
    xz/src/liblzma/check
    xz/src/liblzma/lz
    xz/src/liblzma/rangecoder
    xz/src/liblzma/lzma
    xz/src/liblzma/delta
    xz/src/liblzma/simple
    xz/src/common
    xz/src/common)

  squash_source_file_set_c99 (lzma_sources)

  CHECK_INCLUDE_FILE ("inttypes.h" HAVE_INTTYPES_H)
  CHECK_INCLUDE_FILE ("stdint.h"   HAVE_STDINT_H)
  CHECK_INCLUDE_FILE ("limits.h"   HAVE_LIMITS_H)
  CHECK_INCLUDE_FILE ("stdbool.h"  HAVE_STDBOOL_H)
  CHECK_INCLUDE_FILE ("string.h"   HAVE_STRING_H)
  CHECK_INCLUDE_FILE ("strings.h"  HAVE_STRINGS_H)
  CHECK_INCLUDE_FILE ("memory.h"   HAVE_MEMORY_H)
  configure_file (config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
  squash_plugin_add_include_directories (lzma ${CMAKE_CURRENT_BINARY_DIR})
  add_definitions (-DHAVE_CONFIG_H)

  if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    add_definitions (-DMYTHREAD_WIN95)
  else ()
    add_definitions (-DMYTHREAD_POSIX)
  endif ()
else ()
  squash_plugin_add_for_package(lzma LZMA lzma_sources)
endif ()
