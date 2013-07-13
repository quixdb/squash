# Plugins #

Squash currently ships with plugins which provide implementations many
popular compression codecs.  There is no "default" codec because there
is no codec which works best across the wide range of applications
which Squash endeavors to support, so understanding the differences
between codecs is important for making a good choice.

Broadly speaking, there is a trade-off between the resources required
to compress something and the compression ratio.  Some algorithms
compress data extremely well but are generally considered to be too
slow for practical use.  Other algorithms are blazingly fast but
offer relatively poor compression ratios.

I'll try to put together some benchmarks for the supported codecs
soon.  Meanwhile, you'll just have to do some more research.  Here are
the plugins currently supported (in alphabetical order):

 * [bzip2](@ref md_bzip2)
 * lz4
 * [lzma](@ref md_lzma)
 * lzo
 * [snappy](@ref md_snappy)
 * [zlib](@ref md_zlib)

Broadly speaking, the algorithms designed for real-time use
(compressing things which need to be accessed regularly such as
databases and files on disk) are: lz4, lzo, and snappy.  bzip2 and
lzma achieve better compression but are slower.  zlib is generally
somewhere in the middle.
