// Gradients of the arbitrary-order Lagrange interpolants on the quadrilateral.
//
// Each 1-d interpolant and its derivative are computed along each parametric
// axis (see EdgeCnBasis.h and EdgeCnGradient.h), then combined by the product
// rule to form the gradient of the tensor-product basis.
RealT invspcR = RealT(order[0]) / 2.;
RealT invspcS = RealT(order[1]) / 2.;

WORKSPACE(RealT, rtmp, order[0] + 1);
WORKSPACE(RealT, stmp, order[1] + 1);
WORKSPACE(RealT, drtmp, order[0] + 1);
WORKSPACE(RealT, dstmp, order[1] + 1);

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

// Compute the tensor product of basis derivatives. Like the fixed-order HGrad
// gradients, store a full 3-tuple per basis function and zero the axes the cell
// does not parameterize.
int idx = 0;
for (int mm = 0; mm <= order[1]; ++mm)
{
  for (int ll = 0; ll <= order[0]; ++ll)
  {
    basisGradient[idx++] = drtmp[ll] * stmp[mm]; // ∂/∂r
    basisGradient[idx++] = rtmp[ll] * dstmp[mm]; // ∂/∂s
    basisGradient[idx++] = 0.;
  }
}
