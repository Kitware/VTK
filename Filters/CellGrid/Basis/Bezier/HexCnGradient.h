// Gradients of the arbitrary-order Bernstein-Bezier basis on the hexahedron.
//
// Each 1-d Bernstein polynomial and its derivative are computed along each
// parametric axis (see EdgeCnBasis.h and EdgeCnGradient.h, including the
// dx/dr = 1/2 factor the change of variables introduces), then combined by the
// product rule.
WORKSPACE(RealT, rtmp, order[0] + 1);
WORKSPACE(RealT, drtmp, order[0] + 1);
WORKSPACE(RealT, stmp, order[1] + 1);
WORKSPACE(RealT, dstmp, order[1] + 1);
WORKSPACE(RealT, ttmp, order[2] + 1);
WORKSPACE(RealT, dttmp, order[2] + 1);

{
  RealT xx = 0.5 * (rr + 1.);
  RealT yy = 1. - xx;
  WORKSPACE(RealT, xpow, order[0] + 1);
  WORKSPACE(RealT, ypow, order[0] + 1);
  xpow[0] = 1.;
  for (int kk = 1; kk <= order[0]; ++kk)
  {
    xpow[kk] = xpow[kk - 1] * xx;
  }
  ypow[order[0]] = 1.;
  for (int kk = order[0] - 1; kk >= 0; --kk)
  {
    ypow[kk] = ypow[kk + 1] * yy;
  }
  RealT coeff = 1.;
  for (int kk = 0; kk <= order[0]; ++kk)
  {
    rtmp[kk] = coeff * xpow[kk] * ypow[kk];
    RealT ddx = 0.;
    if (kk > 0)
    {
      ddx += RealT(kk) * xpow[kk - 1] * ypow[kk];
    }
    if (kk < order[0])
    {
      ddx -= RealT(order[0] - kk) * xpow[kk] * ypow[kk + 1];
    }
    drtmp[kk] = 0.5 * coeff * ddx;
    coeff = coeff * RealT(order[0] - kk) / RealT(kk + 1);
  }
}
{
  RealT xx = 0.5 * (ss + 1.);
  RealT yy = 1. - xx;
  WORKSPACE(RealT, xpow, order[1] + 1);
  WORKSPACE(RealT, ypow, order[1] + 1);
  xpow[0] = 1.;
  for (int kk = 1; kk <= order[1]; ++kk)
  {
    xpow[kk] = xpow[kk - 1] * xx;
  }
  ypow[order[1]] = 1.;
  for (int kk = order[1] - 1; kk >= 0; --kk)
  {
    ypow[kk] = ypow[kk + 1] * yy;
  }
  RealT coeff = 1.;
  for (int kk = 0; kk <= order[1]; ++kk)
  {
    stmp[kk] = coeff * xpow[kk] * ypow[kk];
    RealT ddx = 0.;
    if (kk > 0)
    {
      ddx += RealT(kk) * xpow[kk - 1] * ypow[kk];
    }
    if (kk < order[1])
    {
      ddx -= RealT(order[1] - kk) * xpow[kk] * ypow[kk + 1];
    }
    dstmp[kk] = 0.5 * coeff * ddx;
    coeff = coeff * RealT(order[1] - kk) / RealT(kk + 1);
  }
}
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

// Compute the tensor product of basis derivatives as (d/dr, d/ds, d/dt) tuples:
int idx = 0;
for (int qq = 0; qq <= order[2]; ++qq)
{
  for (int pp = 0; pp <= order[1]; ++pp)
  {
    for (int oo = 0; oo <= order[0]; ++oo)
    {
      basisGradient[idx++] = drtmp[oo] * stmp[pp] * ttmp[qq]; // d/dr
      basisGradient[idx++] = rtmp[oo] * dstmp[pp] * ttmp[qq]; // d/ds
      basisGradient[idx++] = rtmp[oo] * stmp[pp] * dttmp[qq]; // d/dt
    }
  }
}
