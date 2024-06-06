RealT uu = 1. - rr - ss - tt;
RealT rs = rr * ss;
RealT rt = rr * tt;
RealT st = ss * tt;
RealT umr = uu - rr;
RealT ums = uu - ss;
RealT umt = uu - tt;

basisGradient[0] = 1.0 - 4.0 * uu + 3.0 * ((ss + tt) * umr - st) - 4.0 * st * umr;
basisGradient[1] = 1.0 - 4.0 * uu + 3.0 * ((rr + tt) * ums - rt) - 4.0 * rt * ums;
basisGradient[2] = 1.0 - 4.0 * uu + 3.0 * ((rr + ss) * umt - rs) - 4.0 * rs * umt;

basisGradient[3] = 1.0 - 2.0 * (umr + ss + tt) + 3.0 * ((ss + tt) * umr + st) - 4.0 * st * umr;
basisGradient[4] = 3.0 * rr * ums - 4.0 * rt * ums;
basisGradient[5] = 3.0 * rr * umt - 4.0 * rs * umt;

basisGradient[6] = 3.0 * ss * umr - 4.0 * st * umr;
basisGradient[7] = 1 - 2.0 * (rr + ums + tt) + 3.0 * ((rr + tt) * ums + rt) - 4.0 * rt * ums;
basisGradient[8] = 3.0 * ss * umt - 4.0 * rs * umt;

basisGradient[9] = 3.0 * tt * umr - 4.0 * st * umr;
basisGradient[10] = 3.0 * tt * ums - 4.0 * rt * ums;
basisGradient[11] = 1 - 2.0 * (umt + rr + ss) + 3.0 * ((rr + ss) * umt + rs) - 4.0 * rs * umt;

basisGradient[12] = 4.0 * umr - 12.0 * umr * (ss + tt) + 32.0 * st * umr;
basisGradient[13] = -4.0 * rr - 12.0 * rr * (ums - tt) + 32.0 * rt * ums;
basisGradient[14] = -4.0 * rr - 12.0 * rr * (umt - ss) + 32.0 * rs * umt;

basisGradient[15] = 4.0 * ss - 12.0 * ss * (umr + tt) + 32.0 * st * umr;
basisGradient[16] = 4.0 * rr - 12.0 * rr * (ums + tt) + 32.0 * rt * ums;
basisGradient[17] = 32.0 * rs * umt;

basisGradient[18] = -4.0 * ss - 12.0 * ss * (umr - tt) + 32.0 * st * umr;
basisGradient[19] = 4.0 * ums - 12.0 * ums * (rr + tt) + 32.0 * rt * ums;
basisGradient[20] = -4.0 * ss - 12.0 * ss * (umt - rr) + 32.0 * rs * umt;

basisGradient[21] = -4.0 * tt - 12.0 * tt * (umr - ss) + 32.0 * st * umr;
basisGradient[22] = -4.0 * tt - 12.0 * tt * (ums - rr) + 32.0 * rt * ums;
basisGradient[23] = 4.0 * umt - 12.0 * umt * (rr + ss) + 32.0 * rs * umt;

basisGradient[24] = 4.0 * tt - 12.0 * tt * (umr + ss) + 32.0 * st * umr;
basisGradient[25] = 32.0 * rt * ums;
basisGradient[26] = 4.0 * rr - 12.0 * rr * (umt + ss) + 32.0 * rs * umt;

basisGradient[27] = 32.0 * st * umr;
basisGradient[28] = 4.0 * tt - 12.0 * tt * (rr + ums) + 32.0 * rt * ums;
basisGradient[29] = 4.0 * ss - 12.0 * ss * (rr + umt) + 32.0 * rs * umt;

basisGradient[30] = 27.0 * ss * umr - 108.0 * st * umr;
basisGradient[31] = 27.0 * rr * ums - 108.0 * rt * ums;
basisGradient[32] = -27.0 * rs - 108.0 * rs * umt;

basisGradient[33] = 27.0 * tt * umr - 108.0 * st * umr;
basisGradient[34] = -27.0 * rt - 108.0 * rt * ums;
basisGradient[35] = 27.0 * rr * umt - 108.0 * rs * umt;

basisGradient[36] = 27.0 * st - 108.0 * st * umr;
basisGradient[37] = 27.0 * rt - 108.0 * rt * ums;
basisGradient[38] = 27.0 * rs - 108.0 * rs * umt;

basisGradient[39] = -27.0 * st - 108.0 * st * umr;
basisGradient[40] = 27.0 * tt * ums - 108.0 * rt * ums;
basisGradient[41] = 27.0 * ss * umt - 108.0 * rs * umt;

basisGradient[42] = 256.0 * st * umr;
basisGradient[43] = 256.0 * rt * ums;
basisGradient[44] = 256.0 * rs * umt;
