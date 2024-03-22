// Adapted from P. Silvester, "High-Order Polynomial Triangular Finite
// Elements for Potential Problems". Int. J. Engng Sci. Vol. 7, pp. 849-861.
// Pergamon Press, 1969. The generic method is valid for all orders, but we
// unroll the first two orders to reduce computational cost.

RealT uu = 1. - rr - ss - tt;

RealT ur = uu * rr;
RealT us = uu * ss;
RealT ut = uu * tt;
RealT rs = rr * ss;
RealT rt = rr * tt;
RealT st = ss * tt;
RealT urs = ur * ss;
RealT urt = ur * tt;
RealT ust = us * tt;
RealT rst = rs * tt;
RealT urst = urs * tt;

basis[0] = uu - 2.0 * (ur + us + ut) + 3.0 * (urs + urt + ust) - 4.0 * urst;
basis[1] = rr - 2.0 * (ur + rs + rt) + 3.0 * (urs + urt + rst) - 4.0 * urst;
basis[2] = ss - 2.0 * (rs + us + st) + 3.0 * (urs + rst + ust) - 4.0 * urst;
basis[3] = tt - 2.0 * (ut + rt + st) + 3.0 * (urt + ust + rst) - 4.0 * urst;
basis[4] = 4.0 * ur - 12.0 * (urs + urt) + 32.0 * urst;
basis[5] = 4.0 * rs - 12.0 * (urs + rst) + 32.0 * urst;
basis[6] = 4.0 * us - 12.0 * (urs + ust) + 32.0 * urst;
basis[7] = 4.0 * ut - 12.0 * (urt + ust) + 32.0 * urst;
basis[8] = 4.0 * rt - 12.0 * (urt + rst) + 32.0 * urst;
basis[9] = 4.0 * st - 12.0 * (rst + ust) + 32.0 * urst;
basis[10] = 27.0 * urs - 108.0 * urst;
basis[11] = 27.0 * urt - 108.0 * urst;
basis[12] = 27.0 * rst - 108.0 * urst;
basis[13] = 27.0 * ust - 108.0 * urst;
basis[14] = 256.0 * urst;
