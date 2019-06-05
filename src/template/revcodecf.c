/* inverse block-floating-point transform from signed integers */
static void
_t1(rev_inv_cast, Scalar)(const Int* iblock, Scalar* fblock, uint n, int emax)
{
  /* test for all-zero block, which needs special treatment */
  if (emax != -EBIAS)
    _t1(inv_cast, Scalar)(iblock, fblock, n, emax);
  else
    while (n--)
      *fblock++ = 0;
}
