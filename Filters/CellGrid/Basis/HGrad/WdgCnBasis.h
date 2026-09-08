// Arbitrary-order Lagrange interpolants on the reference wedge.
//
// A wedge is the product of a triangle (the r- and s-axes) with an edge (the
// t-axis), so its basis is the product of the triangle's Silvester polynomials
// (see TriCnBasis.h) with the edge's 1-d Lagrange interpolants (see
// EdgeCnBasis.h. The two factors may have different orders, but the triangle
// has a single total degree shared by its two axes: order[0] is the order of
// the triangular cross-section and order[2] the order along t.
//
// The degrees of freedom are enumerated with the r-axis varying fastest and t
// slowest, so the triangle's degrees of freedom repeat once per t-node.
RealT nn = RealT(order[0]);
RealT invspcT = RealT(order[2]) / 2.;

WORKSPACE(RealT, bary, 3);
bary[0] = 1. - rr - ss;
bary[1] = rr;
bary[2] = ss;

WORKSPACE(RealT, pval, 3 * (order[0] + 1));
for (int kk = 0; kk < 3; ++kk)
{
  int base = kk * (order[0] + 1);
  pval[base] = 1.;
  for (int mm = 1; mm <= order[0]; ++mm)
  {
    RealT rmm = RealT(mm);
    pval[base + mm] = pval[base + mm - 1] * (nn * bary[kk] - (rmm - 1.)) / rmm;
  }
}

WORKSPACE(RealT, ttmp, order[2] + 1);
for (int ll = 0; ll <= order[2]; ++ll)
{
  ttmp[ll] = 1.;
  for (int oo = 0; oo <= order[2]; ++oo)
  {
    if (oo == ll)
    {
      continue;
    }
    ttmp[ll] *= (tt * invspcT - (RealT(oo) - invspcT)) / RealT(ll - oo);
  }
}

int idx = 0;
for (int it = 0; it <= order[2]; ++it)
{
  for (int i2 = 0; i2 <= order[0]; ++i2)
  {
    for (int i1 = 0; i1 <= order[0] - i2; ++i1)
    {
      int i0 = order[0] - i1 - i2;
      basis[idx++] =
        pval[i0] * pval[(order[0] + 1) + i1] * pval[2 * (order[0] + 1) + i2] * ttmp[it];
    }
  }
}
