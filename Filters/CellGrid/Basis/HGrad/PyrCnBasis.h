// Arbitrary-order Lagrange interpolants on the reference pyramid.
//
// A pyramid's reference cell (see vtkDGPyr::Parameters) has its quadrilateral
// base at t = 0, r,s in [-w, w], and its apex at t = 1, where w = 1 - t is the
// base's own half-width at that height. Writing a = (r + w)/(2w),
// b = (s + w)/(2w), c = t undoes that collapse, and in those coordinates the
// order-n pyramid space is
//
//   V_n = span{ a^p b^q (1-c)^max(p,q) g(c) : deg g <= n - max(p,q) },
//
// that is: the coefficient of a^p b^q is a polynomial in c of degree at most
// n that is divisible by (1-c)^max(p,q). (At n = 1 this is the classical
// rational space span{1, r, s, t, rs/(1-t)}; see PyrC1Basis.h.)
//
// That is the rational pyramid space of M. Bergot, G. Cohen and M. Durufle,
// "Higher-order Finite Elements for Hybrid Meshes Using New Nodal Pyramidal
// Elements", J. Sci. Comput. 42:345-381 (2010), doi:10.1007/s10915-009-9334-9;
// its dimension is (n+1)(n+2)(2n+3)/6, which is what
// vtkDGOperatorEntry::PyramidFunctionCount returns, and PyrGnBasis.h and
// Basis/Bezier/PyrCnBasis.h span the same space at the same order. The space is
// rational because a conforming pyramid element cannot be polynomial: its
// quadrilateral face must carry the same traces as a hexahedron's and its
// triangular faces the same as a tetrahedron's, which no single polynomial
// space does (N. Nigam and J. Phillips, "High-order conforming finite elements
// on pyramids", IMA J. Numer. Anal. 32:448-483, 2012, prove it). So every
// pyramid basis in this directory divides by 1 - t, PyrC1Basis.h and
// PyrC2Basis.h included; this one is no more rational than they are. The paper
// is the reference for the space; the particular nodal construction below - the
// spanning set psi_{i,j,d}, the layered lattice and the block back substitution
// over it - was worked out for this file rather than transcribed from it, and
// is one of several ways to interpolate on that space.
//
// The degrees of freedom sit on a layered lattice: layer d (0 <= d <= n) lies
// at c = d/n and carries the uniform (m+1)^2 grid a_i = i/m, b_j = j/m with
// m = n - d - the cross-section's own node count shrinking with it, to one node
// at the apex. This is the lattice vtkDGLagrangePoints reports.
//
// Unlike the wedge (see WdgCnBasis.h), no product mu(a) nu(b) phi(c) can be
// one of these interpolants once m >= 2: mu and nu would be the layer's
// degree-m Lagrange functions, whose a^m b^m term forces (1-c)^m | phi, while
// phi must also vanish at the other n-1 layers - 2n-d-1 > n roots in a
// polynomial of degree at most n. So the interpolants genuinely mix the (a, b)
// and c directions, and are built here instead by solving for them.
//
// Take the spanning set of V_n
//
//   psi_{i,j,d}(a, b, c) = l_i(a) l_j(b) (1-c)^m omega_d(c),
//   omega_d(c) = prod_{k<d} (c - k/n),   0 <= i,j <= m = n-d,  0 <= d <= n,
//
// where l_i is the degree-m Lagrange polynomial on layer d's own grid. Each
// psi lies in V_n because its a^p b^q coefficient carries the full (1-c)^m,
// and m >= max(p,q); its degree in c is m + d = n. The d-th group vanishes on
// every layer below d, so ordering the nodes by layer and the functions by d
// makes the collocation matrix V (V[node][function]) block lower-triangular,
// and - this being the point of using Lagrange factors rather than monomials -
// its d-th diagonal block is the *diagonal* matrix (1-c_d)^m omega_d(c_d) I,
// since l_i is 1 at layer d's i-th node and 0 at the others. The interpolants
// are then the solution of V^T L(x) = psi(x) by block back substitution from
// d = n down to 0, with nothing to invert but that scalar.
//
// Using monomials a^p b^q in place of l_i l_j spans the same space and gives
// the same interpolants in exact arithmetic, but makes each diagonal block a
// tensor-product Vandermonde matrix, and inverting one of those through the
// Lagrange polynomials' monomial coefficients loses roughly a digit per order:
// that costs 1e-3 by order 8 and 0.2 by order 9, with partition of unity
// holding throughout, so the basis would stop interpolating without looking
// wrong. Nothing here forms a monomial coefficient at all - l_i is evaluated as
// the product prod_{k!=i} (x - x_k)/(x_i - x_k), whose relative error stays at
// rounding - and the worst |L_i(x_j) - delta_ij| over the lattice measures
// 2e-15 at order 4, 3e-15 at 6, 1e-14 at 9, 1e-13 at 12 and 1e-9 at 20. What
// growth is left is the Lebesgue constant of equispaced nodes, which the
// hexahedron's and wedge's bases have too (HexCnBasis.h measures 7e-15 at
// order 12 and 9e-13 at 20 on the same test), so this basis needs no order
// bound of its own either.
//
// Everything the substitution needs is closed form: omega_d(c_K) = K!/((K-d)! n^d)
// and (1-c_K)^M = ((n-K)/n)^M.
//
// The degrees of freedom are enumerated with i (the a-axis) varying fastest,
// then j (the b-axis), then d (the layer) slowest, matching every other
// arbitrary-order basis in this module. order[0], order[1] and order[2] are
// always equal (see vtkDGOperatorEntry::PyramidFunctionCount) - a single
// degree n, not one per axis - and every workspace is sized off order[2]
// alone so its size stays a literal constant once compiled to GLSL.
if (abs(tt - 1.) < eps)
{
  tt = tt <= 1. ? 1. - eps : 1. + eps;
}
RealT ww = 1. - tt;
RealT aa = (rr + ww) / (2. * ww);
RealT bb = (ss + ww) / (2. * ww);

