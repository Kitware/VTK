// Gradients of the arbitrary-order Lagrange interpolants on the tetrahedron.
//
// See TetCnBasis.h for the interpolants and TriCnGradient.h for the chain rule;
// here ∂λ0/∂r = -1, ∂λ1/∂r = 1 and the other barycentric coordinates are
// constant along r (and likewise for s and t), so
//
//   ∂L/∂r = P_{i2} P_{i3} (P'_{i1} P_{i0} - P'_{i0} P_{i1})
RealT nn = RealT(order[0]);

WORKSPACE(RealT, bary, 4);
bary[0] = 1. - rr - ss - tt;
bary[1] = rr;
bary[2] = ss;
bary[3] = tt;

WORKSPACE(RealT, pval, 4 * (order[0] + 1));
WORKSPACE(RealT, pder, 4 * (order[0] + 1));
for (int kk = 0; kk < 4; ++kk)
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

int idx = 0;
for (int i3 = 0; i3 <= order[0]; ++i3)
{
  for (int i2 = 0; i2 <= order[0] - i3; ++i2)
  {
    for (int i1 = 0; i1 <= order[0] - i2 - i3; ++i1)
    {
      int i0 = order[0] - i1 - i2 - i3;
      RealT p0 = pval[i0];
      RealT p1 = pval[(order[0] + 1) + i1];
      RealT p2 = pval[2 * (order[0] + 1) + i2];
      RealT p3 = pval[3 * (order[0] + 1) + i3];
      RealT d0 = pder[i0];
      RealT d1 = pder[(order[0] + 1) + i1];
      RealT d2 = pder[2 * (order[0] + 1) + i2];
      RealT d3 = pder[3 * (order[0] + 1) + i3];
      basisGradient[idx++] = p2 * p3 * (d1 * p0 - d0 * p1); // ∂/∂r
      basisGradient[idx++] = p1 * p3 * (d2 * p0 - d0 * p2); // ∂/∂s
      basisGradient[idx++] = p1 * p2 * (d3 * p0 - d0 * p3); // ∂/∂t
    }
  }
}
