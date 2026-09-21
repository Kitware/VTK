// Bernstein-Bezier basis functions of arbitrary order on the reference wedge.
//
// A wedge is the product of a triangle (the r- and s-axes) with an edge (the
// t-axis), so its basis is the product of the triangle's barycentric Bernstein
// polynomials - see TriCnBasis.h - with the edge's 1-d ones - see
// EdgeCnBasis.h. The two factors may have different orders, but the triangle
// has a single total degree shared by its two axes: order[0] is the order of
// the triangular cross-section and order[2] the order along t.
//
// The degrees of freedom are enumerated with the r-axis varying fastest and t
// slowest, so the triangle's degrees of freedom repeat once per control point
// along t.
RealT nfact = 1.;
for (int mm = 2; mm <= order[0]; ++mm)
{
  nfact *= RealT(mm);
}

WORKSPACE(RealT, bary, 3);
bary[0] = 1. - rr - ss;
bary[1] = rr;
bary[2] = ss;

WORKSPACE(RealT, wval, 3 * (order[0] + 1));
for (int kk = 0; kk < 3; ++kk)
{
  int base = kk * (order[0] + 1);
  wval[base] = 1.;
  for (int mm = 1; mm <= order[0]; ++mm)
  {
    wval[base + mm] = wval[base + mm - 1] * bary[kk] / RealT(mm);
  }
}

WORKSPACE(RealT, ttmp, order[2] + 1);
{
  RealT xx = 0.5 * (tt + 1.);
  RealT yy = 1. - xx;
  WORKSPACE(RealT, xpow, order[2] + 1);
  WORKSPACE(RealT, ypow, order[2] + 1);
  xpow[0] = 1.;
  for (int kk = 1; kk <= order[2]; ++kk)
  {
    xpow[kk] = xpow[kk - 1] * xx;
  }
  ypow[order[2]] = 1.;
  for (int kk = order[2] - 1; kk >= 0; --kk)
  {
    ypow[kk] = ypow[kk + 1] * yy;
  }
  RealT coeff = 1.;
  for (int kk = 0; kk <= order[2]; ++kk)
  {
    ttmp[kk] = coeff * xpow[kk] * ypow[kk];
    coeff = coeff * RealT(order[2] - kk) / RealT(kk + 1);
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
        nfact * wval[i0] * wval[(order[0] + 1) + i1] * wval[2 * (order[0] + 1) + i2] * ttmp[it];
    }
  }
}
