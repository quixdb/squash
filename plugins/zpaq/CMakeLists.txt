include (SquashPlugin)

set_compiler_specific_flags(
  VARIABLE compiler_flags
  GCCISH -Wno-cast-align -Wno-variadic-macros
  MSVC /wd4244
  INTEL -wd69)

squash_plugin (
  NAME zpaq
  SOURCES squash-zpaq.cpp
  EMBED_SOURCES zpaq/libzpaq.cpp
  COMPILER_FLAGS
    "-include \"${CMAKE_CURRENT_SOURCE_DIR}/zpaq-config.h\""
    ${compiler_flags}
  INCLUDE_DIRS zpaq
  EMBED_COMPILER_FLAGS
    -Wno-shift-negative-value)
