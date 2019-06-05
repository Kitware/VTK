/* private functions ------------------------------------------------------- */

/* reversible forward decorrelating 1D transform */
static void
_t2(rev_fwd_xform, Int, 1)(Int* p)
{
  /* transform along x */
  _t1(rev_fwd_lift, Int)(p, 1);
}
