int count = 0;
RealT tt_tmp = 1. - tt + eps;
for (int ii = 0; ii <= order; ++ii)
{
  for (int jj = 0; jj <= order - ii; ++jj)
  {
    for (int kk = 0; kk <= order - ii; ++kk)
    {
      int maxjjkk = (kk > jj) ? kk : jj;
      RealT term1 = jacobi(kk, 0., 0., rr / tt_tmp);
      RealT term2 = jacobi(jj, 0., 0., ss / tt_tmp);
      RealT term3 = power(tt_tmp, maxjjkk);
      RealT term4 = jacobi(ii, (2. * maxjjkk + 2.), 0., tt);
      basis[count] = term1 * term2 * term3 * term4;
      ++count;
    }
  }
}
