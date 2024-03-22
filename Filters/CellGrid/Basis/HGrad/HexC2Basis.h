basis[0] = 0.125 * (-1. + rr) * rr * (-1. + ss) * ss * (-1. + tt) * tt;
basis[1] = 0.125 * rr * (1. + rr) * (-1. + ss) * ss * (-1. + tt) * tt;
basis[2] = 0.125 * rr * (1. + rr) * ss * (1. + ss) * (-1. + tt) * tt;
basis[3] = 0.125 * (-1. + rr) * rr * ss * (1. + ss) * (-1. + tt) * tt;
basis[4] = 0.125 * (-1. + rr) * rr * (-1. + ss) * ss * tt * (1. + tt);
basis[5] = 0.125 * rr * (1. + rr) * (-1. + ss) * ss * tt * (1. + tt);
basis[6] = 0.125 * rr * (1. + rr) * ss * (1. + ss) * tt * (1. + tt);
basis[7] = 0.125 * (-1. + rr) * rr * ss * (1. + ss) * tt * (1. + tt);

basis[8] = 0.25 * (1. - rr) * (1. + rr) * (-1. + ss) * ss * (-1. + tt) * tt;
basis[9] = 0.25 * rr * (1. + rr) * (1. - ss) * (1. + ss) * (-1. + tt) * tt;
basis[10] = 0.25 * (1. - rr) * (1. + rr) * ss * (1. + ss) * (-1. + tt) * tt;
basis[11] = 0.25 * (-1. + rr) * rr * (1. - ss) * (1. + ss) * (-1. + tt) * tt;

basis[12] = 0.25 * (-1. + rr) * rr * (-1. + ss) * ss * (1. - tt) * (1. + tt);
basis[13] = 0.25 * rr * (1. + rr) * (-1. + ss) * ss * (1. - tt) * (1. + tt);
basis[14] = 0.25 * rr * (1. + rr) * ss * (1. + ss) * (1. - tt) * (1. + tt);
basis[15] = 0.25 * (-1. + rr) * rr * ss * (1. + ss) * (1. - tt) * (1. + tt);

basis[16] = 0.25 * (1. - rr) * (1. + rr) * (-1. + ss) * ss * tt * (1. + tt);
basis[17] = 0.25 * rr * (1. + rr) * (1. - ss) * (1. + ss) * tt * (1. + tt);
basis[18] = 0.25 * (1. - rr) * (1. + rr) * ss * (1. + ss) * tt * (1. + tt);
basis[19] = 0.25 * (-1. + rr) * rr * (1. - ss) * (1. + ss) * tt * (1. + tt);

basis[20] = (1. - rr) * (1. + rr) * (1. - ss) * (1. + ss) * (1. - tt) * (1. + tt);

basis[21] = 0.5 * (1. - rr) * (1. + rr) * (1. - ss) * (1. + ss) * (-1. + tt) * tt;
basis[22] = 0.5 * (1. - rr) * (1. + rr) * (1. - ss) * (1. + ss) * tt * (1. + tt);
basis[23] = 0.5 * (-1. + rr) * rr * (1. - ss) * (1. + ss) * (1. - tt) * (1. + tt);
basis[24] = 0.5 * rr * (1. + rr) * (1. - ss) * (1. + ss) * (1. - tt) * (1. + tt);
basis[25] = 0.5 * (1. - rr) * (1. + rr) * (-1. + ss) * ss * (1. - tt) * (1. + tt);
basis[26] = 0.5 * (1. - rr) * (1. + rr) * ss * (1. + ss) * (1. - tt) * (1. + tt);
