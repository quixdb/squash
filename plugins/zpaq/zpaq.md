# zpaq Plugin #

The zpaq plugin provides an interface for the ZPAQ library.

For more information about ZPAQ see http://mattmahoney.net/dc/zpaq.html

## Security ##

ZPAQ is *not* safe for decompressing untrusted input.  In order to
allow changes to the algorithms ZPAQ embeds decompressors written in a
language called ZPAQL in the compressed file.  It is possible for
malicious actors to create an infinite loop, effectively allowing
denial of service attacks.

## Codecs ##

- **zpaq** — ZPAQ archive

## Options ##

- **level** (integer, 1-5, default 1): The compression level provided
  to ZPAQ.  1 is fastest while 3 provides the highest compression
  ratio.

## License ##

The zlib plugin is licensed under the [MIT
License](http://opensource.org/licenses/MIT), and libzpaq is public
domain.
