# zstd Plugin #

Zstandard is a fast and efficient compression algorithm

For more information about zstd, see
https://github.com/Cyan4973/zstd

## Codecs ##

- **zstd** — Raw zstd data.

## Options ##

### Compression-only ###

- **level** — (integer, 7-15, default 7): compression level.  7
  corresponds to the default zstd algorithm (`ZSTD_compress`).  Levels
  above 7 correspond to the HC implementation (`ZSTD_HC_compress`),
  with a level of `(level - 7) * 3`

## License ##

The zstd plugin is licensed under the [MIT
License](http://opensource.org/licenses/MIT), and zstd is licensed
under a [2-clause BSD
license](http://opensource.org/licenses/BSD-2-Clause).
