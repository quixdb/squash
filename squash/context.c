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
 *   Evan Nemerson <evan@nemerson.com>
 */

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE
#define _POSIX_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <squash/internal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(_MSC_VER)
#include <strings.h>
#include <unistd.h>
#endif

#if !defined(_WIN32)
  #include <dirent.h>
#else
  #include <tchar.h>
  #include <strsafe.h>
  #include <windows.h>
#endif

#include "tinycthread/source/tinycthread.h"

#if !defined(_WIN32)
#define SQUASH_STRTOK_R(str,delim,saveptr) strtok_r(str,delim,saveptr)
#define squash_strndup(s,n) strndup(s,n)
#else
static char* squash_strndup(const char* s, size_t n);
#define SQUASH_STRTOK_R(str,delim,saveptr) strtok_s(str,delim,saveptr)
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
 * @struct SquashContext_
 * @brief Context for all Squash operations.
 */

static SquashCodecRef*
squash_context_get_codec_ref (SquashContext* context, const char* codec) {
  SquashCodec key_codec = { 0, };
  SquashCodecRef key = { &key_codec, };

  assert (context != NULL);
  assert (codec != NULL);

  key_codec.name = (char*) codec;

  return SQUASH_TREE_FIND (&(context->codecs), SquashCodecRef_, tree, &key);
}

static SquashCodecRef*
squash_context_get_codec_ref_from_extension (SquashContext* context, const char* extension) {
  SquashCodec key_codec = { 0, };
  SquashCodecRef key = { &key_codec, };

  assert (context != NULL);
  assert (extension != NULL);

  key_codec.extension = (char*) extension;

  return SQUASH_TREE_FIND (&(context->extensions), SquashCodecRef_, tree, &key);
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
  const char* sep_pos = strchr (codec, ':');
  if (sep_pos != NULL) {
    char* plugin_name = (char*) malloc ((sep_pos - codec) + 1);

    strncpy (plugin_name, codec, sep_pos - codec);
    plugin_name[sep_pos - codec] = 0;
    codec = sep_pos + 1;

    SquashPlugin* plugin = squash_context_get_plugin (context, plugin_name);
    free (plugin_name);
    if (plugin == NULL)
      return NULL;

    return squash_plugin_get_codec (plugin, codec);
  } else {
    SquashCodecRef* codec_ref = squash_context_get_codec_ref (context, codec);
    if (codec_ref != NULL) {
      /* TODO: we should probably see if we can load the codec from a
         different plugin if this fails.  */
      return (squash_codec_init (codec_ref->codec) == SQUASH_OK) ? codec_ref->codec : NULL;
    } else {
      return NULL;
    }
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

  return SQUASH_TREE_FIND (&(context->plugins), SquashPlugin_, tree, &plugin_dummy);
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

  plugin = SQUASH_TREE_FIND (&(context->plugins), SquashPlugin_, tree, &plugin_dummy);
  if (plugin == NULL) {
    plugin = squash_plugin_new (name, directory, context);

    SQUASH_TREE_INSERT(&(context->plugins), SquashPlugin_, tree, plugin);
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
    SQUASH_TREE_INSERT (&(context->codecs), SquashCodecRef_, tree, codec_ref);
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
      SQUASH_TREE_INSERT (&(context->extensions), SquashCodecRef_, tree, codec_ref);
    } else if (codec->priority > codec_ref->codec->priority) {
      codec_ref->codec = codec;
    }
  }
}

/**
 * @private
 */
typedef struct SquashCodecsFileParser_ {
  SquashPlugin* plugin;
  SquashCodec* codec;
} SquashCodecsFileParser;

