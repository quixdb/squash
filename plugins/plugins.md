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
