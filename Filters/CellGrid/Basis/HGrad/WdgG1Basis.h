#define size_tri ((order + 1) * (order + 2) / 2)
#define size_lin (order + 1)

WORKSPACE(RealT, term_tri, size_tri);
WORKSPACE(RealT, term_lin, size_lin);

term_tri[0] = 5. / 3. - 2. * rr - 2. * ss;
term_tri[1] = -1. / 3. + 2. * rr;
term_tri[2] = -1. / 3. + 2. * ss;
term_lin[0] = 1. / 2. - sqrt(3) * tt / 2.;
term_lin[1] = 1. / 2. + sqrt(3) * tt / 2.;

for (int ii = 0; ii < size_lin; ++ii)
{
  for (int jj = 0; jj < size_tri; ++jj)
  {
    int i_sp = ii * size_tri + jj;
    basis[i_sp] = term_tri[jj] * term_lin[ii];
  }
}
