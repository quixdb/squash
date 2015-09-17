User Guide
==========

While the Squash API is fairly large, it is conceptually fairly
simple.

Most of the functions are really just layers of convenience wrappers
which follow common conventions; for example, many functions will have
versions for string and @ref SquashCodec parameters for the codec, and
variadic arguments or a @ref SquashOptions parameter for the options,
which means four versions for every function, but only one concept to
learn.

Squash's API follows some pretty standard conventions:

- Type names are CamelCase (with the first letter uppercase)
- Functions are lowercase_with_underscores
- Binary data is two parameters—a `size_t` followed by a `uint8_t*`.
- Instance arguments come first, followed by any output arguments,
  followed by input arguments.

The heart of Squash is the [codec](@ref SquashCodec).  A codec is an
implementation of an algorithm, and codecs with the same name are
assumed to be compatible.  A single [plugin](@ref SquashPlugin) can,
and often does, provide an implementation of several different codecs.

Typically you will not deal with [contexts](@ref SquashContext) or
plugins directly, and you may only deal with strings to represent
codecs.

@section buffers Buffer API

If you have a block of data in memory which you want to compress (or
decompress), the easiest way to do so is using the buffer API.

~~~{.c}
SquashStatus
squash_compress (const char* codec,
                 size_t* compressed_length,
                 uint8_t compressed[],
                 size_t uncompressed_length,
                 const uint8_t uncompressed[],
                 ...);
~~~

Like many functions in Squash, ::squash_compress returns a
[SquashStatus](@ref SquashStatus).  If the operation was successful, a
positive number is returned (generally ::SQUASH_OK), and a negative
number is returned on failure.

The first argument is the name of the codec.  To list available
codecs, you can use the `squash -L` command; the convention is that
codecs are just the lowercase name of the algorithm.  For example,
"gzip", "lz4", and "bzip2" are all what you would probably expect.

The @a compressed and @a compressed_length arguments represent the
buffer which you wish to compress the data into.  Notice that @a
compressed_length is a `size_t*`, not a `size_t`.  When you call the
function it should be a pointer to the number of bytes available for
writing in the compressed buffer, and the function will alter the
value to the true size of the compressed data upon successful
compression.

@a uncompressed and @a uncompressed_length are the buffer you wish to
compress.  @a uncompressed_length is a `size_t`, not a `size_t`,
because the function does not need to modify the value.

Finally, this function is variadic—it accepts an arbitrary number of
options, which you can use to control things like the compression
level.  Options are key/value pairs of strings, terminated by a *NULL*
sentinel… More on that later, but for now, just know that passing
*NULL* there will use the defaults, which is generally what you want.

Knowing how big the @a compressed buffer needs to be generally
requires another function:

~~~{.c}
size_t
squash_get_max_compressed_size (const char* codec, size_t uncompressed_length);
~~~

This function returns the maximum buffer size necessary to hold @a
uncompressed_length worth of compressed data, even in the worst case
scenario (uncompressable data).  Typically it is slightly larger than
@a uncompressed_length.

So, if you wanted to compress some data using the deflate algorithm,
you end up with something like this:

~~~{.c}
const char* uncompressed = "Hello, world!";
size_t uncompressed_length = strlen (uncompressed);
size_t compressed_length = squash_get_max_compressed_size ("deflate", uncompressed_length);
uint8_t* compressed = (uint8_t*) malloc (compressed_length);

SquashStatus res =
  squash_compress ("deflate",
                   &compressed_length, compressed,
                   uncompressed_length, (const uint8_t*) uncompressed,
                   NULL);

if (res != SQUASH_OK) {
  fprintf (stderr, "Unable to compress data [%d]: %s\n",
           res, squash_status_to_string (res));
  exit (EXIT_FAILURE);
}

fprintf (stdout, "Compressed a %lu byte buffer to %lu bytes.\n",
         uncompressed_length, compressed_length);
~~~

Decompression is basically the same:

~~~{.c}
size_t decompressed_length = uncompressed_length + 1;
char* decompressed = (char*) malloc (uncompressed_length + 1);

