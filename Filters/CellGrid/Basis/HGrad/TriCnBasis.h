// Arbitrary-order Lagrange interpolants on the reference triangle.
//
// The Lagrange points form the principal lattice: those whose barycentric
// coordinates are all multiples of 1/n. Each interpolant is a product of
// Silvester polynomials, one per barycentric coordinate,
//
//   P_m(x) = ∏_{a=0}^{m-1} (n x - a) / (m - a),
//
// selected by a multi-index (i0, i1, i2) of non-negative integers summing to
// the order n. P_m has degree m, so each interpolant has total degree n, and
// P_m(j/n) is 1 when j == m and 0 when j < m, which gives the Kronecker delta
// property, since two distinct multi-indices summing to n must have some
// component where the first is the larger.
//
// The multi-index is enumerated with i1 (the r-axis) varying fastest, matching
// the lexicographic convention the prismatic Cn bases use.
RealT nn = RealT(order[0]);

WORKSPACE(RealT, bary, 3);
bary[0] = 1. - rr - ss;
bary[1] = rr;
bary[2] = ss;

// Tabulate P_m for every barycentric coordinate and every degree up to n,
// using P_m(x) = P_{m-1}(x) * (n x - (m - 1)) / m.
WORKSPACE(RealT, pval, 3 * (order[0] + 1));
for (int kk = 0; kk < 3; ++kk)
{
  int base = kk * (order[0] + 1);
  pval[base] = 1.;
  for (int mm = 1; mm <= order[0]; ++mm)
  {
    RealT rmm = RealT(mm);
    pval[base + mm] = pval[base + mm - 1] * (nn * bary[kk] - (rmm - 1.)) / rmm;
  }
}

int idx = 0;
for (int i2 = 0; i2 <= order[0]; ++i2)
{
  for (int i1 = 0; i1 <= order[0] - i2; ++i1)
  {
    int i0 = order[0] - i1 - i2;
    basis[idx++] = pval[i0] * pval[(order[0] + 1) + i1] * pval[2 * (order[0] + 1) + i2];
  }
}
