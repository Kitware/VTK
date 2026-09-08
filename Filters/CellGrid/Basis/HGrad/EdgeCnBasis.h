// Lagrange interpolants of arbitrary order on the reference edge [-1, 1].
//
// The order[0]+1 Lagrange points are uniformly spaced, with
//   x_o = -1 + 2*o/order[0]  for o in [0, order[0]].
//
// \a invspc is the inverse of the distance between Lagrange points.
// Note that formulae below are written in terms of invspc rather than the
// spacing itself: invspc always has 2 for a denominator and so is exactly
// representable, while the spacing has order[0] for a denominator and is not.
//
//   (rr - x_o) / (x_l - x_o) = (rr * invspc - (o - invspc)) / (l - o)
RealT invspc = RealT(order[0]) / 2.;

for (int ll = 0; ll <= order[0]; ++ll)
{
  basis[ll] = 1.;
  for (int oo = 0; oo <= order[0]; ++oo)
  {
    if (oo == ll)
    {
      continue;
    }
    basis[ll] *= (rr * invspc - (RealT(oo) - invspc)) / RealT(ll - oo);
  }
}
