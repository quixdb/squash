# wflz Plugin #

For information about wfLZ, see https://github.com/ShaneWF/wflz

## Codecs ##

- **wflz** — Raw wfLZ data.
- **wflz-chunked** — Chunked wfLZ.

## Options ##

- **level** (integer, 1-2, default 1) — compression level.  Lower is
  faster, higher has a better compression ratio.  2 is currently
  *very* slow.
- **chunk-size** (*wflz-chunked*-only, integer, 4096 - UINT32_MAX,
  multiple of 16, default 16384) — chunk size.

### Compression Only ###

- **endianness** (enum, "little" or "big", default "little") —
  Endianness of the compressed file.

## License ##

The wflz plugin is licensed under the [MIT
License](http://opensource.org/licenses/MIT), and wfLZ is licensed
under the [WTFPL](http://www.wtfpl.net/).
