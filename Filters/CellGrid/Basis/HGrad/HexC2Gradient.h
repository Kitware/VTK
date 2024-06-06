basisGradient[0 * 3 + 0] = (-0.125 + 0.25 * rr) * (-1. + ss) * ss * (-1. + tt) * tt;
basisGradient[0 * 3 + 1] = (-1. + rr) * rr * (-0.125 + 0.25 * ss) * (-1. + tt) * tt;
basisGradient[0 * 3 + 2] = (-1. + rr) * rr * (-1. + ss) * ss * (-0.125 + 0.25 * tt);

basisGradient[1 * 3 + 0] = (0.125 + 0.25 * rr) * (-1. + ss) * ss * (-1. + tt) * tt;
basisGradient[1 * 3 + 1] = rr * (1. + rr) * (-0.125 + 0.25 * ss) * (-1. + tt) * tt;
basisGradient[1 * 3 + 2] = rr * (1. + rr) * (-1. + ss) * ss * (-0.125 + 0.25 * tt);

basisGradient[2 * 3 + 0] = (0.125 + 0.25 * rr) * ss * (1. + ss) * (-1. + tt) * tt;
basisGradient[2 * 3 + 1] = rr * (1. + rr) * (0.125 + 0.25 * ss) * (-1. + tt) * tt;
basisGradient[2 * 3 + 2] = rr * (1. + rr) * ss * (1. + ss) * (-0.125 + 0.25 * tt);

basisGradient[3 * 3 + 0] = (-0.125 + 0.25 * rr) * ss * (1. + ss) * (-1. + tt) * tt;
basisGradient[3 * 3 + 1] = (-1. + rr) * rr * (0.125 + 0.25 * ss) * (-1. + tt) * tt;
basisGradient[3 * 3 + 2] = (-1. + rr) * rr * ss * (1. + ss) * (-0.125 + 0.25 * tt);

basisGradient[4 * 3 + 0] = (-0.125 + 0.25 * rr) * (-1. + ss) * ss * tt * (1. + tt);
basisGradient[4 * 3 + 1] = (-1. + rr) * rr * (-0.125 + 0.25 * ss) * tt * (1. + tt);
basisGradient[4 * 3 + 2] = (-1. + rr) * rr * (-1. + ss) * ss * (0.125 + 0.25 * tt);

basisGradient[5 * 3 + 0] = (0.125 + 0.25 * rr) * (-1. + ss) * ss * tt * (1. + tt);
basisGradient[5 * 3 + 1] = rr * (1. + rr) * (-0.125 + 0.25 * ss) * tt * (1. + tt);
basisGradient[5 * 3 + 2] = rr * (1. + rr) * (-1. + ss) * ss * (0.125 + 0.25 * tt);

basisGradient[6 * 3 + 0] = (0.125 + 0.25 * rr) * ss * (1. + ss) * tt * (1. + tt);
basisGradient[6 * 3 + 1] = rr * (1. + rr) * (0.125 + 0.25 * ss) * tt * (1. + tt);
basisGradient[6 * 3 + 2] = rr * (1. + rr) * ss * (1. + ss) * (0.125 + 0.25 * tt);

basisGradient[7 * 3 + 0] = (-0.125 + 0.25 * rr) * ss * (1. + ss) * tt * (1. + tt);
basisGradient[7 * 3 + 1] = (-1. + rr) * rr * (0.125 + 0.25 * ss) * tt * (1. + tt);
basisGradient[7 * 3 + 2] = (-1. + rr) * rr * ss * (1. + ss) * (0.125 + 0.25 * tt);

basisGradient[8 * 3 + 0] = -0.5 * rr * (-1. + ss) * ss * (-1. + tt) * tt;
basisGradient[8 * 3 + 1] = (1. - rr) * (1. + rr) * (-0.25 + 0.5 * ss) * (-1. + tt) * tt;
basisGradient[8 * 3 + 2] = (1. - rr) * (1. + rr) * (-1. + ss) * ss * (-0.25 + 0.5 * tt);

