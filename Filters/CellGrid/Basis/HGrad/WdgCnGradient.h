// Gradients of the arbitrary-order Lagrange interpolants on the wedge.
//
// See WdgCnBasis.h for the interpolants. The r- and s-derivatives come from the
// triangle factor (TriCnGradient.h) scaled by the edge factor, and the
// t-derivative from the edge factor (EdgeCnGradient.h) scaled by the triangle.
RealT nn = RealT(order[0]);
RealT invspcT = RealT(order[2]) / 2.;

WORKSPACE(RealT, bary, 3);
bary[0] = 1. - rr - ss;
bary[1] = rr;
bary[2] = ss;

WORKSPACE(RealT, pval, 3 * (order[0] + 1));
WORKSPACE(RealT, pder, 3 * (order[0] + 1));
for (int kk = 0; kk < 3; ++kk)
{
  int base = kk * (order[0] + 1);
  pval[base] = 1.;
  pder[base] = 0.;
  for (int mm = 1; mm <= order[0]; ++mm)
  {
    RealT rmm = RealT(mm);
    RealT factor = nn * bary[kk] - (rmm - 1.);
    pder[base + mm] = (pder[base + mm - 1] * factor + pval[base + mm - 1] * nn) / rmm;
    pval[base + mm] = pval[base + mm - 1] * factor / rmm;
  }
}

WORKSPACE(RealT, ttmp, order[2] + 1);
WORKSPACE(RealT, dttmp, order[2] + 1);
for (int ll = 0; ll <= order[2]; ++ll)
{
  RealT termSum = 0.;
  ttmp[ll] = 1.;
  for (int kk = 0; kk <= order[2]; ++kk)
  {
    if (kk == ll)
    {
      continue;
    }
    ttmp[ll] *= (tt * invspcT - (RealT(kk) - invspcT)) / RealT(ll - kk);
    RealT term = invspcT / RealT(ll - kk);
    for (int ii = 0; ii <= order[2]; ++ii)
    {
      if (ii == kk || ii == ll)
      {
        continue;
      }
      term *= (tt * invspcT - (RealT(ii) - invspcT)) / RealT(ll - ii);
    }
    termSum += term;
  }
  dttmp[ll] = termSum;
}

int idx = 0;
for (int it = 0; it <= order[2]; ++it)
{
  for (int i2 = 0; i2 <= order[0]; ++i2)
  {
    for (int i1 = 0; i1 <= order[0] - i2; ++i1)
    {
      int i0 = order[0] - i1 - i2;
      RealT p0 = pval[i0];
      RealT p1 = pval[(order[0] + 1) + i1];
      RealT p2 = pval[2 * (order[0] + 1) + i2];
      RealT d0 = pder[i0];
      RealT d1 = pder[(order[0] + 1) + i1];
      RealT d2 = pder[2 * (order[0] + 1) + i2];
      basisGradient[idx++] = ttmp[it] * p2 * (d1 * p0 - d0 * p1); // ∂/∂r
      basisGradient[idx++] = ttmp[it] * p1 * (d2 * p0 - d0 * p2); // ∂/∂s
      basisGradient[idx++] = dttmp[it] * p0 * p1 * p2;            // ∂/∂t
    }
  }
}
