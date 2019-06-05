#ifdef _OPENMP

/* compress 1d contiguous array in parallel */
static void
_t2(compress_omp, Scalar, 1)(zfp_stream* stream, const zfp_field* field)
{
  /* array metadata */
  const Scalar* data = (const Scalar*)field->data;
  uint nx = field->nx;

  /* number of omp threads, blocks, and chunks */
  uint threads = thread_count_omp(stream);
  uint blocks = (nx + 3) / 4;
  uint chunks = chunk_count_omp(stream, blocks, threads);

  /* allocate per-thread streams */
  bitstream** bs = compress_init_par(stream, field, chunks, blocks);
  if (!bs)
    return;

  /* compress chunks of blocks in parallel */
  int chunk;
  #pragma omp parallel for num_threads(threads)
  for (chunk = 0; chunk < (int)chunks; chunk++) {
    /* determine range of block indices assigned to this thread */
    uint bmin = chunk_offset(blocks, chunks, chunk + 0);
    uint bmax = chunk_offset(blocks, chunks, chunk + 1);
    uint block;
    /* set up thread-local bit stream */
    zfp_stream s = *stream;
    zfp_stream_set_bit_stream(&s, bs[chunk]);
    /* compress sequence of blocks */
    for (block = bmin; block < bmax; block++) {
      /* determine block origin x within array */
      const Scalar* p = data;
      uint x = 4 * block;
      p += x;
      /* compress partial or full block */
      if (nx - x < 4)
        _t2(zfp_encode_partial_block_strided, Scalar, 1)(&s, p, MIN(nx - x, 4u), 1);
      else
        _t2(zfp_encode_block, Scalar, 1)(&s, p);
    }
  }

  /* concatenate per-thread streams */
  compress_finish_par(stream, bs, chunks);
}

/* compress 1d strided array in parallel */
static void
_t2(compress_strided_omp, Scalar, 1)(zfp_stream* stream, const zfp_field* field)
{
  /* array metadata */
  const Scalar* data = (const Scalar*)field->data;
  uint nx = field->nx;
  int sx = field->sx ? field->sx : 1;

  /* number of omp threads, blocks, and chunks */
  uint threads = thread_count_omp(stream);
  uint blocks = (nx + 3) / 4;
  uint chunks = chunk_count_omp(stream, blocks, threads);

  /* allocate per-thread streams */
  bitstream** bs = compress_init_par(stream, field, chunks, blocks);
  if (!bs)
    return;

  /* compress chunks of blocks in parallel */
  int chunk;
  #pragma omp parallel for num_threads(threads)
  for (chunk = 0; chunk < (int)chunks; chunk++) {
    /* determine range of block indices assigned to this thread */
    uint bmin = chunk_offset(blocks, chunks, chunk + 0);
    uint bmax = chunk_offset(blocks, chunks, chunk + 1);
    uint block;
    /* set up thread-local bit stream */
    zfp_stream s = *stream;
    zfp_stream_set_bit_stream(&s, bs[chunk]);
    /* compress sequence of blocks */
    for (block = bmin; block < bmax; block++) {
      /* determine block origin x within array */
      const Scalar* p = data;
      uint x = 4 * block;
      p += sx * (ptrdiff_t)x;
      /* compress partial or full block */
      if (nx - x < 4)
        _t2(zfp_encode_partial_block_strided, Scalar, 1)(&s, p, MIN(nx - x, 4u), sx);
      else
        _t2(zfp_encode_block_strided, Scalar, 1)(&s, p, sx);
    }
  }

  /* concatenate per-thread streams */
  compress_finish_par(stream, bs, chunks);
}

/* compress 2d strided array in parallel */
static void
_t2(compress_strided_omp, Scalar, 2)(zfp_stream* stream, const zfp_field* field)
{
  /* array metadata */
  const Scalar* data = (const Scalar*)field->data;
  uint nx = field->nx;
  uint ny = field->ny;
  int sx = field->sx ? field->sx : 1;
  int sy = field->sy ? field->sy : (int)nx;

  /* number of omp threads, blocks, and chunks */
  uint threads = thread_count_omp(stream);
  uint bx = (nx + 3) / 4;
  uint by = (ny + 3) / 4;
  uint blocks = bx * by;
  uint chunks = chunk_count_omp(stream, blocks, threads);

  /* allocate per-thread streams */
  bitstream** bs = compress_init_par(stream, field, chunks, blocks);
  if (!bs)
    return;

  /* compress chunks of blocks in parallel */
  int chunk;
  #pragma omp parallel for num_threads(threads)
  for (chunk = 0; chunk < (int)chunks; chunk++) {
    /* determine range of block indices assigned to this thread */
    uint bmin = chunk_offset(blocks, chunks, chunk + 0);
    uint bmax = chunk_offset(blocks, chunks, chunk + 1);
    uint block;
    /* set up thread-local bit stream */
    zfp_stream s = *stream;
    zfp_stream_set_bit_stream(&s, bs[chunk]);
    /* compress sequence of blocks */
    for (block = bmin; block < bmax; block++) {
      /* determine block origin (x, y) within array */
      const Scalar* p = data;
      uint b = block;
      uint x, y;
      x = 4 * (b % bx); b /= bx;
      y = 4 * b;
      p += sx * (ptrdiff_t)x + sy * (ptrdiff_t)y;
      /* compress partial or full block */
      if (nx - x < 4 || ny - y < 4)
        _t2(zfp_encode_partial_block_strided, Scalar, 2)(&s, p, MIN(nx - x, 4u), MIN(ny - y, 4u), sx, sy);
      else
        _t2(zfp_encode_block_strided, Scalar, 2)(&s, p, sx, sy);
    }
  }

  /* concatenate per-thread streams */
  compress_finish_par(stream, bs, chunks);
}

