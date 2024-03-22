basisGradient[0] = ((-3 + 4 * rr + 4 * ss) * (-1 + tt) * tt) / 2.;
basisGradient[1] = ((-3 + 4 * rr + 4 * ss) * (-1 + tt) * tt) / 2.;
basisGradient[2] = ((-1 + rr + ss) * (-1 + 2 * rr + 2 * ss) * (-1 + 2 * tt)) / 2.;

basisGradient[3] = ((-1 + 4 * rr) * (-1 + tt) * tt) / 2.;
basisGradient[4] = 0.;
basisGradient[5] = (rr * (-1 + 2 * rr) * (-1 + 2 * tt)) / 2.;

basisGradient[6] = 0.;
basisGradient[7] = ((-1 + 4 * ss) * (-1 + tt) * tt) / 2.;
basisGradient[8] = (ss * (-1 + 2 * ss) * (-1 + 2 * tt)) / 2.;

basisGradient[9] = ((-3 + 4 * rr + 4 * ss) * tt * (1 + tt)) / 2.;
basisGradient[10] = ((-3 + 4 * rr + 4 * ss) * tt * (1 + tt)) / 2.;
basisGradient[11] = ((-1 + rr + ss) * (-1 + 2 * rr + 2 * ss) * (1 + 2 * tt)) / 2.;

basisGradient[12] = ((-1 + 4 * rr) * tt * (1 + tt)) / 2.;
basisGradient[13] = 0.;
basisGradient[14] = (rr * (-1 + 2 * rr) * (1 + 2 * tt)) / 2.;

basisGradient[15] = 0.;
basisGradient[16] = ((-1 + 4 * ss) * tt * (1 + tt)) / 2.;
basisGradient[17] = (ss * (-1 + 2 * ss) * (1 + 2 * tt)) / 2.;

basisGradient[18] = -2 * (-1 + 2 * rr + ss) * (-1 + tt) * tt;
basisGradient[19] = -2 * rr * (-1 + tt) * tt;
basisGradient[20] = 2 * rr * (-1 + rr + ss) * (1 - 2 * tt);

basisGradient[21] = 2 * ss * (-1 + tt) * tt;
basisGradient[22] = 2 * rr * (-1 + tt) * tt;
basisGradient[23] = 2 * rr * ss * (-1 + 2 * tt);

basisGradient[24] = -2 * ss * (-1 + tt) * tt;
basisGradient[25] = -2 * (-1 + rr + 2 * ss) * (-1 + tt) * tt;
basisGradient[26] = 2 * ss * (-1 + rr + ss) * (1 - 2 * tt);

basisGradient[27] = -(-3 + 4 * rr + 4 * ss) * (-1 + tt * tt);
basisGradient[28] = -(-3 + 4 * rr + 4 * ss) * (-1 + tt * tt);
basisGradient[29] = -2 * (1 + 2 * rr * rr - 3 * ss + 2 * ss * ss + rr * (-3 + 4 * ss)) * tt;

basisGradient[30] = -(-1 + 4 * rr) * (-1 + tt * tt);
basisGradient[31] = 0;
basisGradient[32] = 2 * (1 - 2 * rr) * rr * tt;

basisGradient[33] = 0;
basisGradient[34] = -(-1 + 4 * ss) * (-1 + tt * tt);
basisGradient[35] = 2 * (1 - 2 * ss) * ss * tt;

basisGradient[36] = -2 * (-1 + 2 * rr + ss) * tt * (1 + tt);
basisGradient[37] = -2 * rr * tt * (1 + tt);
basisGradient[38] = -2 * rr * (-1 + rr + ss) * (1 + 2 * tt);

basisGradient[39] = 2 * ss * tt * (1 + tt);
basisGradient[40] = 2 * rr * tt * (1 + tt);
basisGradient[41] = 2 * rr * ss * (1 + 2 * tt);

basisGradient[42] = -2 * ss * tt * (1 + tt);
basisGradient[43] = -2 * (-1 + rr + 2 * ss) * tt * (1 + tt);
basisGradient[44] = -2 * ss * (-1 + rr + ss) * (1 + 2 * tt);

basisGradient[45] = 4 * (-1 + 2 * rr + ss) * (-1 + tt * tt);
basisGradient[46] = 4 * rr * (-1 + tt) * (1 + tt);
basisGradient[47] = 8 * rr * (-1 + rr + ss) * tt;

basisGradient[48] = -4 * ss * (-1 + tt) * (1 + tt);
basisGradient[49] = -4 * rr * (-1 + tt) * (1 + tt);
basisGradient[50] = -8 * rr * ss * tt;

basisGradient[51] = 4 * ss * (-1 + tt) * (1 + tt);
basisGradient[52] = 4 * (-1 + rr + 2 * ss) * (-1 + tt * tt);
basisGradient[53] = 8 * ss * (-1 + rr + ss) * tt;
