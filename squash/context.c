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

#define _BSD_SOURCE
#define _POSIX_SOURCE

#include <assert.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <stddef.h>
#include <ctype.h>
#include <stdio.h>

#include <ltdl.h>

#include "config.h"
#include "squash.h"
#include "internal.h"

#if HAVE_PTHREAD
#include <pthread.h>
#endif

/**
 * @defgroup SquashContext SquashContext
 * @brief Library context.
 *
 * @ref SquashContext is a singleton which is created the first time
 * ::squash_context_get_default is invoked.  You generally need not deal
 * with the @ref SquashContext directly as Squash provides numerous wrapper
 * functions which you can use instead.
 *
 * @{
 */

/**
 * @struct _SquashContext
 * @brief Context for all Squash operations.
 */

static SquashCodecRef*
squash_context_get_codec_ref (SquashContext* context, const char* codec) {
  SquashCodec key_codec = { 0, };
  SquashCodecRef key = { &key_codec, };

  assert (context != NULL);
  assert (codec != NULL);

  key_codec.name = (char*) codec;

  return SQUASH_TREE_FIND (&(context->codecs), _SquashCodecRef, tree, &key);
}

static SquashCodecRef*
squash_context_get_codec_ref_from_extension (SquashContext* context, const char* extension) {
  SquashCodec key_codec = { 0, };
  SquashCodecRef key = { &key_codec, };

  assert (context != NULL);
  assert (extension != NULL);

  key_codec.extension = (char*) extension;

  return SQUASH_TREE_FIND (&(context->extensions), _SquashCodecRef, tree, &key);
}

/**
 * @brief Retrieve a @ref SquashCodec from a @ref SquashContext.
 *
 * @param context The context to use.
 * @param codec Name of the codec to retrieve.
 * @return The @ref SquashCodec, or *NULL* on failure.  This is owned by
 *   Squash and must never be freed or unreffed.
 */
SquashCodec*
squash_context_get_codec (SquashContext* context, const char* codec) {
  SquashCodecRef* codec_ref = squash_context_get_codec_ref (context, codec);
  if (codec_ref != NULL) {
    /* TODO: we should probably see if we can load the codec from a
       different plugin if this fails.  */
    return (squash_codec_init (codec_ref->codec) == SQUASH_OK) ? codec_ref->codec : NULL;
  } else {
    return NULL;
  }
}

/**
 * @brief Retrieve a @ref SquashCodec.
 *
 * @param codec Name of the codec to retrieve.
 * @return The @ref SquashCodec.  This is owned by Squash and must never be
 *   freed or unreffed.
 */
SquashCodec*
squash_get_codec (const char* codec) {
  return squash_context_get_codec (squash_context_get_default (), codec);
}

/**
 * @brief Retrieve a codec from a context based on an extension
 *
 * @param context The context
 * @param extension The extension
 * @return A ref SquashCodec or *NULL* on failure
 */
SquashCodec*
squash_context_get_codec_from_extension (SquashContext* context, const char* extension) {
  SquashCodecRef* codec_ref = squash_context_get_codec_ref_from_extension (context, extension);
  if (codec_ref != NULL) {
    return (squash_codec_init (codec_ref->codec) == SQUASH_OK) ? codec_ref->codec : NULL;
  } else {
    return NULL;
  }
}

/**
 * @brief Retrieve a codec based on an extension
 *
 * @param extension The extension
 * @return A ref SquashCodec or *NULL* on failure
 */
SquashCodec*
squash_get_codec_from_extension (const char* extension) {
  return squash_context_get_codec_from_extension (squash_context_get_default (), extension);
}

/**
 * @brief Retrieve a @ref SquashPlugin from a @ref SquashContext.
 *
 * @param context The context to use.
 * @param plugin Name of the plugin to retrieve.
 * @return The @ref SquashPlugin.  This is owned by Squash and must never be
 *   freed or unreffed.
 */
SquashPlugin*
squash_context_get_plugin (SquashContext* context, const char* plugin) {
  SquashPlugin plugin_dummy = { 0, };

  assert (context != NULL);
  assert (plugin != NULL);

  plugin_dummy.name = (char*) plugin;

  return SQUASH_TREE_FIND (&(context->plugins), _SquashPlugin, tree, &plugin_dummy);
}

/**
 * @brief Retrieve a @ref SquashPlugin.
 *
 * @param plugin - Name of the plugin to retrieve.
 * @return The @ref SquashPlugin.  This is owned by Squash and must never be
 *   freed or unreffed.
 */
SquashPlugin*
squash_get_plugin (const char* plugin) {
  return squash_context_get_plugin (squash_context_get_default (), plugin);
}

