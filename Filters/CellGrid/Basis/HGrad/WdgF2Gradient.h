RealT uu = 1.0 - rr - ss;
RealT rs = rr * ss;
RealT tp = 1. + tt;
RealT tm = 1. - tt;

basisGradient[0] =
  -0.5 * tt * tm * (3 * ss - 2) * uu + 0.5 * tt * tm * (3 * rs - 2 * rr - 2 * ss + 1);
basisGradient[1] =
  -0.5 * tt * tm * (3 * rr - 2) * uu + 0.5 * tt * tm * (3 * rs - 2 * rr - 2 * ss + 1);
basisGradient[2] =
  0.5 * tt * uu * (3 * rs - 2 * rr - 2 * ss + 1) - 0.5 * tm * uu * (3 * rs - 2 * rr - 2 * ss + 1);

basisGradient[3] = -0.5 * tt * tm * (-3 * rs + 4 * rr + ss * (-3 * rr - 3 * ss + 3) - 1);
basisGradient[4] = -0.5 * tt * tm * (-3 * rs + rr * (-3 * rr - 3 * ss + 3));
basisGradient[5] = 0.5 * tt * (rs * (-3 * rr - 3 * ss + 3) - 2 * rs - 2 * rr * uu + rr) -
  0.5 * tm * (rs * (-3 * rr - 3 * ss + 3) - 2 * rs - 2 * rr * uu + rr);

basisGradient[6] = -0.5 * tt * tm * (-3 * rs + ss * (-3 * rr - 3 * ss + 3));
basisGradient[7] = -0.5 * tt * tm * (-3 * rs + rr * (-3 * rr - 3 * ss + 3) + 4 * ss - 1);
basisGradient[8] = 0.5 * tt * (rs * (-3 * rr - 3 * ss + 3) - 2 * rs - 2 * ss * uu + ss) -
  0.5 * tm * (rs * (-3 * rr - 3 * ss + 3) - 2 * rs - 2 * ss * uu + ss);

basisGradient[9] =
  0.5 * tt * (3 * ss - 2) * tp * uu - 0.5 * tt * tp * (3 * rs - 2 * rr - 2 * ss + 1);
basisGradient[10] =
  0.5 * tt * (3 * rr - 2) * tp * uu - 0.5 * tt * tp * (3 * rs - 2 * rr - 2 * ss + 1);
basisGradient[11] =
  0.5 * tt * uu * (3 * rs - 2 * rr - 2 * ss + 1) + 0.5 * tp * uu * (3 * rs - 2 * rr - 2 * ss + 1);

basisGradient[12] = 0.5 * tt * tp * (-3 * rs + 4 * rr + ss * (-3 * rr - 3 * ss + 3) - 1);
basisGradient[13] = 0.5 * tt * tp * (-3 * rs + rr * (-3 * rr - 3 * ss + 3));
basisGradient[14] = 0.5 * tt * (rs * (-3 * rr - 3 * ss + 3) - 2 * rs - 2 * rr * uu + rr) +
  0.5 * tp * (rs * (-3 * rr - 3 * ss + 3) - 2 * rs - 2 * rr * uu + rr);

basisGradient[15] = 0.5 * tt * tp * (-3 * rs + ss * (-3 * rr - 3 * ss + 3));
basisGradient[16] = 0.5 * tt * tp * (-3 * rs + rr * (-3 * rr - 3 * ss + 3) + 4 * ss - 1);
basisGradient[17] = 0.5 * tt * (rs * (-3 * rr - 3 * ss + 3) - 2 * rs - 2 * ss * uu + ss) +
  0.5 * tp * (rs * (-3 * rr - 3 * ss + 3) - 2 * rs - 2 * ss * uu + ss);

basisGradient[18] = -0.5 * tt * tm * (4 - 12 * ss) * uu + 0.5 * tt * tm * (-12 * rs + 4 * rr);
basisGradient[19] = 6.0 * rr * tt * tm * uu + 0.5 * tt * tm * (-12 * rs + 4 * rr);
basisGradient[20] = 0.5 * tt * (-12 * rs + 4 * rr) * uu - 0.5 * tm * (-12 * rs + 4 * rr) * uu;

basisGradient[21] = -0.5 * tt * tm * (12 * rs - ss * (-12 * rr - 12 * ss + 12) + 4 * ss);
basisGradient[22] = -0.5 * tt * tm * (12 * rs - rr * (-12 * rr - 12 * ss + 12) + 4 * rr);
basisGradient[23] = 0.5 * tt * (-rs * (-12 * rr - 12 * ss + 12) + 4 * rs) -
  0.5 * tm * (-rs * (-12 * rr - 12 * ss + 12) + 4 * rs);

basisGradient[24] = 6.0 * ss * tt * tm * uu + 0.5 * tt * tm * (-12 * rs + 4 * ss);
basisGradient[25] = -0.5 * tt * tm * (4 - 12 * rr) * uu + 0.5 * tt * tm * (-12 * rs + 4 * ss);
basisGradient[26] = 0.5 * tt * (-12 * rs + 4 * ss) * uu - 0.5 * tm * (-12 * rs + 4 * ss) * uu;

