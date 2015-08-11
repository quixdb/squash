# brotli Plugin #

Brotli is a general-puprose compression algorithm

For more information about Brotli, see https://github.com/google/brotli

## Codecs ##

- **brotli** — Raw Brotli data.

## Options ##

### Encoder only ###

- **level** (integer, 1-11, default 11): Compression level.  1 will
   result in the fastest compression while 11 will result in the
   highest compression ratio.
- **mode** (enumeration, "text" or "font", default "text")
- **enable-transforms** (boolean, default false)

## License ##

The brotli plugin is licensed under the [MIT
License](http://opensource.org/licenses/MIT), and Brotli is licensed
under the [Apache 2.0
license](http://opensource.org/licenses/Apache-2.0).
