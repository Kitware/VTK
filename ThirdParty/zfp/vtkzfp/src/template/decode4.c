/* private functions ------------------------------------------------------- */

/* scatter 4*4*4*4 block to strided array */
static void
_t2(scatter, Scalar, 4)(const Scalar* q, Scalar* p, int sx, int sy, int sz, int sw)
{
  uint x, y, z, w;
  for (w = 0; w < 4; w++, p += sw - 4 * sz)
    for (z = 0; z < 4; z++, p += sz - 4 * sy)
      for (y = 0; y < 4; y++, p += sy - 4 * sx)
        for (x = 0; x < 4; x++, p += sx)
          *p = *q++;
}

/* scatter nx*ny*nz*nw block to strided array */
static void
_t2(scatter_partial, Scalar, 4)(const Scalar* q, Scalar* p, uint nx, uint ny, uint nz, uint nw, int sx, int sy, int sz, int sw)
{
  uint x, y, z, w;
  for (w = 0; w < nw; w++, p += sw - (ptrdiff_t)nz * sz, q += 16 * (4 - nz))
    for (z = 0; z < nz; z++, p += sz - (ptrdiff_t)ny * sy, q += 4 * (4 - ny))
      for (y = 0; y < ny; y++, p += sy - (ptrdiff_t)nx * sx, q += 1 * (4 - nx))
        for (x = 0; x < nx; x++, p += sx, q++)
          *p = *q;
}

/* inverse decorrelating 4D transform */
static void
_t2(inv_xform, Int, 4)(Int* p)
{
  uint x, y, z, w;
  /* transform along w */
  for (z = 0; z < 4; z++)
    for (y = 0; y < 4; y++)
      for (x = 0; x < 4; x++)
        _t1(inv_lift, Int)(p + 1 * x + 4 * y + 16 * z, 64);
  /* transform along z */
  for (y = 0; y < 4; y++)
    for (x = 0; x < 4; x++)
      for (w = 0; w < 4; w++)
        _t1(inv_lift, Int)(p + 64 * w + 1 * x + 4 * y, 16);
  /* transform along y */
  for (x = 0; x < 4; x++)
    for (w = 0; w < 4; w++)
      for (z = 0; z < 4; z++)
        _t1(inv_lift, Int)(p + 16 * z + 64 * w + 1 * x, 4);
  /* transform along x */
  for (w = 0; w < 4; w++)
    for (z = 0; z < 4; z++)
      for (y = 0; y < 4; y++)
        _t1(inv_lift, Int)(p + 4 * y + 16 * z + 64 * w, 1);
}

/* public functions -------------------------------------------------------- */

/* decode 4*4*4*4 floating-point block and store at p using strides (sx, sy, sz, sw) */
uint
_t2(zfp_decode_block_strided, Scalar, 4)(zfp_stream* stream, Scalar* p, int sx, int sy, int sz, int sw)
{
  /* decode contiguous block */
  cache_align_(Scalar fblock[256]);
  uint bits = _t2(zfp_decode_block, Scalar, 4)(stream, fblock);
  /* scatter block to strided array */
  _t2(scatter, Scalar, 4)(fblock, p, sx, sy, sz, sw);
  return bits;
}

/* decode nx*ny*nz*nw floating-point block and store at p using strides (sx, sy, sz, sw) */
uint
_t2(zfp_decode_partial_block_strided, Scalar, 4)(zfp_stream* stream, Scalar* p, uint nx, uint ny, uint nz, uint nw, int sx, int sy, int sz, int sw)
{
  /* decode contiguous block */
  cache_align_(Scalar fblock[256]);
  uint bits = _t2(zfp_decode_block, Scalar, 4)(stream, fblock);
  /* scatter block to strided array */
  _t2(scatter_partial, Scalar, 4)(fblock, p, nx, ny, nz, nw, sx, sy, sz, sw);
  return bits;
}
