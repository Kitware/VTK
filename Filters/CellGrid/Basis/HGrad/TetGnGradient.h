int count = 0;
for (int ii = 0; ii <= order; ++ii)
{
  for (int jj = 0; jj <= order - ii; ++jj)
  {
    for (int kk = 0; kk <= order - ii - jj; ++kk)
    {
      RealT term1 = jacobi(ii, 0., 0., 2. * rr / (1. - ss - tt + eps) - 1.);
      RealT term2 = power(1. - ss - tt, ii);
      RealT term3 = jacobi(jj, (2. * ii + 1.), 0., 2. * ss / (1. - tt + eps) - 1.);
      RealT term4 = power(1. - tt, jj);
      RealT term5 = jacobi(kk, (2. * (ii + jj) + 2.), 0., 2. * tt - 1.);

      RealT dterm1_dr =
        jacobi_dx(ii, 0., 0., 2. * rr / (1. - ss - tt + eps) - 1.) * 2. / (1. - ss - tt + eps);
      RealT dterm1_ds = jacobi_dx(ii, 0., 0., 2. * rr / (1. - ss - tt + eps) - 1.) * 2. * rr /
        power(1. - ss - tt + eps, 2.);
      RealT dterm1_dt = dterm1_ds;
      RealT dterm2_ds = -ii * power(1. - ss - tt, ii) / (1. - ss - tt);
      RealT dterm2_dt = dterm2_ds;
      RealT dterm3_ds =
        jacobi_dx(jj, (2. * ii + 1.), 0., 2. * ss / (1. - tt + eps) - 1.) * 2. / (1. - tt + eps);
      RealT dterm3_dt = jacobi_dx(jj, (2. * ii + 1.), 0., 2. * ss / (1. - tt + eps) - 1.) * 2. *
        ss / power(1. - tt + eps, 2.);
      RealT dterm4_dt = -jj * power(1. - tt, jj - 1.);
      RealT dterm5_dt = jacobi_dx(kk, (2. * (ii + jj) + 2.), 0., 2. * tt - 1.) * 2.;

      // ∂/∂r:
      basisGradient[count++] = dterm1_dr * term2 * term3 * term4 * term5;
      // ∂/∂s
      basisGradient[count++] =
        (dterm1_ds * term2 * term3 + term1 * dterm2_ds * term3 + term1 * term2 * dterm3_ds) *
        term4 * term5;
      // ∂/∂t
      basisGradient[count++] = dterm1_dt * term2 * term3 * term4 * term5 +
        term1 * dterm2_dt * term3 * term4 * term5 + term1 * term2 * dterm3_dt * term4 * term5 +
        term1 * term2 * term3 * dterm4_dt * term5 + term1 * term2 * term3 * term4 * dterm5_dt;
    }
  }
}
