#ifdef _OPENMP

/* block index at which chunk begins */
static uint
chunk_offset(uint blocks, uint chunks, uint chunk)
{
  return (uint)((blocks * (uint64)chunk) / chunks);
}

/* initialize per-thread bit streams for parallel compression */
static bitstream**
compress_init_par(zfp_stream* stream, const zfp_field* field, uint chunks, uint blocks)
{
  bitstream** bs;
  size_t size;
  int copy = 0;
  uint i;

  /* determine maximum size buffer needed per thread */
  zfp_field f = *field;
  switch (zfp_field_dimensionality(field)) {
    case 1:
      f.nx = 4 * (blocks + chunks - 1) / chunks;
      break;
    case 2:
      f.nx = 4;
      f.ny = 4 * (blocks + chunks - 1) / chunks;
      break;
    case 3:
      f.nx = 4;
      f.ny = 4;
      f.nz = 4 * (blocks + chunks - 1) / chunks;
      break;
    case 4:
      f.nx = 4;
      f.ny = 4;
      f.nz = 4;
      f.nw = 4 * (blocks + chunks - 1) / chunks;
      break;
    default:
      return NULL;
  }
  size = zfp_stream_maximum_size(stream, &f);

  /* avoid copies in fixed-rate mode when each bitstream is word aligned */
  copy |= stream->minbits != stream->maxbits;
  copy |= (stream->maxbits % stream_word_bits) != 0;
  copy |= (stream_wtell(stream->stream) % stream_word_bits) != 0;

  /* set up buffer for each thread to compress to */
  bs = (bitstream**)malloc(chunks * sizeof(bitstream*));
  if (!bs)
    return NULL;
  for (i = 0; i < chunks; i++) {
    uint block = chunk_offset(blocks, chunks, i);
    void* buffer = copy ? malloc(size) : (uchar*)stream_data(stream->stream) + stream_size(stream->stream) + block * stream->maxbits / CHAR_BIT;
    if (!buffer)
      break;
    bs[i] = stream_open(buffer, size);
  }

  /* handle memory allocation failure */
  if (copy && i < chunks) {
    while (i--) {
      free(stream_data(bs[i]));
      stream_close(bs[i]);
    }
    free(bs);
    bs = NULL;
  }

  return bs;
}

/* flush and concatenate bit streams if needed */
static void
compress_finish_par(zfp_stream* stream, bitstream** src, uint chunks)
{
  bitstream* dst = zfp_stream_bit_stream(stream);
  int copy = (stream_data(dst) != stream_data(*src));
  size_t offset = stream_wtell(dst);
  uint i;
  for (i = 0; i < chunks; i++) {
    size_t bits = stream_wtell(src[i]);
    offset += bits;
    stream_flush(src[i]);
    /* concatenate streams if they are not already contiguous */
    if (copy) {
      stream_rewind(src[i]);
      stream_copy(dst, src[i], bits);
      free(stream_data(src[i]));
    }
    stream_close(src[i]);
  }
  free(src);
  if (!copy)
    stream_wseek(dst, offset);
}

#endif
