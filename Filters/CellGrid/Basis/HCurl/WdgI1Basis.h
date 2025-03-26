basis[0] = (1.0 - tt) * (1.0 - ss) / 2.0;
basis[1] = rr * (1.0 - tt) / 2.0;
basis[2] = 0.0;

basis[3] = ss * (tt - 1.0) / 2.0;
basis[4] = rr * (1.0 - tt) / 2.0;
basis[5] = 0.0;

basis[6] = ss * (tt - 1.0) / 2.0;
basis[7] = (1.0 - rr) * (tt - 1.0) / 2.0;
basis[8] = 0.0;

basis[9] = (1.0 - ss) * (1.0 + tt) / 2.0;
basis[10] = rr * (1.0 + tt) / 2.0;
basis[11] = 0.0;

basis[12] = -ss * (1.0 + tt) / 2.0;
basis[13] = rr * (1.0 + tt) / 2.0;
basis[14] = 0.0;

basis[15] = -ss * (1.0 + tt) / 2.0;
basis[16] = (rr - 1.0) * (1.0 + tt) / 2.0;
basis[17] = 0.0;

basis[18] = 0.0;
basis[19] = 0.0;
basis[20] = (1.0 - rr - ss) / 2.0;

basis[21] = 0.0;
basis[22] = 0.0;
basis[23] = rr / 2.0;

basis[24] = 0.0;
basis[25] = 0.0;
basis[26] = ss / 2.0;
