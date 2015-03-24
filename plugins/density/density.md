# density Plugin #

DENSITY is a superfast compression library

For more information about DENSITY, see
https://github.com/centaurean/density

## Codecs ##

- **density** — DENSITY data.

## Options ##

Includes excerpts from the DENSITY README.

- **level** (integer, 1 or 9, default 1)
  - 1 — Chameleon algorithm: "This is a dictionary lookup based
    compression algorithm. It is designed for absolute speed and
    usually reaches a ~60% compression ratio on compressible
    data. Decompression is just as fast. This algorithm is a great
    choice when main concern is speed."
  - 7 — Cheetah algorithm: "This algorithm was developed in
    conjunction with [Piotr Tarsa](https://github.com/tarsa). It uses
    swapped double dictionary lookups and predictions. It can be
    extremely good with highly compressible data (ratio reaching ~5%
    or less). On typical compressible data compression ratio is ~50%
    or less. It is still extremely fast for both compression and
    decompression and is a great, efficient all-rounder algorithm."

  - 9 — Lion algorithm: "Lion is a multiform compression algorithm,
    just like a variable geometry aircraft it adapts to the incoming
    data and tries to offer the most efficient compression technique
    available.

    It uses swapped double dictionary lookups, multiple predictions,
    shifting sub-word dictionary lookups, symbol and forms rank
    entropy coding.

    Lion provides the best compression ratio of all three algorithms
    under any circumstance, and is still very fast."
- **checksum** (boolean, default false) — whether or not to include a
  checksum (SpookyHash) for integrity checks during decompression.

## License ##

The density plugin is licensed under the [MIT
License](http://opensource.org/licenses/MIT), and DENSITY is licensed
under a [3-clause BSD](http://opensource.org/licenses/BSD-3-Clause)
license.