res = squash_decompress ("deflate",
                         &decompressed_length, (uint8_t*) decompressed
                         compressed_length, compressed, NULL);

if (res != SQUASH_OK) {
  fprintf (stderr, "Unable to decompress data [%d]: %s\n",
           res, squash_status_to_string (res));
  exit (EXIT_FAILURE);
}

/* Notice that we didn't compress the *NULL* byte at the end of the
   string.  We could have, it's just a waste to do so. */
decompressed[decompressed_length] = '\0';

if (strcmp (decompressed, uncompressed) != 0) {
  fprintf (stderr, "Bad decompressed data.\n");
  exit (EXIT_FAILURE);
}

fprintf (stdout, "Successfully decompressed.\n");
~~~

It is worth noting that "Hello, world!" is really too short to
compress (it "compresses" from 13 bytes to 15 bytes here), but using a
longer string will yield better results.  You can find a complete,
self-contained example in [simple.c](@ref simple.c).

@section streams File I/O API

While the buffer API is very easy to use it can be a bit limiting.  If
you want to compress a lot of data without loading it into memory then
a streaming API is much more appropriate.

Squash has two streaming APIs; the first is modeled after the API
provided by zlib, which has been copied by many other libraries.  It
is a bit of a pain to use, but extremely powerful.  The second API is
a higher-level convenience API which wraps the lower-level API and
provides an interface similar to the standard C I/O API (*i.e.* fopen,
fclose, fflush, fread, and fwrite).

We will not go into detail about the lower-level API now, but if
you're interested you can look at the API documentation for @ref
SquashStream for details.

The higher level API is documented in @ref SquashFile, but some of the
prototypes are reproduced here:

~~~{.c}
SquashFile*
squash_file_open (const char* filename,
                  const char* mode,
                  const char* codec,
                  ...);
SquashStatus
squash_file_read (SquashFile* file,
                  size_t* decompressed_length,
                  uint8_t decompressed[]);
SquashStatus
squash_file_write (SquashFile* file,
                   size_t uncompressed_length,
                   const uint8_t uncompressed[]);
SquashStatus
squash_file_flush (SquashFile* file);
SquashStatus
squash_file_close (SquashFile* file);
~~~

Like most of the Squash API, most of these functions return @ref
SQUASH_OK on success and a negative error code on failure.  The
exception to this is @ref squash_file_open which, like `fopen`,
returns *NULL* on failure.  The @a mode string is passed verbatim to
`fopen`.

Going back to our "Hello, world!" example from above, to write a file
with the compressed version of "Hello, world!", all you would have to
do is something like:

~~~{.c}
const char* uncompressed = "Hello, world!";

SquashFile* file = squash_file_open ("hello.gz", "w+", "gzip", NULL);
if (file == NULL) {
  fprintf (stderr, "Unable to open file for writing\n");
  exit (EXIT_FAILURE);
}

SquashStatus res = squash_file_write (file, strlen (uncompressed), uncompressed);
if (res != SQUASH_OK) {
  fprintf (stderr, "Unable to write compressed data [%d]: %s\n",
           res, squash_status_to_string (res));
  exit (EXIT_FAILURE);
}

SquashStatus res = squash_file_close (file);
if (res != SQUASH_OK) {
  fprintf (stderr, "Unable to close file [%d]: %s\n",
           res, squash_status_to_string (res));
  exit (EXIT_FAILURE);
}
~~~

If you want to simply splice the contents of one file to another
(decompressing of compressing in the process), you can use the
`squash_splice` family of functions, which looks like:

~~~{.c}
SquashStatus
squash_splice (const char* codec,
               SquashStreamType stream_type,
               FILE* fp_out,
               FILE* fp_in,
               size_t length,
               ...);
~~~

Beyond just being convenient, this function has the advantage that for
codecs which don't implement streaming natively Squash will attempt to
memory map the files and pass those addresses directly to the
buffer-to-buffer compression function.  This eliminates a lot of
buffering which, for larger files, can consume significant amounts of
memory.

@example simple.c
