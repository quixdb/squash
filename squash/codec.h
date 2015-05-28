/* Copyright (c) 2013-2015 The Squash Authors
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
 *   Evan Nemerson <evan@nemerson.com>
 */

#ifndef SQUASH_CODEC_H
#define SQUASH_CODEC_H

#if !defined (SQUASH_H_INSIDE) && !defined (SQUASH_COMPILATION)
#error "Only <squash/squash.h> can be included directly."
#endif

#include <stdio.h>

SQUASH_BEGIN_DECLS

typedef enum {
  SQUASH_CODEC_INFO_CAN_FLUSH               = 1 <<  0,
  SQUASH_CODEC_INFO_RUN_IN_THREAD           = 1 <<  1,
  SQUASH_CODEC_INFO_DECOMPRESS_SAFE         = 1 <<  2,

  SQUASH_CODEC_INFO_AUTO_MASK               = 0x00ff0000,
  SQUASH_CODEC_INFO_VALID                   = 1 << 16,
  SQUASH_CODEC_INFO_KNOWS_UNCOMPRESSED_SIZE = 1 << 17,
  SQUASH_CODEC_INFO_NATIVE_STREAMING        = 1 << 18,

  SQUASH_CODEC_INFO_MASK                    = 0xffffffff
} SquashCodecInfo;

#define SQUASH_CODEC_INFO_INVALID ((SquashCodecInfo) 0)

struct _SquashCodecImpl {
  SquashCodecInfo           info;

  const SquashOptionInfo*   options;

  /* Streams */
  SquashStream*           (* create_stream)            (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options);
  SquashStatus            (* process_stream)           (SquashStream* stream, SquashOperation operation);

  /* Buffers */
  SquashStatus            (* decompress_buffer)        (SquashCodec* codec,
                                                        size_t* decompressed_length,
                                                        uint8_t decompressed[SQUASH_ARRAY_PARAM(*decompressed_length)],
                                                        size_t compressed_length,
                                                        const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_length)],
                                                        SquashOptions* options);
  SquashStatus            (* compress_buffer)          (SquashCodec* codec,
                                                        size_t* compressed_length,
                                                        uint8_t compressed[SQUASH_ARRAY_PARAM(*compressed_length)],
                                                        size_t uncompressed_length,
                                                        const uint8_t uncompressed[SQUASH_ARRAY_PARAM(uncompressed_length)],
                                                        SquashOptions* options);
  SquashStatus            (* compress_buffer_unsafe)   (SquashCodec* codec,
                                                        size_t* compressed_length,
                                                        uint8_t compressed[SQUASH_ARRAY_PARAM(*compressed_length)],
                                                        size_t uncompressed_length,
                                                        const uint8_t uncompressed[SQUASH_ARRAY_PARAM(uncompressed_length)],
                                                        SquashOptions* options);

  /* Codecs */
  size_t                  (* get_uncompressed_size)    (SquashCodec* codec,
                                                        size_t compressed_length,
                                                        const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_length)]);
  size_t                  (* get_max_compressed_size)  (SquashCodec* codec, size_t uncompressed_length);

  /* Reserved */
  void                    (* _reserved1)               (void);
  void                    (* _reserved2)               (void);
  void                    (* _reserved3)               (void);
  void                    (* _reserved4)               (void);
  void                    (* _reserved5)               (void);
  void                    (* _reserved6)               (void);
  void                    (* _reserved7)               (void);
  void                    (* _reserved8)               (void);
};

typedef void (*SquashCodecForeachFunc) (SquashCodec* codec, void* data);

SQUASH_API SquashStatus            squash_codec_init                         (SquashCodec* codec);
SQUASH_API const char*             squash_codec_get_name                     (SquashCodec* codec);
SQUASH_API unsigned int            squash_codec_get_priority                 (SquashCodec* codec);
SQUASH_API SquashPlugin*           squash_codec_get_plugin                   (SquashCodec* codec);
SQUASH_API const char*             squash_codec_get_extension                (SquashCodec* codec);

SQUASH_API size_t                  squash_codec_get_uncompressed_size        (SquashCodec* codec,
                                                                              size_t compressed_length,
                                                                              const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_length)]);
SQUASH_API size_t                  squash_codec_get_max_compressed_size      (SquashCodec* codec, size_t uncompressed_length);

SQUASH_API SquashStream*           squash_codec_create_stream                (SquashCodec* codec, SquashStreamType stream_type, ...);
SQUASH_API SquashStream*           squash_codec_create_stream_with_options   (SquashCodec* codec, SquashStreamType stream_type, SquashOptions* options);

SQUASH_API SquashStatus            squash_codec_compress                     (SquashCodec* codec,
                                                                              size_t* compressed_length,
                                                                              uint8_t compressed[SQUASH_ARRAY_PARAM(*compressed_length)],
                                                                              size_t uncompressed_length,
                                                                              const uint8_t uncompressed[SQUASH_ARRAY_PARAM(uncompressed_length)],
                                                                              ...);
SQUASH_API SquashStatus            squash_codec_compress_with_options        (SquashCodec* codec,
                                                                              size_t* compressed_length,
                                                                              uint8_t compressed[SQUASH_ARRAY_PARAM(*compressed_length)],
                                                                              size_t uncompressed_length,
                                                                              const uint8_t uncompressed[SQUASH_ARRAY_PARAM(uncompressed_length)],
                                                                              SquashOptions* options);
