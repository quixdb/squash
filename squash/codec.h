/* Copyright (c) 2013 The Squash Authors
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *   Evan Nemerson <evan@coeus-group.com>
 */

#ifndef __SQUASH_CODEC_H__
#define __SQUASH_CODEC_H__

#if !defined (__SQUASH_H_INSIDE__) && !defined (SQUASH_COMPILATION)
#error "Only <squash/squash.h> can be included directly."
#endif

#include <stdio.h>

SQUASH_BEGIN_DECLS

struct _SquashCodecFuncs {
  /* Options */
  SquashOptions*  (* create_options)          (SquashCodec* codec);
  SquashStatus    (* parse_option)            (SquashOptions* options, const char* key, const char* value);

  /* Streams */
  SquashStream*   (* create_stream)           (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options);
  SquashStatus    (* process_stream)          (SquashStream* stream);
  SquashStatus    (* flush_stream)            (SquashStream* stream);
  SquashStatus    (* finish_stream)           (SquashStream* stream);

  /* Codecs */
  size_t          (* get_uncompressed_size)   (SquashCodec* codec, const uint8_t* compressed, size_t compressed_length);
  size_t          (* get_max_compressed_size) (SquashCodec* codec, size_t uncompressed_length);

  /* Buffers */
  SquashStatus    (* decompress_buffer)       (SquashCodec* codec,
                                               uint8_t* decompressed, size_t* decompressed_length,
                                               const uint8_t* compressed, size_t compressed_length,
                                               SquashOptions* options);
  SquashStatus    (* compress_buffer)         (SquashCodec* codec,
                                               uint8_t* compressed, size_t* compressed_length,
                                               const uint8_t* uncompressed, size_t uncompressed_length,
                                               SquashOptions* options);
};

typedef void (*SquashCodecForeachFunc) (SquashCodec* codec, void* data);

SQUASH_API SquashStatus  squash_codec_init                        (SquashCodec* codec);
SQUASH_API const char*   squash_codec_get_name                    (SquashCodec* codec);
SQUASH_API unsigned int  squash_codec_get_priority                (SquashCodec* codec);
SQUASH_API SquashPlugin* squash_codec_get_plugin                  (SquashCodec* codec);

SQUASH_API bool          squash_codec_get_knows_uncompressed_size (SquashCodec* codec);
SQUASH_API size_t        squash_codec_get_uncompressed_size       (SquashCodec* codec, const uint8_t* compressed, size_t compressed_length);
SQUASH_API size_t        squash_codec_get_max_compressed_size     (SquashCodec* codec, size_t uncompressed_length);

SQUASH_API SquashStream* squash_codec_create_stream               (SquashCodec* codec, SquashStreamType stream_type, ...);
SQUASH_API SquashStream* squash_codec_create_stream_with_options  (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options);

SQUASH_API SquashStatus  squash_codec_compress                    (SquashCodec* codec,
                                                                   uint8_t* compressed, size_t* compressed_length,
                                                                   const uint8_t* uncompressed, size_t uncompressed_length, ...);
SQUASH_API SquashStatus  squash_codec_compress_with_options       (SquashCodec* codec,
                                                                   uint8_t* compressed, size_t* compressed_length,
                                                                   const uint8_t* uncompressed, size_t uncompressed_length,
                                                                   SquashOptions* options);
SQUASH_API SquashStatus  squash_codec_decompress                  (SquashCodec* codec,
                                                                   uint8_t* decompressed, size_t* decompressed_length,
                                                                   const uint8_t* compressed, size_t compressed_length, ...);
SQUASH_API SquashStatus  squash_codec_decompress_with_options     (SquashCodec* codec,
                                                                   uint8_t* decompressed, size_t* decompressed_length,
                                                                   const uint8_t* compressed, size_t compressed_length,
                                                                   SquashOptions* options);
SQUASH_API SquashStatus  squash_codec_compress_file_with_options  (SquashCodec* codec,
                                                                   FILE* compressed, FILE* uncompressed,
                                                                   SquashOptions* options);
SQUASH_API SquashStatus  squash_codec_decompress_file_with_options (SquashCodec* codec,
                                                                   FILE* decompressed, FILE* compressed,
                                                                   SquashOptions* options);
SQUASH_API SquashStatus  squash_codec_compress_file               (SquashCodec* codec,
                                                                   FILE* compressed, FILE* uncompressed,
                                                                   ...);
SQUASH_API SquashStatus  squash_codec_decompress_file             (SquashCodec* codec,
                                                                   FILE* decompressed, FILE* compressed,
                                                                   ...);


SQUASH_API size_t        squash_get_max_compressed_size           (const char* codec, size_t uncompressed_length);
SQUASH_API SquashStatus  squash_compress                          (const char* codec,
                                                                   uint8_t* compressed, size_t* compressed_length,
                                                                   const uint8_t* uncompressed, size_t uncompressed_length, ...);
SQUASH_API SquashStatus  squash_compress_with_options             (const char* codec,
                                                                   uint8_t* compressed, size_t* compressed_length,
                                                                   const uint8_t* uncompressed, size_t uncompressed_length,
                                                                   SquashOptions* options);
SQUASH_API SquashStatus  squash_decompress                        (const char* codec,
                                                                   uint8_t* decompressed, size_t* decompressed_length,
                                                                   const uint8_t* compressed, size_t compressed_length, ...);
SQUASH_API SquashStatus  squash_decompress_with_options           (const char* codec,
                                                                   uint8_t* decompressed, size_t* decompressed_length,
                                                                   const uint8_t* compressed, size_t compressed_length,
                                                                   SquashOptions* options);
SQUASH_API SquashStatus  squash_compress_file_with_options        (const char* codec,
                                                                   FILE* compressed, FILE* uncompressed,
                                                                   SquashOptions* options);
SQUASH_API SquashStatus  squash_decompress_file_with_options      (const char* codec,
                                                                   FILE* decompressed, FILE* compressed,
                                                                   SquashOptions* options);
SQUASH_API SquashStatus  squash_compress_file                     (const char* codec,
                                                                   FILE* compressed, FILE* uncompressed,
                                                                   ...);
SQUASH_API SquashStatus  squash_decompress_file                   (const char* codec,
                                                                   FILE* decompressed, FILE* compressed,
                                                                   ...);

SQUASH_END_DECLS

#endif /* __SQUASH_CODEC_H__ */