basisGradient[9 * 3 + 0] = (0.25 + 0.5 * rr) * (1. - ss) * (1. + ss) * (-1. + tt) * tt;
basisGradient[9 * 3 + 1] = rr * (1. + rr) * (-0.5 * ss) * (-1. + tt) * tt;
basisGradient[9 * 3 + 2] = rr * (1. + rr) * (1. - ss) * (1. + ss) * (-0.25 + 0.5 * tt);

basisGradient[10 * 3 + 0] = -0.5 * rr * ss * (1. + ss) * (-1. + tt) * tt;
basisGradient[10 * 3 + 1] = (1. - rr) * (1. + rr) * (0.25 + 0.5 * ss) * (-1. + tt) * tt;
basisGradient[10 * 3 + 2] = (1. - rr) * (1. + rr) * ss * (1. + ss) * (-0.25 + 0.5 * tt);

basisGradient[11 * 3 + 0] = (-0.25 + 0.5 * rr) * (1. - ss) * (1. + ss) * (-1. + tt) * tt;
basisGradient[11 * 3 + 1] = (-1. + rr) * rr * (-0.5 * ss) * (-1. + tt) * tt;
basisGradient[11 * 3 + 2] = (-1. + rr) * rr * (1. - ss) * (1. + ss) * (-0.25 + 0.5 * tt);

basisGradient[12 * 3 + 0] = (-0.25 + 0.5 * rr) * (-1. + ss) * ss * (1. - tt) * (1. + tt);
basisGradient[12 * 3 + 1] = (-1. + rr) * rr * (-0.25 + 0.5 * ss) * (1. - tt) * (1. + tt);
basisGradient[12 * 3 + 2] = (-1. + rr) * rr * (-1. + ss) * ss * (-0.5 * tt);

basisGradient[13 * 3 + 0] = (0.25 + 0.5 * rr) * (-1. + ss) * ss * (1. - tt) * (1. + tt);
basisGradient[13 * 3 + 1] = rr * (1. + rr) * (-0.25 + 0.5 * ss) * (1. - tt) * (1. + tt);
basisGradient[13 * 3 + 2] = rr * (1. + rr) * (-1. + ss) * ss * (-0.5 * tt);

basisGradient[14 * 3 + 0] = (0.25 + 0.5 * rr) * ss * (1. + ss) * (1. - tt) * (1. + tt);
basisGradient[14 * 3 + 1] = rr * (1. + rr) * (0.25 + 0.5 * ss) * (1. - tt) * (1. + tt);
basisGradient[14 * 3 + 2] = rr * (1. + rr) * ss * (1. + ss) * (-0.5 * tt);

basisGradient[15 * 3 + 0] = (-0.25 + 0.5 * rr) * ss * (1. + ss) * (1. - tt) * (1. + tt);
basisGradient[15 * 3 + 1] = (-1. + rr) * rr * (0.25 + 0.5 * ss) * (1. - tt) * (1. + tt);
basisGradient[15 * 3 + 2] = (-1. + rr) * rr * ss * (1. + ss) * (-0.5 * tt);

basisGradient[16 * 3 + 0] = -0.5 * rr * (-1. + ss) * ss * tt * (1. + tt);
basisGradient[16 * 3 + 1] = (1. - rr) * (1. + rr) * (-0.25 + 0.5 * ss) * tt * (1. + tt);
basisGradient[16 * 3 + 2] = (1. - rr) * (1. + rr) * (-1. + ss) * ss * (0.25 + 0.5 * tt);

basisGradient[17 * 3 + 0] = (0.25 + 0.5 * rr) * (1. - ss) * (1. + ss) * tt * (1. + tt);
basisGradient[17 * 3 + 1] = rr * (1. + rr) * (-0.5 * ss) * tt * (1. + tt);
basisGradient[17 * 3 + 2] = rr * (1. + rr) * (1. - ss) * (1. + ss) * (0.25 + 0.5 * tt);

