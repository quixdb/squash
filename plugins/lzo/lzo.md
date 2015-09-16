# lzo Plugin #

For information about LZO, see http://www.oberhumer.com/opensource/lzo/

## Codecs ##

* **lzo1b**
* **lzo1c**
* **lzo1f**
* **lzo1x**
* **lzo1y**
* **lzo1z**

Note that support for *lzo1* and *lzo1a* are included in the plugin
itself, but are disabled in the *squash.ini* file because there are no
safe decompressors (*i.e.*, to `lzo1_decompress_safe` and
`lzo1a_decompress_safe` functions).  Squash requires all decompression
implementations to be safe.

According to the documentation in LZO, both algorithms "… are mainly
included for compatibility reasons"; it is unlikely you really want to
use them, and due to the absence of a safe decompressor doing so is
**strongly** discouraged, especially with untrusted input.  However,
in the event that you do need to use them you can simply add them to
the lzo plugin's *squash.ini*.

From the [LZO documentation](http://www.oberhumer.com/opensource/lzo/lzodoc.php):

> I first published LZO1 and LZO1A in the Internet newsgroups
> comp.compression and comp.compression.research in March 1996.  They
> are mainly included for compatibility reasons. The LZO2A
> decompressor is too slow, and there is no fast compressor anyway.
>
> My experiments have shown that LZO1B is good with a large blocksize
> or with very redundant data, LZO1F is good with a small blocksize or
> with binary data and that LZO1X is often the best choice of all.
> LZO1Y and LZO1Z are almost identical to LZO1X - they can achieve a
> better compression ratio on some files.  Beware, your mileage may
> vary.
>
> — http://www.oberhumer.com/opensource/lzo/lzodoc.php

## Options ##

* **level** — (integer, range varies by codec, default is the fastest)
  * **lzo1** — 1, 99
  * **lzo1a** — 1, 99
  * **lzo1b** — 1-9, 99, 999
  * **lzo1c** — 1-9, 99, 999
  * **lzo1f** — 1, 999
  * **lzo1x** — 1, 11-15, 999
  * **lzo1y** — 1, 999
  * **lzo1z** — 999
  > Range 1-9 indicates the fast standard levels using 64 KiB memory
  > for compression. Level 99 offers better compression at the cost of
  > more memory (256 KiB), and is still reasonably fast.  Level 999
  > achieves nearly optimal compression - but it is slow and uses much
  > memory, and is mainly intended for generating pre-compressed data.
  >
  > — http://www.oberhumer.com/opensource/lzo/lzodoc.php

## License ##

The lzo plugin is licensed under the [MIT
License](http://opensource.org/licenses/MIT), and LZO is licensed
under the [GNU GPLv2+](https://gnu.org/licenses/old-licenses/gpl-2.0.html).
