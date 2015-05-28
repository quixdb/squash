# lz4 Plugin #

LZ4 is a small, fast compression library.

For more information about LZ4, see
http://fastcompression.blogspot.com/p/lz4.html

## Codecs ##

- **lz4** — Raw LZ4 data.

## Options ##

### lz4 ###

- **level** (integer, 7-14, default 7) — higher level corresponds to
  better compression ratio but slower compression speed.
  - **7** — The default algorithm (LZ4_compress)
  - **8** — LZ4HC level 2
  - **9** — LZ4HC level 4
  - **10** — LZ4HC level 6
  - **11** — LZ4HC level 9
  - **12** — LZ4HC level 12
  - **13** — LZ4HC level 14
  - **14** — LZ4HC level 16

### lz4f ###

- **level** (integer, 0 - 16, default 0) — higher = better
  compression ratio but slower compression speed
- **block-size** (enum, default 4) — input block size
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
