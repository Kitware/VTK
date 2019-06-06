/* private functions ------------------------------------------------------- */

/* reversible inverse decorrelating 1D transform */
static void
_t2(rev_inv_xform, Int, 1)(Int* p)
{
  /* transform along x */
  _t1(rev_inv_lift, Int)(p, 1);
}
