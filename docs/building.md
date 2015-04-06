# Building Squash

Note: [Squash currently does not support
Windows](https://github.com/quixdb/squash/issues/86).

Squash currently requires a system copy of liblzma (from xz utils).
We're working on removing this limitation, but in the meantime there
is probably a package available for your system… xz-devel on Fedora,
liblzma-dev on Debian/Ubuntu, liblzma on FreeBSD, etc.

## Building from git

If you are building from a release, skip this section.

Squash makes very heavy use of git submodules to pull in code from
other projects, mostly for the libraries used by each plugin.  When
possible Squash will use system libraries instead of its own copy, but
only a relatively small number of plugins use libraries which are
typically packaged by distributions.

If you are on a UNIX-like system with bash, you can simply call the
`autogen.sh` script (located in the top level of the Squash sources)
just as you would call the `configure` script.  If not you can
update the submodules manually—from the top level of the Squash
sources:

~~~{.c}
git submodule update --init --recursive
~~~

`autogen.sh` simply invokes this command then calls `configure` with
the arguments you passed to it.

Once you have all the submodules your source tree should be in
basically the same state as a release (except you still have all the
git data), so proceed to the next section.

## Building from a release

If you are on a UNIX-like system (basically non-Windows) with bash
(basically non-BSD), simply run the `configure` script with the same
arguments you would pass to the `configure` script of any
autotools-based project:

~~~{.c}
./configure
~~~

This will translate the arugments you pass to the CMake versions and
invoke CMake.

For a list of supported arguments, pass `--help`.  If you don't have
bash, you'll have to call CMake manually:

~~~{.c}
cmake .
~~~

At this point, all you need to is call `make` likely `make install`,
just as you would for any other project.
