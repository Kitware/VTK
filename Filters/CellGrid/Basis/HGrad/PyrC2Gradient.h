const RealT ZERO = 0.;
const RealT HALF = 0.5;
const RealT FOURTH = 0.25;
const RealT NINTH = 1. / 9.;
const RealT FOURNINTHS = 4. / 9.;
const RealT ONE = 1.;
const RealT TWO = 2.;
const RealT FOUR = 4.;
const RealT EIGHT = 9.;
const RealT TWENTYSEVEN = 27.;

const RealT K1 = -FOURTH;
const RealT K5 = ONE;
const RealT K6 = -HALF;
const RealT K10 = -ONE;
const RealT K14 = ONE;
const RealT K15A = TWENTYSEVEN / EIGHT;
const RealT K15B = -TWENTYSEVEN / EIGHT;

// Evaluate common rr,ss,tt multiplication terms
RealT r2 = rr * rr;
RealT s2 = ss * ss;
RealT t2 = tt * tt;
RealT rs = rr * ss;
RealT rt = rr * tt;
RealT st = ss * tt;
RealT r2s = r2 * ss;
RealT rs2 = rr * s2;
RealT r2s2 = r2 * s2;

// Evaluate numerical values of (tt-1), (tt-1)^2, (tt-1)^3, and (tt-1)^4
RealT tm = tt - ONE;
RealT tm2 = tm * tm;
RealT tm3 = tm2 * tm;
RealT tm4 = tm3 * tm;

// Evaluate numerical value of 1/(tt-1)
RealT mt = abs(tm) > eps ? ONE / tm : ZERO;
RealT m2t = abs(tm2) > eps ? ONE / tm2 : ZERO;
RealT m3t = abs(tm3) > eps ? ONE / tm3 : ZERO;
RealT m4t = abs(tm4) > eps ? ONE / tm4 : ZERO;

// BASELINE DHDR
basisGradient[0] = K1 * (rs * mt - rr - ss + tt + (ss * mt - ONE) * (rr + ss - ONE) - ONE);
basisGradient[3] = K1 * (rs * mt - rr + ss - tt + (-ss * mt + ONE) * (-rr + ss - ONE) + ONE);
basisGradient[6] = K1 * (-rs * mt - rr - ss - tt + (ss * mt + ONE) * (-rr - ss - ONE) + ONE);
basisGradient[9] = K1 * (-rs * mt - rr + ss + tt + (-ss * mt - ONE) * (rr - ss - ONE) - ONE);
basisGradient[12] = ZERO;
basisGradient[15] = K6 * (-TWO * rs * mt + TWO * rr);
basisGradient[18] = K6 * (s2 * mt - tt + ONE);
basisGradient[21] = K6 * (TWO * rs * mt + TWO * rr);
basisGradient[24] = K6 * (-s2 * mt + tt - ONE);
basisGradient[27] = K10 * (ss + ss * mt - tt);
basisGradient[30] = K10 * (-ss - ss * mt + tt);
basisGradient[33] = K10 * (ss + ss * mt + tt);
basisGradient[36] = K10 * (-ss - ss * mt - tt);
basisGradient[39] = K14 * (TWO * rs2 * m2t - TWO * rr);
basisGradient[42] = K15A * (TWO * rs + TWO * rs * mt - TWO * rs2 * mt - TWO * rs2 * m2t);
basisGradient[45] = K15B * (TWO * rs2 * mt + TWO * rs2 * m2t - TWO * rt + s2 + s2 * mt + tt - t2);
basisGradient[48] = K15B * (TWO * rs + TWO * rs * mt + TWO * rs2 * mt + TWO * rs2 * m2t);
basisGradient[51] = K15A * (-TWO * rs2 * mt - TWO * rs2 * m2t + TWO * rt + s2 + s2 * mt + tt - t2);

// BASELINE DHDS

