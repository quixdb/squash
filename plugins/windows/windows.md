# windows Plugin #

This plugin uses the Windows compression API (RtlCompressBuffer et. al.).

For more information about this API, see the
[RtlCompressBuffer documentation](https://msdn.microsoft.com/en-us/library/windows/hardware/ff552127(v=vs.85).aspx)

Note: this plugin is only available on Windows.  For a cross-platform
implementation of LZNT1 and Xpress-huffman, see the ms-compress
plugin.

## Codecs ##

- **lznt1** — LZNT1 data.
- **xpress** — Xpress data.
- **xpress-huffman** — Xpress-huffman data.

## Options ##

### Compression Only ###

- **level** (integer, 1 or 2, default 1): whether to use the standard
  or maximume compression engine.

## License ##

The windows plugin is licensed under the
[MIT License](http://opensource.org/licenses/MIT).  The Windows API is
proprietary.
