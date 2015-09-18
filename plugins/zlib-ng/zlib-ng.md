# zlib-ng Plugin #

The zlib-ng plugin provides an interface for the zlib-ng library,
which is a performance-oriented fork of the popular zlib library.

For more information about zlib-ng see https://github.com/Dead2/zlib-ng

## Codecs ##

- **gzip** — deflate data with a gzip header
  [RFC 1952](https://www.ietf.org/rfc/rfc1952.txt)).
- **zlib** — deflate data with a zlib header
  ([RFC 1950](https://www.ietf.org/rfc/rfc1950.txt)).
- **deflate** — raw deflate data
  ([RFC 1951](https://www.ietf.org/rfc/rfc1951.txt)).

## Options ##

- **window-bits** (integer, 8-15, default 15): The base two logarithm
    of the maximum window size.  The value passed to the decompressor
    **must** be greater than or equal to the value passed to the
    compressor.

### Encoder Only ###

- **level** (integer, 1-9, default 6): Compression level.  1 will
   result in the fastest compression while 9 will result in the
   highest compression ratio.
- **mem-level** (integer, 1-9, default 8):
- **strategy** (enumeration, default "default"): Descriptions adapted
   from the [deflateInit2 zlib
   documentation](http://www.zlib.net/manual.html#Advanced):
  - *default* — Use this for normal data.
  - *filtered* — For data produced by a filter (or predictor).  The
     effect of *filtered* is to force more Huffman coding and less
     string matching; it is somewhat intermediate between *default*
     and *huffman*.
  - *huffman* — Force Huffman encoding only (no string match)
  - *rle* — Limit match distances to one (run-length encoding)
  - *fixed* — Prevent the use of dynamic Huffman codes, allowing for a
     simpler decoder for special applications.

## License ##

The zlib plugin is licensed under the [MIT
License](http://opensource.org/licenses/MIT), and zlib is licensed
under the [zlib License](http://www.gzip.org/zlib/zlib_license.html),
a permissive [OSI-approved](http://opensource.org/licenses/Zlib)
license.
