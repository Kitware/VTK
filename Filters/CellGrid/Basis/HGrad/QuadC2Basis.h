basis[0] = rr * (rr - 1.0) * ss * (ss - 1.0) / 4.0;
basis[1] = rr * (rr + 1.0) * ss * (ss - 1.0) / 4.0;
basis[2] = rr * (rr + 1.0) * ss * (ss + 1.0) / 4.0;
basis[3] = rr * (rr - 1.0) * ss * (ss + 1.0) / 4.0;
// edge midpoints basis functions
basis[4] = (1.0 - rr) * (1.0 + rr) * ss * (ss - 1.0) / 2.0;
basis[5] = rr * (rr + 1.0) * (1.0 - ss) * (1.0 + ss) / 2.0;
basis[6] = (1.0 - rr) * (1.0 + rr) * ss * (ss + 1.0) / 2.0;
basis[7] = rr * (rr - 1.0) * (1.0 - ss) * (1.0 + ss) / 2.0;
// quad bubble basis function
basis[8] = (1.0 - rr) * (1.0 + rr) * (1.0 - ss) * (1.0 + ss);
