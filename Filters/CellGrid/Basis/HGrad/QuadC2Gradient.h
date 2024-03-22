basisGradient[0] = (-0.25 + 0.5 * rr) * (-1. + ss) * ss;
basisGradient[1] = (-1.0 + rr) * rr * (-0.25 + 0.5 * ss);
basisGradient[2] = 0.0;

basisGradient[3] = (0.25 + 0.5 * rr) * (-1. + ss) * ss;
basisGradient[4] = rr * (1. + rr) * (-0.25 + 0.5 * ss);
basisGradient[5] = 0.0;

basisGradient[6] = (0.25 + 0.5 * rr) * ss * (1. + ss);
basisGradient[7] = rr * (1. + rr) * (0.25 + 0.5 * ss);
basisGradient[8] = 0.0;

basisGradient[9] = (-0.25 + 0.5 * rr) * ss * (1. + ss);
basisGradient[10] = (-1. + rr) * rr * (0.25 + 0.5 * ss);
basisGradient[11] = 0.0;

basisGradient[12] = rr * (1.0 - ss) * ss;
basisGradient[13] = 0.5 * (1.0 - rr) * (1.0 + rr) * (-1.0 + 2.0 * ss);
basisGradient[14] = 0.0;

basisGradient[15] = 0.5 * (1.0 - ss) * (1.0 + ss) * (1.0 + 2.0 * rr);
basisGradient[16] = -rr * (1.0 + rr) * ss;
basisGradient[17] = 0.0;

basisGradient[18] = -ss * (1.0 + ss) * rr;
basisGradient[19] = 0.5 * (1.0 - rr) * (1.0 + rr) * (1.0 + 2.0 * ss);
basisGradient[20] = 0.0;

basisGradient[21] = 0.5 * (1.0 - ss) * (1.0 + ss) * (-1.0 + 2.0 * rr);
basisGradient[22] = (1.0 - rr) * rr * ss;
basisGradient[23] = 0.0;

basisGradient[24] = -2.0 * (1.0 - ss) * (1.0 + ss) * rr;
basisGradient[25] = -2.0 * (1.0 - rr) * (1.0 + rr) * ss;
basisGradient[26] = 0.0;