static bool
squash_codecs_file_parser_callback (const char* section,
                                    const char* key,
                                    const char* value,
                                    size_t value_length,
                                    void* user_data) {
  SquashCodecsFileParser* parser = (SquashCodecsFileParser*) user_data;

  if (key == NULL) {
    if (parser->codec != NULL)
      squash_plugin_add_codec (parser->plugin, parser->codec);
    parser->codec = squash_codec_new (parser->plugin, section);
  } else {
    if (strcasecmp (key, "license") == 0) {
      size_t n = 0;
      if (parser->plugin->license != NULL) {
        free (parser->plugin->license);
        parser->plugin->license = NULL;
      }

      char* licenses = strdup (value);
      char* saveptr = NULL;
      char* license = strtok_r (licenses, ";", &saveptr);

      while (license != NULL) {
        SquashLicense license_value = squash_license_from_string (license);
        if (license_value != SQUASH_LICENSE_UNKNOWN) {
          parser->plugin->license = realloc (parser->plugin->license, sizeof (SquashLicense) * (n + 2));
          parser->plugin->license[n++] = squash_license_from_string (license);
          parser->plugin->license[n] = SQUASH_LICENSE_UNKNOWN;

          n++;
        }

        license = strtok_r (NULL, ";", &saveptr);
      };

      free (licenses);
    } else if (strcasecmp (key, "priority") == 0) {
      char* endptr = NULL;
      long priority = strtol (value, &endptr, 0);
      if (*endptr == '\0') {
        squash_codec_set_priority (parser->codec, (unsigned int) priority);
      }
    } else if (strcasecmp (key, "extension") == 0) {
      squash_codec_set_extension (parser->codec, value);
    }
  }

  return true;
}

static void
squash_codecs_file_parser_init (SquashCodecsFileParser* parser, SquashPlugin* plugin) {
  SquashCodecsFileParser _parser = { plugin, NULL };

  *parser = _parser;
}

static SquashStatus
squash_codecs_file_parser_parse (SquashCodecsFileParser* parser, FILE* input) {
  bool res = squash_ini_parse (input, squash_codecs_file_parser_callback, parser);

  if (SQUASH_LIKELY(res && parser->codec != NULL)) {
    squash_plugin_add_codec (parser->plugin, parser->codec);
    return SQUASH_OK;
  } else {
    return squash_error (SQUASH_FAILED);
  }
}

#if defined(_WIN32)
static char*
squash_strndup(const char* s, size_t n) {
	const char* eos = (const char*) memchr (s, '\0', n);
	const size_t res_len = (eos == NULL) ? n : (size_t) (eos - s);
  char* res = (char*) malloc (res_len + 1);
	memcpy (res, s, res_len);
	res[res_len] = '\0';

	return res;
}
#endif

static void
squash_context_check_directory_for_plugin (SquashContext* context, const char* directory_name, const char* plugin_name) {
  size_t directory_name_size = strlen (directory_name);
  size_t plugin_name_size = strlen (plugin_name);

  size_t codecs_file_name_size = directory_name_size + (plugin_name_size * 2) + 10;
  char* codecs_file_name = (char*) malloc (codecs_file_name_size + 1);
  snprintf (codecs_file_name, codecs_file_name_size, "%s/%s/squash.ini",
            directory_name, plugin_name);

  FILE* codecs_file = fopen (codecs_file_name, "r");

  if (codecs_file != NULL) {
    size_t plugin_directory_name_size = directory_name_size + plugin_name_size + 1;
    char* plugin_directory_name = (char*) malloc (plugin_directory_name_size + 1);
    snprintf (plugin_directory_name, plugin_directory_name_size + 1, "%s/%s",
              directory_name, plugin_name);

    SquashPlugin* plugin = squash_context_add_plugin (context, squash_strndup (plugin_name, 32), plugin_directory_name);
    if (plugin != NULL) {
      SquashCodecsFileParser parser;

      squash_codecs_file_parser_init (&parser, plugin);
      squash_codecs_file_parser_parse (&parser, codecs_file);
    }

    fclose (codecs_file);
  }

  free (codecs_file_name);
}