static int
squash_codec_ref_compare (SquashCodecRef* a, SquashCodecRef* b) {
  return squash_codec_compare (a->codec, b->codec);
}

static int
squash_codec_ref_extension_compare (SquashCodecRef* a, SquashCodecRef* b) {
  return squash_codec_extension_compare (a->codec, b->codec);
}

static SquashPlugin*
squash_context_add_plugin (SquashContext* context, char* name, char* directory) {
  SquashPlugin* plugin = NULL;
  SquashPlugin plugin_dummy = { 0, };

  assert (context != NULL);
  assert (name != NULL);
  assert (directory != NULL);

  plugin_dummy.name = name;

  plugin = SQUASH_TREE_FIND (&(context->plugins), _SquashPlugin, tree, &plugin_dummy);
  if (plugin == NULL) {
    plugin = squash_plugin_new (name, directory, context);

    SQUASH_TREE_INSERT(&(context->plugins), _SquashPlugin, tree, plugin);
  } else {
    free (name);
    free (directory);

    plugin = NULL;
  }

  return plugin;
}

/**
 * @brief Add a reference to the codec
 * @private
 *
 * Adds a SquashCodecRef entry to the context for the given codec if
 * no other codec with the same name already has a reference.  If
 * another codec with the same name already exists and references a
 * codec with a lower priority, this will switch the reference to @a
 * codec.
 *
 * @param context The context
 * @param codec The codec
 */
void
squash_context_add_codec (SquashContext* context, SquashCodec* codec) {
  SquashCodecRef* codec_ref;

  assert (context != NULL);
  assert (codec != NULL);

  codec_ref = squash_context_get_codec_ref (context, codec->name);
  if (codec_ref == NULL) {
    /* Insert a new entry into context->codecs */
    codec_ref = (SquashCodecRef*) malloc (sizeof (SquashCodecRef));
    codec_ref->codec = codec;
    SQUASH_TREE_ENTRY_INIT(codec_ref->tree);
    SQUASH_TREE_INSERT (&(context->codecs), _SquashCodecRef, tree, codec_ref);
  } else if (codec->priority > codec_ref->codec->priority) {
    /* Switch the existing context codec's details to this one */
    codec_ref->codec = codec;
  }

  if (codec->extension != NULL) {
    codec_ref = squash_context_get_codec_ref_from_extension (context, codec->extension);
    if (codec_ref == NULL) {
      codec_ref = (SquashCodecRef*) malloc (sizeof (SquashCodecRef));
      codec_ref->codec = codec;
      SQUASH_TREE_ENTRY_INIT(codec_ref->tree);
      SQUASH_TREE_INSERT (&(context->extensions), _SquashCodecRef, tree, codec_ref);
    } else if (codec->priority > codec_ref->codec->priority) {
      codec_ref->codec = codec;
    }
  }
}

/**
 * @private
 */
typedef struct _SquashCodecsFileParser {
  SquashIniParser parser;
  SquashPlugin* plugin;
  SquashCodec* codec;
} SquashCodecsFileParser;

static int
squash_codecs_file_parser_section_begin_cb (SquashIniParser* ini_parser, const char* name, void* user_data) {
  SquashCodecsFileParser* parser = (SquashCodecsFileParser*) ini_parser;

  parser->codec = squash_codec_new (parser->plugin, name);

  return SQUASH_OK;
}

static int
squash_codecs_file_parser_section_end_cb (SquashIniParser* ini_parser, const char* name, void* user_data) {
  SquashCodecsFileParser* parser = (SquashCodecsFileParser*) ini_parser;

  squash_plugin_add_codec (parser->plugin, parser->codec);

  return SQUASH_OK;
}

static int
squash_codecs_file_parser_key_read_cb (SquashIniParser* ini_parser,
                                       const char* section,
                                       const char* key,
                                       const char* detail,
                                       const char* value,
                                       void* user_data) {
  SquashCodecsFileParser* parser = (SquashCodecsFileParser*) ini_parser;

  if (strcasecmp (key, "priority") == 0 && detail == NULL) {
    char* endptr = NULL;
    long priority = strtol (value, &endptr, 0);
    if (*endptr == '\0') {
      squash_codec_set_priority (parser->codec, (unsigned int) priority);
    }
  } else if (strcasecmp (key, "extension") == 0 && detail == NULL) {
    squash_codec_set_extension (parser->codec, value);
  }

  return SQUASH_OK;
}

static int
squash_codecs_file_parser_error_cb (SquashIniParser* ini_parser,
                                    SquashIniParserError e,
                                    int line_number,
                                    int position,
                                    const char* line,
                                    void* user_data) {
  return SQUASH_OK;
}