/* compress 3d strided array in parallel */
static void
_t2(compress_strided_omp, Scalar, 3)(zfp_stream* stream, const zfp_field* field)
{
  /* array metadata */
  const Scalar* data = (const Scalar*)field->data;
  uint nx = field->nx;
  uint ny = field->ny;
  uint nz = field->nz;
  int sx = field->sx ? field->sx : 1;
  int sy = field->sy ? field->sy : (int)nx;
  int sz = field->sz ? field->sz : (int)(nx * ny);

  /* number of omp threads, blocks, and chunks */
  uint threads = thread_count_omp(stream);
  uint bx = (nx + 3) / 4;
  uint by = (ny + 3) / 4;
  uint bz = (nz + 3) / 4;
  uint blocks = bx * by * bz;
  uint chunks = chunk_count_omp(stream, blocks, threads);

  /* allocate per-thread streams */
  bitstream** bs = compress_init_par(stream, field, chunks, blocks);
  if (!bs)
    return;

  /* compress chunks of blocks in parallel */
  int chunk;
  #pragma omp parallel for num_threads(threads)
  for (chunk = 0; chunk < (int)chunks; chunk++) {
    /* determine range of block indices assigned to this thread */
    uint bmin = chunk_offset(blocks, chunks, chunk + 0);
    uint bmax = chunk_offset(blocks, chunks, chunk + 1);
    uint block;
    /* set up thread-local bit stream */
    zfp_stream s = *stream;
    zfp_stream_set_bit_stream(&s, bs[chunk]);
    /* compress sequence of blocks */
    for (block = bmin; block < bmax; block++) {
      /* determine block origin (x, y, z) within array */
      const Scalar* p = data;
      uint b = block;
      uint x, y, z;
      x = 4 * (b % bx); b /= bx;
      y = 4 * (b % by); b /= by;
      z = 4 * b;
      p += sx * (ptrdiff_t)x + sy * (ptrdiff_t)y + sz * (ptrdiff_t)z;
      /* compress partial or full block */
      if (nx - x < 4 || ny - y < 4 || nz - z < 4)
        _t2(zfp_encode_partial_block_strided, Scalar, 3)(&s, p, MIN(nx - x, 4u), MIN(ny - y, 4u), MIN(nz - z, 4u), sx, sy, sz);
      else
        _t2(zfp_encode_block_strided, Scalar, 3)(&s, p, sx, sy, sz);
    }
  }

  /* concatenate per-thread streams */
  compress_finish_par(stream, bs, chunks);
}

/* compress 4d strided array in parallel */
static void
_t2(compress_strided_omp, Scalar, 4)(zfp_stream* stream, const zfp_field* field)
{
  /* array metadata */
  const Scalar* data = field->data;
  uint nx = field->nx;
  uint ny = field->ny;
  uint nz = field->nz;
  uint nw = field->nw;
  int sx = field->sx ? field->sx : 1;
  int sy = field->sy ? field->sy : (int)nx;
  int sz = field->sz ? field->sz : (int)(nx * ny);
  int sw = field->sw ? field->sw : (int)(nx * ny * nz);

  /* number of omp threads, blocks, and chunks */
  uint threads = thread_count_omp(stream);
  uint bx = (nx + 3) / 4;
  uint by = (ny + 3) / 4;
  uint bz = (nz + 3) / 4;
  uint bw = (nw + 3) / 4;
  uint blocks = bx * by * bz * bw;
  uint chunks = chunk_count_omp(stream, blocks, threads);

  /* allocate per-thread streams */
  bitstream** bs = compress_init_par(stream, field, chunks, blocks);
  if (!bs)
    return;

  /* compress chunks of blocks in parallel */
  int chunk;
  #pragma omp parallel for num_threads(threads)
  for (chunk = 0; chunk < (int)chunks; chunk++) {
    /* determine range of block indices assigned to this thread */
    uint bmin = chunk_offset(blocks, chunks, chunk + 0);
    uint bmax = chunk_offset(blocks, chunks, chunk + 1);
    uint block;
    /* set up thread-local bit stream */
    zfp_stream s = *stream;
    zfp_stream_set_bit_stream(&s, bs[chunk]);
    /* compress sequence of blocks */
    for (block = bmin; block < bmax; block++) {
      /* determine block origin (x, y, z, w) within array */
      const Scalar* p = data;
      uint b = block;
      uint x, y, z, w;
      x = 4 * (b % bx); b /= bx;
      y = 4 * (b % by); b /= by;
      z = 4 * (b % bz); b /= bz;
      w = 4 * b;
      p += sx * (ptrdiff_t)x + sy * (ptrdiff_t)y + sz * (ptrdiff_t)z + sw * (ptrdiff_t)w;
      /* compress partial or full block */
      if (nx - x < 4 || ny - y < 4 || nz - z < 4 || nw - w < 4)
        _t2(zfp_encode_partial_block_strided, Scalar, 4)(&s, p, MIN(nx - x, 4u), MIN(ny - y, 4u), MIN(nz - z, 4u), MIN(nw - w, 4u), sx, sy, sz, sw);
      else
        _t2(zfp_encode_block_strided, Scalar, 4)(&s, p, sx, sy, sz, sw);
    }
  }

  /* concatenate per-thread streams */
  compress_finish_par(stream, bs, chunks);
}

#endif
