#define size_tri ((order + 1) * (order + 2) / 2)
#define size_lin (order + 1)

WORKSPACE(RealT, term_tri, size_tri);
WORKSPACE(RealT, dterm_dr_tri, size_tri);
WORKSPACE(RealT, dterm_ds_tri, size_tri);

WORKSPACE(RealT, term_lin, size_lin);
WORKSPACE(RealT, dterm_dt_lin, size_lin);

// clang-format off
term_tri[0] = 5. / 3. - 2. * rr - 2. * ss;
term_tri[1] = -1. / 3. + 2. * rr;
term_tri[2] = -1. / 3. + 2. * ss;

dterm_dr_tri[0] = - 2.;
dterm_dr_tri[1] =   2.;
dterm_dr_tri[2] =   0.;

dterm_ds_tri[0] = - 2.;
dterm_ds_tri[1] =   0.;
dterm_ds_tri[2] =   2.;

term_lin[0] = 1. / 2. - sqrt(3) * tt / 2.;
term_lin[1] = 1. / 2. + sqrt(3) * tt / 2.;

dterm_dt_lin[0] = - sqrt(3) / 2.;
dterm_dt_lin[1] =   sqrt(3) / 2.;

for (int ii = 0; ii < size_lin; ++ii)
{
  for (int jj = 0; jj < size_tri; ++jj)
  {
    int i_sp = ii * size_tri + jj;
    // ∂/∂r, ∂/∂s, ∂/∂t:
    basisGradient[3 * i_sp    ] = dterm_dr_tri[jj] * term_lin[ii]                  /* zero */;
    basisGradient[3 * i_sp + 1] = dterm_ds_tri[jj] * term_lin[ii]                  /* zero */;
    basisGradient[3 * i_sp + 2] = /* zero */                        term_tri[jj] * dterm_dt_lin[ii];
  }
}
// clang-format on