basisGradient[1] = K1 * (rs * mt - rr - ss + tt + (rr * mt - ONE) * (rr + ss - ONE) - ONE);
basisGradient[4] = K1 * (-rs * mt + rr - ss + tt + (-rr * mt - ONE) * (-rr + ss - ONE) - ONE);
basisGradient[7] = K1 * (-rs * mt - rr - ss - tt + (rr * mt + ONE) * (-rr - ss - ONE) + ONE);
basisGradient[10] = K1 * (rs * mt + rr - ss - tt + (-rr * mt + ONE) * (rr - ss - ONE) + ONE);
basisGradient[13] = ZERO;
basisGradient[16] = K6 * (-r2 * mt + tt - ONE);
basisGradient[19] = K6 * (TWO * rs * mt + TWO * ss);
basisGradient[22] = K6 * (r2 * mt - tt + ONE);
basisGradient[25] = K6 * (-TWO * rs * mt + TWO * ss);
basisGradient[28] = K10 * (rr + rr * mt - tt);
basisGradient[31] = K10 * (-rr - rr * mt - tt);
basisGradient[34] = K10 * (rr + rr * mt + tt);
basisGradient[37] = K10 * (-rr - rr * mt + tt);
basisGradient[40] = K14 * (TWO * r2s * m2t - TWO * ss);
basisGradient[43] = K15A * (-TWO * r2s * mt - TWO * r2s * m2t + r2 + r2 * mt + TWO * st + tt - t2);
basisGradient[46] = K15B * (TWO * rs + TWO * rs * mt + TWO * r2s * mt + TWO * r2s * m2t);
basisGradient[49] = K15B * (TWO * r2s * mt + TWO * r2s * m2t + r2 + r2 * mt - TWO * st + tt - t2);
basisGradient[52] = K15A * (TWO * rs + TWO * rs * mt - TWO * r2s * mt - TWO * r2s * m2t);

// BASELINE DHDT

basisGradient[2] = K1 * (-rs * m2t + ONE) * (rr + ss - ONE);
basisGradient[5] = K1 * (rs * m2t + ONE) * (-rr + ss - ONE);
basisGradient[8] = K1 * (-rs * m2t + ONE) * (-rr - ss - ONE);
basisGradient[11] = K1 * (rs * m2t + ONE) * (rr - ss - ONE);
basisGradient[14] = K5 * (FOUR * tt - ONE);
basisGradient[17] = K6 * (r2s * m2t + ss - TWO * tt + TWO);
basisGradient[20] = K6 * (-rs2 * m2t - rr - TWO * tt + TWO);
basisGradient[23] = K6 * (-r2s * m2t - ss - TWO * tt + TWO);
basisGradient[26] = K6 * (rs2 * m2t + rr - TWO * tt + TWO);
basisGradient[29] = K10 * (-rs * m2t - rr - ss + TWO * tt - ONE);
basisGradient[32] = K10 * (rs * m2t + rr - ss + TWO * tt - ONE);
basisGradient[35] = K10 * (-rs * m2t + rr + ss + TWO * tt - ONE);
basisGradient[38] = K10 * (rs * m2t - rr + ss + TWO * tt - ONE);
basisGradient[41] = K14 * (-TWO * r2s2 * m3t + TWO * tt - TWO);
basisGradient[44] =
  K15A * (-r2s * m2t - r2s2 * (-TWO * tt + TWO) * m4t + r2s2 * m2t - TWO * st + ss + s2);
basisGradient[47] =
  K15B * (-rs2 * m2t - TWO * rt + rr + r2s2 * (-TWO * tt + TWO) * m4t - r2s2 * m2t - r2);
basisGradient[50] =
  K15B * (-r2s * m2t + r2s2 * (-TWO * tt + TWO) * m4t - r2s2 * m2t - TWO * st + ss - s2);
basisGradient[53] =
  K15A * (-rs2 * m2t - TWO * rt + rr - r2s2 * (-TWO * tt + TWO) * m4t + r2s2 * m2t + r2);

// MODIFICATIONS TO DHDR

basisGradient[0] =
  basisGradient[0] + basisGradient[39] * FOURTH + NINTH * (basisGradient[42] + basisGradient[51]);
basisGradient[3] =
  basisGradient[3] + basisGradient[39] * FOURTH + NINTH * (basisGradient[42] + basisGradient[45]);
basisGradient[6] =
  basisGradient[6] + basisGradient[39] * FOURTH + NINTH * (basisGradient[45] + basisGradient[48]);
basisGradient[9] =
  basisGradient[9] + basisGradient[39] * FOURTH + NINTH * (basisGradient[48] + basisGradient[51]);
basisGradient[12] = basisGradient[12] +
  NINTH * (basisGradient[42] + basisGradient[45] + basisGradient[48] + basisGradient[51]);
