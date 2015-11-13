#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#if defined(_WIN32)
#include <io.h>
#endif

#if !defined(_MSC_VER)
#include <unistd.h>
#include <strings.h>
#else
#define strcasecmp _stricmp
#define snprintf _snprintf
#endif

#include "parg/parg.h"

#if !defined(EXIT_SUCCESS)
#define EXIT_SUCCESS (0)
#endif

#if !defined(EXIT_FAILURE)
#define EXIT_FAILURE (-1)
#endif

#include <squash/squash.h>

#if defined(__GNUC__)
__attribute__((__noreturn__))
#endif
static void
print_help_and_exit (int argc, char** argv, int exit_code) {
  fprintf (stderr, "Usage: %s [OPTION]... INPUT [OUTPUT]\n", argv[0]);
  fprintf (stderr, "Compress and decompress files.\n");
  fprintf (stderr, "\n");
  fprintf (stderr, "Options:\n");
  fprintf (stderr, "\t-k, --keep              Keep input file when finished.\n");
  fprintf (stderr, "\t-o, --option key=value  Pass the option to the encoder/decoder.\n");
  fprintf (stderr, "\t-1 .. -9                Pass the compression level to the encoder.\n");
  fprintf (stderr, "\t                        Equivalent to -o level=N\n");
  fprintf (stderr, "\t-c, --codec codec       Use the specified codec.  By default squash will\n");
  fprintf (stderr, "\t                        attempt to guess it based on the extension.\n");
  fprintf (stderr, "\t-L, --list-codecs       List available codecs and exit\n");
  fprintf (stderr, "\t-P, --list-plugins      List available plugins and exit\n");
  fprintf (stderr, "\t-f, --force             Overwrite the output file if it exists.\n");
  fprintf (stderr, "\t-d, --decompress        Decompress\n");
  fprintf (stderr, "\t-V, --version           Print version number and exit\n");
  fprintf (stderr, "\t-h, --help              Print this help screen and exit.\n");

  exit (exit_code);
}

#if defined(__GNUC__)
__attribute__((__noreturn__))
#endif
static void
print_version_and_exit (int argc, char** argv, int exit_code) {
  const unsigned int libversion = squash_version ();
  fprintf (stdout, "squash version %d.%d.%d (library version %d.%d.%d)\n",
           SQUASH_VERSION_MAJOR, SQUASH_VERSION_MINOR, SQUASH_VERSION_REVISION,
           SQUASH_VERSION_EXTRACT_MAJOR(libversion),
           SQUASH_VERSION_EXTRACT_MINOR(libversion),
           SQUASH_VERSION_EXTRACT_REVISION(libversion));

  exit (exit_code);
}

static void
parse_option (char*** keys, char*** values, const char* option) {
  char* key;
  char* value;
  size_t length = 0;

  key = strdup (option);
  value = strchr (key, '=');
  if (value == NULL) {
    fprintf (stderr, "Invalid option (\"%s\").", option);
    exit (EXIT_FAILURE);
  }
  *value = '\0';
  value++;

  if (*keys != NULL) {
    while ( (*keys)[length] != NULL ) {
      if ( strcmp ((*keys)[length], key) == 0 ) {
        free ((*values)[length]);
        (*values)[length] = strdup (value);
        free (key);
        return;
      }
      length++;
    }
  }

  *keys = (char**) realloc (*keys, sizeof (char*) * (length + 2));
  *values = (char**) realloc (*values, sizeof (char*) * (length + 2));

  (*keys)[length] = key;
  (*values)[length] = strdup (value);
  (*keys)[length + 1] = NULL;
  (*values)[length + 1] = NULL;
}

static void
list_codecs_foreach_cb (SquashCodec* codec, void* data) {
  if (data != NULL)
    fputs ((char*) data, stdout);
  fputs (squash_codec_get_name (codec), stdout);
  fputc ('\n', stdout);
}

static void
list_plugins_foreach_cb (SquashPlugin* plugin, void* data) {
  fputs (squash_plugin_get_name (plugin), stdout);
  fputc ('\n', stdout);
}

