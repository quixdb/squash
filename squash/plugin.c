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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#if !defined(_WIN32)
#include <dlfcn.h>
#else
#include <windows.h>
#endif

#include "squash.h"
#include "internal.h"

/**
 * @defgroup SquashPlugin SquashPlugin
 * @brief A plugin.
 *
 * @{
 */

/**
 * @struct _SquashPlugin
 * @brief A plugin.
 */

/**
 * @typedef SquashPluginForeachFunc
 * @brief Squashlback to be invoked on each @ref SquashPlugin in a set.
 *
 * @param plugin A plugin
 * @param data User-supplied data
 */

/**
 * @brief Add a new %SquashCodec
 * @private
 *
 * @param plugin The plugin to add the codec to.
 * @param name The codec name (transfer full).
 * @param priority The codec priority.
 */
void
squash_plugin_add_codec (SquashPlugin* plugin, SquashCodec* codec) {
  SquashContext* context;

  assert (plugin != NULL);
  assert (codec != NULL);

  context = plugin->context;

  /* Insert a new entry into plugin->codecs */
  SQUASH_TREE_INSERT(&(plugin->codecs), _SquashCodec, tree, codec);

  squash_context_add_codec (context, codec);
}

SQUASH_MTX_DEFINE(plugin_init)

/**
 * @brief load a %SquashPlugin
 *
 * @note This function is generally only useful inside of a callback
 * passed to ::squash_foreach_plugin.  Every other way to get a plugin
 * (such as ::squash_get_plugin) will initialize the plugin as well
 * (and return *NULL* instead of the plugin if initialization fails).
 * The foreach functions, however, do not initialize the plugin since
 * doing so requires actually loading the plugin.
 *
 * @param plugin The plugin to load.
 * @return A status code.
 * @retval SQUASH_OK The plugin has been loaded.
 * @retval SQUASH_UNABLE_TO_LOAD Unable to load plugin.
 */
SquashStatus
squash_plugin_init (SquashPlugin* plugin) {
  if (plugin->plugin == NULL) {
#if !defined(_WIN32)
    void* handle;
#else
    HMODULE handle;
#endif
    char* plugin_file_name;
    size_t plugin_dir_length;
    size_t plugin_name_length;
    size_t plugin_file_name_max_length;
    size_t squash_version_api_length = strlen (SQUASH_VERSION_API);

    plugin_dir_length = strlen (plugin->directory);
    plugin_name_length = strlen (plugin->name);
    plugin_file_name_max_length = plugin_dir_length + squash_version_api_length + plugin_name_length + 19 + strlen (SQUASH_SHARED_LIBRARY_SUFFIX);
    plugin_file_name = (char*) malloc (plugin_file_name_max_length + 1);

    snprintf (plugin_file_name, plugin_file_name_max_length + 1,
              "%s/libsquash%s-plugin-%s%s", plugin->directory, SQUASH_VERSION_API, plugin->name, SQUASH_SHARED_LIBRARY_SUFFIX);

#if !defined(_WIN32)
    handle = dlopen (plugin_file_name, RTLD_LAZY);
#else
    handle = LoadLibrary (TEXT(plugin_file_name));
#endif

    if (handle != NULL) {
      SQUASH_MTX_LOCK(plugin_init);
      if (plugin->plugin == NULL) {
        plugin->plugin = handle;
        handle = NULL;
      }
      SQUASH_MTX_UNLOCK(plugin_init);
    } else {
      return SQUASH_UNABLE_TO_LOAD;
    }

    if (handle != NULL) {
#if !defined(_WIN32)
      dlclose (handle);
#else
      FreeLibrary (handle);
#endif
    } else {
      SquashStatus (*init_func) (SquashPlugin*);
#if !defined(_WIN32)
      *(void **) (&init_func) = dlsym (plugin->plugin, "squash_plugin_init");
#else
      *(void **) (&init_func) = GetProcAddress (handle, "squash_plugin_init");
#endif
      if (init_func != NULL) {
        init_func (plugin);
      }
    }

    free (plugin_file_name);
  }

  return (plugin->plugin != NULL) ? SQUASH_OK : SQUASH_UNABLE_TO_LOAD;
}

