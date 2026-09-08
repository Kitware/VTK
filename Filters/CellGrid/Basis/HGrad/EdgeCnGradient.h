// Derivatives of the arbitrary-order Lagrange interpolants on the edge [-1, 1].
//
// See EdgeCnBasis.h for the definition of the Lagrange points and of invspc.
//
// Differentiating the product that defines the l-th interpolant gives a sum
// over the omitted factor:
//
//   L'_l(rr) = ∑_{k≠l} 1/(x_l - x_k) ∏_{i≠k,l} (rr - x_i)/(x_l - x_i)
//
// The reciprocal of the leading difference is invspc/(l - k) and each factor
// of the product is written exactly as it is in EdgeCnBasis.h.
RealT invspc = RealT(order[0]) / 2.;

for (int ll = 0; ll <= order[0]; ++ll)
{
  RealT termSum = 0.;
  for (int kk = 0; kk <= order[0]; ++kk)
  {
    if (kk == ll)
    {
      continue;
    }
    RealT term = invspc / RealT(ll - kk);
    for (int ii = 0; ii <= order[0]; ++ii)
    {
      if (ii == kk || ii == ll)
      {
        continue;
      }
      term *= (rr * invspc - (RealT(ii) - invspc)) / RealT(ll - ii);
    }
    termSum += term;
  }
  // Like the fixed-order HGrad gradients, store a full 3-tuple per basis
  // function and zero the axes the cell does not parameterize.
  basisGradient[3 * ll + 0] = termSum; // ∂/∂r
  basisGradient[3 * ll + 1] = 0.;
  basisGradient[3 * ll + 2] = 0.;
}
