// Bernstein-Bezier basis functions of arbitrary order on the reference edge.
//
// The Bernstein polynomials are defined on [0, 1] while the reference edge is
// parameterized over [-1, 1], so the coordinate is mapped with x = (rr + 1)/2:
//
//   B_{n,k}(x) = C(n,k) x^k (1-x)^(n-k)
//
// Unlike a Lagrange basis these do not interpolate their control values except
// at the two ends of the edge, but they are non-negative and sum to one, so the
// curve lies within the convex hull of its control points.
//
// The powers and the binomial coefficient are accumulated rather than computed
// with pow() and a binomial() helper. Neither is portable to GLSL, where these
// kernels are also compiled: there is no binomial(), and pow(0, 0) is undefined
// there - which is exactly what the ends of the edge would ask for.
RealT xx = 0.5 * (rr + 1.);
RealT yy = 1. - xx;

WORKSPACE(RealT, xpow, order[0] + 1);
WORKSPACE(RealT, ypow, order[0] + 1);
// xpow[k] holds x^k and ypow[k] holds (1-x)^(n-k).
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

// C(n,k+1) = C(n,k) (n-k)/(k+1), so the coefficient comes along for free.
RealT coeff = 1.;
for (int kk = 0; kk <= order[0]; ++kk)
{
  basis[kk] = coeff * xpow[kk] * ypow[kk];
  coeff = coeff * RealT(order[0] - kk) / RealT(kk + 1);
}