basisGradient[18 * 3 + 0] = -0.5 * rr * ss * (1. + ss) * tt * (1. + tt);
basisGradient[18 * 3 + 1] = (1. - rr) * (1. + rr) * (0.25 + 0.5 * ss) * tt * (1. + tt);
basisGradient[18 * 3 + 2] = (1. - rr) * (1. + rr) * ss * (1. + ss) * (0.25 + 0.5 * tt);

basisGradient[19 * 3 + 0] = (-0.25 + 0.5 * rr) * (1. - ss) * (1. + ss) * tt * (1. + tt);
basisGradient[19 * 3 + 1] = (-1. + rr) * rr * (-0.5 * ss) * tt * (1. + tt);
basisGradient[19 * 3 + 2] = (-1. + rr) * rr * (1. - ss) * (1. + ss) * (0.25 + 0.5 * tt);

basisGradient[20 * 3 + 0] = -2. * rr * (1. - ss) * (1. + ss) * (1. - tt) * (1. + tt);
basisGradient[20 * 3 + 1] = (1. - rr) * (1. + rr) * (-2. * ss) * (1. - tt) * (1. + tt);
basisGradient[20 * 3 + 2] = (1. - rr) * (1. + rr) * (1. - ss) * (1. + ss) * (-2. * tt);

basisGradient[21 * 3 + 0] = -rr * (1. - ss) * (1. + ss) * (-1. + tt) * tt;
basisGradient[21 * 3 + 1] = (1. - rr) * (1. + rr) * (-ss) * (-1. + tt) * tt;
basisGradient[21 * 3 + 2] = (1. - rr) * (1. + rr) * (1. - ss) * (1. + ss) * (-0.5 + tt);

basisGradient[22 * 3 + 0] = -rr * (1. - ss) * (1. + ss) * tt * (1. + tt);
basisGradient[22 * 3 + 1] = (1. - rr) * (1. + rr) * (-ss) * tt * (1. + tt);
basisGradient[22 * 3 + 2] = (1. - rr) * (1. + rr) * (1. - ss) * (1. + ss) * (0.5 + tt);

basisGradient[23 * 3 + 0] = (-0.5 + rr) * (1. - ss) * (1. + ss) * (1. - tt) * (1. + tt);
basisGradient[23 * 3 + 1] = (-1. + rr) * rr * (-ss) * (1. - tt) * (1. + tt);
basisGradient[23 * 3 + 2] = (-1. + rr) * rr * (1. - ss) * (1. + ss) * (-tt);

basisGradient[24 * 3 + 0] = (0.5 + rr) * (1. - ss) * (1. + ss) * (1. - tt) * (1. + tt);
basisGradient[24 * 3 + 1] = rr * (1. + rr) * (-ss) * (1. - tt) * (1. + tt);
basisGradient[24 * 3 + 2] = rr * (1. + rr) * (1. - ss) * (1. + ss) * (-tt);

basisGradient[25 * 3 + 0] = -rr * (-1. + ss) * ss * (1. - tt) * (1. + tt);
basisGradient[25 * 3 + 1] = (1. - rr) * (1. + rr) * (-0.5 + ss) * (1. - tt) * (1. + tt);
basisGradient[25 * 3 + 2] = (1. - rr) * (1. + rr) * (-1. + ss) * ss * (-tt);

basisGradient[26 * 3 + 0] = -rr * ss * (1. + ss) * (1. - tt) * (1. + tt);
basisGradient[26 * 3 + 1] = (1. - rr) * (1. + rr) * (0.5 + ss) * (1. - tt) * (1. + tt);
basisGradient[26 * 3 + 2] = (1. - rr) * (1. + rr) * ss * (1. + ss) * (-tt);
