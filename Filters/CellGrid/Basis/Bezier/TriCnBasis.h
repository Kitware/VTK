// Bernstein-Bezier basis functions of arbitrary order on the reference triangle.
//
// The barycentric Bernstein polynomials of degree n are indexed by a
// multi-index (b0, b1, b2) of non-negative integers summing to n:
//
//   B_b(lambda) = n!/(b0! b1! b2!) lambda0^b0 lambda1^b1 lambda2^b2
//
// Writing this as n! * prod_t (lambda_t^{b_t} / b_t!) lets each factor be built
// by the recurrence w_m = w_{m-1} lambda / m, which avoids pow() and any
// factorial helper - neither of which is portable to the GLSL these kernels are
// also compiled to.
//
// Unlike the Lagrange basis these do not interpolate their control values
// except at the three corners, but they are non-negative and sum to one, so the
// surface lies within the convex hull of its control points.
//
// The multi-index is enumerated with b1 (the r-axis) varying fastest, matching
// the convention the Lagrange bases use. Note that the triangle's parameter
// space is the barycentric one, so unlike the prismatic shapes there is no
// change of variables here.
RealT nfact = 1.;
for (int mm = 2; mm <= order[0]; ++mm)
{
  nfact *= RealT(mm);
}

WORKSPACE(RealT, bary, 3);
bary[0] = 1. - rr - ss;
bary[1] = rr;
bary[2] = ss;

// wval[k * (n + 1) + m] holds lambda_k^m / m!.
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

int idx = 0;
for (int i2 = 0; i2 <= order[0]; ++i2)
{
  for (int i1 = 0; i1 <= order[0] - i2; ++i1)
  {
    int i0 = order[0] - i1 - i2;
    basis[idx++] = nfact * wval[i0] * wval[(order[0] + 1) + i1] * wval[2 * (order[0] + 1) + i2];
  }
}
