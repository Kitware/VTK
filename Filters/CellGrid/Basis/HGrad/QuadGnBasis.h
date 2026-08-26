WORKSPACE(RealT, rterm, order + 1);
WORKSPACE(RealT, sterm, order + 1);

// Since the order is the same along each axis, we can use one loop to compute terms.
for (int ii = 0; ii <= order; ++ii)
{
  rterm[ii] = 1.;
  sterm[ii] = 1.;
  for (int jj = 0; jj <= order; ++jj)
  {
    if (ii == jj)
    {
      continue;
    }
    rterm[ii] *= (rr - gaussPoint(order, jj)) / (gaussPoint(order, ii) - gaussPoint(order, jj));
    sterm[ii] *= (ss - gaussPoint(order, jj)) / (gaussPoint(order, ii) - gaussPoint(order, jj));
  }
}

int term = 0;
// Now apply terms to compute basis functions
for (int jj = 0; jj <= order; ++jj)
{
  for (int ii = 0; ii <= order; ++ii)
  {
    basis[term++] = rterm[ii] * sterm[jj];
  }
}
