const RealT ZERO = 0.;
const RealT HALF = 0.5;
const RealT FOURTH = 0.25;
const RealT NINTH = 1. / 9.;
const RealT FOURNINTHS = 4. / 9.;
const RealT ONE = 1.;
const RealT TWO = 2.;
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
RealT r2t = r2 * tt;
RealT rs2 = rr * s2;
RealT s2t = s2 * tt;
RealT rt2 = rr * t2;
RealT st2 = ss * t2;
RealT r2s2 = r2 * s2;

// Evaluate numerical values of (tt-1), (tt-1)^2, (tt-1)^3, and (tt-1)^4
RealT tm = tt - ONE;
RealT tm2 = tm * tm;

// Evaluate numerical value of 1/(tt-1)
RealT mt = abs(tm) > eps ? ONE / tm : ZERO;
RealT m2t = abs(tm2) > eps ? ONE / tm2 : ZERO;

// Baseline shape functions:
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

// MODIFICATIONS TO H

basis[0] = basis[0] + FOURTH * basis[13] + NINTH * (basis[14] + basis[17]);
basis[1] = basis[1] + FOURTH * basis[13] + NINTH * (basis[14] + basis[15]);
basis[2] = basis[2] + FOURTH * basis[13] + NINTH * (basis[15] + basis[16]);
basis[3] = basis[3] + FOURTH * basis[13] + NINTH * (basis[16] + basis[17]);
basis[4] = basis[4] + NINTH * (basis[14] + basis[15] + basis[16] + basis[17]);
basis[5] = basis[5] - FOURNINTHS * basis[14] - basis[13] * HALF;
basis[6] = basis[6] - FOURNINTHS * basis[15] - basis[13] * HALF;
basis[7] = basis[7] - FOURNINTHS * basis[16] - basis[13] * HALF;
basis[8] = basis[8] - FOURNINTHS * basis[17] - basis[13] * HALF;
basis[9] = basis[9] - FOURNINTHS * (basis[14] + basis[17]);
basis[10] = basis[10] - FOURNINTHS * (basis[14] + basis[15]);
basis[11] = basis[11] - FOURNINTHS * (basis[15] + basis[16]);
basis[12] = basis[12] - FOURNINTHS * (basis[16] + basis[17]);
