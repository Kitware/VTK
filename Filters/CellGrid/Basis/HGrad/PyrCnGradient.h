// Gradients of the arbitrary-order Lagrange interpolants on the pyramid.
//
// See PyrCnBasis.h for the space, the layered lattice, the spanning set
// psi_{i,j,d} = l_i(a) l_j(b) (1-c)^m omega_d(c) and the block back
// substitution that turns psi into the interpolants, and for the reference for
// the space (Bergot, Cohen and Durufle, J. Sci. Comput. 42:345-381, 2010).
// That substitution solves V^T L(x) = psi(x) with V constant, so
// differentiating it is just the same solve run again on each of d(psi)/dr,
// d(psi)/ds and d(psi)/dt - the same resampling coefficients, the same one
// division per layer.
//
// With a = (r + w)/(2w), b = (s + w)/(2w), c = t and w = 1 - t, the chain rule
// gives da/dr = db/ds = 1/(2w), da/dt = r/(2w^2), db/dt = s/(2w^2) and
// dc/dt = 1 (t is already parameterized over [0, 1], unlike every prismatic
// axis elsewhere in this module, so there is no extra factor of 1/2 here).
// Both a and b depend on t as well as on their own axis, so d(psi)/dt picks up
// their terms alongside those of (1-c)^m and omega_d(c):
//
//   d(psi)/dr = l_i'(a) (da/dr) l_j(b) (1-c)^m omega_d(c),
//   d(psi)/ds = l_i(a) l_j'(b) (db/ds) (1-c)^m omega_d(c),
//   d(psi)/dt = [l_i'(a) (da/dt) l_j(b) + l_i(a) l_j'(b) (db/dt)]
//                 (1-c)^m omega_d(c)
//             + l_i(a) l_j(b) [-m (1-c)^(m-1) omega_d(c)
//                              + (1-c)^m omega_d'(c)].
//
// Like the basis, this forms no monomial coefficient anywhere: l_i and l_i' are
// evaluated from the products prod_{k!=i} (x - x_k)/(x_i - x_k) and
// sum_{k!=i} 1/(x_i - x_k) prod_{l!=i,k} (x - x_l)/(x_i - x_l).
if (abs(tt - 1.) < eps)
{
  tt = tt <= 1. ? 1. - eps : 1. + eps;
}
RealT ww = 1. - tt;
RealT aa = (rr + ww) / (2. * ww);
RealT bb = (ss + ww) / (2. * ww);
RealT dadr = 1. / (2. * ww);
RealT dadt = rr / (2. * ww * ww);
RealT dbdt = ss / (2. * ww * ww);

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

// omega_d(c) and its derivative, by the product rule on the same recurrence.
WORKSPACE(RealT, omega, order[2] + 1);
WORKSPACE(RealT, domega, order[2] + 1);
omega[0] = 1.;
domega[0] = 0.;
for (int dd = 1; dd <= order[2]; ++dd)
{
  RealT shift = tt - RealT(dd - 1) / RealT(order[2]);
  domega[dd] = domega[dd - 1] * shift + omega[dd - 1];
  omega[dd] = omega[dd - 1] * shift;
}

WORKSPACE(RealT, la, order[2] + 1);
WORKSPACE(RealT, lb, order[2] + 1);
WORKSPACE(RealT, dla, order[2] + 1);
WORKSPACE(RealT, dlb, order[2] + 1);
WORKSPACE(RealT, resamp, (order[2] + 1) * (order[2] + 1));
WORKSPACE(RealT, rhs, 3 * (order[2] + 1) * (order[2] + 1));
WORKSPACE(RealT, tmp, 3 * (order[2] + 1) * (order[2] + 1));

for (int dd = order[2]; dd >= 0; --dd)
{
  int mm = order[2] - dd;

  // Layer d's Lagrange polynomials and their derivatives at this point.
  for (int ii = 0; ii <= mm; ++ii)
  {
    la[ii] = 1.;
    lb[ii] = 1.;
    dla[ii] = 0.;
    dlb[ii] = 0.;
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

      // The term of l_i' in which the (x - x_kk) factor is the one
      // differentiated away.
      RealT terma = scale;
      RealT termb = scale;
      for (int ll = 0; ll <= mm; ++ll)
      {
        if (ll == ii || ll == kk)
        {
          continue;
        }
        RealT xl = RealT(ll) / RealT(mm);
        RealT scalel = RealT(mm) / RealT(ii - ll);
        terma *= (aa - xl) * scalel;
        termb *= (bb - xl) * scalel;
      }
      dla[ii] += terma;
      dlb[ii] += termb;
    }
  }

  RealT dwM = mm > 0 ? -RealT(mm) * wpow[mm - 1] : 0.;
  for (int jj = 0; jj <= mm; ++jj)
  {
    for (int ii = 0; ii <= mm; ++ii)
    {
      int idx = jj * (mm + 1) + ii;
      rhs[3 * idx + 0] = dla[ii] * dadr * lb[jj] * wpow[mm] * omega[dd];
      rhs[3 * idx + 1] = la[ii] * dlb[jj] * dadr * wpow[mm] * omega[dd];
      rhs[3 * idx + 2] =
        (dla[ii] * dadt * lb[jj] + la[ii] * dlb[jj] * dbdt) * wpow[mm] * omega[dd] +
        la[ii] * lb[jj] * (dwM * omega[dd] + wpow[mm] * domega[dd]);
    }
  }

  for (int KK = dd + 1; KK <= order[2]; ++KK)
  {
    int mK = order[2] - KK;

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

    for (int ii = 0; ii <= mm; ++ii)
    {
      for (int j2 = 0; j2 <= mK; ++j2)
      {
        for (int axis = 0; axis < 3; ++axis)
        {
          RealT sum = 0.;
          for (int i2 = 0; i2 <= mK; ++i2)
          {
            sum += resamp[ii * (mK + 1) + i2] *
              basisGradient[3 * (offset[KK] + j2 * (mK + 1) + i2) + axis];
          }
          tmp[3 * (ii * (order[2] + 1) + j2) + axis] = sum;
        }
      }
    }
    for (int jj = 0; jj <= mm; ++jj)
    {
      for (int ii = 0; ii <= mm; ++ii)
      {
        for (int axis = 0; axis < 3; ++axis)
        {
          RealT sum = 0.;
          for (int j2 = 0; j2 <= mK; ++j2)
          {
            sum += resamp[jj * (mK + 1) + j2] * tmp[3 * (ii * (order[2] + 1) + j2) + axis];
          }
          rhs[3 * (jj * (mm + 1) + ii) + axis] -= weight * sum;
        }
      }
    }
  }

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
      for (int axis = 0; axis < 3; ++axis)
      {
        basisGradient[3 * (offset[dd] + jj * (mm + 1) + ii) + axis] =
          rhs[3 * (jj * (mm + 1) + ii) + axis] / denom;
      }
    }
  }
}
