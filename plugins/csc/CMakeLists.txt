include (SquashPlugin)

squash_plugin (
  NAME csc
  DEFAULT_DISABLED
  SOURCES squash-csc.cpp
  EMBED_SOURCES
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
    csc/src/libcsc/decomp.cpp
  DEFINES _7Z_TYPES_
  INCLUDE_DIRS csc/src/libcsc)
