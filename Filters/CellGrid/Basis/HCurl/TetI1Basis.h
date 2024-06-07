// Intrepid2
basis[0 * 3 + 0] = 2.0 * (1.0 - ss - tt);
basis[0 * 3 + 1] = 2.0 * rr;
basis[0 * 3 + 2] = 2.0 * rr;

basis[1 * 3 + 0] = -2.0 * ss;
basis[1 * 3 + 1] = 2.0 * rr;
basis[1 * 3 + 2] = 0.0;

basis[2 * 3 + 0] = -2.0 * ss;
basis[2 * 3 + 1] = 2.0 * (-1.0 + rr + tt);
basis[2 * 3 + 2] = -2.0 * ss;

basis[3 * 3 + 0] = 2.0 * tt;
basis[3 * 3 + 1] = 2.0 * tt;
basis[3 * 3 + 2] = 2.0 * (1.0 - rr - ss);

basis[4 * 3 + 0] = -2.0 * tt;
basis[4 * 3 + 1] = 0.0;
basis[4 * 3 + 2] = 2.0 * rr;

basis[5 * 3 + 0] = 0.0;
basis[5 * 3 + 1] = -2.0 * tt;
basis[5 * 3 + 2] = 2.0 * ss;
