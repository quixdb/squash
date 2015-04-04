set (csc_sources
  squash-csc.cpp
  csc/src/libcsc/csc_analyzer.cpp
  csc/src/libcsc/csc_coder.cpp
  csc/src/libcsc/csc_dec.cpp
  csc/src/libcsc/csc_enc.cpp
  csc/src/libcsc/csc_encoder_main.cpp
  csc/src/libcsc/csc_filters.cpp
  csc/src/libcsc/csc_lz.cpp
  csc/src/libcsc/csc_memio.cpp
  csc/src/libcsc/csc_mf.cpp
  csc/src/libcsc/csc_model.cpp
  csc/src/libcsc/csc_profiler.cpp
  csc/src/libcsc/decomp.cpp)

add_definitions(-D_7Z_TYPES_)
squash_plugin_add (csc csc_sources)
target_include_directories (squash${SQUASH_VERSION_API}-plugin-csc PRIVATE csc/src/libcsc)