basisGradient[15] = basisGradient[15] - basisGradient[39] * HALF - basisGradient[42] * FOURNINTHS;
basisGradient[18] = basisGradient[18] - basisGradient[39] * HALF - basisGradient[45] * FOURNINTHS;
basisGradient[21] = basisGradient[21] - basisGradient[39] * HALF - basisGradient[48] * FOURNINTHS;
basisGradient[24] = basisGradient[24] - basisGradient[39] * HALF - basisGradient[51] * FOURNINTHS;
basisGradient[27] = basisGradient[27] + FOURNINTHS * (-basisGradient[42] - basisGradient[51]);
basisGradient[30] = basisGradient[30] + FOURNINTHS * (-basisGradient[42] - basisGradient[45]);
basisGradient[33] = basisGradient[33] + FOURNINTHS * (-basisGradient[45] - basisGradient[48]);
basisGradient[36] = basisGradient[36] + FOURNINTHS * (-basisGradient[48] - basisGradient[51]);

// MODIFICATIONS TO DHDS

basisGradient[1] =
  basisGradient[1] + basisGradient[40] * FOURTH + NINTH * (basisGradient[43] + basisGradient[52]);
basisGradient[4] =
  basisGradient[4] + basisGradient[40] * FOURTH + NINTH * (basisGradient[43] + basisGradient[46]);
basisGradient[7] =
  basisGradient[7] + basisGradient[40] * FOURTH + NINTH * (basisGradient[46] + basisGradient[49]);
basisGradient[10] =
  basisGradient[10] + basisGradient[40] * FOURTH + NINTH * (basisGradient[49] + basisGradient[52]);
basisGradient[13] = basisGradient[13] +
  NINTH * (basisGradient[43] + basisGradient[46] + basisGradient[49] + basisGradient[52]);
basisGradient[16] = basisGradient[16] - basisGradient[40] * HALF - basisGradient[43] * FOURNINTHS;
basisGradient[19] = basisGradient[19] - basisGradient[40] * HALF - basisGradient[46] * FOURNINTHS;
basisGradient[22] = basisGradient[22] - basisGradient[40] * HALF - basisGradient[49] * FOURNINTHS;
basisGradient[25] = basisGradient[25] - basisGradient[40] * HALF - basisGradient[52] * FOURNINTHS;
basisGradient[28] = basisGradient[28] + FOURNINTHS * (-basisGradient[43] - basisGradient[52]);
basisGradient[31] = basisGradient[31] + FOURNINTHS * (-basisGradient[43] - basisGradient[46]);
basisGradient[34] = basisGradient[34] + FOURNINTHS * (-basisGradient[46] - basisGradient[49]);
basisGradient[37] = basisGradient[37] + FOURNINTHS * (-basisGradient[49] - basisGradient[52]);

// MODIFICATIONS TO DHDT

basisGradient[2] =
  basisGradient[2] + basisGradient[41] * FOURTH + NINTH * (basisGradient[44] + basisGradient[53]);
basisGradient[5] =
  basisGradient[5] + basisGradient[41] * FOURTH + NINTH * (basisGradient[44] + basisGradient[47]);
basisGradient[8] =
  basisGradient[8] + basisGradient[41] * FOURTH + NINTH * (basisGradient[47] + basisGradient[50]);
basisGradient[11] =
  basisGradient[11] + basisGradient[41] * FOURTH + NINTH * (basisGradient[50] + basisGradient[53]);
basisGradient[14] = basisGradient[14] +
  NINTH * (basisGradient[44] + basisGradient[47] + basisGradient[50] + basisGradient[53]);
basisGradient[17] = basisGradient[17] - basisGradient[41] * HALF - basisGradient[44] * FOURNINTHS;
basisGradient[20] = basisGradient[20] - basisGradient[41] * HALF - basisGradient[47] * FOURNINTHS;
basisGradient[23] = basisGradient[23] - basisGradient[41] * HALF - basisGradient[50] * FOURNINTHS;
basisGradient[26] = basisGradient[26] - basisGradient[41] * HALF - basisGradient[53] * FOURNINTHS;
basisGradient[29] = basisGradient[29] + FOURNINTHS * (-basisGradient[44] - basisGradient[53]);
basisGradient[32] = basisGradient[32] + FOURNINTHS * (-basisGradient[44] - basisGradient[47]);
basisGradient[35] = basisGradient[35] + FOURNINTHS * (-basisGradient[47] - basisGradient[50]);
basisGradient[38] = basisGradient[38] + FOURNINTHS * (-basisGradient[50] - basisGradient[53]);
