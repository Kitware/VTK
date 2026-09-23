// Gradients of the arbitrary-order Bernstein-Bezier basis on the pyramid.
//
// See PyrCnBasis.h for the basis of Chan and Warburton (arXiv:1508.05609), the
// a = (r + w) / (2w) etc. change of variables (w = 1 - t), and why every
// workspace here is sized off order[2] alone. Each 1-d Bernstein polynomial
// and its derivative (d/da, d/db, d/dc) are computed as in EdgeCnGradient.h,
// then combined by the product rule - but unlike every other shape in this
// module, a and b both depend on t as well as their own axis (through w), so
// d/dt also picks up two extra terms:
//
//   d/dr [A(a) B(b) C(c)] = A'(a) (da/dr) B(b) C(c),
//   d/ds [A(a) B(b) C(c)] = A(a) B'(b) (db/ds) C(c),
//   d/dt [A(a) B(b) C(c)] = A'(a) (da/dt) B(b) C(c)
//                         + A(a) B'(b) (db/dt) C(c)
//                         + A(a) B(b) C'(c) (dc/dt),
//
// where da/dr = db/ds = 1 / (2w), da/dt = r / (2w^2), db/dt = s / (2w^2) (from
// differentiating a = (r + w) / (2w) with w = 1 - t), and dc/dt = 1 (c = t
// directly - t is already parameterized over [0, 1], unlike every prismatic
// axis elsewhere in this module, so there is no extra 0.5 factor here).
if (abs(tt - 1.) < eps)
{
  tt = tt <= 1. ? 1. - eps : 1. + eps;
}
RealT ww = 1. - tt;

WORKSPACE(RealT, ctmp, order[2] + 1);
WORKSPACE(RealT, dctmp, order[2] + 1);
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
    RealT ddc = 0.;
    if (kk > 0)
    {
      ddc += RealT(kk) * xpow[kk - 1] * ypow[kk];
    }
    if (kk < order[2])
    {
      ddc -= RealT(order[2] - kk) * xpow[kk] * ypow[kk + 1];
    }
    dctmp[kk] = coeff * ddc;
    coeff = coeff * RealT(order[2] - kk) / RealT(kk + 1);
  }
}

int idx = 0;
for (int kk = 0; kk <= order[2]; ++kk)
{
  int mm = order[2] - kk;
  RealT aa = (rr + ww) / (2. * ww);
  RealT bb = (ss + ww) / (2. * ww);
  RealT dadr = 1. / (2. * ww);
  RealT dadt = rr / (2. * ww * ww);
  RealT dbds = dadr;
  RealT dbdt = ss / (2. * ww * ww);

  WORKSPACE(RealT, atmp, order[2] + 1);
  WORKSPACE(RealT, datmp, order[2] + 1);
  WORKSPACE(RealT, btmp, order[2] + 1);
  WORKSPACE(RealT, dbtmp, order[2] + 1);
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
      RealT dda = 0.;
      if (ii > 0)
      {
        dda += RealT(ii) * xpow[ii - 1] * ypow[ii];
      }
      if (ii < mm)
      {
        dda -= RealT(mm - ii) * xpow[ii] * ypow[ii + 1];
      }
      datmp[ii] = coeff * dda;
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
      RealT ddb = 0.;
      if (jj > 0)
      {
        ddb += RealT(jj) * xpow[jj - 1] * ypow[jj];
      }
      if (jj < mm)
      {
        ddb -= RealT(mm - jj) * xpow[jj] * ypow[jj + 1];
      }
      dbtmp[jj] = coeff * ddb;
      coeff = coeff * RealT(mm - jj) / RealT(jj + 1);
    }
  }

  for (int jj = 0; jj <= mm; ++jj)
  {
    for (int ii = 0; ii <= mm; ++ii)
    {
      basisGradient[3 * idx + 0] = datmp[ii] * dadr * btmp[jj] * ctmp[kk];
      basisGradient[3 * idx + 1] = atmp[ii] * dbtmp[jj] * dbds * ctmp[kk];
      basisGradient[3 * idx + 2] = datmp[ii] * dadt * btmp[jj] * ctmp[kk] +
        atmp[ii] * dbtmp[jj] * dbdt * ctmp[kk] + atmp[ii] * btmp[jj] * dctmp[kk];
      ++idx;
    }
  }
}
