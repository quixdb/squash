# Building Squash

Note: [Squash currently does not support
Windows](https://github.com/quixdb/squash/issues/86).  It should work
on at least Linux, BSD, and OS X.  If you encounter problems please
file an issue.

## Dependencies

In order to build Squash, you'll need CMake, make, a C compiler, a C++
compiler, and [Ragel](http://www.colm.net/open-source/ragel/).
Additionally, if you want to build the tests (which is a good idea)
you'll need glib.  The necessary packages vary by distribution, but
for some of the more popular distributions:

* **Debian/Ubuntu** — gcc g++ ragel cmake make libglib2.0-dev
* **Fedora/RHEL/CentOS** — gcc gcc-c++ ragel cmake make glib2-devel

Squash includes copies of all the libraries it uses for
compression/decompression.  That said, you may prefer to use system
copies when available.  By default, if a system library is installed
when configuring the build Squash will use it, if not it will fall
back on its internal copy.  Again, the package names vary by
distribution, but for some of the more popular distributions:

* **Debian/Ubuntu** — libbz2-dev liblzma-dev liblzo2-dev libsnappy-dev
  zlib1g-dev
* **Fedora/RHEL/CentOS** — bzip2-devel xz-devel liblzf-devel lzo-devel
  snappy-devel zlib-devel

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

~~~{.sh}
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

~~~{.sh}
./configure
~~~

This will translate the arugments you pass to the CMake versions and
invoke CMake.

For a list of supported arguments, pass `--help`.  If you don't have
bash, you'll have to call CMake manually:

~~~{.sh}
cmake .
~~~

At this point, all you need to is call `make`, and likely `make
install`, just as you would for any other project.
