// Gradients of the arbitrary-order Lagrange interpolants on the triangle.
//
// See TriCnBasis.h for the definition of the interpolants. Differentiating
// the product of Silvester polynomials by the chain rule, and using
// ∂λ0/∂r = -1, ∂λ1/∂r = 1, ∂λ2/∂r = 0 (and likewise for s), gives
//
//   ∂L/∂r = P_{i2}(λ2) (P'_{i1}(λ1) P_{i0}(λ0) - P'_{i0}(λ0) P_{i1}(λ1))
//   ∂L/∂s = P_{i1}(λ1) (P'_{i2}(λ2) P_{i0}(λ0) - P'_{i0}(λ0) P_{i2}(λ2))
RealT nn = RealT(order[0]);

WORKSPACE(RealT, bary, 3);
bary[0] = 1. - rr - ss;
bary[1] = rr;
bary[2] = ss;

// Tabulate P_m and its derivative for every barycentric coordinate and every
// degree up to n. Differentiating the recurrence used in TriCnBasis.h gives
// P'_m(x) = (P'_{m-1}(x) (n x - (m - 1)) + P_{m-1}(x) n) / m.
WORKSPACE(RealT, pval, 3 * (order[0] + 1));
WORKSPACE(RealT, pder, 3 * (order[0] + 1));
for (int kk = 0; kk < 3; ++kk)
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
for (int i2 = 0; i2 <= order[0]; ++i2)
{
  for (int i1 = 0; i1 <= order[0] - i2; ++i1)
  {
    int i0 = order[0] - i1 - i2;
    RealT p0 = pval[i0];
    RealT p1 = pval[(order[0] + 1) + i1];
    RealT p2 = pval[2 * (order[0] + 1) + i2];
    RealT d0 = pder[i0];
    RealT d1 = pder[(order[0] + 1) + i1];
    RealT d2 = pder[2 * (order[0] + 1) + i2];
    basisGradient[idx++] = p2 * (d1 * p0 - d0 * p1); // ∂/∂r
    basisGradient[idx++] = p1 * (d2 * p0 - d0 * p2); // ∂/∂s
    basisGradient[idx++] = 0.;
  }
}
