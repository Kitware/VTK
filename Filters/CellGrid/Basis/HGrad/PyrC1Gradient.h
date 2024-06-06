// Be sure that the basis functions are defined when tt is very close to 1.
// Warning: the derivatives are discontinuous in (0, 0, 1).
if (abs(tt - 1.0) < eps)
{
  if (tt <= 1.0)
  {
    tt = 1.0 - eps;
  }
  else
  {
    tt = 1.0 + eps;
  }
}

RealT ttTerm = 0.25 / (1.0 - tt);
RealT ttTerm2 = 4.0 * ttTerm * ttTerm;

basisGradient[0] = (ss + tt - 1.0) * ttTerm;
basisGradient[1] = (rr + tt - 1.0) * ttTerm;
basisGradient[2] = rr * ss * ttTerm2 - 0.25;

basisGradient[3] = (1.0 - ss - tt) * ttTerm;
basisGradient[4] = (tt - rr - 1.0) * ttTerm;
basisGradient[5] = rr * ss * ttTerm2 - 0.25;

basisGradient[6] = (1.0 + ss - tt) * ttTerm;
basisGradient[7] = (1.0 + rr - tt) * ttTerm;
basisGradient[8] = rr * ss * ttTerm2 - 0.25;

basisGradient[9] = (tt - ss - 1.0) * ttTerm;
basisGradient[10] = (1.0 - rr - tt) * ttTerm;
basisGradient[11] = rr * ss * ttTerm2 - 0.25;

basisGradient[12] = 0.0;
basisGradient[13] = 0.0;
basisGradient[14] = 1.0;
