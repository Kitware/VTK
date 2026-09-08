// Gradients of the arbitrary-order Bernstein-Bezier basis on the wedge.
//
// See WdgCnBasis.h for the basis. The r- and s-derivatives come from the
// triangle factor (TriCnGradient.h) scaled by the edge factor, and the
// t-derivative from the edge factor (EdgeCnGradient.h, including the dx/dt =
// 1/2 that the change of variables introduces) scaled by the triangle.
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
WORKSPACE(RealT, dttmp, order[2] + 1);
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
    RealT ddx = 0.;
    if (kk > 0)
    {
      ddx += RealT(kk) * xpow[kk - 1] * ypow[kk];
    }
    if (kk < order[2])
    {
      ddx -= RealT(order[2] - kk) * xpow[kk] * ypow[kk + 1];
    }
    dttmp[kk] = 0.5 * coeff * ddx;
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
      RealT w0 = wval[i0];
      RealT w1 = wval[(order[0] + 1) + i1];
      RealT w2 = wval[2 * (order[0] + 1) + i2];
      RealT d0 = i0 > 0 ? wval[i0 - 1] : 0.;
      RealT d1 = i1 > 0 ? wval[(order[0] + 1) + i1 - 1] : 0.;
      RealT d2 = i2 > 0 ? wval[2 * (order[0] + 1) + i2 - 1] : 0.;
      basisGradient[idx++] = nfact * ttmp[it] * w2 * (d1 * w0 - d0 * w1); // d/dr
      basisGradient[idx++] = nfact * ttmp[it] * w1 * (d2 * w0 - d0 * w2); // d/ds
      basisGradient[idx++] = nfact * dttmp[it] * w0 * w1 * w2;            // d/dt
    }
  }
}
