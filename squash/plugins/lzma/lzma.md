# lzma Plugin #

The lzma plugin provides an interface for liblzma from the [XZ Utils
project](http://tukaani.org/xz/).

## Codecs ##

 * **xz**: The stream format used by default in the xz tool.
 * **lzma**: Legacy format used by the lzma tool.  You should use xz
   instead unless you need this for compatibility reasons.
 * **lzma2**: Format with a single-byte header used by xz.  Requires
   *dict-size* to be present for the decoder.  Relying on the default
   *value is dangerous as it can change from version to version.
 * **lzma1**: Raw format used by lzma.  The following options should
   be present: *dict-size*, *lc*, *lp*, *pb*.  Relying on default
   values for those options is dangerous as those defaults can change
   from version to version.

## Options ##

 * **level** (integer, 1-9, default 6): Set the compression level.  1
   will result in the fastest compression while 9 will result in the
   highest compression ratio.
 * **check** (enumeration, default crc64, xz only): Set the algorithm
     used to verify the compressed data.  Available values:
   * *none*: do not verify
   * *crc32*: [CRC](https://en.wikipedia.org/wiki/Cyclic_redundancy_check)-32, suitable mainly for small files.
   * *crc64*: [CRC](https://en.wikipedia.org/wiki/Cyclic_redundancy_check)-64, suitable for larger files.
   * *sha256*: [SHA-256](https://en.wikipedia.org/wiki/SHA-2), for
      when security is important.
 * **dict-size** (integer, 4096-1610612736 (2^30 + 2^29), default
     8388608 (2^23), required for lzma1 and lzma2): From the xz man
     page:
   > Dictionary (history buffer) size indicates how many bytes of the
   > recently processed uncompressed data is kept in memory.  The
   > algorithm tries to find repeating byte sequences (matches) in the
   > uncompressed data, and replace them with references to the data
   > currently in the dictionary.  The bigger the dictionary, the
   > higher is the chance to find a match.  Thus, increasing
   > dictionary size usually improves compression ratio, but a
   > dictionary bigger than the uncompressed file is waste of
   > memory.
   >
   > Typical dictionary size is from 64 KiB to 64 MiB.  The minimum is
   > 4 KiB.  The maximum for compression is currently 1.5 GiB (1536
   > MiB).  The decompressor already supports dictionaries up to one
   > byte less than 4 GiB, which is the maximum for the LZMA1 and
   > LZMA2 stream formats.
   >
   > Dictionary size and match finder (mf) together determine the
   > memory usage of the LZMA1 or LZMA2 encoder.  The same (or bigger)
   > dictionary size is required for decompressing that was used when
   > compressing, thus the memory usage of the decoder is determined
   > by the dictionary size used when compressing.  The .xz headers
   > store the dictionary size either as 2^n or 2^n + 2^(n-1), so
   > these sizes are somewhat preferred for compression.  Other sizes
   > will get rounded up when stored in the .xz headers.
 * **lc**: (integer, 0-4, default 3, required for lzma1): From the xz
     man page:
   > Specify the number of literal context bits.  The minimum is 0 and
   > the maximum is 4; the default is 3.  In addition, the sum of lc
   > and lp must not exceed 4.
   >
   > All bytes that cannot be encoded as matches are encoded as
   > literals.  That is, literals are simply 8-bit bytes that are
   > encoded one at a time.
   >
   > The literal coding makes an assumption that the highest lc bits
   > of the previous uncompressed byte correlate with the next byte.
   > E.g. in typical English text, an uppercase letter is often
   > followed by a lower-case letter, and a lower-case letter is
   > usually followed by another lower-case letter.  In the US-ASCII
   > character set, the highest three bits are 010 for upper-case
   > letters and 011 for lower-case letters.  When lc is at least 3,
   > the literal coding can take advantage of this property in the
   > uncompressed data.
   >
   > The default value (3) is usually good.  If you want maximum
   > compression, test lc=4.  Sometimes it helps a little, and
   > sometimes it makes compression worse.  If it makes it worse, test
   > e.g. lc=2 too.
 * **lp** (integer, 0-4, default 0, required for lzma1): From the xz
     man page:
   > Specify the number of literal position bits.  The minimum is 0
   > and the maximum is 4; the default is 0.
   >
   > Lp affects what kind of alignment in the uncompressed data is
   > assumed when encoding literals.  See pb below for more
   > information about alignment.
 * **pb** (integer, 0-4, default 2, required for lzma1): From the xz
     man page:
   > Specify the number of position bits.  The minimum is 0 and the
   > maximum is 4; the default is 2.
   >
   > Pb affects what kind of alignment in the uncompressed data is
   > assumed in general.  The default means four-byte alignment
   > (2^pb=2^2=4), which is often a good choice when there's no better
   > guess.
   >
   > When the aligment is known, setting pb accordingly may reduce the
   > file size a little.  E.g. with text files having one-byte
   > alignment (US-ASCII, ISO-8859-*, UTF-8), setting pb=0 can improve
   > compression slightly.  For UTF-16 text, pb=1 is a good choice.
   > If the alignment is an odd number like 3 bytes, pb=0 might be the
   > best choice.
   >
   > Even though the assumed alignment can be adjusted with pb and lp,
   > LZMA1 and LZMA2 still slightly favor 16-byte alignment.  It might
   > be worth taking into account when designing file formats that are
   > likely to be often compressed with LZMA1 or LZMA2.

### Decoder-only ###

 * **memlimit** (integer, default UINT64_MAX): Memory limit to use
   while decoding.  Note that decoding could fail if this is set too
   low.

## License ##

The lzma plugin is licensed under the [MIT
License](http://opensource.org/licenses/MIT), and liblzma public
domain.
