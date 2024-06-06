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

basis[0] = (1.0 - rr - tt) * (1.0 - ss - tt) * ttTerm;
basis[1] = (1.0 + rr - tt) * (1.0 - ss - tt) * ttTerm;
basis[2] = (1.0 + rr - tt) * (1.0 + ss - tt) * ttTerm;
basis[3] = (1.0 - rr - tt) * (1.0 + ss - tt) * ttTerm;
basis[4] = tt;
