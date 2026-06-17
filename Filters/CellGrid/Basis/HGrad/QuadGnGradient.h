WORKSPACE(RealT, rterm, order + 1);
WORKSPACE(RealT, sterm, order + 1);
WORKSPACE(RealT, drterm, order + 1);
WORKSPACE(RealT, dsterm, order + 1);

// Since the order is the same along each axis, we can use one loop to compute terms.
for (int ii = 0; ii <= order; ++ii)
{
  rterm[ii] = 1.;
  sterm[ii] = 1.;
  drterm[ii] = 0.;
  dsterm[ii] = 0.;
  for (int jj = 0; jj <= order; ++jj)
  {
    if (ii == jj)
    {
      continue;
    }
    rterm[ii] *= (rr - gpts[order][jj]) / (gpts[order][ii] - gpts[order][jj]);
    sterm[ii] *= (ss - gpts[order][jj]) / (gpts[order][ii] - gpts[order][jj]);
    double drt = 1.;
    double dst = 1.;
    for (int kk = 0; kk <= order; ++kk)
    {
      if (kk == ii)
      {
        continue;
      }
      drt *= (kk == jj ? 1. : (rr - gpts[order][jj])) / (gpts[order][kk] - gpts[order][jj]);
      dst *= (kk == jj ? 1. : (ss - gpts[order][jj])) / (gpts[order][kk] - gpts[order][jj]);
    }
    drterm[ii] += drt;
    dsterm[ii] += dst;
  }
}

int term = 0;
// Now apply terms to compute basis functions
for (int jj = 0; jj <= order; ++jj)
{
  for (int ii = 0; ii <= order; ++ii)
  {
    basisGradient[term++] = drterm[ii] * sterm[jj];
    basisGradient[term++] = rterm[ii] * dsterm[jj];
    basisGradient[term++] = 0.;
  }
}
