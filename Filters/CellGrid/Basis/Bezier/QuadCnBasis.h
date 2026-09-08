// Bernstein-Bezier basis functions of arbitrary order on the reference quadrilateral.
//
// The basis is the tensor product of the 1-d Bernstein polynomials along each
// parametric axis; see EdgeCnBasis.h for their definition and for why the
// powers and binomial coefficients are accumulated rather than computed with
// pow(). The order along each axis is independent.
WORKSPACE(RealT, rtmp, order[0] + 1);
WORKSPACE(RealT, stmp, order[1] + 1);

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
    coeff = coeff * RealT(order[1] - kk) / RealT(kk + 1);
  }
}

// Compute the tensor product of the 1-dimensional basis functions:
int idx = 0;
for (int pp = 0; pp <= order[1]; ++pp)
{
  for (int oo = 0; oo <= order[0]; ++oo)
  {
    basis[idx++] = rtmp[oo] * stmp[pp];
  }
}
