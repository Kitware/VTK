/* private functions ------------------------------------------------------- */

/* reversible forward decorrelating 2D transform */
static void
_t2(rev_fwd_xform, Int, 2)(Int* p)
{
  uint x, y;
  /* transform along x */
  for (y = 0; y < 4; y++)
    _t1(rev_fwd_lift, Int)(p + 4 * y, 1);
  /* transform along y */
  for (x = 0; x < 4; x++)
    _t1(rev_fwd_lift, Int)(p + 1 * x, 4);
}
