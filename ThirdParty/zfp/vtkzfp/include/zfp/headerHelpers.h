// "Handle" classes useful when throwing exceptions

// buffer holds aligned memory for header, suitable for bitstream r/w (word-aligned)
class AlignedBufferHandle {
  public:
    size_t buffer_size_bytes;
    // uint64 alignment guarantees bitstream alignment
    uint64* buffer;

    // can copy a header into aligned buffer
    AlignedBufferHandle(const zfp::array::header* h = 0) {
      size_t num_64bit_entries = DIV_ROUND_UP(ZFP_HEADER_SIZE_BITS, CHAR_BIT * sizeof(uint64));
      buffer = new uint64[num_64bit_entries];
      buffer_size_bytes = num_64bit_entries * sizeof(uint64);

      if (h)
        memcpy(buffer, h->buffer, BITS_TO_BYTES(ZFP_HEADER_SIZE_BITS));
    }

    ~AlignedBufferHandle() {
      delete[] buffer;
    }

    void copy_to_header(zfp::array::header* h) {
      memcpy(h, buffer, BITS_TO_BYTES(ZFP_HEADER_SIZE_BITS));
    }
};

// redirect zfp_stream->bitstream to header while object remains in scope
class DualBitstreamHandle {
  public:
    bitstream* old_bs;
    bitstream* new_bs;
    zfp_stream* zfp;

    DualBitstreamHandle(zfp_stream* zfp, AlignedBufferHandle& abh) :
      zfp(zfp)
    {
      old_bs = zfp_stream_bit_stream(zfp);
      new_bs = stream_open(abh.buffer, abh.buffer_size_bytes);

      stream_rewind(new_bs);
      zfp_stream_set_bit_stream(zfp, new_bs);
    }

    ~DualBitstreamHandle() {
      zfp_stream_set_bit_stream(zfp, old_bs);
      stream_close(new_bs);
    }
};

class ZfpFieldHandle {
  public:
    zfp_field* field;

    ZfpFieldHandle() {
      field = zfp_field_alloc();
    }

    ZfpFieldHandle(zfp_type type, int nx, int ny, int nz) {
      field = zfp_field_3d(0, type, nx, ny, nz);
    }

    ~ZfpFieldHandle() {
      zfp_field_free(field);
    }
};

class ZfpStreamHandle {
  public:
    bitstream* bs;
    zfp_stream* stream;

    ZfpStreamHandle(AlignedBufferHandle& abh) {
      bs = stream_open(abh.buffer, abh.buffer_size_bytes);
      stream = zfp_stream_open(bs);
    }

    ~ZfpStreamHandle() {
      zfp_stream_close(stream);
      stream_close(bs);
    }
};

// verify buffer is large enough, with what header describes
static bool is_valid_buffer_size(const zfp_stream* stream, uint nx, uint ny, uint nz, size_t expected_buffer_size_bytes)
{
  uint mx = ((std::max(nx, 1u)) + 3) / 4;
  uint my = ((std::max(ny, 1u)) + 3) / 4;
  uint mz = ((std::max(nz, 1u)) + 3) / 4;
  size_t blocks = (size_t)mx * (size_t)my * (size_t)mz;
  // no rounding because fixed-rate wra implies rate is multiple of word size
  size_t described_buffer_size_bytes = blocks * stream->maxbits / CHAR_BIT;

  return expected_buffer_size_bytes >= described_buffer_size_bytes;
}

static void read_header_contents(const zfp::array::header& header, size_t expected_buffer_size_bytes, uint& dims, zfp_type& type, double& rate, uint n[4])
{
  // create zfp_stream and zfp_field structs to call C API zfp_read_header()
  AlignedBufferHandle abh;
  memcpy(abh.buffer, header.buffer, BITS_TO_BYTES(ZFP_HEADER_SIZE_BITS));

  ZfpStreamHandle zsh(abh);
  ZfpFieldHandle zfh;

  if (!zfp_read_header(zsh.stream, zfh.field, ZFP_HEADER_FULL))
    throw zfp::array::header::exception("Invalid ZFP header.");

  // gather metadata
  dims = zfp_field_dimensionality(zfh.field);
  type = zfp_field_type(zfh.field);

  uint num_block_entries = 1u << (2 * dims);
  rate = (double)zsh.stream->maxbits / num_block_entries;

  zfp_field_size(zfh.field, n);

  // validate header
  std::string err_msg = "";
  verify_header_contents(zsh.stream, zfh.field, err_msg);

  if (!err_msg.empty())
    throw zfp::array::header::exception(err_msg);

  if (expected_buffer_size_bytes && !is_valid_buffer_size(zsh.stream, zfh.field->nx, zfh.field->ny, zfh.field->nz, expected_buffer_size_bytes))
    throw zfp::array::header::exception("ZFP header expects a longer buffer than what was passed in.");
}

// verifies metadata on zfp_stream and zfp_field describe a valid compressed array
static void verify_header_contents(const zfp_stream* stream, const zfp_field* field, std::string& err_msg)
{
  // verify read-header contents
  zfp_type type = zfp_field_type(field);
  if (type != zfp_type_float && type != zfp_type_double)
    zfp::array::header::concat_sentence(err_msg, "ZFP compressed arrays do not yet support scalar types beyond floats and doubles.");

  uint dims = zfp_field_dimensionality(field);
  if (dims < 1 || dims > 3)
    zfp::array::header::concat_sentence(err_msg, "ZFP compressed arrays do not yet support dimensionalities beyond 1, 2, and 3.");

  if (zfp_stream_compression_mode(stream) != zfp_mode_fixed_rate)
    zfp::array::header::concat_sentence(err_msg, "ZFP header specified a non fixed-rate mode, unsupported by this object.");
}
