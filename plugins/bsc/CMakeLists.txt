include (SquashPlugin)

squash_plugin (
  NAME bsc
  SOURCES squash-bsc.c
  NO_BIG_ENDIAN
  EMBED_SOURCES
    libbsc/libbsc/bwt/divsufsort/divsufsort.c
    libbsc/libbsc/adler32/adler32.cpp
    libbsc/libbsc/bwt/bwt.cpp
    libbsc/libbsc/coder/coder.cpp
    libbsc/libbsc/coder/qlfc/qlfc.cpp
    libbsc/libbsc/coder/qlfc/qlfc_model.cpp
    libbsc/libbsc/filters/detectors.cpp
    libbsc/libbsc/filters/preprocessing.cpp
    libbsc/libbsc/libbsc/libbsc.cpp
    libbsc/libbsc/lzp/lzp.cpp
    libbsc/libbsc/platform/platform.cpp
    libbsc/libbsc/st/st.cpp
  INCLUDE_DIRS
    libbsc)