basisGradient[27] = 0.5 * tt * (4 - 12 * ss) * tp * uu - 0.5 * tt * tp * (-12 * rs + 4 * rr);
basisGradient[28] = -6.0 * rr * tt * tp * uu - 0.5 * tt * tp * (-12 * rs + 4 * rr);
basisGradient[29] = 0.5 * tt * (-12 * rs + 4 * rr) * uu + 0.5 * tp * (-12 * rs + 4 * rr) * uu;

basisGradient[30] = 0.5 * tt * tp * (12 * rs - ss * (-12 * rr - 12 * ss + 12) + 4 * ss);
basisGradient[31] = 0.5 * tt * tp * (12 * rs - rr * (-12 * rr - 12 * ss + 12) + 4 * rr);
basisGradient[32] = 0.5 * tt * (-rs * (-12 * rr - 12 * ss + 12) + 4 * rs) +
  0.5 * tp * (-rs * (-12 * rr - 12 * ss + 12) + 4 * rs);

basisGradient[33] = -6.0 * ss * tt * tp * uu - 0.5 * tt * tp * (-12 * rs + 4 * ss);
basisGradient[34] = 0.5 * tt * (4 - 12 * rr) * tp * uu - 0.5 * tt * tp * (-12 * rs + 4 * ss);
basisGradient[35] = 0.5 * tt * (-12 * rs + 4 * ss) * uu + 0.5 * tp * (-12 * rs + 4 * ss) * uu;

basisGradient[36] = tm * (3 * ss - 2) * tp * uu - tm * tp * (3 * rs - 2 * rr - 2 * ss + 1);
basisGradient[37] = tm * (3 * rr - 2) * tp * uu - tm * tp * (3 * rs - 2 * rr - 2 * ss + 1);
basisGradient[38] =
  tm * uu * (3 * rs - 2 * rr - 2 * ss + 1) - tp * uu * (3 * rs - 2 * rr - 2 * ss + 1);

basisGradient[39] = tm * tp * (-3 * rs + 4 * rr + ss * (-3 * rr - 3 * ss + 3) - 1);
basisGradient[40] = tm * tp * (-3 * rs + rr * (-3 * rr - 3 * ss + 3));
basisGradient[41] = tm * (rs * (-3 * rr - 3 * ss + 3) - 2 * rs - 2 * rr * uu + rr) -
  tp * (rs * (-3 * rr - 3 * ss + 3) - 2 * rs - 2 * rr * uu + rr);

basisGradient[42] = tm * tp * (-3 * rs + ss * (-3 * rr - 3 * ss + 3));
basisGradient[43] = tm * tp * (-3 * rs + rr * (-3 * rr - 3 * ss + 3) + 4 * ss - 1);
basisGradient[44] = tm * (rs * (-3 * rr - 3 * ss + 3) - 2 * rs - 2 * ss * uu + ss) -
  tp * (rs * (-3 * rr - 3 * ss + 3) - 2 * rs - 2 * ss * uu + ss);

basisGradient[45] = 13.5 * rs * tt * tm - 13.5 * ss * tt * tm * uu;
basisGradient[46] = 13.5 * rs * tt * tm - 13.5 * rr * tt * tm * uu;
basisGradient[47] = 13.5 * rs * tt * uu - 13.5 * rs * tm * uu;

basisGradient[48] = -13.5 * rs * tt * tp + 13.5 * ss * tt * tp * uu;
basisGradient[49] = -13.5 * rs * tt * tp + 13.5 * rr * tt * tp * uu;
basisGradient[50] = 13.5 * rs * tt * uu + 13.5 * rs * tp * uu;

basisGradient[51] = tm * (4 - 12 * ss) * tp * uu - tm * tp * (-12 * rs + 4 * rr);
basisGradient[52] = -12 * rr * tm * tp * uu - tm * tp * (-12 * rs + 4 * rr);
basisGradient[53] = tm * (-12 * rs + 4 * rr) * uu - tp * (-12 * rs + 4 * rr) * uu;

basisGradient[54] = tm * tp * (12 * rs - ss * (-12 * rr - 12 * ss + 12) + 4 * ss);
basisGradient[55] = tm * tp * (12 * rs - rr * (-12 * rr - 12 * ss + 12) + 4 * rr);
basisGradient[56] =
  tm * (-rs * (-12 * rr - 12 * ss + 12) + 4 * rs) - tp * (-rs * (-12 * rr - 12 * ss + 12) + 4 * rs);

basisGradient[57] = -12 * ss * tm * tp * uu - tm * tp * (-12 * rs + 4 * ss);
basisGradient[58] = tm * (4 - 12 * rr) * tp * uu - tm * tp * (-12 * rs + 4 * ss);
basisGradient[59] = tm * (-12 * rs + 4 * ss) * uu - tp * (-12 * rs + 4 * ss) * uu;

basisGradient[60] = -rs * tm * (27 * tt + 27) + ss * tm * (27 * tt + 27) * uu;
basisGradient[61] = -rs * tm * (27 * tt + 27) + rr * tm * (27 * tt + 27) * uu;
basisGradient[62] = 27 * rs * tm * uu - rs * (27 * tt + 27) * uu;
