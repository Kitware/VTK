int count = 0;
for (int kk = 0; kk <= order; ++kk)
{
  for (int jj = 0; jj <= order - kk; ++jj)
  {
    for (int ii = 0; ii <= order - kk - jj; ++ii)
    {
      RealT term1 = jacobi(kk, 0., 0., 2. * rr / (1. - ss - tt + eps) - 1.);
      RealT term2 = power(1. - ss - tt, kk);
      RealT term3 = jacobi(jj, (2. * kk + 1.), 0., 2. * ss / (1. - tt + eps) - 1.);
      RealT term4 = power(1. - tt, jj);
      RealT term5 = jacobi(ii, (2. * (kk + jj) + 2.), 0., 2. * tt - 1.);
      basis[count] = term1 * term2 * term3 * term4 * term5;
      ++count;
    }
  }
}
