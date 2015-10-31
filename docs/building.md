# Building Squash

## Dependencies

In order to build Squash, you'll need CMake, make, a C compiler, and a
C++ compiler.  There are optional dependencies on
[Ragel](http://www.colm.net/open-source/ragel/) (for regenerating
parsers) and [GLib](https://wiki.gnome.org/Projects/GLib) (used for
unit tests).

The necessary packages vary by distribution, but for some of the more
popular distributions:

* **Debian/Ubuntu** — gcc g++ ragel cmake make libglib2.0-dev
* **Fedora/RHEL/CentOS** — gcc gcc-c++ ragel cmake make glib2-devel
* **Homebrew (OS X)** — clang cmake ragel glib
* **FreeBSD** — cmake ragel glib

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
* **Homebrew (OS X)** — xz lzo
* **FreeBSD** — bzip2 lzo2

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

### UNIX-like

If you are on a UNIX-like system (basically non-Windows), you can use
the `configure` bash script.  As far as I know, the only UNIX-like OS
where bash may not be installed by default is FreeBSD.  However, it
can be installed from the FreeBSD ports collection (the package name
is "bash").

To configure Squash, simply run the `configure` script with the same
arguments you would pass to the `configure` script of any
autotools-based project:

~~~{.sh}
./configure
~~~

This will translate the arugments you pass to the CMake versions and
invoke CMake.  For a list of supported arguments, pass `--help`.
Typically you'll want to pass at least a '--prefix' argument, and
possibly '--libdir'.

If you don't have bash, you'll have to call CMake manually (see the
"CMake arguments" section at the end of this document for information
on arguments):

~~~{.sh}
cmake .
~~~

At this point, all you need to do is call `make`, and likely `make
install`, just as you would for any other project.  If you encounter
an error at this point please file a bug—all misconfigurations should
be detected by `configure`/`cmake`.

### Windows

Squash's Windows support is still
[incomplete](https://github.com/quixdb/squash/labels/windows), but it
does build.  Building should be roughly the same as any other project
which uses CMake.

## CMake arguments

The `configure` wrapper script which comes with Squash provides
documentation of useful options which you can view by passing `--help`
to the script.  In the event you cannot use that script (for example,
if you are on a system without bash, notably Windows), you can still
call CMake directly, but there is no documentation of the variables
Squash uses.

Each plugin can be disabled (or, if disabled by default, enabled) by
passing `-DENABLE_PLUGIN_NAME=yes|no`.  "PLUGIN_NAME" is the name of
the plugin converted to uppercase with all non-alphanumeric characters
replaced with an underscore.  For example, to disable the ms-compress
plugin you would pass `-DENABLE_MS_COMPRESS=no`.

If you would like to use the in-tree copies of various libraries
shipped with Squash, even when the library in question is installed
system-wide, you can pass `-DFORCE_IN_TREE_DEPENDENCIES=yes`.

Finally, there are two variables that you will generally only want to
use on Windows: you can specify the directory to install plugins to
using the "PLUGIN_DIRECTORY" variable, and you can specify a *list* of
directories Squash will search at runtime for plugins using the
"SEARCH_PATH" variable.  On Windows, the search path is a semi-colon
separated list of directories, everywhere else it is colon-separated.
