@section intro Introduction

Squash is an abstraction layer which provides a single API to access
many compression libraries.

For a high-level overview of how to use Squash, please see the
[Squash User Guide](@ref md_docs_user-guide).

@subsection bindings Bindings

Squash is written in C and has minimal hard dependencies, meaning it
should be straightforward to create bindings for many languages.  In
order help with testing, bindings are generally distributed with
Squash.

Squash currently supports the following languages:

- C
- Vala

We hope to add more soon.  If you're interested in helping, please
contact us!

@subsection plugins Plugins

The actual integration with individual compression libraries is done
through plugins which can be installed separately from Squash itself
and are not loaded until they are required.  This allows Squash
consumers to utilize a great many compression algorithms without
rewriting code or unnecessary bloat.  Plugins need not be distributed
with Squash, though it is preferred.

Squash currently contains plugins for the following libraries:

- [BriefLZ](@ref md_plugins_brieflz_brieflz)
- [Brotli](@ref md_plugins_brotli_brotli)
- [bsc](@ref md_plugins_bsc_bsc)
- [bzip2](@ref md_plugins_bzip2_bzip2)
- [copy](@ref md_plugins_copy_copy)
- [CRUSH](@ref md_plugins_crush_crush)
- [csc](@ref md_plugins_csc_csc)
- [DENSITY](@ref md_plugins_density_density)
- [Doboz](@ref md_plugins_doboz_doboz)
- [FastARI](@ref md_plugins_fari_fari)
- [FastLZ](@ref md_plugins_fastlz_fastlz)
- [Gipfeli](@ref md_plugins_gipfeli_gipfeli)
- [heatshrink](@ref md_plugins_heatshrink_heatshrink)
- [LZ4](@ref md_plugins_lz4_lz4)
- [LZF](@ref md_plugins_lzf_lzf)
- [libdeflate](@ref md_plugins_libdeflate_libdeflate)
- [liblzg](@ref md_plugins_lzg_lzg)
- [LZHAM](@ref md_plugins_lzham_lzham)
- [LZJB](@ref md_plugins_lzjb_lzjb)
- [liblzma](@ref md_plugins_lzma_lzma)
- [LZO](@ref md_plugins_lzo_lzo)
- [miniz](@ref md_plugins_miniz_miniz)
- [ms-compress](@ref md_plugins_ms-compress_ms-compress)
- [ncompress](@ref md_plugins_ncompress_ncompress)
- [Pithy](@ref md_plugins_pithy_pithy)
- [QuickLZ](@ref md_plugins_quicklz_quicklz)
- [Snappy](@ref md_plugins_snappy_snappy)
- [wfLZ](@ref md_plugins_wflz_wflz)
- [yalz77](@ref md_plugins_yalz77_yalz77)
- [zlib](@ref md_plugins_zlib_zlib)
- [zlib-ng](@ref md_plugins_zlib-ng_zlib-ng)
- [zling](@ref md_plugins_zling_zling)
- [ZPAQ](@ref md_plugins_zpaq_zpaq)
- [Zstandard](@ref md_plugins_zstd_zstd)

We hope to add more soon.  If you're interested in helping, please
contact us!

@subsection license License

Squash is licensed under the [MIT
license](http://opensource.org/licenses/MIT).  Although this license
does not place many restrictions on using Squash itself, please keep
in mind that some plugins use libraries which are subject to more
restrictive terms (such as
[LZO](http://www.oberhumer.com/opensource/lzo/)).
