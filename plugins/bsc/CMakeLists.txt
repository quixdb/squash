set (bsc_sources
  squash-bsc.c
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
  libbsc/libbsc/st/st.cpp)

squash_plugin_add (bsc bsc_sources)
squash_plugin_add_include_directories (bsc bsc)
