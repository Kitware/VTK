basisGradient[0 * 3 + 0] = -0.125 * (1.0 - ss) * (1.0 - tt) * (-rr - ss - tt - 2.0) -
  0.125 * (1.0 - rr) * (1.0 - ss) * (1.0 - tt);
basisGradient[0 * 3 + 1] = -0.125 * (1.0 - rr) * (1.0 - tt) * (-rr - ss - tt - 2.0) -
  0.125 * (1.0 - rr) * (1.0 - ss) * (1.0 - tt);
basisGradient[0 * 3 + 2] = -0.125 * (1.0 - rr) * (1.0 - ss) * (-rr - ss - tt - 2.0) -
  0.125 * (1.0 - rr) * (1.0 - ss) * (1.0 - tt);

basisGradient[1 * 3 + 0] = 0.125 * (1.0 - ss) * (1.0 - tt) * (rr - ss - tt - 2.0) +
  0.125 * (1.0 + rr) * (1.0 - ss) * (1.0 - tt);
basisGradient[1 * 3 + 1] = -0.125 * (1.0 + rr) * (1.0 - tt) * (rr - ss - tt - 2.0) -
  0.125 * (1.0 + rr) * (1.0 - ss) * (1.0 - tt);
basisGradient[1 * 3 + 2] = -0.125 * (1.0 + rr) * (1.0 - ss) * (rr - ss - tt - 2.0) -
  0.125 * (1.0 + rr) * (1.0 - ss) * (1.0 - tt);

basisGradient[2 * 3 + 0] = 0.125 * (1.0 + ss) * (1.0 - tt) * (rr + ss - tt - 2.0) +
  0.125 * (1.0 + rr) * (1.0 + ss) * (1.0 - tt);
basisGradient[2 * 3 + 1] = 0.125 * (1.0 + rr) * (1.0 - tt) * (rr + ss - tt - 2.0) +
  0.125 * (1.0 + rr) * (1.0 + ss) * (1.0 - tt);
basisGradient[2 * 3 + 2] = -0.125 * (1.0 + rr) * (1.0 + ss) * (rr + ss - tt - 2.0) -
  0.125 * (1.0 + rr) * (1.0 + ss) * (1.0 - tt);

basisGradient[3 * 3 + 0] = -0.125 * (1.0 + ss) * (1.0 - tt) * (-rr + ss - tt - 2.0) -
  0.125 * (1.0 - rr) * (1.0 + ss) * (1.0 - tt);
basisGradient[3 * 3 + 1] = 0.125 * (1.0 - rr) * (1.0 - tt) * (-rr + ss - tt - 2.0) +
  0.125 * (1.0 - rr) * (1.0 + ss) * (1.0 - tt);
basisGradient[3 * 3 + 2] = -0.125 * (1.0 - rr) * (1.0 + ss) * (-rr + ss - tt - 2.0) -
  0.125 * (1.0 - rr) * (1.0 + ss) * (1.0 - tt);

basisGradient[4 * 3 + 0] = -0.125 * (1.0 - ss) * (1.0 + tt) * (-rr - ss + tt - 2.0) -
  0.125 * (1.0 - rr) * (1.0 - ss) * (1.0 + tt);
basisGradient[4 * 3 + 1] = -0.125 * (1.0 - rr) * (1.0 + tt) * (-rr - ss + tt - 2.0) -
  0.125 * (1.0 - rr) * (1.0 - ss) * (1.0 + tt);
basisGradient[4 * 3 + 2] = 0.125 * (1.0 - rr) * (1.0 - ss) * (-rr - ss + tt - 2.0) +
  0.125 * (1.0 - rr) * (1.0 - ss) * (1.0 + tt);

basisGradient[5 * 3 + 0] = 0.125 * (1.0 - ss) * (1.0 + tt) * (rr - ss + tt - 2.0) +
  0.125 * (1.0 + rr) * (1.0 - ss) * (1.0 + tt);
basisGradient[5 * 3 + 1] = -0.125 * (1.0 + rr) * (1.0 + tt) * (rr - ss + tt - 2.0) -
  0.125 * (1.0 + rr) * (1.0 - ss) * (1.0 + tt);
basisGradient[5 * 3 + 2] = 0.125 * (1.0 + rr) * (1.0 - ss) * (rr - ss + tt - 2.0) +
  0.125 * (1.0 + rr) * (1.0 - ss) * (1.0 + tt);

basisGradient[6 * 3 + 0] = 0.125 * (1.0 + ss) * (1.0 + tt) * (rr + ss + tt - 2.0) +
  0.125 * (1.0 + rr) * (1.0 + ss) * (1.0 + tt);
basisGradient[6 * 3 + 1] = 0.125 * (1.0 + rr) * (1.0 + tt) * (rr + ss + tt - 2.0) +
  0.125 * (1.0 + rr) * (1.0 + ss) * (1.0 + tt);
basisGradient[6 * 3 + 2] = 0.125 * (1.0 + rr) * (1.0 + ss) * (rr + ss + tt - 2.0) +
  0.125 * (1.0 + rr) * (1.0 + ss) * (1.0 + tt);

