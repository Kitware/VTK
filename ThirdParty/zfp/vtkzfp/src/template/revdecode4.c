/* private functions ------------------------------------------------------- */

/* reversible inverse decorrelating 4D transform */
static void
_t2(rev_inv_xform, Int, 4)(Int* p)
{
  uint x, y, z, w;
  /* transform along w */
  for (z = 0; z < 4; z++)
    for (y = 0; y < 4; y++)
      for (x = 0; x < 4; x++)
        _t1(rev_inv_lift, Int)(p + 1 * x + 4 * y + 16 * z, 64);
  /* transform along z */
  for (y = 0; y < 4; y++)
    for (x = 0; x < 4; x++)
      for (w = 0; w < 4; w++)
        _t1(rev_inv_lift, Int)(p + 64 * w + 1 * x + 4 * y, 16);
  /* transform along y */
  for (x = 0; x < 4; x++)
    for (w = 0; w < 4; w++)
      for (z = 0; z < 4; z++)
        _t1(rev_inv_lift, Int)(p + 16 * z + 64 * w + 1 * x, 4);
  /* transform along x */
  for (w = 0; w < 4; w++)
    for (z = 0; z < 4; z++)
      for (y = 0; y < 4; y++)
        _t1(rev_inv_lift, Int)(p + 4 * y + 16 * z + 64 * w, 1);
}
