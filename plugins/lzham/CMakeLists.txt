include (SquashPlugin)

squash_plugin(
  NAME lzham
  SOURCES squash-lzham.c
  EMBED_SOURCES
    lzham/lzhamcomp/lzham_lzbase.cpp
    lzham/lzhamcomp/lzham_lzcomp.cpp
    lzham/lzhamcomp/lzham_lzcomp_internal.cpp
    lzham/lzhamcomp/lzham_lzcomp_state.cpp
    lzham/lzhamcomp/lzham_match_accel.cpp
    lzham/lzhamcomp/lzham_pthreads_threading.cpp
    lzham/lzhamdecomp/lzham_assert.cpp
    lzham/lzhamdecomp/lzham_checksum.cpp
    lzham/lzhamdecomp/lzham_huffman_codes.cpp
    lzham/lzhamdecomp/lzham_lzdecomp.cpp
    lzham/lzhamdecomp/lzham_lzdecompbase.cpp
    lzham/lzhamdecomp/lzham_mem.cpp
    lzham/lzhamdecomp/lzham_platform.cpp
    lzham/lzhamdecomp/lzham_prefix_coding.cpp
    lzham/lzhamdecomp/lzham_symbol_codec.cpp
    lzham/lzhamdecomp/lzham_timer.cpp
    lzham/lzhamdecomp/lzham_vector.cpp
    lzham/lzhamlib/lzham_lib.cpp
    lzham/lzhamtest/lzhamtest.cpp
    lzham/lzhamtest/timer.cpp
  INCLUDE_DIRS
    lzham/include
    lzham/lzhamdecomp
    lzham/lzhamcomp)
