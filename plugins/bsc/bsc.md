# bsc Plugin #

For information about libbsc, see http://libbsc.com/

## Codecs ##

- **bsc** — libbsc data.

## Options ##

- **fast-mode** (boolean, deafault true): enable fast-mode
- **multi-threading** (boolean, deafault true): enable multi-threading
- **large-pages** (boolean, deafault false): enable large-page support
- **cuda** (boolean, deafault false): enable CUDA support

### Encoder Only ###

- **lzp-hash-size** (integer, 0 or 10-28, default 16): size of the LZP
  hash table, or 0 for disabled
- **lzp-min-len** (integer, 0 or 4-255, default 128): Minimum LZP match
  length, or 0 for disabled
- **block-sorter** (enumeration, default bwt): block sorting algorithm
  - *bwt*: Burrows–Wheeler transform
  - *none*
- **coder** (enumeration, default qlfc-static)
  - *none*
  - *qflc-static*
  - *qflc-adaptive*

## License ##

The bsc plugin is licensed under the [MIT
License](http://opensource.org/licenses/MIT), and libbsc is licensed
under the [Apache 2.0
License](https://www.apache.org/licenses/LICENSE-2.0).
