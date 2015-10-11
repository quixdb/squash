# Internals

Squash is primarily a library which offers a single interface for a
large number of implementations of general-purpose compression
algorithms.  Squash uses a different plugin to interface with each
external library, and plugins are loaded on-demand.

This document is intended primarily for those who want, or need, a
deeper understanding of how Squash works.  If you simply wish to use
the library and don't particularly care how it works, you needn't read
this (the [User Guide](@ref user-guide.md) should be all you need).
If you intend to write a plugin then this document may be helpful, but
is not required.

## Plugin Interfaces

Each plugin contains implementations for one or interface for
processing data, which Squash will then use to implement several
convenient APIs.  Often these APIs simply invoke the relevant callback
in the requested plugin, but sometimes Squash has to do more work
since plugins don't implement all the available interfaces.

There are currently three primary interfaces which a plugin may
implement.  The precise details of each are beyond the scope of this
document (see the [Plugin Guide](@ref plugin-guide.md) for that), but
the remainder of this section provides a conceptual overview.

### All-In-One

The simplest API is also the most popular for plugins to implement.
It takes two buffers; one for the input, and one for the output.  The
input is processed (compressed or decompressed), and the result is
written to the output within the space of a single function call.
This requires that the buffers for the entire input and the entire
output are available.

### Streaming

The low-level streaming interface is modeled after zlib's API.  It is
not particularly easy to use, but is exceptionally versatile.  If you
are already familiar with zlib's API, or that of a library which also
implements an API inspired by zlib's (such as bzip2, lzma, or lzham),
you should feel quite comfortable with it.

Like the all-in-one API, the consumer provides buffers for the input
and output.  However, unlike the all-in-one API, the streaming API
does not require the entire input and enough room for the entire
output be available.  Instead, you are allowed to call
(squash_stream_process)[@ref squash_stream_process] repeatedly with
buffers as small as a single byte.

This is the preferred interface for implementing Squash plugins.
Unfortunately, not all libraries provide an API flexible enough to
implement this interface.

### Splicing

The splicing API is used to create plugins for libraries which take
callbacks similar to `fread` and `fwrite`.  This allows them to
support a limited form of streaming with substantially less complexity
than the streaming API.

## Squash Interfaces

Squash provides a number of interfaces for use in different
situations.  The [User Guide](@ref user-guide.md) explains how to use
most of them, so I will not spend much time on the details here.
Instead, the primary purpose of this section is to explain how each
interface is implemented.

### All-In-One

Unsurprisingly, the preferred path for the all-in-one interface is to
simply call the relevant callback in the plugin.  There are, however,
some complications.

If the plugin doesn't implement the all-in-one interface then Squash
will first attempt to use the streaming interface to emulate it.  The
buffers provided to Squash are passed along directly to the streaming
API, and the data is processed until it is finished.

If the plugin implements neither the all-in-one nor the streaming
interface, Squash will use the splicing interface.  Squash will
provide pointers to the different sections of the buffers it received
as appropriate, until all input has been consumed.

If the plugin does implement the all-in-one interface, it is important
to know that Squash assumes the all-in-one interface will not read or
write beyond the bounds of the buffers provided to it, even in the
event that there is not enough room in the output buffer to contain
the entire input.

All plugins *must* provide a safe decompression function, but they
have the option of providing an unsafe *compression* function.  In
this case, Squash will check to ensure that the output buffer has
enough room to contain the "compressed" contents of the input buffer
in the worst case scenario (i.e., random data) and, if it does not,
will allocate a temporary buffer which does contain sufficient room.
Once compression to the temporary buffer is complete, if there is
enough room Squash will copy the data over to the output buffer,
otherwise it will return @ref SQUASH_BUFFER_FULL.

### Streaming

Optimially, the streaming API exposed by Squash is just a thin wrapper
atop the streaming interface implemented by the plugin.  In that case
Squash will generally just pass the request through to the plugin
after doing some sanity checks.  However, in order to cut down on the
workload in plugins Squash contains some logic which will handle
certain cases more elegantly.  For example, if a user simply calls
@ref squash_stream_finish with the entire input in the `next_in` field
without ever having called @ref squash_stream_process, Squash will
call the `process` callback until the input has been consumed *then*
call `finish`.

If a plugin doesn't implement the streaming interface but does
implement the splicing interface, Squash will spawn a new thread and
call the process interface in it.  If there is insufficent space in
the output buffer the thread will simply write what it can and block
until more becomes available (e.g., through subsequent calls to @ref
squash_stream_process).  Similarly, if the input buffer contains less
data than requested, the thread will block until more input becomes
available.  The overhead of creating a thread can be a significant
performance hit, especially when compressing small pieces of data.
However, it is generally preferable to what happens if the plugin
doesn't implement the splicing interface…

If a plugin only implements the all-in-one interface Squash will
buffer all input until @ref squash_stream_finish is called, then
process the entire contents at once.

### Splicing

While the splicing API is implemented using the splicing interface
when possible, it can be emulated using the streaming interface
without significant performance degredation.

If the streaming interface is also unavailable, the splicing API will
fall back on buffering and using the all-in-one interface just like
the streaming interface does.

### File I/O

Squash provides an API very similar to the one provided by the C
standard library, which any C program should already be familiar with.
Data is transparently compressed as it is written and decompressed as
it is read.

The first choice for implementing this API is the splicing API.  This
allows plugins to deal with data in their native block sizes, and
should help reduce buffering.

If the splicing interface is not available, Squash will attempt to use
the streaming interface.  In order to accomplish this data will be
processed in fixed-size blocks (currently 1 MiB, but this may change)
until the desired amount of data has been processed.

If only the all-in-one interface and the consumer is splicing files,
Squash will first attempt to memory map the input and output data,
then pass those mappings to the plugin's all-in-one interface.  This
has rather complicated performance implications when compared to
reading the entire input into memory, processing it, and writing the
result.  Typically memory-mapped files are a bit faster (especially
looking at wall-clock time not CPU time), but they can be slower.  The
main benefit, though, is that it is not necessary to have two
potentially huge buffers in main memory.  On systems with limited
memory it can also mean significantly less thrashing.

If the memory mapping fails, Squash will fall back on reading the
entire input into RAM, allocate a buffer large enough to hold the
entire output, use the all-in-one interface to compress/decompress,
then write the output.
