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
    BUFFER_EMPTY,
    STATE,
    INVALID_OPERATION,
    NOT_FOUND,
    INVALID_BUFFER,
    IO,
    RANGE;

    public unowned string to_string ();
  }

  [Compact, CCode (ref_function = "squash_object_ref", unref_function = "squash_object_unref", ref_sink_function = "squash_object_ref_sink")]
  public abstract class Object {
    protected void init (bool is_floating, Squash.DestroyNotify destroy_notify);
    protected void destroy ();

    public uint ref_count { get; }
  }

  [Compact]
  public class Options : Squash.Object {
		[CCode (returns_floating_reference = true)]
    public Options (Squash.Codec codec, ...);
    [CCode (cname = "squash_options_newv", returns_floating_reference = true)]
    public Options.v (Squash.Codec codec, va_list options);
    [CCode (cname = "squash_options_newa", returns_floating_reference = true)]
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

  [CCode (has_type_id = false)]
  public enum StreamState {
    IDLE,
    RUNNING,
    FLUSHING,
    FINISHING,
    FINISHED
  }

  [CCode (has_type_id = false)]
  public enum Operation {
    PROCESS,
    FLUSH,
    FINISH
  }

  [Compact, CCode (free_function = "squash_file_close")]
  public class File {
    [CCode (cname = "squash_file_open")]
    public File (string filename, string mode, string codec, ...);
    [CCode (cname = "squash_file_open_codec")]
    public File.codec (string filename, string mode, Squash.Codec codec, ...);
    [CCode (cname = "squash_file_open_with_options")]
    public File.with_options (string filename, string mode, string codec, Squash.Options? options = null);
    [CCode (cname = "squash_file_open_codec_with_options")]
    public File.codec_with_options (string filename, string mode, Squash.Codec codec, Squash.Options? options = null);

    [CCode (cname = "squash_file_steal")]
    public File.steal (owned GLib.FileStream file, string codec, ...);
    [CCode (cname = "squash_file_steal_codec")]
    public File.steal_codec (owned GLib.FileStream file, Squash.Codec codec, ...);
    [CCode (cname = "squash_file_steal_with_options")]
    public File.steal_with_options (owned GLib.FileStream file, string codec, Squash.Options? options = null);
    [CCode (cname = "squash_file_steal_codec_with_options")]
    public File.steal_codec_with_options (owned GLib.FileStream file, Squash.Codec codec, Squash.Options? options = null);

    public Squash.Status read (ref size_t decompressed_size, [CCode (has_array_length = false)] uint8[] decompressed);
    public Squash.Status write ([CCode (array_length_type = "size_t", array_length_pos = 0.5)] uint8[] uncompressed);
    public Squash.Status flush ();
    public bool eof ();

    public void @lock ();
    public void unlock ();
    public Squash.Status read_unlocked (ref size_t decompressed_size, [CCode (has_array_length = false)] uint8[] decompressed);
    public Squash.Status write_unlocked ([CCode (array_length_type = "size_t", array_length_pos = 0.5)] uint8[] uncompressed);
    public Squash.Status flush_unlocked ();

    [DestroysInstance]
    public Squash.Status free_to_file (out GLib.FileStream fp);
  }

  [Compact]
  public class Stream : Squash.Object {
    public Stream (string codec, Squash.StreamType stream_type, ...);
    public Stream.with_options (string codec, Squash.StreamType stream_type, Squash.Options? options = null);
    [CCode (cname = "squash_stream_new_codec")]
    public Stream.with_codec (Squash.Codec codec, Squash.StreamType stream_type, ...);
    public Stream.codec_with_options (Squash.Codec codec, Squash.StreamType stream_type, Squash.Options? options = null);

    [CCode (array_length_cname = "avail_in", array_length_type = "size_t")]
    public unowned uint8[] next_in;
    public size_t total_in;
    [CCode (array_length_cname = "avail_out", array_length_type = "size_t")]
    public unowned uint8[] next_out;
    public size_t total_out;
    public unowned Squash.Codec codec;
    public Squash.Options options;
    public Squash.StreamType stream_type;
    public Squash.StreamState state;
    public void* user_data;
    public Squash.DestroyNotify destroy_user_data;

    [CCode (cname = "squash_stream_newv")]
    public Stream.v (string codec, Squash.StreamType stream_type, va_list options);
    [CCode (cname = "squash_stream_newa")]
    public Stream.array (string codec, Squash.StreamType stream_type, [CCode (array_length = false, array_null_terminated = true)] string[] keys, [CCode (array_length = false, array_null_terminated = true)] string[] values);

    public Squash.Status process ();
    public Squash.Status flush ();
    public Squash.Status finish ();

    protected void init (Squash.Codec codec, Squash.StreamType stream_type, Squash.Options? options = null, Squash.DestroyNotify? destroy_notify = null);
    protected void destroy ();
    protected Squash.Operation @yield (Squash.Status status);
  }

  [Flags, CCode (has_type_id = false)]
  public enum CodecInfo {
    CAN_FLUSH,
    RUN_IN_THREAD,
    DECOMPRESS_UNSAFE,

    AUTO_MASK,
    VALID,
    KNOWS_UNCOMPRESSED_SIZE,
    NATIVE_STREAMING,

    MASK
  }

  [Compact]
  public class Codec {
    private Codec ();
    [CCode (cname = "squash_get_codec")]
    public static unowned Codec? from_name (string plugin);
    [CCode (cname = "squash_get_codec_from_extension")]
    public static unowned Codec? from_extension (string extension);

    public void init ();

    public string name { get; }
    public uint priority { get; }
    public Squash.Plugin plugin { get; }
    public string extension { get; }

    public size_t get_uncompressed_size ([CCode (array_length_type = "size_t", array_length_pos = 0.5)] uint8[] compressed);
    public size_t get_max_compressed_size (size_t uncompressed_size);

    public Squash.Stream create_stream (Squash.StreamType stream_type, ...);
    public Squash.Stream create_stream_with_options (Squash.StreamType stream_type, Squash.Options? options = null);

    public Squash.Status compress (ref size_t compressed_size, [CCode (array_length = false)] uint8[] compressed, [CCode (array_length_type = "size_t", array_length_pos = 2.5)] uint8[] uncompressed, ...);
    public Squash.Status compress_with_options (ref size_t compressed_size, [CCode (array_length = false)] uint8[] compressed, [CCode (array_length_type = "size_t", array_length_pos = 2.5)] uint8[] uncompressed, Squash.Options? options = null);

    public Squash.Status decompress (ref size_t decompressed_length, [CCode (array_length = false)] uint8[] decompressed, [CCode (array_length_type = "size_t", array_length_pos = 2.5)] uint8[] compressed, ...);
    public Squash.Status decompress_with_options (ref size_t decompressed_size, [CCode (array_length = false)] uint8[] decompressed, [CCode (array_length_type = "size_t", array_length_pos = 2.5)] uint8[] uncompressed, Squash.Options? options = null);

    public Squash.CodecInfo get_info ();
  }

  [Flags, CCode (has_type_id = false)]
  public enum License {
    UNKNOWN,

    PERMISSIVE,
    STRONG_COPYLEFT,
    WEAK_COPYLEFT,
    PREPRIETARY,
    TYPE_MASK,

    COPYLEFT_INCOMPATIBLE,
    OR_GREATER,
    FLAGS_MASK,

    PUBLIC_DOMAIN,
    BSD2,
    BSD3,
    BSD4,
    MIT,
    ZLIB,
    WTFPL,
    X11,
    APACHE,
    APACHE2,
    CDDL,
    MSPL,
    ISC,

    MPL,
    LGPL2P1,
    LG3L2P1_PLUS,
    LGPL3,
    LGPL3_PLUS,

    GPL1,
    GPL1_PLUS,
    GPL2,
    GPL2_PLUS,
    GPL3,
    GPL3_PLUS;

    public static Squash.License from_string (string license);
    public unowned string to_string ();
  }

  public delegate void PluginForeachFunc (Squash.Plugin plugin);
  public delegate void CodecForeachFunc (Squash.Codec codec);

  [Compact]
  public class Plugin {
    private Plugin ();
    [CCode (cname = "squash_get_plugin")]
    public static unowned Plugin from_name (string plugin);
    public void init ();

    [CCode (array_length = false, array_null_terminated = true)]
    public Squash.License[]? get_licenses ();
    public Squash.Codec? get_codec (string name);

    public string name { get; }
  }

  [Compact]
  public class Context {
    private Context ();
    public static unowned Squash.Context get_default ();

    public unowned Squash.Plugin? get_plugin (string plugin);
    public unowned Squash.Codec? get_codec (string codec);
    public void foreach_plugin (Squash.PluginForeachFunc func);
    public void foreach_codec (Squash.CodecForeachFunc func);
    public unowned Squash.Codec get_codec_from_extension (string extension);
  }

  [CCode (has_target = false)]
  public delegate void DestroyNotify (void* data);

  public static unowned Squash.Plugin? get_plugin (string plugin);
  public static unowned Squash.Codec? get_codec (string codec);
  public static void foreach_plugin (Squash.PluginForeachFunc func);
  public static void foreach_codec (Squash.CodecForeachFunc func);

  public static Squash.CodecInfo get_info (string codec);
  public static size_t get_uncompressed_size (string codec, [CCode (array_length_type = "size_t", array_length_pos = 1.5)] uint8[] uncompressed);
  public static size_t get_max_compressed_size (string codec, size_t uncompressed_size);
  public static Squash.Status compress (string codec, ref size_t compressed_size, [CCode (array_length = false)] uint8[] compressed, [CCode (array_length_type = "size_t", array_length_pos = 3.5)] uint8[] uncompressed, ...);
  public static Squash.Status compress_with_options (string codec, ref size_t compressed_size, [CCode (array_length = false)] uint8[] compressed, [CCode (array_length_type = "size_t", array_length_pos = 3.5)] uint8[] uncompressed, Squash.Options? options = null);
  public static Squash.Status decompress (string codec, ref size_t decompressed_size, [CCode (array_length = false)] uint8[] decompressed, [CCode (array_length_type = "size_t", array_length_pos = 3.5)] uint8[] compressed, ...);
  public static Squash.Status decompress_with_options (string codec, ref size_t decompressed_size, [CCode (array_length = false)] uint8[] decompressed, [CCode (array_length_type = "size_t", array_length_pos = 3.5)] uint8[] compressed, Squash.Options? options = null);

  [CCode (cname = "squash_splice")]
  public static Squash.Status splice (string codec, Squash.StreamType stream_type, GLib.FileStream fp_out, GLib.FileStream fp_in, size_t size, ...);
  [CCode (cname = "squash_splice_codec")]
  public static Squash.Status splice_codec (Squash.Codec codec, Squash.StreamType stream_type, GLib.FileStream fp_out, GLib.FileStream fp_in, size_t size, ...);
  [CCode (cname = "squash_splice_with_options")]
  public static Squash.Status splice_with_options (string codec, Squash.StreamType stream_type, GLib.FileStream fp_out, GLib.FileStream fp_in, size_t size, Squash.Options? options = null);
  [CCode (cname = "squash_splice_codec_with_options")]
  public static Squash.Status splice_codec_with_options (Squash.Codec codec, Squash.StreamType stream_type, GLib.FileStream fp_out, GLib.FileStream fp_in, size_t size, Squash.Options? options = null);
}