static void
list_plugins_and_codecs_foreach_cb (SquashPlugin* plugin, void* data) {
  const char* indent = "\t";
  list_plugins_foreach_cb (plugin, data);
  squash_plugin_foreach_codec (plugin, list_codecs_foreach_cb, (void*) indent);
}

#if !defined(_WIN32)
#define squash_strndup(s,n) strndup(s,n)
#else
static char* squash_strndup(const char* s, size_t n);

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

int main (int argc, char** argv) {
  SquashStatus res;
  SquashCodec* codec = NULL;
  SquashOptions* options = NULL;
  SquashStreamType direction = SQUASH_STREAM_COMPRESS;
  FILE* input = NULL;
  FILE* output = NULL;
  char* input_name = NULL;
  char* output_name = NULL;
  bool list_codecs = false;
  bool list_plugins = false;
  char** option_keys = NULL;
  char** option_values = NULL;
  bool keep = false;
  bool force = false;
  int opt;
  int optc = 0;
  char* tmp_string;
  int retval = EXIT_SUCCESS;
  struct parg_state ps;
  int optend;
  const struct parg_option squash_options[] = {
    {"keep", PARG_NOARG, NULL, 'k'},
    {"option", PARG_REQARG, NULL, 'o'},
    {"codec", PARG_REQARG, NULL, 'c'},
    {"list-codecs", PARG_NOARG, NULL, 'L'},
    {"list-plugins", PARG_NOARG, NULL, 'P'},
    {"force", PARG_NOARG, NULL, 'f'},
    {"decompress", PARG_NOARG, NULL, 'd'},
    {"version", PARG_NOARG, NULL, 'V'},
    {"help", PARG_NOARG, NULL, 'h'},
    {NULL, 0, NULL, 0}
  };

  option_keys = (char**) malloc (sizeof (char*));
  option_values = (char**) malloc (sizeof (char*));
  *option_keys = NULL;
  *option_values = NULL;

  optend = parg_reorder (argc, argv, "c:ko:123456789LPfdhb:V", squash_options);

  parg_init(&ps);

  while ( (opt = parg_getopt_long (&ps, optend, argv, "c:ko:123456789LPfdhb:V", squash_options, NULL)) != -1 ) {
    switch ( opt ) {
      case 'c':
        codec = squash_get_codec (ps.optarg);
        if ( codec == NULL ) {
          fprintf (stderr, "Unable to find codec '%s'\n", ps.optarg);
          retval = EXIT_FAILURE;
          goto cleanup;
        }
        break;
      case 'k':
        keep = true;
        break;
      case 'o':
        parse_option (&option_keys, &option_values, ps.optarg);
        break;
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        tmp_string = malloc (8);
        snprintf (tmp_string, 8, "level=%c", (char) opt);
        parse_option (&option_keys, &option_values, tmp_string);
        free (tmp_string);
        break;
      case 'L':
        list_codecs = true;
        break;
      case 'P':
        list_plugins = true;
        break;
      case 'f':
        force = true;
        break;
      case 'h':
        print_help_and_exit (argc, argv, EXIT_SUCCESS);
        break;
      case 'd':
        direction = SQUASH_STREAM_DECOMPRESS;
        break;
      case 'V':
        print_version_and_exit (argc, argv, EXIT_SUCCESS);
        break;
    }

    optc++;
  }

  if (list_plugins) {
    if (list_codecs)
      squash_foreach_plugin (list_plugins_and_codecs_foreach_cb, NULL);
    else
      squash_foreach_plugin (list_plugins_foreach_cb, NULL);

    goto cleanup;
  } else if (list_codecs) {
    squash_foreach_codec (list_codecs_foreach_cb, NULL);
    goto cleanup;
  }

  if ( ps.optind < argc ) {
    input_name = argv[ps.optind++];

    if ( (direction == SQUASH_STREAM_DECOMPRESS) && codec == NULL ) {
      char* extension;

      extension = strrchr (input_name, '.');
      if (extension != NULL)
        extension++;

      if (extension != NULL)
        codec = squash_get_codec_from_extension (extension);
    }
  } else {
    fprintf (stderr, "You must provide an input file name.\n");
    retval = EXIT_FAILURE;
    goto cleanup;
  }

  if ( ps.optind < argc ) {
    output_name = strdup (argv[ps.optind++]);

    if ( codec == NULL && direction == SQUASH_STREAM_COMPRESS ) {
      const char* extension = strrchr (output_name, '.');
      if (extension != NULL)
        extension++;

      if (extension != NULL)
        codec = squash_get_codec_from_extension (extension);
    }
  } else {
    if ( codec != NULL ) {
      const char* extension = squash_codec_get_extension (codec);
      if (extension != NULL) {
        if (strcmp (input_name, "-") == 0) {
          output_name = strdup ("-");
        } else {
          size_t extension_length = strlen (extension);
          size_t input_name_length = strlen (input_name);

          if ( (extension_length + 1) < input_name_length &&
               input_name[input_name_length - (1 + extension_length)] == '.' &&
               strcasecmp (extension, input_name + (input_name_length - (extension_length))) == 0 ) {
            output_name = squash_strndup (input_name, input_name_length - (1 + extension_length));
          }
        }
      }
    }
  }

  if ( ps.optind < argc ) {
    fprintf (stderr, "Too many arguments.\n");
  }

  if ( codec == NULL ) {
    fprintf (stderr, "Unable to determine codec.  Please pass -c \"codec\", or -L to see a list of available codecs.\n");
    retval = EXIT_FAILURE;
    goto cleanup;
  }

  if ( output_name == NULL ) {
    fprintf (stderr, "Unable to determine output file.\n");
    retval = EXIT_FAILURE;
    goto cleanup;
  }

  if ( strcmp (input_name, "-") == 0 ) {
    input = stdin;
  } else {
    input = fopen (input_name, "rb");
    if ( input == NULL ) {
      perror ("Unable to open input file");
      retval = EXIT_FAILURE;
      goto cleanup;
    }
  }

  if ( strcmp (output_name, "-") == 0 ) {
    output = stdout;
  } else {
    int output_fd = open (output_name,
#if !defined(_WIN32)
                          O_RDWR | O_CREAT | (force ? O_TRUNC : O_EXCL),
                          S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH
#else
                          O_RDWR | O_CREAT | (force ? O_TRUNC : O_EXCL) | O_BINARY,
                          S_IREAD | S_IWRITE
#endif
    );
    if ( output_fd < 0 ) {
      perror ("Unable to open output file");
      retval = EXIT_FAILURE;
      goto cleanup;
    }
    output = fdopen (output_fd, "wb");
    if ( output == NULL ) {
      perror ("Unable to open output");
      retval = EXIT_FAILURE;
      goto cleanup;
    }
  }

  options = squash_options_newa (codec, (const char * const*) option_keys, (const char * const*) option_values);

  res = squash_splice_with_options (codec, direction, output, input, 0, options);

  if ( res != SQUASH_OK ) {
    fprintf (stderr, "Failed to %s: %s\n",
             (direction == SQUASH_STREAM_COMPRESS) ? "compress" : "decompress",
             squash_status_to_string (res));
    retval = EXIT_FAILURE;
    goto cleanup;
  }

  if ( !keep && input != stdin ) {
    fclose (input);
    if ( unlink (input_name) != 0 ) {
      perror ("Unable to remove input file");
    }
    input = NULL;
  }

 cleanup:

  if (input != stdin && input != NULL)
    fclose (stdin);

  if (output != stdout)
    fclose (stdout);

  if (option_keys != NULL) {
    for (opt = 0 ; option_keys[opt] != NULL ; opt++) {
      free(option_keys[opt]);
    }
    free (option_keys);
  }

  if (option_values != NULL) {
    for (opt = 0 ; option_values[opt] != NULL ; opt++) {
      free(option_values[opt]);
    }
    free (option_values);
  }

  free (output_name);

  return retval;
}
