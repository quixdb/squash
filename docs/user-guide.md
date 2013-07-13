User Guide
==========

While the Squash API is fairly large, in general you will only need a
handful of functions.

Squash's API follows some pretty standard conventions:

- Type names are CamelCase (with the first letter uppercase)
- Functions are lowercase_with_underscores
- Binary data is two variables—a `uint8_t*` and a `size_t`.
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
                 uint8_t* compressed, size_t* compressed_length,
                 const uint8_t* uncompressed, size_t uncompressed_length, ...);
~~~

Like many functions in Squash, ::squash_compress returns a
[SquashStatus](@ref SquashStatus).  If the operation was successful, a
positive number is returned (generally ::SQUASH_OK), and a negative
number is returned on failure.

The first argument is the name of the codec.

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
                   compressed, &compressed_length,
                   (const uint8_t*) uncompressed, uncompressed_length,
                   NULL);

if (res != SQUASH_OK) {
  fprintf (stderr, "Unable to compress data [%d]: %s\n",
           res, squash_status_to_string (res));
  exit (res);
}

fprintf (stdout, "Compressed a %lu byte buffer to %lu bytes.\n",
         uncompressed_length, compressed_length);
~~~

Decompression is basically the same:

~~~{.c}
size_t decompressed_length = uncompressed_length + 1;
char* decompressed = (char*) malloc (uncompressed_length + 1);

res = squash_decompress ("deflate",
                         (uint8_t*) decompressed, &decompressed_length,
                         compressed, compressed_length, NULL);

if (res != SQUASH_OK) {
  fprintf (stderr, "Unable to decompress data [%d]: %s\n",
           res, squash_status_to_string (res));
  exit (res);
}

/* Notice that we didn't compress the *NULL* byte at the end of the
   string.  We could have, it's just a waste to do so. */
decompressed[decompressed_length] = '\0';

if (strcmp (decompressed, uncompressed) != 0) {
  fprintf (stderr, "Bad decompressed data.\n");
  exit (-1);
}

fprintf (stdout, "Successfully decompressed.\n");
~~~

It is worth noting that "Hello, world!" is really too short to
compress (it "compresses" from 13 bytes to 15 bytes here), but using a
longer string will yield better results.  You can find a complete,
self-contained example in [simple.c](@ref simple.c).

@section streams Stream API

While the buffer API is very easy to use, if you want to compress a
lot of data without loading it into memory then the stream API is much
more appropriate.

If you've ever worked with a compression API in the past then the
stream API will probably feel pretty familiar to you.  The basic idea
is that you have a data structure, a @ref SquashStream, and you
shuttle data into one buffer and out of another.

@example simple.c