static void
squash_codecs_file_parser_init (SquashCodecsFileParser* parser, SquashPlugin* plugin) {
  SquashCodecsFileParser _parser = { { 0, }, plugin, NULL };

  *parser = _parser;
  squash_ini_parser_init ((SquashIniParser*) parser,
                          squash_codecs_file_parser_section_begin_cb,
                          squash_codecs_file_parser_section_end_cb,
                          squash_codecs_file_parser_key_read_cb,
                          squash_codecs_file_parser_error_cb,
                          plugin, NULL);
}

static SquashStatus
squash_codecs_file_parser_parse (SquashCodecsFileParser* parser, FILE* input) {
  return (SquashStatus) squash_ini_parser_parse ((SquashIniParser*) parser, input);
}

static void
squash_context_find_plugins_in_directory (SquashContext* context, const char* directory_name) {
  DIR* directory = opendir (directory_name);
  size_t directory_name_length = strlen (directory_name);
  struct dirent* result = NULL;
  struct dirent* entry = NULL;

  if (directory == NULL) {
    return;
  } else {
    long name_max = pathconf (directory_name, _PC_NAME_MAX);
    if (name_max == -1)
      name_max = 255;
    entry = (struct dirent*) malloc (offsetof (struct dirent, d_name) + name_max + 1);
  }

  while ( readdir_r (directory, entry, &result) == 0 && result != NULL ) {
#ifdef _DIRENT_HAVE_D_TYPE
    if ( entry->d_type != DT_DIR &&
         entry->d_type != DT_UNKNOWN &&
         entry->d_type != DT_LNK )
      continue;
#endif

    if (strcmp (entry->d_name, "..") == 0 ||
        strcmp (entry->d_name, ".") == 0)
      continue;

    size_t plugin_name_length = strlen (entry->d_name);
    char* plugin_name = entry->d_name;

    size_t codecs_file_name_length = directory_name_length + (plugin_name_length * 2) + 10;
    char* codecs_file_name = (char*) malloc (codecs_file_name_length + 1);
    snprintf (codecs_file_name, codecs_file_name_length, "%s/%s/%s.codecs",
              directory_name, plugin_name, plugin_name);

    FILE* codecs_file = fopen (codecs_file_name, "r");

    if (codecs_file != NULL) {
      size_t plugin_directory_name_length = directory_name_length + plugin_name_length + 1;
      char* plugin_directory_name = (char*) malloc (plugin_directory_name_length + 1);
      snprintf (plugin_directory_name, plugin_directory_name_length + 1, "%s/%s",
                directory_name, plugin_name);

      SquashPlugin* plugin = squash_context_add_plugin (context, strdup (plugin_name), plugin_directory_name);
      if (plugin != NULL) {
        SquashCodecsFileParser parser;

        squash_codecs_file_parser_init (&parser, plugin);
        squash_codecs_file_parser_parse (&parser, codecs_file);
      }
    }

    free (codecs_file_name);
  }

  free (entry);
  closedir (directory);
}

static void
squash_context_find_plugins (SquashContext* context) {
  char* directories;

  assert (context != NULL);

  directories = getenv ("SQUASH_PLUGINS");

  if (directories != NULL) {
    char* saveptr = NULL;
    char* directory_name = NULL;

    directories = strdup (directories);

    for ( directory_name = strtok_r (directories, ":", &saveptr) ;
          directory_name != NULL ;
          directory_name = strtok_r (NULL, ":", &saveptr) ) {
      squash_context_find_plugins_in_directory (context, directory_name);
    }

    free (directories);
  } else {
    squash_context_find_plugins_in_directory (context, SQUASH_PLUGIN_DIR);
  }
}

/**
 * @brief Execute a callback for every loaded plugin.
 *
 * @param context The context to use
 * @param func The callback to execute
 * @param data Data to pass to the callback
 */
void
squash_context_foreach_plugin (SquashContext* context, SquashPluginForeachFunc func, void* data) {
  SQUASH_TREE_FORWARD_APPLY(&(context->plugins), _SquashPlugin, tree, func, data);
}

static void
squash_context_foreach_codec_ref (SquashContext* context, void(*func)(SquashCodecRef*, void*), void* data) {
  SQUASH_TREE_FORWARD_APPLY(&(context->codecs), _SquashCodecRef, tree, func, data);
}

/**
 * @private
 */
struct SquashContextForeachCodecRefCbData {
  SquashCodecForeachFunc func;
  void* data;
};

