# lzo Plugin #

For information about LZO, see http://www.oberhumer.com/opensource/lzo/

## Codecs ##

* **lzo1**
* **lzo1a**
* **lzo1b**
* **lzo1c**
* **lzo1f**
* **lzo1x**
* **lzo1y**
* **lzo1z**

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

* **level**
  > Range 1-9 indicates the fast standard levels using 64 KiB memory
  > for compression. Level 99 offers better compression at the cost of
  > more memory (256 KiB), and is still reasonably fast.  Level 999
  > achieves nearly optimal compression - but it is slow and uses much
  > memory, and is mainly intended for generating pre-compressed data.
  >
  > — http://www.oberhumer.com/opensource/lzo/lzodoc.php

## License ##

The snappy plugin is licensed under the [MIT
License](http://opensource.org/licenses/MIT), and Snappy is licensed
under a [3-clause BSD
License](http://opensource.org/licenses/BSD-3-Clause).