#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <squash/squash.h>

static void
print_help_and_exit (int argc, char** argv, int exit_code) {
  fprintf (stderr, "Usage: %s [OPTION]... INPUT OUTPUT\n", argv[0]);
  fprintf (stderr, "Compress and decompress files.\n");
  fprintf (stderr, "\n");
  fprintf (stderr, "Options:\n");
  fprintf (stderr, "\t-k            Keep input file when finished.\n");
  fprintf (stderr, "\t-o key=value  Pass the option to the encoder/decoder.\n");
  fprintf (stderr, "\t-1 .. -9      Pass the compression level to the encoder.\n");
  fprintf (stderr, "\t              Equivalent to -o level=N\n");
  fprintf (stderr, "\t-e codec      Use the specified codec.  By default squash will\n");
  fprintf (stderr, "\t              attempt to guess it based on the extension.\n");
  fprintf (stderr, "\t-L            List available codecs and exit\n");
  fprintf (stderr, "\t-P            List available plugins and exit\n");
  fprintf (stderr, "\t-f            Overwrite the output file if it exists.\n");
  fprintf (stderr, "\t-d            Decompress\n");
  fprintf (stderr, "\t-h            Print this help screen and exit.\n");

  exit (exit_code);
}

static void
parse_option (char*** keys, char*** values, const char* option) {
  char* key;
  char* value;
  size_t length = 0;

  key = strdup (option);
  value = index (key, '=');
  if (value == NULL) {
    fprintf (stderr, "Invalid option (\"%s\").", option);
    exit (-1);
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
  char* indent = "\t";
  list_plugins_foreach_cb (plugin, data);
  squash_plugin_foreach_codec (plugin, list_codecs_foreach_cb, (void*) indent);
}

static SquashCodec*
codec_from_extension (const char* extension) {
  if (strcasecmp (extension, "bz2") == 0) {
    return squash_get_codec ("bzip2");
  } else if (strcasecmp (extension, "gz") == 0) {
    return squash_get_codec ("gzip");
  } else if (strcasecmp (extension, "xz") == 0) {
    return squash_get_codec ("xz");
  }

  return NULL;
}

static const char*
extension_from_codec (SquashCodec* codec) {
  const char* name = squash_codec_get_name (codec);

  if (strcmp (name, "bzip2") == 0) {
    return "bz2";
  } else if (strcmp (name, "gzip") == 0) {
    return "gz";
  } else if (strcmp (name, "xz") == 0) {
    return "xz";
  }

  return NULL;
}

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
  bool force = true;
  int opt;
  int optc;
  char* tmp_string;

  option_keys = (char**) malloc (sizeof (char*));
  option_values = (char**) malloc (sizeof (char*));
  *option_keys = NULL;
  *option_values = NULL;

  while ( (opt = getopt(argc, argv, "c:ko:123456789LPfdhb:")) != -1 ) {
    switch ( opt ) {
      case 'c':
        codec = squash_get_codec (optarg);
        if ( codec == NULL ) {
          fprintf (stderr, "Unable to find codec '%s'\n", optarg);
          return -1;
        }
        break;
      case 'k':
        keep = true;
        break;
      case 'o':
        parse_option (&option_keys, &option_values, optarg);
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
        print_help_and_exit (argc, argv, 0);
        break;
      case 'd':
        direction = SQUASH_STREAM_DECOMPRESS;
        break;
      default:
        fprintf (stderr, "%c: %s\n", (char) opt, optarg);
        break;
    }

    optc++;
  }

  if (list_plugins) {
    if (list_codecs)
      squash_foreach_plugin (list_plugins_and_codecs_foreach_cb, NULL);
    else
      squash_foreach_plugin (list_plugins_foreach_cb, NULL);

    return 0;
  } else if (list_codecs) {
    squash_foreach_codec (list_codecs_foreach_cb, NULL);
    return 0;
  }

  if ( optind < argc ) {
    input_name = argv[optind++];

    if ( (direction == SQUASH_STREAM_DECOMPRESS) && codec == NULL ) {
      char* extension;

      extension = rindex (input_name, '.');
      if (extension != NULL)
        extension++;

      codec = codec_from_extension (extension);
    }
  } else {
    fprintf (stderr, "You must provide an input file name.\n");
    return -1;
  }

  if ( optind < argc ) {
    output_name = argv[optind++];

    if ( codec == NULL && direction == SQUASH_STREAM_COMPRESS ) {
      const char* extension = rindex (output_name, '.');
      if (extension != NULL)
        extension++;

      codec = codec_from_extension (extension);
    }
  } else {
    if ( codec != NULL ) {
      const char* extension = extension_from_codec (codec);
      if (extension != NULL) {
        size_t extension_length = strlen (extension);
        size_t input_name_length = strlen (input_name);

        if ( (extension_length + 1) < input_name_length &&
             input_name[input_name_length - (1 + extension_length)] == '.' &&
             strcasecmp (extension, input_name + (input_name_length - (extension_length))) == 0 ) {
          output_name = strndup (input_name, input_name_length - (1 + extension_length));
        }
      }
    }
  }

  if ( optind < argc ) {
    fprintf (stderr, "Too many arguments.\n");
  }

  if ( codec == NULL ) {
    fprintf (stderr, "Unable to determine codec.  Please pass -c \"codec\", or -L to see a list of available codecs.\n");
    return -1;
  }

  if ( output_name == NULL ) {
    fprintf (stderr, "Unable to determine output file.\n");
    return -1;
  }

  if ( strcmp (input_name, "-") == 0 ) {
    input = stdin;
  } else {
    input = fopen (input_name, "r");
    if ( input == NULL ) {
      perror ("Unable to open input file");
      return -1;
    }
  }

  if ( strcmp (output_name, "-") == 0 ) {
    output = stdout;
  } else {
    int output_fd = open (output_name,
                          O_WRONLY | O_CREAT | (force ? O_TRUNC : O_EXCL),
                          S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if ( output_fd < 0 ) {
      perror ("Unable to open output file");
      return -1;
    }
    output = fdopen (output_fd, "w");
    if ( output == NULL ) {
      perror ("Unable to open output");
      return -1;
    }
  }

  options = squash_options_newa (codec, (const char * const*) option_keys, (const char * const*) option_values);

  if ( direction == SQUASH_STREAM_COMPRESS ) {
    res = squash_codec_compress_file_with_options (codec, output, input, options);
  } else {
    res = squash_codec_decompress_file_with_options (codec, output, input, options);
  }

  if ( res != SQUASH_OK ) {
    fprintf (stderr, "Failed to %s: %s\n",
             (direction == SQUASH_STREAM_COMPRESS) ? "compress" : "decompress",
             squash_status_to_string (res));
    return -1;
  }

  if ( !keep && input != stdin ) {
    if ( unlink (input_name) == 0 ) {
      perror ("Unable to remove input file");
    }
  }

  return 0;
}
