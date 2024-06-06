RealT ww = 1.0 - rr - ss;

basis[0] = 0.5 * ww * (2.0 * ww - 2.0 - tt) * (1.0 - tt);
basis[1] = 0.5 * rr * (2.0 * rr - 2.0 - tt) * (1.0 - tt);
basis[2] = 0.5 * ss * (2.0 * ss - 2.0 - tt) * (1.0 - tt);
basis[3] = 0.5 * ww * (2.0 * ww - 2.0 + tt) * (1.0 + tt);
basis[4] = 0.5 * rr * (2.0 * rr - 2.0 + tt) * (1.0 + tt);
basis[5] = 0.5 * ss * (2.0 * ss - 2.0 + tt) * (1.0 + tt);

basis[6] = 2.0 * ww * rr * (1.0 - tt);
basis[7] = 2.0 * rr * ss * (1.0 - tt);
basis[8] = 2.0 * ss * ww * (1.0 - tt);
basis[9] = ww * (1.0 - tt * tt);
basis[10] = rr * (1.0 - tt * tt);
basis[11] = ss * (1.0 - tt * tt);
basis[12] = 2.0 * ww * rr * (1.0 + tt);
basis[13] = 2.0 * rr * ss * (1.0 + tt);
basis[14] = 2.0 * ss * ww * (1.0 + tt);