/**
 * @brief Get the name of a plugin.
 *
 * @param plugin The plugin.
 * @return The name.
 */
const char*
squash_plugin_get_name (SquashPlugin* plugin) {
  assert (plugin != NULL);

  return plugin->name;
}

/**
 * @brief Get a codec from a plugin by name.
 *
 * @param plugin The plugin.
 * @param codec The codec name.
 * @return The codec, or *NULL* if it could not be found.
 */
SquashCodec*
squash_plugin_get_codec (SquashPlugin* plugin, const char* codec) {
  SquashCodec key = { 0, };
  SquashCodec* codec_real;

  assert (plugin != NULL);
  assert (codec != NULL);

  key.name = (char*) codec;

  codec_real = SQUASH_TREE_FIND (&(plugin->codecs), _SquashCodec, tree, &key);
  return (squash_codec_init (codec_real) == SQUASH_OK) ? codec_real : NULL;
}

/**
 * @brief Compare two plugins.
 * @private
 *
 * This is used for sorting the plugin tree.
 *
 * @param a A plugin.
 * @param b Another plugin.
 * @returns The comparison result.
 */
int
squash_plugin_compare (SquashPlugin* a, SquashPlugin* b) {
  assert (a != NULL);
  assert (b != NULL);

  return strcmp (a->name, b->name);
}

SQUASH_MTX_DEFINE(codec_init)

/**
 * @brief Initialize a codec
 * @private
 *
 * @param plugin The plugin.
 * @param codec The codec to initialize.
 * @param funcs The function table to fill.
 * @returns A status code.
 */
SquashStatus
squash_plugin_init_codec (SquashPlugin* plugin, SquashCodec* codec, SquashCodecFuncs* funcs) {
  SquashStatus res = SQUASH_OK;

  assert (plugin != NULL);

  if (plugin->plugin == NULL) {
    res = squash_plugin_init (plugin);
    if (res != SQUASH_OK) {
      return res;
    }
  }

  if (codec->initialized == 0) {
    SquashStatus (*init_codec_func) (SquashCodec*, SquashCodecFuncs*);

#if !defined(_WIN32)
    *(void **) (&init_codec_func) = dlsym (plugin->plugin, "squash_plugin_init_codec");
#else
    *(void **) (&init_codec_func) = GetProcAddress (plugin->plugin, "squash_plugin_init_codec");
#endif

    if (init_codec_func == NULL) {
      return SQUASH_UNABLE_TO_LOAD;
    }

    SQUASH_MTX_LOCK(codec_init);
    res = init_codec_func (codec, funcs);
    codec->initialized = (res == SQUASH_OK);
    SQUASH_MTX_UNLOCK(codec_init);
  }

  return res;
}

/**
 * @brief Execute a callback for every codec in the plugin.
 *
 * @note @a func will be invoked for all codecs supplied by this
 * plugin, even if a higher-priority implementation exists in another
 * plugin.  If you only want to list the codecs which supply the
 * highest-priority implementations available, you can use
 * ::squash_foreach_codec.  If not jumping around the hierarchy is
 * important, you can test to see if a codec provides the highest
 * priority implementation by comparing the codec to the return value
 * of ::squash_get_codec.
 *
 * @param plugin The plugin
 * @param func The callback to execute
 * @param data Data to pass to the callback
 */
void
squash_plugin_foreach_codec (SquashPlugin* plugin, SquashCodecForeachFunc func, void* data) {
  SQUASH_TREE_FORWARD_APPLY(&(plugin->codecs), _SquashCodec, tree, func, data);
}

/**
 * @brief Create a new plugin
 * @private
 *
 * @param name Plugin name.
 * @param directory Directory where the plugin is located.
 * @param context Context for the plugin.
 */
SquashPlugin*
squash_plugin_new (char* name, char* directory, SquashContext* context) {
  SquashPlugin* plugin = (SquashPlugin*) malloc (sizeof (SquashPlugin));

  plugin->name = name;
  plugin->context = context;
  plugin->directory = directory;
  plugin->plugin = NULL;
  SQUASH_TREE_ENTRY_INIT(plugin->tree);
  SQUASH_TREE_INIT(&(plugin->codecs), squash_codec_compare);

  return plugin;
}

/**
 * @}
 */