// Where each group of order[2] - d + 1 squared functions (and each layer's
// equally many nodes) begins.
WORKSPACE(int, offset, order[2] + 2);
offset[0] = 0;
for (int dd = 0; dd <= order[2]; ++dd)
{
  int mm = order[2] - dd;
  offset[dd + 1] = offset[dd] + (mm + 1) * (mm + 1);
}

// (1-c)^M for M = 0 .. n, at the point being evaluated.
WORKSPACE(RealT, wpow, order[2] + 1);
wpow[0] = 1.;
for (int pp = 1; pp <= order[2]; ++pp)
{
  wpow[pp] = wpow[pp - 1] * ww;
}

// omega_d(c), the Newton basis on the layer heights c_k = k/n.
WORKSPACE(RealT, omega, order[2] + 1);
omega[0] = 1.;
for (int dd = 1; dd <= order[2]; ++dd)
{
  omega[dd] = omega[dd - 1] * (tt - RealT(dd - 1) / RealT(order[2]));
}

WORKSPACE(RealT, la, order[2] + 1);
WORKSPACE(RealT, lb, order[2] + 1);
WORKSPACE(RealT, resamp, (order[2] + 1) * (order[2] + 1));
WORKSPACE(RealT, rhs, (order[2] + 1) * (order[2] + 1));
WORKSPACE(RealT, tmp, (order[2] + 1) * (order[2] + 1));

