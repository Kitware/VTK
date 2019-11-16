//-----------------------------------------------------------------------------
// Super cursor traversal table to retrieve the child index for each cursor
// of the parent node. There are (2*d+1)*f^d entries in the table.
// d = 1 f = 2
static const unsigned int VonNeumannChildCursorToParentCursorTable12[6] = {
  0, 1, 1,
  1, 1, 2,
};
// d = 1 f = 3
static const unsigned int VonNeumannChildCursorToParentCursorTable13[9] = {
  0, 1, 1,
  1, 1, 1,
  1, 1, 2,
};
// d = 2 f = 2
static const unsigned int VonNeumannChildCursorToParentCursorTable22[20] = {
  0, 1, 2, 2, 2,
  0, 2, 2, 3, 2,
  2, 1, 2, 2, 4,
  2, 2, 2, 3, 4,
};
// d = 2 f = 3
static const unsigned int VonNeumannChildCursorToParentCursorTable23[45] = {
  0, 1, 2, 2, 2,
  0, 2, 2, 2, 2,
  0, 2, 2, 3, 2,
  2, 1, 2, 2, 2,
  2, 2, 2, 2, 2,
  2, 2, 2, 3, 2,
  2, 1, 2, 2, 4,
  2, 2, 2, 2, 4,
  2, 2, 2, 3, 4,
};
// d = 3 f = 2
static const unsigned int VonNeumannChildCursorToParentCursorTable32[56] = {
  0, 1, 2, 3, 3, 3, 3,
  0, 1, 3, 3, 4, 3, 3,
  0, 3, 2, 3, 3, 5, 3,
  0, 3, 3, 3, 4, 5, 3,
  3, 1, 2, 3, 3, 3, 6,
  3, 1, 3, 3, 4, 3, 6,
  3, 3, 2, 3, 3, 5, 6,
  3, 3, 3, 3, 4, 5, 6,
};
// d = 3 f = 3
static const unsigned int VonNeumannChildCursorToParentCursorTable33[189] = {
  0, 1, 2, 3, 3, 3, 3,
  0, 1, 3, 3, 3, 3, 3,
  0, 1, 3, 3, 4, 3, 3,
  0, 3, 2, 3, 3, 3, 3,
  0, 3, 3, 3, 3, 3, 3,
  0, 3, 3, 3, 4, 3, 3,
  0, 3, 2, 3, 3, 5, 3,
  0, 3, 3, 3, 3, 5, 3,
  0, 3, 3, 3, 4, 5, 3,
  3, 1, 2, 3, 3, 3, 3,
  3, 1, 3, 3, 3, 3, 3,
  3, 1, 3, 3, 4, 3, 3,
  3, 3, 2, 3, 3, 3, 3,
  3, 3, 3, 3, 3, 3, 3,
  3, 3, 3, 3, 4, 3, 3,
  3, 3, 2, 3, 3, 5, 3,
  3, 3, 3, 3, 3, 5, 3,
  3, 3, 3, 3, 4, 5, 3,
  3, 1, 2, 3, 3, 3, 6,
  3, 1, 3, 3, 3, 3, 6,
  3, 1, 3, 3, 4, 3, 6,
  3, 3, 2, 3, 3, 3, 6,
  3, 3, 3, 3, 3, 3, 6,
  3, 3, 3, 3, 4, 3, 6,
  3, 3, 2, 3, 3, 5, 6,
  3, 3, 3, 3, 3, 5, 6,
  3, 3, 3, 3, 4, 5, 6,
};
static const unsigned int* VonNeumannChildCursorToParentCursorTable[3][2] = {
  {VonNeumannChildCursorToParentCursorTable12,
   VonNeumannChildCursorToParentCursorTable13},
  {VonNeumannChildCursorToParentCursorTable22,
   VonNeumannChildCursorToParentCursorTable23},
  {VonNeumannChildCursorToParentCursorTable32,
   VonNeumannChildCursorToParentCursorTable33}
};
//-----------------------------------------------------------------------------
// Super cursor traversal table to go retrieve the child index for each cursor
// of the child node. There are (2*d+1)*f^d entries in the table.
// d = 1 f = 2
static const unsigned int VonNeumannChildCursorToChildTable12[6] = {
  1, 0, 1,
  0, 1, 0,
};
// d = 1 f = 3
static const unsigned int VonNeumannChildCursorToChildTable13[9] = {
  2, 0, 1,
  0, 1, 2,
  1, 2, 0,
};
// d = 2 f = 2
static const unsigned int VonNeumannChildCursorToChildTable22[20] = {
  2, 1, 0, 1, 2,
  3, 0, 1, 0, 3,
  0, 3, 2, 3, 0,
  1, 2, 3, 2, 1,
};
// d = 2 f = 3
static const unsigned int VonNeumannChildCursorToChildTable23[45] = {
  6, 2, 0, 1, 3,
  7, 0, 1, 2, 4,
  8, 1, 2, 0, 5,
  0, 5, 3, 4, 6,
  1, 3, 4, 5, 7,
  2, 4, 5, 3, 8,
  3, 8, 6, 7, 0,
  4, 6, 7, 8, 1,
  5, 7, 8, 6, 2,
};
// d = 3 f = 2
static const unsigned int VonNeumannChildCursorToChildTable32[56] = {
  4, 2, 1, 0, 1, 2, 4,
  5, 3, 0, 1, 0, 3, 5,
  6, 0, 3, 2, 3, 0, 6,
  7, 1, 2, 3, 2, 1, 7,
  0, 6, 5, 4, 5, 6, 0,
  1, 7, 4, 5, 4, 7, 1,
  2, 4, 7, 6, 7, 4, 2,
  3, 5, 6, 7, 6, 5, 3,
};
// d = 3 f = 3
static const unsigned int VonNeumannChildCursorToChildTable33[189] = {
  18, 6, 2, 0, 1, 3, 9,
  19, 7, 0, 1, 2, 4, 10,
  20, 8, 1, 2, 0, 5, 11,
  21, 0, 5, 3, 4, 6, 12,
  22, 1, 3, 4, 5, 7, 13,
  23, 2, 4, 5, 3, 8, 14,
  24, 3, 8, 6, 7, 0, 15,
  25, 4, 6, 7, 8, 1, 16,
  26, 5, 7, 8, 6, 2, 17,
  0, 15, 11, 9, 10, 12, 18,
  1, 16, 9, 10, 11, 13, 19,
  2, 17, 10, 11, 9, 14, 20,
  3, 9, 14, 12, 13, 15, 21,
  4, 10, 12, 13, 14, 16, 22,
  5, 11, 13, 14, 12, 17, 23,
  6, 12, 17, 15, 16, 9, 24,
  7, 13, 15, 16, 17, 10, 25,
  8, 14, 16, 17, 15, 11, 26,
  9, 24, 20, 18, 19, 21, 0,
  10, 25, 18, 19, 20, 22, 1,
  11, 26, 19, 20, 18, 23, 2,
  12, 18, 23, 21, 22, 24, 3,
  13, 19, 21, 22, 23, 25, 4,
  14, 20, 22, 23, 21, 26, 5,
  15, 21, 26, 24, 25, 18, 6,
  16, 22, 24, 25, 26, 19, 7,
  17, 23, 25, 26, 24, 20, 8,
};
static const unsigned int* VonNeumannChildCursorToChildTable[3][2] = {
  {VonNeumannChildCursorToChildTable12,
   VonNeumannChildCursorToChildTable13},
  {VonNeumannChildCursorToChildTable22,
   VonNeumannChildCursorToChildTable23},
  {VonNeumannChildCursorToChildTable32,
   VonNeumannChildCursorToChildTable33}
};
//-----------------------------------------------------------------------------
