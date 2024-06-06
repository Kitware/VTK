basis[0] = (-1. + rr + ss + tt) * (-1. + 2. * rr + 2. * ss + 2. * tt);
basis[1] = rr * (-1. + 2. * rr);
basis[2] = ss * (-1. + 2. * ss);
basis[3] = tt * (-1. + 2. * tt);

basis[4] = -4. * rr * (-1. + rr + ss + tt);
basis[5] = 4. * rr * ss;
basis[6] = -4. * ss * (-1. + rr + ss + tt);
basis[7] = -4. * tt * (-1. + rr + ss + tt);
basis[8] = 4. * rr * tt;
basis[9] = 4. * ss * tt;
