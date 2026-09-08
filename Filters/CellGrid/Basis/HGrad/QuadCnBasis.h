// Arbitrary-order Lagrange interpolants on the reference quadrilateral.
//
// The basis is the tensor product of the 1-d interpolants along each
// parametric axis; see EdgeCnBasis.h for their definition. The order along
// the r- and s-axes need not match.
RealT invspcR = RealT(order[0]) / 2.;
RealT invspcS = RealT(order[1]) / 2.;

WORKSPACE(RealT, rtmp, order[0] + 1);
WORKSPACE(RealT, stmp, order[1] + 1);

for (int ll = 0; ll <= order[0]; ++ll)
{
  rtmp[ll] = 1.;
  for (int oo = 0; oo <= order[0]; ++oo)
  {
    if (oo == ll)
    {
      continue;
    }
    rtmp[ll] *= (rr * invspcR - (RealT(oo) - invspcR)) / RealT(ll - oo);
  }
}

for (int mm = 0; mm <= order[1]; ++mm)
{
  stmp[mm] = 1.;
  for (int oo = 0; oo <= order[1]; ++oo)
  {
    if (oo == mm)
    {
      continue;
    }
    stmp[mm] *= (ss * invspcS - (RealT(oo) - invspcS)) / RealT(mm - oo);
  }
}

// Compute the tensor product of the 1-dimensional basis functions:
int idx = 0;
for (int mm = 0; mm <= order[1]; ++mm)
{
  for (int ll = 0; ll <= order[0]; ++ll)
  {
    basis[idx++] = rtmp[ll] * stmp[mm];
  }
}
