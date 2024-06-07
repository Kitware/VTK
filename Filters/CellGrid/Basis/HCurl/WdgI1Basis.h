// Intrepid2
basis[0 * 3 + 0] = (1.0 - tt) * (1.0 - ss);
basis[0 * 3 + 1] = rr * (1.0 - tt);
basis[0 * 3 + 2] = 0.0;

basis[1 * 3 + 0] = ss * (tt - 1.0);
basis[1 * 3 + 1] = rr * (1.0 - tt);
basis[1 * 3 + 2] = 0.0;

basis[2 * 3 + 0] = ss * (tt - 1.0);
basis[2 * 3 + 1] = (1.0 - rr) * (tt - 1.0);
basis[2 * 3 + 2] = 0.0;

basis[3 * 3 + 0] = (1.0 - ss) * (1.0 + tt);
basis[3 * 3 + 1] = rr * (1.0 + tt);
basis[3 * 3 + 2] = 0.0;

basis[4 * 3 + 0] = -ss * (1.0 + tt);
basis[4 * 3 + 1] = rr * (1.0 + tt);
basis[4 * 3 + 2] = 0.0;

basis[5 * 3 + 0] = -ss * (1.0 + tt);
basis[5 * 3 + 1] = (rr - 1.0) * (1.0 + tt);
basis[5 * 3 + 2] = 0.0;

basis[6 * 3 + 0] = 0.0;
basis[6 * 3 + 1] = 0.0;
basis[6 * 3 + 2] = (1.0 - rr - ss);

basis[7 * 3 + 0] = 0.0;
basis[7 * 3 + 1] = 0.0;
basis[7 * 3 + 2] = rr;

basis[8 * 3 + 0] = 0.0;
basis[8 * 3 + 1] = 0.0;
basis[8 * 3 + 2] = ss;
