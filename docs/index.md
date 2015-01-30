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

- [Blosc](@ref md_plugins_blosc_blosc)
- [Brotli](@ref md_plugins_brotli_brotli)
- [bzip2](@ref md_plugins_bzip2_bzip2)
- [Doboz](@ref md_plugins_doboz_doboz)
- [FastARI](@ref md_plugins_fari_fari)
- [FastLZ](@ref md_plugins_fastlz_fastlz)
- [Gipfeli](@ref md_plugins_gipfeli_gipfeli)
- [Intel® Performance Primitives](@ref md_plugins_ipp_ipp)
- [LZ4](@ref md_plugins_lz4_lz4)
- [LZF](@ref md_plugins_lzf_lzf)
- [liblzg](@ref md_plugins_lzg_lzg)
- [LZHAM](@ref md_plugins_lzham_lzham)
- [LZJB](@ref md_plugins_lzjb_lzjb)
- [liblzma](@ref md_plugins_lzma_lzma)
- [LZMAT](@ref md_plugins_lzmat_lzmat)
- [LZO](@ref md_plugins_lzo_lzo)
- [QuickLZ](@ref md_plugins_quicklz_quicklz)
- [SHARC](@ref md_plugins_sharc_sharc)
- [Snappy](@ref md_plugins_snappy_snappy)
- [wfLZ](@ref md_plugins_wflz_wflz)
- [zlib](@ref md_plugins_zlib_zlib)
- [zling](@ref md_plugins_zling_zling)
- [ZPAQ](@ref md_plugins_zpaq_zpaq)

We hope to add more soon.  If you're interested in helping, please
contact us!

@subsection license License

Squash is licensed under the [MIT
license](http://opensource.org/licenses/MIT).  Although this license
does not place many restrictions on using Squash itself, please keep
in mind that some plugins use libraries which are subject to more
restrictive terms (such as
[LZO](http://www.oberhumer.com/opensource/lzo/)).
