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

### Encoder and Decoder ###

If non-default values for these options are used when compressing, the
same values *must* be passed to the decompressing or decompression
will fail.


- **dict-size-log2** — (int, 15-29, default 24): dictionary size.
  Larger values mean better compression but higher memory usage.
  Maximum value 32-bit architectures is 26.
- **update-rate** — (int, 2-20, default 8): controls tradeoff between
  ratio and decompression throughput. Higher values yield faster
  operation but lower ratio.
- **update-interval** — (int, 12-128, default 64): max interval
  between table updates.  Larger values correspond to faster
  decode with a lower compression ratio.

## License ##

The lzham plugin and LZHAM are both licensed under the [MIT
License](http://opensource.org/licenses/MIT)
