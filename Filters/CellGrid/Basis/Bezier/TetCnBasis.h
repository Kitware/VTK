// Bernstein-Bezier basis functions of arbitrary order on the reference
// tetrahedron.
//
// This is the three-dimensional case of the construction described in
// TriCnBasis.h: n! times a product of lambda_t^{b_t} / b_t! over the four
// barycentric coordinates, selected by a multi-index summing to the order n and
// enumerated with b1 (the r-axis) varying fastest.
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
      basis[idx++] = nfact * wval[i0] * wval[(order[0] + 1) + i1] * wval[2 * (order[0] + 1) + i2] *
        wval[3 * (order[0] + 1) + i3];
    }
  }
}
