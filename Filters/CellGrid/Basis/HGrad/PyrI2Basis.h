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

RealT ww = 1.0 / (1.0 - tt);

basis[0] = 0.25 * (-rr - ss - 1.0) * ((1.0 - rr) * (1.0 - ss) - tt + rr * ss * tt * ww);
basis[1] = 0.25 * (rr - ss - 1.0) * ((1.0 + rr) * (1.0 - ss) - tt - rr * ss * tt * ww);
basis[2] = 0.25 * (rr + ss - 1.0) * ((1.0 + rr) * (1.0 + ss) - tt + rr * ss * tt * ww);
basis[3] = 0.25 * (-rr + ss - 1.0) * ((1.0 - rr) * (1.0 + ss) - tt - rr * ss * tt * ww);

basis[4] = tt * (2.0 * tt - 1.0);

basis[5] = 0.5 * (1.0 + rr - tt) * (1.0 - rr - tt) * (1.0 - ss - tt) * ww;
basis[6] = 0.5 * (1.0 + ss - tt) * (1.0 - ss - tt) * (1.0 + rr - tt) * ww;
basis[7] = 0.5 * (1.0 + rr - tt) * (1.0 - rr - tt) * (1.0 + ss - tt) * ww;
basis[8] = 0.5 * (1.0 + ss - tt) * (1.0 - ss - tt) * (1.0 - rr - tt) * ww;

basis[9] = tt * (1.0 - rr - tt) * (1.0 - ss - tt) * ww;
basis[10] = tt * (1.0 + rr - tt) * (1.0 - ss - tt) * ww;
basis[11] = tt * (1.0 + rr - tt) * (1.0 + ss - tt) * ww;
basis[12] = tt * (1.0 - rr - tt) * (1.0 + ss - tt) * ww;
