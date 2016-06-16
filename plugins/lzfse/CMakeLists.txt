include (SquashPlugin)

set_compiler_specific_flags(
  VARIABLE compiler_flags
  GCCISH
    -Wno-unused-variable
    -Wno-strict-prototypes
    -Wno-unknown-pragmas
    -Wno-sign-compare
    -Wno-packed
    -Wno-pointer-arith
    -Wno-unused-variable
    -Wno-cast-align
  INTEL
    -wd5959
    -wd161
    -wd2257
    -wd1338)

squash_plugin (
  NAME lzfse
  SOURCES squash-lzfse.c
  C_STANDARD c99
  EMBED_SOURCES
    lzfse/src/lzfse_decode_base.c
    lzfse/src/lzfse_decode.c
    lzfse/src/lzfse_encode_base.c
    lzfse/src/lzfse_encode.c
    lzfse/src/lzfse_fse.c
    lzfse/src/lzfse_main.c
    lzfse/src/lzvn_decode_base.c
    lzfse/src/lzvn_encode_base.c
  COMPILER_FLAGS
    ${compiler_flags})
