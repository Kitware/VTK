// Gradients of the arbitrary-order Lagrange interpolants on the hexahedron.
//
// Each 1-d interpolant and its derivative are computed along each parametric
// axis (see EdgeCnBasis.h and EdgeCnGradient.h), then combined by the product
// rule to form the gradient of the tensor-product basis.
RealT invspcR = RealT(order[0]) / 2.;
RealT invspcS = RealT(order[1]) / 2.;
RealT invspcT = RealT(order[2]) / 2.;

WORKSPACE(RealT, rtmp, order[0] + 1);
WORKSPACE(RealT, stmp, order[1] + 1);
WORKSPACE(RealT, ttmp, order[2] + 1);
WORKSPACE(RealT, drtmp, order[0] + 1);
WORKSPACE(RealT, dstmp, order[1] + 1);
WORKSPACE(RealT, dttmp, order[2] + 1);

// Compute the basis and its derivative along the r-axis.
for (int ll = 0; ll <= order[0]; ++ll)
{
  RealT termSum = 0.;
  rtmp[ll] = 1.;
  for (int kk = 0; kk <= order[0]; ++kk)
  {
    if (kk == ll)
    {
      continue;
    }
    rtmp[ll] *= (rr * invspcR - (RealT(kk) - invspcR)) / RealT(ll - kk);
    RealT term = invspcR / RealT(ll - kk);
    for (int ii = 0; ii <= order[0]; ++ii)
    {
      if (ii == kk || ii == ll)
      {
        continue;
      }
      term *= (rr * invspcR - (RealT(ii) - invspcR)) / RealT(ll - ii);
    }
    termSum += term;
  }
  drtmp[ll] = termSum;
}

// Compute the basis and its derivative along the s-axis.
for (int mm = 0; mm <= order[1]; ++mm)
{
  RealT termSum = 0.;
  stmp[mm] = 1.;
  for (int kk = 0; kk <= order[1]; ++kk)
  {
    if (kk == mm)
    {
      continue;
    }
    stmp[mm] *= (ss * invspcS - (RealT(kk) - invspcS)) / RealT(mm - kk);
    RealT term = invspcS / RealT(mm - kk);
    for (int ii = 0; ii <= order[1]; ++ii)
    {
      if (ii == kk || ii == mm)
      {
        continue;
      }
      term *= (ss * invspcS - (RealT(ii) - invspcS)) / RealT(mm - ii);
    }
    termSum += term;
  }
  dstmp[mm] = termSum;
}

// Compute the basis and its derivative along the t-axis.
for (int nn = 0; nn <= order[2]; ++nn)
{
  RealT termSum = 0.;
  ttmp[nn] = 1.;
  for (int kk = 0; kk <= order[2]; ++kk)
  {
    if (kk == nn)
    {
      continue;
    }
    ttmp[nn] *= (tt * invspcT - (RealT(kk) - invspcT)) / RealT(nn - kk);
    RealT term = invspcT / RealT(nn - kk);
    for (int ii = 0; ii <= order[2]; ++ii)
    {
      if (ii == kk || ii == nn)
      {
        continue;
      }
      term *= (tt * invspcT - (RealT(ii) - invspcT)) / RealT(nn - ii);
    }
    termSum += term;
  }
  dttmp[nn] = termSum;
}

// Compute the tensor product of basis derivatives as (∂/∂r, ∂/∂s, ∂/∂t) tuples:
int idx = 0;
for (int nn = 0; nn <= order[2]; ++nn)
{
  for (int mm = 0; mm <= order[1]; ++mm)
  {
    for (int ll = 0; ll <= order[0]; ++ll)
    {
      basisGradient[idx++] = drtmp[ll] * stmp[mm] * ttmp[nn]; // ∂/∂r
      basisGradient[idx++] = rtmp[ll] * dstmp[mm] * ttmp[nn]; // ∂/∂s
      basisGradient[idx++] = rtmp[ll] * stmp[mm] * dttmp[nn]; // ∂/∂t
    }
  }
}
