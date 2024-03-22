const RealT HALF = 0.5;
const RealT ONE = 1.;
const RealT ZERO = 0.;
const RealT TWO = 2.;
const RealT SIXTEENTHIRDS = 16. / 3.;
const RealT EIGHT = 8.;
const RealT TWENTYSEVEN = 27.;
const RealT FOURTH = 0.25;
const RealT NINTH = 1. / 9.;
const RealT THREESIXTYFOURTHS = 3. / 64.;
const RealT EIGHTH = 1. / 8.;
const RealT FOURNINTHS = 4. / 9.;
const RealT THREESIXTEENTHS = 3. / 16.;
const RealT NINESIXTEENTHS = 9. / 16.;

const RealT K1 = -0.25;
const RealT K5 = ONE;
const RealT K6 = -HALF;
const RealT K10 = -ONE;
const RealT K14 = ONE;
const RealT K15A = TWENTYSEVEN / EIGHT;
const RealT K15B = -TWENTYSEVEN / EIGHT;
const RealT K19 = -SIXTEENTHIRDS;

RealT r2 = rr * rr;
RealT s2 = ss * ss;
RealT t2 = tt * tt;
RealT rs = rr * ss;
RealT rt = rr * tt;
RealT st = ss * tt;
RealT r2s = r2 * ss;
RealT r2t = r2 * tt;
RealT rs2 = rr * s2;
RealT s2t = s2 * tt;
RealT rt2 = rr * t2;
RealT st2 = ss * t2;
RealT r2s2 = r2 * s2;

RealT tm = tt - ONE;
RealT tm2 = tm * tm;
RealT tm3 = tm2 * tm;

// Be sure that the basis functions are defined when tt is very close to 1.
RealT mt = abs(tm) > eps ? ONE / tm : ZERO;
RealT m2t = abs(tm2) > eps ? ONE / tm2 : ZERO;
RealT m3t = abs(tm3) > eps ? ONE / tm3 : ZERO;

basis[0] = K1 * (rr + ss - ONE) * (rs * mt - rr - ss + tt - ONE);
basis[1] = K1 * (-rr + ss - ONE) * (-rs * mt + rr - ss + tt - ONE);
basis[2] = K1 * (-rr - ss - ONE) * (rs * mt + rr + ss + tt - ONE);
basis[3] = K1 * (rr - ss - ONE) * (-rs * mt - rr + ss + tt - ONE);
basis[4] = K5 * (-tt + TWO * t2);
basis[5] = K6 * (-r2s * mt + r2 + st - ss - tm2);
basis[6] = K6 * (rs2 * mt - rt + rr + s2 - tm2);
basis[7] = K6 * (r2s * mt + r2 - st + ss - tm2);
basis[8] = K6 * (-rs2 * mt + rt - rr + s2 - tm2);
basis[9] = K10 * (rs + rs * mt - rt - st - tt + t2);
basis[10] = K10 * (-rs - rs * mt + rt - st - tt + t2);
basis[11] = K10 * (rs + rs * mt + rt + st - tt + t2);
basis[12] = K10 * (-rs - rs * mt - rt + st - tt + t2);
basis[13] = K14 * (r2s2 * m2t - r2 - s2 + tm2);
basis[14] = K15A * (r2s + r2s * mt - r2s2 * mt - r2s2 * m2t + st - st2 + s2t);
basis[15] = K15B * (rs2 + rs2 * mt + rt - rt2 + r2s2 * mt + r2s2 * m2t - r2t);
basis[16] = K15B * (r2s + r2s * mt + r2s2 * mt + r2s2 * m2t + st - st2 - s2t);
basis[17] = K15A * (rs2 + rs2 * mt + rt - rt2 - r2s2 * mt - r2s2 * m2t + r2t);
basis[18] = K19 * (r2s2 * m2t + r2s2 * m3t - r2 - r2 * mt - s2 - s2 * mt - tt + t2);

// Modifications
basis[0] =
  basis[0] + FOURTH * basis[13] + NINTH * (basis[14] + basis[17]) + basis[18] * THREESIXTYFOURTHS;
basis[1] =
  basis[1] + FOURTH * basis[13] + NINTH * (basis[14] + basis[15]) + basis[18] * THREESIXTYFOURTHS;
basis[2] =
  basis[2] + FOURTH * basis[13] + NINTH * (basis[15] + basis[16]) + basis[18] * THREESIXTYFOURTHS;
basis[3] =
  basis[3] + FOURTH * basis[13] + NINTH * (basis[16] + basis[17]) + basis[18] * THREESIXTYFOURTHS;
basis[4] = basis[4] + EIGHTH * basis[18] + NINTH * (basis[14] + basis[15] + basis[16] + basis[17]);
basis[5] = basis[5] - FOURNINTHS * basis[14] - basis[13] * HALF;
basis[6] = basis[6] - FOURNINTHS * basis[15] - basis[13] * HALF;
basis[7] = basis[7] - FOURNINTHS * basis[16] - basis[13] * HALF;
basis[8] = basis[8] - FOURNINTHS * basis[17] - basis[13] * HALF;
basis[9] = basis[9] - FOURNINTHS * (basis[14] + basis[17]) - basis[18] * THREESIXTEENTHS;
basis[10] = basis[10] - FOURNINTHS * (basis[14] + basis[15]) - basis[18] * THREESIXTEENTHS;
basis[11] = basis[11] - FOURNINTHS * (basis[15] + basis[16]) - basis[18] * THREESIXTEENTHS;
basis[12] = basis[12] - FOURNINTHS * (basis[16] + basis[17]) - basis[18] * THREESIXTEENTHS;
basis[13] = basis[13] - basis[18] * NINESIXTEENTHS;
