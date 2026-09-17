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

      RealT dterm_dr = jacobi_dx(kk, 0., 0., rr / tt_tmp) * (1. / tt_tmp);
      RealT dterm_ds = jacobi_dx(jj, 0., 0., ss / tt_tmp) * (1. / tt_tmp);
      RealT dterm1_dt = jacobi_dx(kk, 0., 0., rr / tt_tmp) * 2. * rr / power(1. - tt, 2);
      RealT dterm2_dt = jacobi_dx(jj, 0., 0., ss / tt_tmp) * 2. * ss / power(1. - tt, 2);
      RealT dterm3_dt = maxjjkk * power(tt_tmp, maxjjkk - 1) * (-.5);
      RealT dterm4_dt = jacobi_dx(ii, (2. * maxjjkk + 2.), 0., tt);
      // ∂/∂r
      basisGradient[count++] = dterm_dr * term2 * term3 * term4;
      // ∂/∂s
      basisGradient[count++] = term1 * dterm_ds * term3 * term4;
      // ∂/∂t
      basisGradient[count++] = dterm1_dt * term2 * term3 * term4 +
        term1 * dterm2_dt * term3 * term4 + term1 * term2 * dterm3_dt * term4 +
        term1 * term2 * term3 * dterm4_dt;
    }
  }
}
