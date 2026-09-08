// Gradients of the arbitrary-order Bernstein-Bezier basis on the triangle.
//
// See TriCnBasis.h for the basis. Since w_m = lambda^m / m! differentiates to
// w_{m-1}, the derivative with respect to a barycentric coordinate is just the
// same product with that factor's index shifted down by one. Applying the chain
// rule with d(lambda0)/dr = -1, d(lambda1)/dr = 1 and d(lambda2)/dr = 0 (and
// likewise for s) then gives
//
//   dB/dr = P_{b2} (P'_{b1} P_{b0} - P'_{b0} P_{b1})
RealT nfact = 1.;
for (int mm = 2; mm <= order[0]; ++mm)
{
  nfact *= RealT(mm);
}

WORKSPACE(RealT, bary, 3);
bary[0] = 1. - rr - ss;
bary[1] = rr;
bary[2] = ss;

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
    RealT w0 = wval[i0];
    RealT w1 = wval[(order[0] + 1) + i1];
    RealT w2 = wval[2 * (order[0] + 1) + i2];
    RealT d0 = i0 > 0 ? wval[i0 - 1] : 0.;
    RealT d1 = i1 > 0 ? wval[(order[0] + 1) + i1 - 1] : 0.;
    RealT d2 = i2 > 0 ? wval[2 * (order[0] + 1) + i2 - 1] : 0.;
    basisGradient[idx++] = nfact * w2 * (d1 * w0 - d0 * w1); // d/dr
    basisGradient[idx++] = nfact * w1 * (d2 * w0 - d0 * w2); // d/ds
    basisGradient[idx++] = 0.;
  }
}
