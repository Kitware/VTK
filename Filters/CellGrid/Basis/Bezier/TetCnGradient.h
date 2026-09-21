// Gradients of the arbitrary-order Bernstein-Bezier basis on the tetrahedron.
//
// See TetCnBasis.h for the basis and TriCnGradient.h for the chain rule; here
// d(lambda0)/dr = -1, d(lambda1)/dr = 1 and the other barycentric coordinates
// are constant along r (and likewise for s and t).
RealT nfact = 1.;
for (int mm = 2; mm <= order[0]; ++mm)
{
  nfact *= RealT(mm);
}

WORKSPACE(RealT, bary, 4);
bary[0] = 1. - rr - ss - tt;
bary[1] = rr;
bary[2] = ss;
bary[3] = tt;

WORKSPACE(RealT, wval, 4 * (order[0] + 1));
for (int kk = 0; kk < 4; ++kk)
{
  int base = kk * (order[0] + 1);
  wval[base] = 1.;
  for (int mm = 1; mm <= order[0]; ++mm)
  {
    wval[base + mm] = wval[base + mm - 1] * bary[kk] / RealT(mm);
  }
}

int idx = 0;
for (int i3 = 0; i3 <= order[0]; ++i3)
{
  for (int i2 = 0; i2 <= order[0] - i3; ++i2)
  {
    for (int i1 = 0; i1 <= order[0] - i2 - i3; ++i1)
    {
      int i0 = order[0] - i1 - i2 - i3;
      RealT w0 = wval[i0];
      RealT w1 = wval[(order[0] + 1) + i1];
      RealT w2 = wval[2 * (order[0] + 1) + i2];
      RealT w3 = wval[3 * (order[0] + 1) + i3];
      RealT d0 = i0 > 0 ? wval[i0 - 1] : 0.;
      RealT d1 = i1 > 0 ? wval[(order[0] + 1) + i1 - 1] : 0.;
      RealT d2 = i2 > 0 ? wval[2 * (order[0] + 1) + i2 - 1] : 0.;
      RealT d3 = i3 > 0 ? wval[3 * (order[0] + 1) + i3 - 1] : 0.;
      basisGradient[idx++] = nfact * w2 * w3 * (d1 * w0 - d0 * w1); // d/dr
      basisGradient[idx++] = nfact * w1 * w3 * (d2 * w0 - d0 * w2); // d/ds
      basisGradient[idx++] = nfact * w1 * w2 * (d3 * w0 - d0 * w3); // d/dt
    }
  }
}
