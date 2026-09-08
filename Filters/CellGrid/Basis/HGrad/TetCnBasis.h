// Arbitrary-order Lagrange interpolants on the reference tetrahedron.
//
// This is the three-dimensional case of the construction described in
// TriCnBasis.h: a product of Silvester polynomials over the four barycentric
// coordinates, selected by a multi-index (i0, i1, i2, i3) summing to the
// order n, enumerated with i1 (the r-axis) varying fastest.
RealT nn = RealT(order[0]);

WORKSPACE(RealT, bary, 4);
bary[0] = 1. - rr - ss - tt;
bary[1] = rr;
bary[2] = ss;
bary[3] = tt;

WORKSPACE(RealT, pval, 4 * (order[0] + 1));
for (int kk = 0; kk < 4; ++kk)
{
  int base = kk * (order[0] + 1);
  pval[base] = 1.;
  for (int mm = 1; mm <= order[0]; ++mm)
  {
    RealT rmm = RealT(mm);
    pval[base + mm] = pval[base + mm - 1] * (nn * bary[kk] - (rmm - 1.)) / rmm;
  }
}

int idx = 0;
for (int i3 = 0; i3 <= order[0]; ++i3)
{
  for (int i2 = 0; i2 <= order[0] - i3; ++i2)
  {
    for (int i1 = 0; i1 <= order[0] - i2 - i3; ++i1)
    {
      int i0 = order[0] - i1 - i2 - i3;
      basis[idx++] = pval[i0] * pval[(order[0] + 1) + i1] * pval[2 * (order[0] + 1) + i2] *
        pval[3 * (order[0] + 1) + i3];
    }
  }
}
