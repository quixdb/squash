# lzham Plugin #

The lzham plugin provides an interface for LZHAM

For more information about LZHAM see
https://github.com/richgel999/lzham_codec

## Codecs ##

- lzham

## Options ##

### Encoder only ###

- **level** (integer, 0-4, default 2): Compression level.  Higher
  levels better compress the data but take longer to compress.
- **extreme-parsing** (boolean, default false): "Improves ratio by
  allowing the compressor's parse graph to grow 'higher' (up to 4
  parent nodes per output node), but is much slower."
- **deterministic-parsing** (boolean, default false): "Guarantees that
  the compressed output will always be the same given the same input
  and parameters (no variation between runs due to kernel threading
  scheduling)."
- **decompression-rate-for-ratio** (boolean, default false): "If
  enabled, the compressor is free to use any optimizations which could
  lower the decompression rate (such as adaptively resetting the
  Huffman table update rate to maximum frequency, which is costly for
  the decompressor)."

## License ##

The lzham plugin and LZHAM are both licensed under the [MIT
License](http://opensource.org/licenses/MIT)