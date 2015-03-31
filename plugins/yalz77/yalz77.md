# yalz77 Plugin #

The yalz77 plugin provides an interface for the naive yalz77 compressor.

For more information see http://bitbucket.org/tkatchev/yalz77

## Codecs ##

- **yalz77** 

## Options ##

- **search-length** (unsigned integer, default 8): size of each hash table bucket.
  (A higher setting means more compression quality and slower compression.)
- **block-size** (unsigned integer, default 65536): number of hash table buckets.
  (Make this lower if you're only compressing short texts; a higher setting means
  higher memory consumption.)

## License ##

The yalz77 plugin is licensed under the [MIT
License](http://opensource.org/licenses/MIT), and the yalz77.h header is [public domain](http://unlicense.org).
