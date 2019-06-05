/* private functions ------------------------------------------------------- */

/* reversible inverse decorrelating 3D transform */
static void
_t2(rev_inv_xform, Int, 3)(Int* p)
{
  uint x, y, z;
  /* transform along z */
  for (y = 0; y < 4; y++)
    for (x = 0; x < 4; x++)
      _t1(rev_inv_lift, Int)(p + 1 * x + 4 * y, 16);
  /* transform along y */
  for (x = 0; x < 4; x++)
    for (z = 0; z < 4; z++)
      _t1(rev_inv_lift, Int)(p + 16 * z + 1 * x, 4);
  /* transform along x */
  for (z = 0; z < 4; z++)
    for (y = 0; y < 4; y++)
      _t1(rev_inv_lift, Int)(p + 4 * y + 16 * z, 1);
}
