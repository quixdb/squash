# blosc Plugin #

For information about Blosc, see https://www.blosc.org/

## Warning ##

Blosc is not thread safe, so Squash uses a mutex to restrict access to
the underlying (Blosc) API to one thread at a time.  Even with the
Squash stream API it shouldn't be possible to deadlock (since Blosc
doesn't support streams Squash's stream API basically just buffers the
input until you call finish, and it is all sent to the
compress/decompress function at once), but you should be aware that
attempting to use Blosc at the same time from multiple threads can be
slower than you might otherwise expect.

That said, Blosc can use multiple threads, so if you care about
throughput but not latency it could be a good way to go.

## Codecs ##

- **blosc** — Blosc data.

## Options ##

- **threads** (int, default 1) — number of threads to use

### Compression Only ###

- **level** (integer, 0-9, default 6) — compression level.  0 means no
  compression, 9 is the maximum.
- **type-size** (integer, default 1) — number of bytes for the atomic
  type in the uncompressed data.  For example, if you are trying to
  compress 64-bit integers, use 8.
- **shuffle** (boolean, default true) — apply shuffle before
  compression.

## License ##

Both the blosc plugin and Blosc itself are licensed under the [MIT
License](http://opensource.org/licenses/MIT).