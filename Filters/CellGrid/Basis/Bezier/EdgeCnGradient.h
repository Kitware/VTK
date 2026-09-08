// Gradients of the arbitrary-order Bernstein-Bezier basis on the edge.
//
// See EdgeCnBasis.h for the basis and the change of variables. Differentiating
// the monomials directly, and remembering that d(1-x)/dx = -1,
//
//   d/dx [x^k (1-x)^(n-k)] = k x^(k-1) (1-x)^(n-k) - (n-k) x^k (1-x)^(n-k-1),
//
// which the tables below supply with a shift of index. The reference edge is
// parameterized over [-1, 1] and the Bernstein polynomials over [0, 1], so
// every derivative also picks up the factor dx/dr = 1/2.
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
  RealT ddx = 0.;
  if (kk > 0)
  {
    ddx += RealT(kk) * xpow[kk - 1] * ypow[kk];
  }
  if (kk < order[0])
  {
    ddx -= RealT(order[0] - kk) * xpow[kk] * ypow[kk + 1];
  }
  // Like the fixed-order HGrad gradients, store a full 3-tuple per basis
  // function and zero the axes the cell does not parameterize.
  basisGradient[3 * kk + 0] = 0.5 * coeff * ddx; // d/dr
  basisGradient[3 * kk + 1] = 0.;
  basisGradient[3 * kk + 2] = 0.;
  coeff = coeff * RealT(order[0] - kk) / RealT(kk + 1);
}
