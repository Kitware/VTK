/* private functions ------------------------------------------------------- */

/* reversible inverse decorrelating 2D transform */
static void
_t2(rev_inv_xform, Int, 2)(Int* p)
{
  uint x, y;
  /* transform along y */
  for (x = 0; x < 4; x++)
    _t1(rev_inv_lift, Int)(p + 1 * x, 4);
  /* transform along x */
  for (y = 0; y < 4; y++)
    _t1(rev_inv_lift, Int)(p + 4 * y, 1);
}
