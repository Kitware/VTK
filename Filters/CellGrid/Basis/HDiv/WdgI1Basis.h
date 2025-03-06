// Intrepid2
basis[0 * 3 + 0] = rr * 2.0;
basis[0 * 3 + 1] = (ss - 1.0) * 2.0;
basis[0 * 3 + 2] = 0.0;

basis[1 * 3 + 0] = rr * 2.0;
basis[1 * 3 + 1] = ss * 2.0;
basis[1 * 3 + 2] = 0.0;

basis[2 * 3 + 0] = (rr - 1.0) * 2.0;
basis[2 * 3 + 1] = ss * 2.0;
basis[2 * 3 + 2] = 0.0;

basis[3 * 3 + 0] = 0.0;
basis[3 * 3 + 1] = 0.0;
basis[3 * 3 + 2] = (tt - 1.0) / 2.0;

basis[4 * 3 + 0] = 0.0;
basis[4 * 3 + 1] = 0.0;
basis[4 * 3 + 2] = (1.0 + tt) / 2.0;
