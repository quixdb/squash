private const int BUF_SIZE = 4096;

private static int main (string[] args) {
  if (args.length != 3) {
    GLib.stderr.printf ("USAGE: %s codec input-file output-file\n", args[0]);
    return -1;
  }

  uint8[] input_buffer = new uint8[BUF_SIZE];
  uint8[] output_buffer = new uint8[BUF_SIZE];

  GLib.FileStream input = GLib.FileStream.open (args[2], "r");
  if (input == null)
    GLib.error ("Unable to open input stream.");

  GLib.FileStream output = GLib.FileStream.open (args[3], "w");
  if (output == null)
    GLib.error ("Unable to open output stream.");

  unowned Squash.Codec codec = Squash.Codec.from_name (args[1]);
  if (codec == null)
    GLib.error ("Unable to find requested codec.");

  Squash.Stream stream = new Squash.Stream (codec, Squash.StreamType.COMPRESS);
  Squash.Status res = Squash.Status.OK;

  stream.next_out = output_buffer;

  while (!input.eof ()) {
    input_buffer.length = (int) input.read (input_buffer);

    stream.next_in = input_buffer;

    do {
      res = stream.process ();

      if (stream.next_out.length < BUF_SIZE) {
        output.write (output_buffer[0:BUF_SIZE - stream.next_out.length]);

        stream.next_out = output_buffer;
      }
    } while (res == Squash.Status.PROCESSING);
  }

  do {
    stream.next_out = output_buffer;

    res = stream.finish ();

    if (res < 0)
      error (res.to_string ());

    output.write (output_buffer[0:BUF_SIZE - stream.next_out.length]);
  } while (res == Squash.Status.PROCESSING);

  return 0;
}