// Back substitution. Layers are solved from the apex down; by the time layer d
// is reached every layer above it is known, so its own right-hand side is psi
// minus what those already account for.
for (int dd = order[2]; dd >= 0; --dd)
{
  int mm = order[2] - dd;

  // Layer d's Lagrange polynomials at the point being evaluated, each as the
  // product prod_{k!=i} (x - x_k)/(x_i - x_k) over its own grid x_k = k/m.
  for (int ii = 0; ii <= mm; ++ii)
  {
    la[ii] = 1.;
    lb[ii] = 1.;
    for (int kk = 0; kk <= mm; ++kk)
    {
      if (kk == ii)
      {
        continue;
      }
      RealT xk = RealT(kk) / RealT(mm);
      RealT scale = RealT(mm) / RealT(ii - kk);
      la[ii] *= (aa - xk) * scale;
      lb[ii] *= (bb - xk) * scale;
    }
  }
  for (int jj = 0; jj <= mm; ++jj)
  {
    for (int ii = 0; ii <= mm; ++ii)
    {
      rhs[jj * (mm + 1) + ii] = la[ii] * lb[jj] * wpow[mm] * omega[dd];
    }
  }

  for (int KK = dd + 1; KK <= order[2]; ++KK)
  {
    int mK = order[2] - KK;

    // Layer d's Lagrange polynomials resampled onto layer K's coarser grid:
    // resamp[i * (mK + 1) + i2] = l_i(i2 / mK). Every entry is again a plain
    // product, so a node the two grids share lands on an exact 0 or 1.
    for (int ii = 0; ii <= mm; ++ii)
    {
      for (int i2 = 0; i2 <= mK; ++i2)
      {
        RealT yy = mK > 0 ? RealT(i2) / RealT(mK) : 0.;
        RealT value = 1.;
        for (int kk = 0; kk <= mm; ++kk)
        {
          if (kk == ii)
          {
            continue;
          }
          value *= (yy - RealT(kk) / RealT(mm)) * RealT(mm) / RealT(ii - kk);
        }
        resamp[ii * (mK + 1) + i2] = value;
      }
    }

    // psi_{i,j,d} at layer K's nodes shares the factor omega_d(c_K) (1-c_K)^m.
    RealT om = 1.;
    for (int kk = 0; kk < dd; ++kk)
    {
      om *= RealT(KK - kk) / RealT(order[2]);
    }
    RealT wK = order[2] > 0 ? RealT(mK) / RealT(order[2]) : 1.;
    RealT wKpow = 1.;
    for (int pp = 1; pp <= mm; ++pp)
    {
      wKpow *= wK;
    }
    RealT weight = om * wKpow;

    // Contract layer K's known interpolants against that resampling, one axis
    // at a time, rather than forming the block itself.
    for (int ii = 0; ii <= mm; ++ii)
    {
      for (int j2 = 0; j2 <= mK; ++j2)
      {
        RealT sum = 0.;
        for (int i2 = 0; i2 <= mK; ++i2)
        {
          sum += resamp[ii * (mK + 1) + i2] * basis[offset[KK] + j2 * (mK + 1) + i2];
        }
        tmp[ii * (order[2] + 1) + j2] = sum;
      }
    }
    for (int jj = 0; jj <= mm; ++jj)
    {
      for (int ii = 0; ii <= mm; ++ii)
      {
        RealT sum = 0.;
        for (int j2 = 0; j2 <= mK; ++j2)
        {
          sum += resamp[jj * (mK + 1) + j2] * tmp[ii * (order[2] + 1) + j2];
        }
        rhs[jj * (mm + 1) + ii] -= weight * sum;
      }
    }
  }

  // The diagonal block is (1-c_d)^m omega_d(c_d) times the identity, so the
  // whole of this layer's solve is one division.
  RealT wd = order[2] > 0 ? RealT(mm) / RealT(order[2]) : 1.;
  RealT denom = 1.;
  for (int pp = 1; pp <= mm; ++pp)
  {
    denom *= wd;
  }
  for (int kk = 0; kk < dd; ++kk)
  {
    denom *= RealT(dd - kk) / RealT(order[2]);
  }
  for (int jj = 0; jj <= mm; ++jj)
  {
    for (int ii = 0; ii <= mm; ++ii)
    {
      basis[offset[dd] + jj * (mm + 1) + ii] = rhs[jj * (mm + 1) + ii] / denom;
    }
  }
}
