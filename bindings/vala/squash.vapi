[CCode (cheader_filename = "squash/squash.h")]
namespace Squash {
  [CCode (cprefix = "SQUASH_", has_type_id = false)]
  public enum Status {
    OK,
    PROCESSING,
    END_OF_STREAM,

    FAILED,
    UNABLE_TO_LOAD_PLUGIN,
    BAD_PARAM,
    BAD_VALUE,
    MEMORY,
    BUFFER_FULL,
    STATE,
    INVALID_OPERATION;

    public unowned string to_string ();
  }

  [Compact, CCode (ref_function = "squash_object_ref", unref_function = "squash_object_unref")]
  public abstract class Object {
    protected void init (bool is_floating, Squash.DestroyNotify destroy_notify);
    protected void destroy ();

    public uint ref_count { get; }
  }

  [Compact]
  public class Options : Squash.Object {
    public Options (Squash.Codec codec, ...);
    [CCode (cname = "squash_options_newv")]
    public Options.v (Squash.Codec codec, va_list options);
    [CCode (cname = "squash_options_newa")]
    public Options.array (Squash.Codec codec, [CCode (array_length = false, array_null_terminated = true)] string[] keys, [CCode (array_length = false, array_null_terminated = true)] string[] values);

    public Squash.Status parse (...);
    public Squash.Status parsev (va_list options);
    public Squash.Status parse_array ([CCode (array_length = false, array_null_terminated = true)] string[] keys, [CCode (array_length = false, array_null_terminated = true)] string[] values);
    public Squash.Status parse_option (string key, string value);

    public new void init (Squash.Codec codec, Squash.DestroyNotify? destroy_notify);
    public new void destroy (Squash.Options options);

    public unowned Squash.Codec codec;
  }

  [CCode (has_type_id = false, cprefix = "SQUASH_STREAM_")]
  public enum StreamType {
    COMPRESS,
    DECOMPRESS
  }

  [Compact]
  public class Stream : Squash.Object {
    [CCode (array_length_cname = "avail_in", array_length_type = "size_t")]
    public unowned uint8[] next_in;
    public size_t total_in;
    [CCode (array_length_cname = "avail_out", array_length_type = "size_t")]
    public unowned uint8[] next_out;
    public size_t total_out;
    public unowned Squash.Codec codec;
    public Squash.Options options;
    public Squash.StreamType stream_type;
    public void* user_data;
    public Squash.DestroyNotify destroy_user_data;

    public Stream (string codec, Squash.StreamType stream_type, ...);
    [CCode (cname = "squash_stream_newv")]
    public Stream.v (string codec, Squash.StreamType stream_type, va_list options);
    [CCode (cname = "squash_stream_newa")]
    public Stream.array (string codec, Squash.StreamType stream_type, [CCode (array_length = false, array_null_terminated = true)] string[] keys, [CCode (array_length = false, array_null_terminated = true)] string[] values);
    public Stream.with_options (string codec, Squash.StreamType stream_type, Squash.Options? options = null);

    public Squash.Status process ();
    public Squash.Status flush ();
    public Squash.Status finish ();

    protected void init (Squash.Codec codec, Squash.StreamType stream_type, Squash.Options? options = null, Squash.DestroyNotify? destroy_notify = null);
    protected void destroy ();
  }

  [Compact]
  public class Codec {
    private Codec ();
    [CCode (cname = "squash_get_codec")]
    public static unowned Codec from_name (string plugin);

    public string name { get; }
    public uint priority { get; }
    public Squash.Plugin plugin { get; }
    public bool knows_uncompressed_size { get; }

    public Squash.Stream create_stream (Squash.StreamType stream_type, ...);
    public Squash.Stream create_stream_with_options (Squash.StreamType stream_type, Squash.Options? options = null);

    public Squash.Status compress ([CCode (array_length_type = "size_t")] uint8[] compressed, [CCode (array_length_type = "size_t")] uint8[] uncompressed, ...);
    public Squash.Status compress_with_options ([CCode (array_length_type = "size_t")] uint8[] compressed, [CCode (array_length_type = "size_t")] uint8[] uncompressed, Squash.Options? options = null);

    public Squash.Status decompress ([CCode (array_length_type = "size_t")] uint8[] decompressed, [CCode (array_length_type = "size_t")] uint8[] compressed, ...);
    public Squash.Status decompress_with_options ([CCode (array_length_type = "size_t")] uint8[] decompressed, [CCode (array_length_type = "size_t")] uint8[] uncompressed, Squash.Options? options = null);

    public Squash.Status compress_file_with_options (GLib.FileStream compressed, GLib.FileStream uncompressed, Squash.Options? options = null);
    public Squash.Status decompress_file_with_options (GLib.FileStream decompressed, GLib.FileStream compressed, Squash.Options? options = null);
    public Squash.Status compress_file (GLib.FileStream compressed, GLib.FileStream uncompressed, ...);
    public Squash.Status decompress_file (GLib.FileStream decompressed, GLib.FileStream compressed, ...);
  }

  [Compact]
  public class Plugin {
    private Plugin ();
    [CCode (cname = "squash_get_plugin")]
    public static unowned Plugin from_name (string plugin);

    public Squash.Codec? get_codec (string name);

    public string name { get; }
  }

  [Compact]
  public class Context {
    private Context ();
    public static unowned Squash.Context get_default ();

    public unowned Squash.Plugin get_plugin (string plugin);
    public unowned Squash.Codec get_codec (string codec);
  }

  [CCode (has_target = false)]
  public delegate void DestroyNotify (void* data);

  public static Squash.Status compress_file (string codec, GLib.FileStream compressed, GLib.FileStream uncompressed, ...);
  public static Squash.Status decompress_file (string codec, GLib.FileStream decompressed, GLib.FileStream compressed, ...);
  public static Squash.Status compress_file_with_options (string codec, GLib.FileStream compressed, GLib.FileStream uncompressed, Squash.Options? options = null);
  public static Squash.Status decompress_file_with_options (string codec, GLib.FileStream decompressed, GLib.FileStream compressed, Squash.Options? options = null);
}
