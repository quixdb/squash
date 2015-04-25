# zling Plugin #

The zling plugin provides an interface for the libzling library.

For more information about libzling see https://github.com/richox/libzling

## Security ##

The libzling decoder contains [security
vulnerabilities](https://github.com/richox/libzling/issues/5), it is
not currently safe to use with untrusted input.

## Codecs ##

- **zling** — raw zling data

## Options ##

- **level** (integer, 0-4, default 0): The compression level provided
  to zling.  0 is fastest while 4 provides the highest compression
  ratio.

## License ##

The zlib plugin is licensed under the [MIT
License](http://opensource.org/licenses/MIT), and libzling is licensed
under a [3-clause BSD
license](http://opensource.org/licenses/BSD-3-Clause).