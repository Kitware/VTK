WORKSPACE(RealT, rterm, order + 1);
WORKSPACE(RealT, sterm, order + 1);
WORKSPACE(RealT, tterm, order + 1);
WORKSPACE(RealT, drterm, order + 1);
WORKSPACE(RealT, dsterm, order + 1);
WORKSPACE(RealT, dtterm, order + 1);

// Since the order is the same along each axis, we can use one loop to compute terms.
for (int ii = 0; ii <= order; ++ii)
{
  rterm[ii] = 1.;
  sterm[ii] = 1.;
  tterm[ii] = 1.;
  drterm[ii] = 0.;
  dsterm[ii] = 0.;
  dtterm[ii] = 0.;
  for (int jj = 0; jj <= order; ++jj)
  {
    if (ii == jj)
    {
      continue;
    }
    rterm[ii] *= (rr - gaussPoint(order, jj)) / (gaussPoint(order, ii) - gaussPoint(order, jj));
    sterm[ii] *= (ss - gaussPoint(order, jj)) / (gaussPoint(order, ii) - gaussPoint(order, jj));
    tterm[ii] *= (tt - gaussPoint(order, jj)) / (gaussPoint(order, ii) - gaussPoint(order, jj));
    double drt = 1.;
    double dst = 1.;
    double dtt = 1.;
    for (int kk = 0; kk <= order; ++kk)
    {
      if (kk == ii)
      {
        continue;
      }
      // clang-format off
      drt *= (kk == jj ? 1. : (rr - gaussPoint(order, jj))) / (gaussPoint(order, kk) - gaussPoint(order, jj));
      dst *= (kk == jj ? 1. : (ss - gaussPoint(order, jj))) / (gaussPoint(order, kk) - gaussPoint(order, jj));
      dtt *= (kk == jj ? 1. : (tt - gaussPoint(order, jj))) / (gaussPoint(order, kk) - gaussPoint(order, jj));
      // clang-format on
    }
    drterm[ii] += drt;
    dsterm[ii] += dst;
    dtterm[ii] += dtt;
  }
}

int term = 0;
// Now apply terms to compute basis functions
for (int kk = 0; kk <= order; ++kk)
{
  for (int jj = 0; jj <= order; ++jj)
  {
    for (int ii = 0; ii <= order; ++ii)
    {
      basisGradient[term++] = drterm[ii] * sterm[jj] * tterm[kk];
      basisGradient[term++] = rterm[ii] * dsterm[jj] * tterm[kk];
      basisGradient[term++] = rterm[ii] * sterm[jj] * dtterm[kk];
    }
  }
}
