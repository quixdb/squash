# lz4 Plugin #

LZ4 is a small, fast compression library.

For more information about LZ4, see
http://fastcompression.blogspot.com/p/lz4.html

## Codecs ##

- **lz4** — Raw LZ4 data.

## Options ##

### lz4 ###

- **level** (integer, 1 or 9) — 1 is the default, 9 will use the high
    compression derivative.

### lz4f ###

- **level** (integer, 0 - 16) — 0 is the default, higher = better
  compression ratio but slower compression speed
- **block-size** (enum) — input block size
  - 4 — 64 KiB
  - 5 — 256 KiB
  - 6 — 1 MiB
  - 7 — 4 MiB
- **checksum** (boolean, default false) — whether or not to include a
  checksum (xxHash) for verification

## License ##

The lz4 plugin is licensed under the [MIT
License](http://opensource.org/licenses/MIT), and LZ4 is licensed
under a [3-clause BSD](http://opensource.org/licenses/BSD-3-Clause)
license.