static void
squash_context_find_plugins_in_directory (SquashContext* context, const char* directory_name) {
#if !defined(_WIN32)
  DIR* directory = opendir (directory_name);
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

    squash_context_check_directory_for_plugin (context, directory_name, entry->d_name);
  }

  free (entry);
  closedir (directory);
#else
  WIN32_FIND_DATA entry;
  TCHAR* directory_query = NULL;
  size_t directory_query_size;
  HANDLE directory_handle = INVALID_HANDLE_VALUE;

  directory_query_size = strlen (directory_name) + 3;

  if (directory_query_size > MAX_PATH) {
    return;
  }

  directory_query = (TCHAR*) malloc (directory_query_size * sizeof(TCHAR));

  StringCchCopy (directory_query, directory_query_size, directory_name);
  StringCchCat (directory_query, directory_query_size, TEXT("\\*"));

  directory_handle = FindFirstFile (directory_query, &entry);

  if (INVALID_HANDLE_VALUE == directory_handle) {
    return;
  }

  do {
    if (entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      if (strcmp (entry.cFileName, "..") == 0 ||
          strcmp (entry.cFileName, ".") == 0)
        continue;

      squash_context_check_directory_for_plugin (context, directory_name, entry.cFileName);
    }
  }
  while (FindNextFile (directory_handle, &entry) != 0);

  FindClose(directory_handle);
  free (directory_query);
#endif /* defined(_WIN32) */
}

#if !defined(SQUASH_SEARCH_PATH_SEPARATOR)
#  if !defined(_WIN32)
#    define SQUASH_SEARCH_PATH_SEPARATOR ':'
#  else
#    define SQUASH_SEARCH_PATH_SEPARATOR ';'
#  endif
#endif

static void
squash_context_find_plugins (SquashContext* context) {
  const char* directories;

  assert (context != NULL);

#if defined(HAVE_SECURE_GETENV)
  directories = secure_getenv ("SQUASH_PLUGINS");
#else
  directories = getenv ("SQUASH_PLUGINS");
#endif
  if (directories == NULL)
    directories = SQUASH_SEARCH_PATH;

  SquashBuffer* sb = squash_buffer_new (32);
  bool quoted = false;
  bool escaped = false;

  for (const char* p = directories ; *p != 0x00 ; p++) {
    if (escaped) {
      squash_buffer_append_c (sb, *p);
      escaped = false;
    } else if (quoted) {
      switch (*p) {
        case '"':
          quoted = false;
          break;
        case '\\':
          escaped = true;
          break;
        default:
          squash_buffer_append_c (sb, *p);
          break;
      }
    } else {
      switch (*p) {
        case SQUASH_SEARCH_PATH_SEPARATOR:
          if (sb->size != 0) {
            squash_buffer_append_c (sb, 0);
            squash_context_find_plugins_in_directory (context, (char*) sb->data);
            squash_buffer_clear (sb);
          }
          break;
        case '\\':
          escaped = true;
          break;
        case '"':
          quoted = true;
          break;
        default:
          squash_buffer_append_c (sb, *p);
          break;
      }
    }
  }

  squash_buffer_append_c (sb, 0);
  squash_context_find_plugins_in_directory (context, (char*) sb->data);

  squash_buffer_free (sb);
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
  SQUASH_TREE_FORWARD_APPLY(&(context->plugins), SquashPlugin_, tree, func, data);
}

static void
squash_context_foreach_codec_ref (SquashContext* context, void(*func)(SquashCodecRef*, void*), void* data) {
  SQUASH_TREE_FORWARD_APPLY(&(context->codecs), SquashCodecRef_, tree, func, data);
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

static SquashContext* squash_context_default = NULL;

static void
squash_context_create_default (void) {
  assert (squash_context_default == NULL);

  squash_context_default = (SquashContext*) squash_context_new ();
}

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
  static once_flag once = ONCE_FLAG_INIT;

  call_once (&once, squash_context_create_default);

  assert (squash_context_default != NULL);

  return squash_context_default;
}

/**
 * @}
 */
