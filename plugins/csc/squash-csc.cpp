/* Copyright (c) 2015-2016 The Squash Authors
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

#include <squash/squash.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <csc_enc.h>
#include <csc_dec.h>
#include <Types.h>

enum SquashCscOptionIndex {
  SQUASH_CSC_OPT_LEVEL = 0,
  SQUASH_CSC_OPT_DICT_SIZE,
  SQUASH_CSC_OPT_DELTA_FILTER,
  SQUASH_CSC_OPT_EXE_FILTER,
  SQUASH_CSC_OPT_TXT_FILTER,
};

/* C++ doesn't allow us to initialize this correctly here (or, at
   least, I can't figure out how to do it), so there is some extra
   code in the init_plugin func to finish it off. */
static SquashOptionInfo squash_csc_options[] = {
  { .name = (char*) "level",
    .type = SQUASH_OPTION_TYPE_RANGE_INT },
  { .name = (char*) "dict-size",
    .type = SQUASH_OPTION_TYPE_RANGE_SIZE },
  { .name = (char*) "delta-filter",
    .type = SQUASH_OPTION_TYPE_BOOL },
  { .name = (char*) "exe-filter",
    .type = SQUASH_OPTION_TYPE_BOOL },
  { .name = (char*) "txt-filter",
    .type = SQUASH_OPTION_TYPE_BOOL },
  { NULL, SQUASH_OPTION_TYPE_NONE, }
};

extern "C" SQUASH_PLUGIN_EXPORT
SquashStatus             squash_plugin_init_codec   (SquashCodec* codec, SquashCodecImpl* impl);
extern "C" SQUASH_PLUGIN_EXPORT
SquashStatus             squash_plugin_init_plugin  (SquashPlugin* plugin);

static void* squash_csc_alloc (void* p, size_t size) {
  return squash_malloc (size);
}

static void squash_csc_free (void* p, void* address) {
  squash_free (address);
}

static const ISzAlloc squash_csc_allocator = {
  squash_csc_alloc,
  squash_csc_free
};

struct SquashCscInStream {
  ISeqInStream stream;
  void* user_data;
  SquashReadFunc reader;
  SquashWriteFunc writer;
};

struct SquashCscOutStream {
  ISeqOutStream os;
  void* user_data;
  SquashReadFunc reader;
  SquashWriteFunc writer;
};

static SRes
squash_csc_reader (void* istream, void* buf, size_t* size) {
  SquashCscInStream* s = (SquashCscInStream*) istream;

  s->reader (size, (uint8_t*) buf, s->user_data);

  return 0;
}

static size_t
squash_csc_writer (void* ostream, const void* buf, size_t size) {
  SquashCscOutStream* s = (SquashCscOutStream*) ostream;

  s->writer (&size, (const uint8_t*) buf, s->user_data);

  return size;
}

static SquashStatus
squash_csc_splice (SquashCodec* codec,
                   SquashOptions* options,
                   SquashStreamType stream_type,
                   SquashReadFunc read_cb,
                   SquashWriteFunc write_cb,
                   void* user_data) {
  const struct SquashCscInStream in_stream = {
    squash_csc_reader,
    user_data,
    read_cb,
    write_cb
  };
  const struct SquashCscOutStream out_stream = {
    squash_csc_writer,
    user_data,
    read_cb,
    write_cb
  };

  CSCProps props;
  unsigned char props_buf[CSC_PROP_SIZE];

  if (stream_type == SQUASH_STREAM_COMPRESS) {
    CSCEncProps_Init (&props,
                      squash_codec_get_option_size_index (codec, options, SQUASH_CSC_OPT_DICT_SIZE),
                      squash_codec_get_option_int_index (codec, options, SQUASH_CSC_OPT_LEVEL));
    props.DLTFilter = squash_codec_get_option_bool_index (codec, options, SQUASH_CSC_OPT_DELTA_FILTER);
    props.EXEFilter = squash_codec_get_option_bool_index (codec, options, SQUASH_CSC_OPT_EXE_FILTER);
    props.TXTFilter = squash_codec_get_option_bool_index (codec, options, SQUASH_CSC_OPT_TXT_FILTER);

    CSCEnc_WriteProperties (&props, props_buf, 0);
    size_t bytes_written = squash_csc_writer ((void*) &out_stream, props_buf, CSC_PROP_SIZE);
    if (SQUASH_UNLIKELY(bytes_written != CSC_PROP_SIZE))
      return squash_error (SQUASH_FAILED);

    CSCEncHandle comp = CSCEnc_Create (&props, (ISeqOutStream*) &out_stream, (ISzAlloc*) &squash_csc_allocator);
    CSCEnc_Encode (comp, (ISeqInStream*) &in_stream, NULL);
    CSCEnc_Encode_Flush (comp);
  } else {
    size_t prop_l = CSC_PROP_SIZE;
    squash_csc_reader ((void*) &in_stream, props_buf, &prop_l);
    if (SQUASH_UNLIKELY(prop_l != CSC_PROP_SIZE))
      return squash_error (SQUASH_FAILED);

    CSCDec_ReadProperties (&props, props_buf);

    CSCDecHandle decomp = CSCDec_Create (&props, (ISeqInStream*) &in_stream, (ISzAlloc*) &squash_csc_allocator);
    CSCDec_Decode (decomp, (ISeqOutStream*) &out_stream, NULL);
  }

  return SQUASH_OK;
}

static size_t
squash_csc_get_max_compressed_size (SquashCodec* codec, size_t uncompressed_size) {
  // TODO: this could probably be improved
  return
    uncompressed_size + 64 + (uncompressed_size / 128);
}

extern "C" SquashStatus
squash_plugin_init_plugin (SquashPlugin* plugin) {
  const SquashOptionInfoRangeInt level_range = { 1, 5, 0, false };
  const SquashOptionInfoRangeSize dict_size_range = { 32768, 1073741824, 0, false };

  squash_csc_options[SQUASH_CSC_OPT_LEVEL].default_value.int_value = 2;
  squash_csc_options[SQUASH_CSC_OPT_LEVEL].info.range_int = level_range;
  squash_csc_options[SQUASH_CSC_OPT_DICT_SIZE].default_value.size_value = (1024 * 1024 * 64);
  squash_csc_options[SQUASH_CSC_OPT_DICT_SIZE].info.range_size = dict_size_range;
  squash_csc_options[SQUASH_CSC_OPT_DELTA_FILTER].default_value.bool_value = false;
  squash_csc_options[SQUASH_CSC_OPT_EXE_FILTER].default_value.bool_value = true;
  squash_csc_options[SQUASH_CSC_OPT_TXT_FILTER].default_value.bool_value = true;

  return SQUASH_OK;
}

extern "C" SquashStatus
squash_plugin_init_codec (SquashCodec* codec, SquashCodecImpl* impl) {
  const char* name = squash_codec_get_name (codec);

  if (SQUASH_LIKELY(strcmp ("csc", name) == 0)) {
    impl->options = squash_csc_options;
    impl->splice = squash_csc_splice;
    impl->get_max_compressed_size = squash_csc_get_max_compressed_size;
  } else {
    return squash_error (SQUASH_UNABLE_TO_LOAD);
  }

  return SQUASH_OK;
}