static void
squash_context_foreach_codec_ref_cb (SquashCodecRef* codec_ref, void* data) {
  struct SquashContextForeachCodecRefCbData* cb_data = (struct SquashContextForeachCodecRefCbData*) data;

  cb_data->func (codec_ref->codec, cb_data->data);
}

/**
 * @brief Execute a callback for every loaded codec.
 *
 * @note If you have multiple plugins which supply a single codec, the
 * callback will only be invoked for the highest-priority codec.  If
 * you would like to invoke a callback even when a higher priority
 * codec exists, you can use ::squash_context_foreach_plugin to iterate
 * through all the plugins and squashl ::squash_plugin_foreach_codec on each
 * @ref SquashPlugin.
 *
 * @param context The context to use
 * @param func The callback to execute
 * @param data Data to pass to the callback
 */
void
squash_context_foreach_codec (SquashContext* context, SquashCodecForeachFunc func, void* data) {
  struct SquashContextForeachCodecRefCbData cb_data = { func, data };

  squash_context_foreach_codec_ref (context, squash_context_foreach_codec_ref_cb, &cb_data);
}

/**
 * @brief Execute a callback for every loaded plugin in the default
 *   context.
 *
 * @param func The callback to execute
 * @param data Data to pass to the callback
 */
void
squash_foreach_plugin (SquashPluginForeachFunc func, void* data) {
  squash_context_foreach_plugin (squash_context_get_default (), func, data);
}

/**
 * @brief Execute a callback for every loaded codec in the default
 *   context.
 *
 * @note If you have multiple plugins which supply a single codec, the
 * callback will only be invoked for the highest-priority codec.  If
 * you would like to invoke a callback even when a higher priority
 * codec exists, you can use ::squash_foreach_plugin to iterate through
 * all the plugins and squashl ::squash_plugin_foreach_codec on each @ref
 * SquashPlugin.
 *
 * @param func The callback to execute
 * @param data Data to pass to the callback
 */
void
squash_foreach_codec (SquashCodecForeachFunc func, void* data) {
  squash_context_foreach_codec (squash_context_get_default (), func, data);
}

static SquashContext*
squash_context_new (void) {
  SquashContext* context = (SquashContext*) malloc (sizeof (SquashContext));
  SquashContext _context = { { 0, }, };

  assert (context != NULL);

  *context = _context;
  SQUASH_TREE_INIT(&(context->codecs), squash_codec_ref_compare);
  SQUASH_TREE_INIT(&(context->plugins), squash_plugin_compare);
  SQUASH_TREE_INIT(&(context->extensions), squash_codec_ref_extension_compare);

  squash_context_find_plugins (context);

  return context;
}

#if HAVE_PTHREAD
static SquashContext* squash_context_default = NULL;

static void
squash_context_create_default (void) {
  squash_context_default = (SquashContext*) squash_context_new ();
}

SquashContext*
squash_context_get_default (void) {
  static pthread_once_t once = PTHREAD_ONCE_INIT;
  int once_res = -1;

  if (once_res != 0) {
    once_res = pthread_once (&once, squash_context_create_default);
  }

  assert (once_res == 0);
  assert (squash_context_default != NULL);

  return squash_context_default;
}
#elif HAVE___SYNC_BOOL_COMPARE_AND_SWAP
/* I don't think this is likely to be a major point of contention, but
   it would be nice to provide a non-spinning implementation for
   Windows (and anyone else who doesn't have pthreads, though I guess
   that's probably a pretty small pool. */

SquashContext*
squash_context_get_default (void) {
  static volatile SquashContext* squash_context_default;
  static volatile unsigned int status = 0;

  if (status == 0) {
    if (__sync_bool_compare_and_swap (&status, 0, 1)) {
      squash_context_default = (SquashContext*) squash_context_new ();
      status = 2;
      __sync_synchronize ();
    }
  }

  while (status != 2) { /* Spin */ }

  return squash_context_default;
}
#else

/* A thread-safe implementation for anyone who hits this would be
   greatly appreciated. */
#warning squash_context_get_default() will not be thread-safe.

/**
 * @brief Retrieve the default @ref SquashContext.
 *
 * If this is the first time squashling this function, a new @ref
 * SquashContext will be created and Squash will attempt to scan the plugin
 * directories for information.
 *
 * @return The @ref SquashContext.  Note that this is owned by Squash and
 *   must never be freed or unreffed.
 */
SquashContext*
squash_context_get_default (void) {
  static SquashContext* squash_context_default = NULL;

  if (squash_context_default == NULL) {
    squash_context_default = (SquashContext*) squash_context_new ();
  }
  return squash_context_default;
}
#endif

/**
 * @}
 */
