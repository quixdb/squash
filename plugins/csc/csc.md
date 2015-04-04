# csc Plugin #

The csc plugin provides an interface for the CSC library.

For more information about CSC see https://github.com/fusiyuan2010/CSC

## Codecs ##

- **csc** — CSC data

## Options ##

### Compression Only ###

- **level** (integer, 1-5, default 2): The compression level provided
  to CSC.  1 is fastest while 6 provides the highest compression
  ratio.
- **dict-size** (integer, 32768-1073741824 [32 KiB - 1 GiB], default
  67108864 (64 MiB).
- **delta-filter** (boolean, default true) — Whether to enable the
  filter for data tables
- **exe-filter** (boolean, default true) — Whether to enable the
  filter for executables
- **txt-filter** (boolean, default true) — Whether to enable the
  filter for English text

## License ##

The csc plugin is licensed under the [MIT
License](http://opensource.org/licenses/MIT) license, and CSC is
public domain, though it currently contains some code copied from
ZPAQ, which was GPL, or possibly libzpaq, which is public domain.