SQUASH_API SquashStatus            squash_codec_decompress                   (SquashCodec* codec,
                                                                              size_t* decompressed_length,
                                                                              uint8_t decompressed[SQUASH_ARRAY_PARAM(*decompressed_length)],
                                                                              size_t compressed_length,
                                                                              const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_length)], ...);
SQUASH_API SquashStatus            squash_codec_decompress_with_options      (SquashCodec* codec,
                                                                              size_t* decompressed_length,
                                                                              uint8_t decompressed[SQUASH_ARRAY_PARAM(*decompressed_length)],
                                                                              size_t compressed_length,
                                                                              const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_length)],
                                                                              SquashOptions* options);
SQUASH_API SquashStatus            squash_codec_compress_file_with_options   (SquashCodec* codec,
                                                                              FILE* compressed, FILE* uncompressed,
                                                                              SquashOptions* options);
SQUASH_API SquashStatus            squash_codec_decompress_file_with_options (SquashCodec* codec,
                                                                              FILE* decompressed, size_t decompressed_length,
                                                                              FILE* compressed,
                                                                              SquashOptions* options);
SQUASH_API SquashStatus            squash_codec_compress_file                (SquashCodec* codec,
                                                                              FILE* compressed, FILE* uncompressed,
                                                                              ...);
SQUASH_API SquashStatus            squash_codec_decompress_file              (SquashCodec* codec,
                                                                              FILE* decompressed,
                                                                              size_t decompressed_length,
                                                                              FILE* compressed,
                                                                              ...);
SQUASH_API SquashCodecInfo         squash_codec_get_info                     (SquashCodec* codec);
SQUASH_API const SquashOptionInfo* squash_codec_get_option_info              (SquashCodec* codec);

SQUASH_API const char*             squash_codec_get_option_string            (SquashCodec* codec,
                                                                              SquashOptions* options,
                                                                              const char* key);
SQUASH_API bool                    squash_codec_get_option_bool              (SquashCodec* codec,
                                                                              SquashOptions* options,
                                                                              const char* key);
SQUASH_API int                     squash_codec_get_option_int               (SquashCodec* codec,
                                                                              SquashOptions* options,
                                                                              const char* key);
SQUASH_API size_t                  squash_codec_get_option_size              (SquashCodec* codec,
                                                                              SquashOptions* options,
                                                                              const char* key);
SQUASH_API const char*             squash_codec_get_option_string_index      (SquashCodec* codec,
                                                                              SquashOptions* options,
                                                                              size_t index);
SQUASH_API bool                    squash_codec_get_option_bool_index        (SquashCodec* codec,
                                                                              SquashOptions* options,
                                                                              size_t index);
SQUASH_API int                     squash_codec_get_option_int_index         (SquashCodec* codec,
                                                                              SquashOptions* options,
                                                                              size_t index);
SQUASH_API size_t                  squash_codec_get_option_size_index        (SquashCodec* codec,
                                                                              SquashOptions* options,
                                                                              size_t index);


SQUASH_API size_t                  squash_get_max_compressed_size            (const char* codec, size_t uncompressed_length);
SQUASH_API SquashStatus            squash_compress                           (const char* codec,
                                                                              size_t* compressed_length,
                                                                              uint8_t compressed[SQUASH_ARRAY_PARAM(*compressed_length)],
                                                                              size_t uncompressed_length,
                                                                              const uint8_t uncompressed[SQUASH_ARRAY_PARAM(uncompressed_length)],
                                                                              ...);
SQUASH_API SquashStatus            squash_compress_with_options              (const char* codec,
                                                                              size_t* compressed_length,
                                                                              uint8_t compressed[SQUASH_ARRAY_PARAM(*compressed_length)],
                                                                              size_t uncompressed_length,
                                                                              const uint8_t uncompressed[SQUASH_ARRAY_PARAM(uncompressed_length)],
                                                                              SquashOptions* options);
SQUASH_API SquashStatus            squash_decompress                         (const char* codec,
                                                                              size_t* decompressed_length,
                                                                              uint8_t decompressed[SQUASH_ARRAY_PARAM(*decompressed_length)],
                                                                              size_t compressed_length,
                                                                              const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_length)],
                                                                              ...);
SQUASH_API SquashStatus            squash_decompress_with_options            (const char* codec,
                                                                              size_t* decompressed_length,
                                                                              uint8_t decompressed[SQUASH_ARRAY_PARAM(*decompressed_length)],
                                                                              size_t compressed_length,
                                                                              const uint8_t compressed[SQUASH_ARRAY_PARAM(compressed_length)],
                                                                              SquashOptions* options);
SQUASH_API SquashStatus            squash_compress_file_with_options         (const char* codec,
                                                                              FILE* compressed, FILE* uncompressed,
                                                                              SquashOptions* options);
SQUASH_API SquashStatus            squash_decompress_file_with_options       (const char* codec,
                                                                              FILE* decompressed,
                                                                              size_t decompressed_length,
                                                                              FILE* compressed,
                                                                              SquashOptions* options);
SQUASH_API SquashStatus            squash_compress_file                      (const char* codec,
                                                                              FILE* compressed, FILE* uncompressed,
                                                                              ...);
SQUASH_API SquashStatus            squash_decompress_file                    (const char* codec,
                                                                              FILE* decompressed,
                                                                              size_t decompressed_length,
                                                                              FILE* compressed,
                                                                              ...);
SQUASH_API SquashCodecInfo         squash_get_info                           (const char* codec);
SQUASH_API const SquashOptionInfo* squash_get_option_info                    (const char* codec);

SQUASH_END_DECLS

#endif /* SQUASH_CODEC_H */
