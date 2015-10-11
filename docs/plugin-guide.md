# Plugin Guide

Squash doesn't implement any compression algorithms itself; instead,
it relies on plugins which interface with code written by
third-parties.  This document attempts to explain why, and how, to
create these plugins.

## Why?

There are numerous advantages to having a plugin for your compression
library, some of which are more obvious tha others.

#### Inclusion in the [Squash Compression Benchmark](https://quixdb.github.io/squash-benchmark/)

The Squash Compression Benchmark has proven to be a relatively popular
tool for comparing compression codecs.  Inclusion provides not only
visibility for your compression codec, but also a useful tool to help
analyze performance and improve your codec.

#### Tools and APIs

Squash provides several convenience APIs which you don't have to
implement.  This gives consumers an easy-to-use API without you having
to write it, which frees you up to focus on core functionality (the
compression/decompression code).

In addition to APIs, Squash provides flexible command-line tools which
allow non-programmers to use your codec without requiring you to write
your own.

It's worth noting that these APIs aren't just convenient, but they can
also help speed up your codec.  Squash includes optimization tricks
which are unlikely to be found in other libraries.

#### Bindings for all languages supported by Squash

Instead of having to write bindings for your library for every
language you want to support, you can write a single Squash plugin and
automatically support every language Squash supports.  The list of
languages supported by Squash is currently limited, but it is growing.

#### Testing

Perhaps the most under-appreciated benefit of a Squash plugin which is
distributed with Squash is the testing:

* Implementing the plugin is a good test of the API.
* Some of the unit tests in Squash are quite evil, and very good at
  catching bugs.
* We run static analyzers and fuzzers against plugins.
* Continuous integration tests are run with different compilers and
  compiler versions (and, hopefully soon, on different platforms).

Squash has been responsible for finding (and occasionally fixing)
dozens of bugs in many different libraries.

#### Visibility

With a Squash plugin, it is trivial for people using Squash to use
your codec.  That means people are more likely to try your codec.

## Are there any restrictions on plugins?

Not really; there are some issues which can complicate things, but
there should always be a way.  Plugins are simply shared libraries and
an ini file placed in the right location in the file system; nothing
is hard-coded, and Squash has no special compile-time knowledge of
plugins.  You don't need Squash's approval to create or distribute a
plugin.

That said, there are certain technical requirements which all plugins,
even those not distributed with Squash, are expected to meet:

* Reliable — plugins are expected to safely encode any data, including
  random data and buffers as small as a single byte.
* Secure — unless they set the @ref
  SQUASH_CODEC_INFO_DECOMPRESS_UNSAFE flag codecs are expected to be
  able to handle untrusted (possibly malicious) data safely; without
  crashes, hangs, or security vulnerabilities.
* Thread-safe — all plugins *must* be thread safe.
* Complete — plugins must supply both a compressor and decompressor.

For plugins distributed with Squash, we are also working towards a
long-term goal of making sure plugins are free from undefined
behavior.

## Creating a Plugin

### ini Files

Plugins consist of two files; a shared library, and an ini file with
some basic information about the plugin and the codecs it supplies.

The simplest ini file is only two lines, but as an example lets look a
theoretical example which shows all the features:

    license=LGPLv2
    [foo]
    extension=foo
    mime-type=application/x-foo
    [bar]
    priority=45

The first entry in the ini file is the license of the library the
plugin is interfacing to.  The license of the plugin is expected to be
either the same as Squash (MIT) or the library.

After the license comes one or more groups, where each group is the
name of a codec provided by the plugin.  In the case of our example
file, the plugin provides implementations for two codecs: *foo* and
*bar*.

Each group consists of zero or more key-value pairs, where the
following keys are valid:

#### extension

If the codec has an associated file extension, it should be added
here.  For example, the "gzip" codec has an extension "gz".  This can
be used by tools such as the `squash` command-line interface to
automatically determine the codec based on the input file.

#### mime-type

If the codec has an associated mime type, set the "mime-type" key to
that value.

#### priority

If multiple plugins exist to handle the same codec and a specific
implementation is not requested, the priority will be used to
determine which codec is chosen.  Lower values equate to a higher
priority, with 1 being the highest priority.  The default value is 50,
which is also the value which should be used for the canonical
implementation of a codec.

Values lower than 50 should be used if a plugin supplies a faster
implementation than the canonical source, and values above 50 if the
plugin supplies a slower implementation.

For example, the ms-compress plugin has a priority of 45 for its lznt1
codec (which is faster than Microsoft's implementation), but xpress
and xpress-huffman have priorities of 85 as those implementations are
not (yet?) superior to Microsoft's.

### The shared library

#### File name

The shared library is expected to be named:
`libsquash${SQUASH_API_VERSION}-plugin-${PLUGIN_NAME}.${EXTENSION}`.

Once Squash obtains API/ABI stability, the API version will be set to
1.0 and will not change until we break that compatibility (which is
not planned, but things happen).  You shouldn't need to provide a
different version of your plugin for every minor version of Squash.

The extension is platform-specific; for Linux it will be "so", for
Windows "dll", and I believe OS X uses "dynlib".

#### Symbols

Every plugin is expected to contain at least one public symbol which
is a function used to intialize each codec:

    SquashStatus
    squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl);

Note that this is a C symbol; if you are using C++ make sure to use
`extern "C"` when you declare this function.

Both arguments to this function are allocated by Squash.  You
shouldn't need to do anything to the @ref SquashCodec argument; it is
primarilty used to determine which codec Squash wants the plugin to
initialize in the event the plugin implements multiple codecs.  The
@ref _SquashCodecImpl argument, on the other hand, you will need to
set at least some fields for.

In addition to the codec initialization function, each codec *may*
implement a function to initialize the plugin itself:

    SquashStatus
    squash_plugin_init_plugin (SquashPlugin* plugin);

If implemented, this function will be invoked before the codec
initialization function, and it will be invoked exactly once per
plugin.  It is mainly just a convenience for plugins so they can
implement global initialization without having to protect a call with
a once or something similar.

#### Codec implementation

The @ref _SquashCodecImpl structure has a number of fields, but most
are optional.  The only ones for which a callback *must* be provided
are @ref _SquashCodecImpl::get_max_compressed_size and one of:

* All-in-one interface: _SquashCodecImpl::decompress_buffer and either
  _SquashCodecImpl::compress_buffer or
  _SquashCodecImpl::compress_buffer_unsafe
* Streaming interface: _SquashCodecImpl::create_stream and
  _SquashCodecImpl::process_stream
* Splice interface: _SquashCodecImpl::splice

For a comparison of the different interfaces, see the @ref Internals
document.  In order to ensure that consumers are always using the
optimal interface to your library you may wish to implement multipe
interfaces.  However, most libraries only provide a single interface.
