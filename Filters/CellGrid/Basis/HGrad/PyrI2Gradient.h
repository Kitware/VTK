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

RealT ww = 1.0 / (1.0 - tt);

basisGradient[0] = 0.25 * (-1.0 - rr - ss) * (-1.0 + ss + ss * tt * ww) -
  0.25 * ((1.0 - rr) * (1.0 - ss) - tt + rr * ss * tt * ww);
basisGradient[1] = 0.25 * (-1.0 - rr - ss) * (-1.0 + rr + rr * tt * ww) -
  0.25 * ((1.0 - rr) * (1.0 - ss) - tt + rr * ss * tt * ww);
basisGradient[2] = 0.25 * (-1.0 - rr - ss) * (-1.0 + rr * ss * ww + rr * ss * tt * ww * ww);

basisGradient[3] = 0.25 * (-1.0 + rr - ss) * (1.0 - ss - ss * tt * ww) +
  0.25 * ((1.0 + rr) * (1.0 - ss) - tt - rr * ss * tt * ww);
basisGradient[4] = 0.25 * (-1.0 + rr - ss) * (-1.0 - rr - rr * tt * ww) -
  0.25 * ((1.0 + rr) * (1.0 - ss) - tt - rr * ss * tt * ww);
basisGradient[5] = 0.25 * (-1.0 + rr - ss) * (-1.0 - rr * ss * ww - rr * ss * tt * ww * ww);

basisGradient[6] = 0.25 * (-1.0 + rr + ss) * (1.0 + ss + ss * tt * ww) +
  0.25 * ((1.0 + rr) * (1.0 + ss) - tt + rr * ss * tt * ww);
basisGradient[7] = 0.25 * (-1.0 + rr + ss) * (1.0 + rr + rr * tt * ww) +
  0.25 * ((1.0 + rr) * (1.0 + ss) - tt + rr * ss * tt * ww);
basisGradient[8] = 0.25 * (-1.0 + rr + ss) * (-1.0 + rr * ss * ww + rr * ss * tt * ww * ww);

basisGradient[9] = 0.25 * (-1.0 - rr + ss) * (-1.0 - ss - ss * tt * ww) -
  0.25 * ((1.0 - rr) * (1.0 + ss) - tt - rr * ss * tt * ww);
basisGradient[10] = 0.25 * (-1.0 - rr + ss) * (1.0 - rr - rr * tt * ww) +
  0.25 * ((1.0 - rr) * (1.0 + ss) - tt - rr * ss * tt * ww);
basisGradient[11] = 0.25 * (-1.0 - rr + ss) * (-1.0 - rr * ss * ww - rr * ss * tt * ww * ww);

basisGradient[12] = 0.0;
basisGradient[13] = 0.0;
basisGradient[14] = -1.0 + 4.0 * tt;

basisGradient[15] = -rr * ww * (1.0 - ss - tt);
basisGradient[16] = -0.5 * (1.0 - rr - tt) * (1.0 + rr - tt) * ww;
basisGradient[17] = 0.5 * ss * rr * rr * ww * ww + 0.5 * ss - 1.0 + tt;

basisGradient[18] = 0.5 * (1.0 - ss - tt) * (1.0 + ss - tt) * ww;
basisGradient[19] = -ss * ww * (1.0 + rr - tt);
basisGradient[20] = -0.5 * rr * ss * ss * ww * ww - 0.5 * rr - 1.0 + tt;

basisGradient[21] = -rr * ww * (1.0 + ss - tt);
basisGradient[22] = 0.5 * (1.0 - rr - tt) * (1.0 + rr - tt) * ww;
basisGradient[23] = -0.5 * ss * rr * rr * ww * ww - 0.5 * ss - 1.0 + tt;

basisGradient[24] = -0.5 * (1.0 - ss - tt) * (1.0 + ss - tt) * ww;
basisGradient[25] = -ss * ww * (1.0 - rr - tt);
basisGradient[26] = 0.5 * rr * ss * ss * ww * ww + 0.5 * rr - 1.0 + tt;

basisGradient[27] = -(1.0 - ss - tt) * tt * ww;
basisGradient[28] = -(1.0 - rr - tt) * tt * ww;
basisGradient[29] = rr * ss * ww * ww + 1.0 - rr - ss - 2.0 * tt;

basisGradient[30] = (1.0 - ss - tt) * tt * ww;
basisGradient[31] = -(1.0 + rr - tt) * tt * ww;
basisGradient[32] = -rr * ss * ww * ww + 1.0 + rr - ss - 2.0 * tt;

basisGradient[33] = (1.0 + ss - tt) * tt * ww;
basisGradient[34] = (1.0 + rr - tt) * tt * ww;
basisGradient[35] = rr * ss * ww * ww + 1.0 + rr + ss - 2.0 * tt;

basisGradient[36] = -(1.0 + ss - tt) * tt * ww;
basisGradient[37] = (1.0 - rr - tt) * tt * ww;
basisGradient[38] = -rr * ss * ww * ww + 1.0 - rr + ss - 2.0 * tt;