basisGradient[7 * 3 + 0] = -0.125 * (1.0 + ss) * (1.0 + tt) * (-rr + ss + tt - 2.0) -
  0.125 * (1.0 - rr) * (1.0 + ss) * (1.0 + tt);
basisGradient[7 * 3 + 1] = 0.125 * (1.0 - rr) * (1.0 + tt) * (-rr + ss + tt - 2.0) +
  0.125 * (1.0 - rr) * (1.0 + ss) * (1.0 + tt);
basisGradient[7 * 3 + 2] = 0.125 * (1.0 - rr) * (1.0 + ss) * (-rr + ss + tt - 2.0) +
  0.125 * (1.0 - rr) * (1.0 + ss) * (1.0 + tt);

basisGradient[8 * 3 + 0] = -0.5 * rr * (1.0 - ss) * (1.0 - tt);
basisGradient[8 * 3 + 1] = -0.25 * (1.0 - rr * rr) * (1.0 - tt);
basisGradient[8 * 3 + 2] = -0.25 * (1.0 - rr * rr) * (1.0 - ss);

basisGradient[9 * 3 + 0] = 0.25 * (1.0 - ss * ss) * (1.0 - tt);
basisGradient[9 * 3 + 1] = -0.5 * ss * (1.0 + rr) * (1.0 - tt);
basisGradient[9 * 3 + 2] = -0.25 * (1.0 + rr) * (1.0 - ss * ss);

basisGradient[10 * 3 + 0] = -0.5 * rr * (1.0 + ss) * (1.0 - tt);
basisGradient[10 * 3 + 1] = 0.25 * (1.0 - rr * rr) * (1.0 - tt);
basisGradient[10 * 3 + 2] = -0.25 * (1.0 - rr * rr) * (1.0 + ss);

basisGradient[11 * 3 + 0] = -0.25 * (1.0 - ss * ss) * (1.0 - tt);
basisGradient[11 * 3 + 1] = -0.5 * ss * (1.0 - rr) * (1.0 - tt);
basisGradient[11 * 3 + 2] = -0.25 * (1.0 - rr) * (1.0 - ss * ss);

basisGradient[12 * 3 + 0] = -0.25 * (1.0 - ss) * (1.0 - tt * tt);
basisGradient[12 * 3 + 1] = -0.25 * (1.0 - rr) * (1.0 - tt * tt);
basisGradient[12 * 3 + 2] = -0.5 * tt * (1.0 - rr) * (1.0 - ss);

basisGradient[13 * 3 + 0] = 0.25 * (1.0 - ss) * (1.0 - tt * tt);
basisGradient[13 * 3 + 1] = -0.25 * (1.0 + rr) * (1.0 - tt * tt);
basisGradient[13 * 3 + 2] = -0.5 * tt * (1.0 + rr) * (1.0 - ss);

basisGradient[14 * 3 + 0] = 0.25 * (1.0 + ss) * (1.0 - tt * tt);
basisGradient[14 * 3 + 1] = 0.25 * (1.0 + rr) * (1.0 - tt * tt);
basisGradient[14 * 3 + 2] = -0.5 * tt * (1.0 + rr) * (1.0 + ss);

basisGradient[15 * 3 + 0] = -0.25 * (1.0 + ss) * (1.0 - tt * tt);
basisGradient[15 * 3 + 1] = 0.25 * (1.0 - rr) * (1.0 - tt * tt);
basisGradient[15 * 3 + 2] = -0.5 * tt * (1.0 - rr) * (1.0 + ss);

basisGradient[16 * 3 + 0] = -0.5 * rr * (1.0 - ss) * (1.0 + tt);
basisGradient[16 * 3 + 1] = -0.25 * (1.0 - rr * rr) * (1.0 + tt);
basisGradient[16 * 3 + 2] = 0.25 * (1.0 - rr * rr) * (1.0 - ss);

basisGradient[17 * 3 + 0] = 0.25 * (1.0 - ss * ss) * (1.0 + tt);
basisGradient[17 * 3 + 1] = -0.5 * ss * (1.0 + rr) * (1.0 + tt);
basisGradient[17 * 3 + 2] = 0.25 * (1.0 + rr) * (1.0 - ss * ss);

basisGradient[18 * 3 + 0] = -0.5 * rr * (1.0 + ss) * (1.0 + tt);
basisGradient[18 * 3 + 1] = 0.25 * (1.0 - rr * rr) * (1.0 + tt);
basisGradient[18 * 3 + 2] = 0.25 * (1.0 - rr * rr) * (1.0 + ss);

basisGradient[19 * 3 + 0] = -0.25 * (1.0 - ss * ss) * (1.0 + tt);
basisGradient[19 * 3 + 1] = -0.5 * ss * (1.0 - rr) * (1.0 + tt);
basisGradient[19 * 3 + 2] = 0.25 * (1.0 - rr) * (1.0 - ss * ss);
