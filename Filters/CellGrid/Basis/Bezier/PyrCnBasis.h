// Bernstein-Bezier basis functions of arbitrary order on the reference pyramid.
//
// A pyramid's reference cell (see vtkDGPyr::Parameters) has its quadrilateral
// base at t = 0, r,s in [-w, w], and its apex at t = 1, where w = 1 - t is the
// base's own half-width at that height - the cross-section shrinks to a point
// as t goes from 0 to 1. Writing a = (r + w) / (2w), b = (s + w) / (2w), c = t
// undoes that collapse: a, b, c range over the unit cube independently of one
// another, wherever w != 0 (every point but the apex itself).
//
// The basis of J. Chan and T. Warburton, "A short note on a Bernstein-Bezier
// basis for the pyramid" (arXiv:1508.05609, 2015), is then simply a product of
// ordinary 1-d Bernstein polynomials of a, b, and c - proven non-negative and
// summing to one, so control values still bound the field within their convex
// hull, the same as every other Bezier basis in this file - except the a- and
// b-axis polynomials drop from degree n - k to degree 0 as k (the c-axis
// index) climbs from 0 to n, shrinking in step with the cross-section itself:
//
//   B_ijk(a, b, c) = B_i^(n-k)(a) B_j^(n-k)(b) B_k^n(c),  0 <= k <= n,
//                                                         0 <= i, j <= n - k
//
// Composed with a = (r + w) / (2w) etc., this is a rational function of r, s,
// t rather than a polynomial one - the same 1/w singularity every rational
// pyramidal basis has at the apex - but it is exactly this basis, just
// expressed in vtkDGPyr's own reference coordinates instead of Chan and
// Warburton's unit cube, and it keeps their proven convex-hull property: a,
// b, c remain in [0, 1] at every point of the pyramid, apex included in the
// limit, so the product of Bernstein polynomials is still non-negative and
// sums to one everywhere.
//
// order[0], order[1], and order[2] are always equal (see
// vtkDGOperatorEntry::PyramidFunctionCount) - there is a single degree n, not
// one per axis - but every workspace below is still sized off order[2] alone
// (rather than a loop-local variable like n - k) so its size stays a literal
// constant after PyrCnBasis is compiled to GLSL, which requires compile-time
// array bounds; only the first n - k + 1 entries of each are used per k.
//
// The degrees of freedom are enumerated with i (the a-axis) varying fastest,
// then j (the b-axis), then k (the c-axis) slowest, matching every other
// prismatic shape in this module.
if (abs(tt - 1.) < eps)
{
  tt = tt <= 1. ? 1. - eps : 1. + eps;
}
RealT ww = 1. - tt;

WORKSPACE(RealT, ctmp, order[2] + 1);
{
  WORKSPACE(RealT, xpow, order[2] + 1);
  WORKSPACE(RealT, ypow, order[2] + 1);
  xpow[0] = 1.;
  for (int kk = 1; kk <= order[2]; ++kk)
  {
    xpow[kk] = xpow[kk - 1] * tt;
  }
  ypow[order[2]] = 1.;
  for (int kk = order[2] - 1; kk >= 0; --kk)
  {
    ypow[kk] = ypow[kk + 1] * ww;
  }
  RealT coeff = 1.;
  for (int kk = 0; kk <= order[2]; ++kk)
  {
    ctmp[kk] = coeff * xpow[kk] * ypow[kk];
    coeff = coeff * RealT(order[2] - kk) / RealT(kk + 1);
  }
}

int idx = 0;
for (int kk = 0; kk <= order[2]; ++kk)
{
  int mm = order[2] - kk;
  RealT aa = (rr + ww) / (2. * ww);
  RealT bb = (ss + ww) / (2. * ww);

  WORKSPACE(RealT, atmp, order[2] + 1);
  WORKSPACE(RealT, btmp, order[2] + 1);
  {
    WORKSPACE(RealT, xpow, order[2] + 1);
    WORKSPACE(RealT, ypow, order[2] + 1);
    xpow[0] = 1.;
    for (int ii = 1; ii <= mm; ++ii)
    {
      xpow[ii] = xpow[ii - 1] * aa;
    }
    ypow[mm] = 1.;
    for (int ii = mm - 1; ii >= 0; --ii)
    {
      ypow[ii] = ypow[ii + 1] * (1. - aa);
    }
    RealT coeff = 1.;
    for (int ii = 0; ii <= mm; ++ii)
    {
      atmp[ii] = coeff * xpow[ii] * ypow[ii];
      coeff = coeff * RealT(mm - ii) / RealT(ii + 1);
    }
  }
  {
    WORKSPACE(RealT, xpow, order[2] + 1);
    WORKSPACE(RealT, ypow, order[2] + 1);
    xpow[0] = 1.;
    for (int jj = 1; jj <= mm; ++jj)
    {
      xpow[jj] = xpow[jj - 1] * bb;
    }
    ypow[mm] = 1.;
    for (int jj = mm - 1; jj >= 0; --jj)
    {
      ypow[jj] = ypow[jj + 1] * (1. - bb);
    }
    RealT coeff = 1.;
    for (int jj = 0; jj <= mm; ++jj)
    {
      btmp[jj] = coeff * xpow[jj] * ypow[jj];
      coeff = coeff * RealT(mm - jj) / RealT(jj + 1);
    }
  }

  for (int jj = 0; jj <= mm; ++jj)
  {
    for (int ii = 0; ii <= mm; ++ii)
    {
      basis[idx++] = atmp[ii] * btmp[jj] * ctmp[kk];
    }
  }
}
