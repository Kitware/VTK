RealT ww = 1.0 - rr - ss;

basisGradient[0] = -(2.0 * ww - 1.0 - 0.5 * tt) * (1.0 - tt);
basisGradient[1] = -(2.0 * ww - 1.0 - 0.5 * tt) * (1.0 - tt);
basisGradient[2] = -0.5 * ww * (2.0 * ww - 1.0 - 2.0 * tt);

basisGradient[3] = (2.0 * rr - 1.0 - 0.5 * tt) * (1.0 - tt);
basisGradient[4] = 0.0;
basisGradient[5] = -0.5 * rr * (2.0 * rr - 1.0 - 2.0 * tt);

basisGradient[6] = 0.0;
basisGradient[7] = (2.0 * ss - 1.0 - 0.5 * tt) * (1.0 - tt);
basisGradient[8] = -0.5 * ss * (2.0 * ss - 1.0 - 2.0 * tt);

basisGradient[9] = -(2.0 * ww - 1.0 + 0.5 * tt) * (1.0 + tt);
basisGradient[10] = -(2.0 * ww - 1.0 + 0.5 * tt) * (1.0 + tt);
basisGradient[11] = 0.5 * ww * (2.0 * ww - 1.0 + 2.0 * tt);

basisGradient[12] = (2.0 * rr - 1.0 + 0.5 * tt) * (1.0 + tt);
basisGradient[13] = 0.0;
basisGradient[14] = 0.5 * rr * (2.0 * rr - 1.0 + 2.0 * tt);

basisGradient[15] = 0.0;
basisGradient[16] = (2.0 * ss - 1.0 + 0.5 * tt) * (1.0 + tt);
basisGradient[17] = 0.5 * ss * (2.0 * ss - 1.0 + 2.0 * tt);

basisGradient[18] = 2.0 * (ww - rr) * (1.0 - tt);
basisGradient[19] = -2.0 * rr * (1.0 - tt);
basisGradient[20] = -2.0 * rr * ww;

basisGradient[21] = 2.0 * ss * (1.0 - tt);
basisGradient[22] = 2.0 * rr * (1.0 - tt);
basisGradient[23] = -2.0 * rr * ss;

basisGradient[24] = -2.0 * ss * (1.0 - tt);
basisGradient[25] = 2.0 * (ww - ss) * (1.0 - tt);
basisGradient[26] = -2.0 * ww * ss;

basisGradient[27] = -(1.0 - tt * tt);
basisGradient[28] = -(1.0 - tt * tt);
basisGradient[29] = -2.0 * tt * ww;

basisGradient[30] = (1.0 - tt * tt);
basisGradient[31] = 0.0;
basisGradient[32] = -2.0 * tt * rr;

basisGradient[33] = 0.0;
basisGradient[34] = (1.0 - tt * tt);
basisGradient[35] = -2.0 * tt * ss;

basisGradient[36] = 2.0 * (ww - rr) * (1.0 + tt);
basisGradient[37] = -2.0 * rr * (1.0 + tt);
basisGradient[38] = 2.0 * rr * ww;

basisGradient[39] = 2.0 * ss * (1.0 + tt);
basisGradient[40] = 2.0 * rr * (1.0 + tt);
basisGradient[41] = 2.0 * rr * ss;

basisGradient[42] = -2.0 * ss * (1.0 + tt);
basisGradient[43] = 2.0 * (ww - ss) * (1.0 + tt);
basisGradient[44] = 2.0 * ww * ss;
