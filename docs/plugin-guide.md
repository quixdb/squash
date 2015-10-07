# Plugin Guide

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

Perhaps the most under-appreciated benefit of a Squash plugin is the
testing:

* Implementing the plugin is a good test of the API.
* Some of the unit tests in Squash are quite evil, and very good at
  catching bugs.  The benchmark also helps with this.
* We run static analyzers and fuzzers against plugins.

Squash has been responsible for finding (and occasionally fixing)
dozens of bugs in many different libraries.

#### Visibility

With a Squash plugin, it is trivial for people using Squash to use
your codec.  That means people are more likely to try your codec.

## Are there any restrictions on plugins?

Plugins are simply shared libraries and an ini file placed in the
right location in the file system; nothing is hard-coded, and Squash
has no special compile-time knowledge of plugins.  You don't need
Squash's approval to create a plugin, and you are free to distribute
it yourself.

If you want your plugin to be distributed *with Squash*, it has to be
open-source.  However, there is no need to distribute a plugin with
Squash.

We are willing to create (and help maintain) a repository for
open-source plugins which link to proprietary codecs.  If you would be
interested in that, please get in touch.

## Creating a Plugin
