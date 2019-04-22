/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkHyperTreeGridNonOrientedMooreSuperCursor.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright Nonice for more information.

=========================================================================*/
#include "vtkHyperTreeGridNonOrientedMooreSuperCursor.h"

#include "vtkBitArray.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridGeometryLevelEntry.h"
#include "vtkHyperTreeGridNonOrientedGeometryCursor.h"
#include "vtkIdList.h"

#include "vtkObjectFactory.h"

#include <cassert>

vtkStandardNewMacro(vtkHyperTreeGridNonOrientedMooreSuperCursor);

//-----------------------------------------------------------------------------
// Super cursor traversal table to retrieve the child index for each cursor
// of the parent node. There are (3*f)^d entries in the table.
// d = 1 f = 2
static const unsigned int MooreChildCursorToChildTable12[6] = {
  1, 0, 1,
  0, 1, 0,
};
// d = 1 f = 3
static const unsigned int MooreChildCursorToChildTable13[9] = {
  2, 0, 1,
  0, 1, 2,
  1, 2, 0,
};
// d = 2 f = 2
static const unsigned int MooreChildCursorToChildTable22[36] = {
  3, 2, 3, 1, 0, 1, 3, 2, 3,
  2, 3, 2, 0, 1, 0, 2, 3, 2,
  1, 0, 1, 3, 2, 3, 1, 0, 1,
  0, 1, 0, 2, 3, 2, 0, 1, 0,
};
// d = 2 f = 3
static const unsigned int MooreChildCursorToChildTable23[81] = {
  8, 6, 7, 2, 0, 1, 5, 3, 4,
  6, 7, 8, 0, 1, 2, 3, 4, 5,
  7, 8, 6, 1, 2, 0, 4, 5, 3,
  2, 0, 1, 5, 3, 4, 8, 6, 7,
  0, 1, 2, 3, 4, 5, 6, 7, 8,
  1, 2, 0, 4, 5, 3, 7, 8, 6,
  5, 3, 4, 8, 6, 7, 2, 0, 1,
  3, 4, 5, 6, 7, 8, 0, 1, 2,
  4, 5, 3, 7, 8, 6, 1, 2, 0,
};

// d = 3 f = 2
static const unsigned int MooreChildCursorToChildTable32[216] = {
  7, 6, 7, 5, 4, 5, 7, 6, 7, 3, 2, 3, 1, 0, 1, 3, 2, 3, 7, 6, 7, 5, 4, 5, 7, 6, 7,
  6, 7, 6, 4, 5, 4, 6, 7, 6, 2, 3, 2, 0, 1, 0, 2, 3, 2, 6, 7, 6, 4, 5, 4, 6, 7, 6,
  5, 4, 5, 7, 6, 7, 5, 4, 5, 1, 0, 1, 3, 2, 3, 1, 0, 1, 5, 4, 5, 7, 6, 7, 5, 4, 5,
  4, 5, 4, 6, 7, 6, 4, 5, 4, 0, 1, 0, 2, 3, 2, 0, 1, 0, 4, 5, 4, 6, 7, 6, 4, 5, 4,
  3, 2, 3, 1, 0, 1, 3, 2, 3, 7, 6, 7, 5, 4, 5, 7, 6, 7, 3, 2, 3, 1, 0, 1, 3, 2, 3,
  2, 3, 2, 0, 1, 0, 2, 3, 2, 6, 7, 6, 4, 5, 4, 6, 7, 6, 2, 3, 2, 0, 1, 0, 2, 3, 2,
  1, 0, 1, 3, 2, 3, 1, 0, 1, 5, 4, 5, 7, 6, 7, 5, 4, 5, 1, 0, 1, 3, 2, 3, 1, 0, 1,
  0, 1, 0, 2, 3, 2, 0, 1, 0, 4, 5, 4, 6, 7, 6, 4, 5, 4, 0, 1, 0, 2, 3, 2, 0, 1, 0,
};

// d = 3 f = 3
static const unsigned int MooreChildCursorToChildTable33[729] = {
  26, 24, 25, 20, 18, 19, 23, 21, 22, 8, 6, 7, 2, 0, 1, 5, 3, 4, 17, 15, 16, 11, 9, 10, 14, 12, 13,
  24, 25, 26, 18, 19, 20, 21, 22, 23, 6, 7, 8, 0, 1, 2, 3, 4, 5, 15, 16, 17, 9, 10, 11, 12, 13, 14,
  25, 26, 24, 19, 20, 18, 22, 23, 21, 7, 8, 6, 1, 2, 0, 4, 5, 3, 16, 17, 15, 10, 11, 9, 13, 14, 12,
  20, 18, 19, 23, 21, 22, 26, 24, 25, 2, 0, 1, 5, 3, 4, 8, 6, 7, 11, 9, 10, 14, 12, 13, 17, 15, 16,
  18, 19, 20, 21, 22, 23, 24, 25, 26, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
  19, 20, 18, 22, 23, 21, 25, 26, 24, 1, 2, 0, 4, 5, 3, 7, 8, 6, 10, 11, 9, 13, 14, 12, 16, 17, 15,
  23, 21, 22, 26, 24, 25, 20, 18, 19, 5, 3, 4, 8, 6, 7, 2, 0, 1, 14, 12, 13, 17, 15, 16, 11, 9, 10,
  21, 22, 23, 24, 25, 26, 18, 19, 20, 3, 4, 5, 6, 7, 8, 0, 1, 2, 12, 13, 14, 15, 16, 17, 9, 10, 11,
  22, 23, 21, 25, 26, 24, 19, 20, 18, 4, 5, 3, 7, 8, 6, 1, 2, 0, 13, 14, 12, 16, 17, 15, 10, 11, 9,
  8, 6, 7, 2, 0, 1, 5, 3, 4, 17, 15, 16, 11, 9, 10, 14, 12, 13, 26, 24, 25, 20, 18, 19, 23, 21, 22,
  6, 7, 8, 0, 1, 2, 3, 4, 5, 15, 16, 17, 9, 10, 11, 12, 13, 14, 24, 25, 26, 18, 19, 20, 21, 22, 23,
  7, 8, 6, 1, 2, 0, 4, 5, 3, 16, 17, 15, 10, 11, 9, 13, 14, 12, 25, 26, 24, 19, 20, 18, 22, 23, 21,
  2, 0, 1, 5, 3, 4, 8, 6, 7, 11, 9, 10, 14, 12, 13, 17, 15, 16, 20, 18, 19, 23, 21, 22, 26, 24, 25,
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
  1, 2, 0, 4, 5, 3, 7, 8, 6, 10, 11, 9, 13, 14, 12, 16, 17, 15, 19, 20, 18, 22, 23, 21, 25, 26, 24,
  5, 3, 4, 8, 6, 7, 2, 0, 1, 14, 12, 13, 17, 15, 16, 11, 9, 10, 23, 21, 22, 26, 24, 25, 20, 18, 19,
  3, 4, 5, 6, 7, 8, 0, 1, 2, 12, 13, 14, 15, 16, 17, 9, 10, 11, 21, 22, 23, 24, 25, 26, 18, 19, 20,
  4, 5, 3, 7, 8, 6, 1, 2, 0, 13, 14, 12, 16, 17, 15, 10, 11, 9, 22, 23, 21, 25, 26, 24, 19, 20, 18,
  17, 15, 16, 11, 9, 10, 14, 12, 13, 26, 24, 25, 20, 18, 19, 23, 21, 22, 8, 6, 7, 2, 0, 1, 5, 3, 4,
  15, 16, 17, 9, 10, 11, 12, 13, 14, 24, 25, 26, 18, 19, 20, 21, 22, 23, 6, 7, 8, 0, 1, 2, 3, 4, 5,
  16, 17, 15, 10, 11, 9, 13, 14, 12, 25, 26, 24, 19, 20, 18, 22, 23, 21, 7, 8, 6, 1, 2, 0, 4, 5, 3,
  11, 9, 10, 14, 12, 13, 17, 15, 16, 20, 18, 19, 23, 21, 22, 26, 24, 25, 2, 0, 1, 5, 3, 4, 8, 6, 7,
  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 0, 1, 2, 3, 4, 5, 6, 7, 8,
  10, 11, 9, 13, 14, 12, 16, 17, 15, 19, 20, 18, 22, 23, 21, 25, 26, 24, 1, 2, 0, 4, 5, 3, 7, 8, 6,
  14, 12, 13, 17, 15, 16, 11, 9, 10, 23, 21, 22, 26, 24, 25, 20, 18, 19, 5, 3, 4, 8, 6, 7, 2, 0, 1,
  12, 13, 14, 15, 16, 17, 9, 10, 11, 21, 22, 23, 24, 25, 26, 18, 19, 20, 3, 4, 5, 6, 7, 8, 0, 1, 2,
  13, 14, 12, 16, 17, 15, 10, 11, 9, 22, 23, 21, 25, 26, 24, 19, 20, 18, 4, 5, 3, 7, 8, 6, 1, 2, 0,
};
static const unsigned int* MooreChildCursorToChildTable[3][2] = {
  {MooreChildCursorToChildTable12,
   MooreChildCursorToChildTable13},
  {MooreChildCursorToChildTable22,
   MooreChildCursorToChildTable23},
  {MooreChildCursorToChildTable32,
   MooreChildCursorToChildTable33}
};
//-----------------------------------------------------------------------------
// Super cursor traversal table to go retrieve the child index for each cursor
// of the child node. There are (3*f)^d entries in the table.
//-----------------------------------------------------------------------------
// d = 1 f = 2
static const unsigned int MooreChildCursorToParentCursorTable12[6] = {
  0, 1, 1,
  1, 1, 2,
};
// d = 1 f = 3
static const unsigned int MooreChildCursorToParentCursorTable13[9] = {
  0, 1, 1,
  1, 1, 1,
  1, 1, 2,
};
// d = 2 f = 2
static const unsigned int MooreChildCursorToParentCursorTable22[36] = {
  0, 1, 1, 3, 4, 4, 3, 4, 4,
  1, 1, 2, 4, 4, 5, 4, 4, 5,
  3, 4, 4, 3, 4, 4, 6, 7, 7,
  4, 4, 5, 4, 4, 5, 7, 7, 8,
};
// d = 2 f = 3
static const unsigned int MooreChildCursorToParentCursorTable23[81] = {
  0, 1, 1, 3, 4, 4, 3, 4, 4,
  1, 1, 1, 4, 4, 4, 4, 4, 4,
  1, 1, 2, 4, 4, 5, 4, 4, 5,
  3, 4, 4, 3, 4, 4, 3, 4, 4,
  4, 4, 4, 4, 4, 4, 4, 4, 4,
  4, 4, 5, 4, 4, 5, 4, 4, 5,
  3, 4, 4, 3, 4, 4, 6, 7, 7,
  4, 4, 4, 4, 4, 4, 7, 7, 7,
  4, 4, 5, 4, 4, 5, 7, 7, 8,
};

// d = 3 f = 2
static const unsigned int MooreChildCursorToParentCursorTable32[216] = {
  0, 1, 1, 3, 4, 4, 3, 4, 4, 9, 10, 10, 12, 13, 13, 12, 13, 13, 9, 10, 10, 12, 13, 13, 12, 13, 13,
  1, 1, 2, 4, 4, 5, 4, 4, 5, 10, 10, 11, 13, 13, 14, 13, 13, 14, 10, 10, 11, 13, 13, 14, 13, 13, 14,
  3, 4, 4, 3, 4, 4, 6, 7, 7, 12, 13, 13, 12, 13, 13, 15, 16, 16, 12, 13, 13, 12, 13, 13, 15, 16, 16,
  4, 4, 5, 4, 4, 5, 7, 7, 8, 13, 13, 14, 13, 13, 14, 16, 16, 17, 13, 13, 14, 13, 13, 14, 16, 16, 17,
  9, 10, 10, 12, 13, 13, 12, 13, 13, 9, 10, 10, 12, 13, 13, 12, 13, 13, 18, 19, 19, 21, 22, 22, 21, 22, 22,
  10, 10, 11, 13, 13, 14, 13, 13, 14, 10, 10, 11, 13, 13, 14, 13, 13, 14, 19, 19, 20, 22, 22, 23, 22, 22, 23,
  12, 13, 13, 12, 13, 13, 15, 16, 16, 12, 13, 13, 12, 13, 13, 15, 16, 16, 21, 22, 22, 21, 22, 22, 24, 25, 25,
  13, 13, 14, 13, 13, 14, 16, 16, 17, 13, 13, 14, 13, 13, 14, 16, 16, 17, 22, 22, 23, 22, 22, 23, 25, 25, 26,
};

// d = 3 f = 3
static const unsigned int MooreChildCursorToParentCursorTable33[729] = {
  0, 1, 1, 3, 4, 4, 3, 4, 4, 9, 10, 10, 12, 13, 13, 12, 13, 13, 9, 10, 10, 12, 13, 13, 12, 13, 13,
  1, 1, 1, 4, 4, 4, 4, 4, 4, 10, 10, 10, 13, 13, 13, 13, 13, 13, 10, 10, 10, 13, 13, 13, 13, 13, 13,
  1, 1, 2, 4, 4, 5, 4, 4, 5, 10, 10, 11, 13, 13, 14, 13, 13, 14, 10, 10, 11, 13, 13, 14, 13, 13, 14,
  3, 4, 4, 3, 4, 4, 3, 4, 4, 12, 13, 13, 12, 13, 13, 12, 13, 13, 12, 13, 13, 12, 13, 13, 12, 13, 13,
  4, 4, 4, 4, 4, 4, 4, 4, 4, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
  4, 4, 5, 4, 4, 5, 4, 4, 5, 13, 13, 14, 13, 13, 14, 13, 13, 14, 13, 13, 14, 13, 13, 14, 13, 13, 14,
  3, 4, 4, 3, 4, 4, 6, 7, 7, 12, 13, 13, 12, 13, 13, 15, 16, 16, 12, 13, 13, 12, 13, 13, 15, 16, 16,
  4, 4, 4, 4, 4, 4, 7, 7, 7, 13, 13, 13, 13, 13, 13, 16, 16, 16, 13, 13, 13, 13, 13, 13, 16, 16, 16,
  4, 4, 5, 4, 4, 5, 7, 7, 8, 13, 13, 14, 13, 13, 14, 16, 16, 17, 13, 13, 14, 13, 13, 14, 16, 16, 17,
  9, 10, 10, 12, 13, 13, 12, 13, 13, 9, 10, 10, 12, 13, 13, 12, 13, 13, 9, 10, 10, 12, 13, 13, 12, 13, 13,
  10, 10, 10, 13, 13, 13, 13, 13, 13, 10, 10, 10, 13, 13, 13, 13, 13, 13, 10, 10, 10, 13, 13, 13, 13, 13, 13,
  10, 10, 11, 13, 13, 14, 13, 13, 14, 10, 10, 11, 13, 13, 14, 13, 13, 14, 10, 10, 11, 13, 13, 14, 13, 13, 14,
  12, 13, 13, 12, 13, 13, 12, 13, 13, 12, 13, 13, 12, 13, 13, 12, 13, 13, 12, 13, 13, 12, 13, 13, 12, 13, 13,
  13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
  13, 13, 14, 13, 13, 14, 13, 13, 14, 13, 13, 14, 13, 13, 14, 13, 13, 14, 13, 13, 14, 13, 13, 14, 13, 13, 14,
  12, 13, 13, 12, 13, 13, 15, 16, 16, 12, 13, 13, 12, 13, 13, 15, 16, 16, 12, 13, 13, 12, 13, 13, 15, 16, 16,
  13, 13, 13, 13, 13, 13, 16, 16, 16, 13, 13, 13, 13, 13, 13, 16, 16, 16, 13, 13, 13, 13, 13, 13, 16, 16, 16,
  13, 13, 14, 13, 13, 14, 16, 16, 17, 13, 13, 14, 13, 13, 14, 16, 16, 17, 13, 13, 14, 13, 13, 14, 16, 16, 17,
  9, 10, 10, 12, 13, 13, 12, 13, 13, 9, 10, 10, 12, 13, 13, 12, 13, 13, 18, 19, 19, 21, 22, 22, 21, 22, 22,
  10, 10, 10, 13, 13, 13, 13, 13, 13, 10, 10, 10, 13, 13, 13, 13, 13, 13, 19, 19, 19, 22, 22, 22, 22, 22, 22,
  10, 10, 11, 13, 13, 14, 13, 13, 14, 10, 10, 11, 13, 13, 14, 13, 13, 14, 19, 19, 20, 22, 22, 23, 22, 22, 23,
  12, 13, 13, 12, 13, 13, 12, 13, 13, 12, 13, 13, 12, 13, 13, 12, 13, 13, 21, 22, 22, 21, 22, 22, 21, 22, 22,
  13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 22, 22, 22, 22, 22, 22, 22, 22, 22,
  13, 13, 14, 13, 13, 14, 13, 13, 14, 13, 13, 14, 13, 13, 14, 13, 13, 14, 22, 22, 23, 22, 22, 23, 22, 22, 23,
  12, 13, 13, 12, 13, 13, 15, 16, 16, 12, 13, 13, 12, 13, 13, 15, 16, 16, 21, 22, 22, 21, 22, 22, 24, 25, 25,
  13, 13, 13, 13, 13, 13, 16, 16, 16, 13, 13, 13, 13, 13, 13, 16, 16, 16, 22, 22, 22, 22, 22, 22, 25, 25, 25,
  13, 13, 14, 13, 13, 14, 16, 16, 17, 13, 13, 14, 13, 13, 14, 16, 16, 17, 22, 22, 23, 22, 22, 23, 25, 25, 26,
};
static const unsigned int* MooreChildCursorToParentCursorTable[3][2] = {
  {MooreChildCursorToParentCursorTable12,
   MooreChildCursorToParentCursorTable13},
  {MooreChildCursorToParentCursorTable22,
   MooreChildCursorToParentCursorTable23},
  {MooreChildCursorToParentCursorTable32,
   MooreChildCursorToParentCursorTable33}
};
//-----------------------------------------------------------------------------
// Corner/leaf traversal tables to retrieve the parent cursor indices of all
// leaves touching a given corner of the parent node.
//-----------------------------------------------------------------------------
static const int CornerNeighborCursorsTable1D0[2] = {
   0, 1, };
static const int CornerNeighborCursorsTable1D1[2] = {
   1, 2, };
static const int* CornerNeighborCursorsTable1D[2] = {
  CornerNeighborCursorsTable1D0,
  CornerNeighborCursorsTable1D1,
};
static const int CornerNeighborCursorsTable2D0[4] = {
   0, 1, 3, 4, };
static const int CornerNeighborCursorsTable2D1[4] = {
   1, 2, 4, 5, };
static const int CornerNeighborCursorsTable2D2[4] = {
   3, 4, 6, 7, };
static const int CornerNeighborCursorsTable2D3[4] = {
   4, 5, 7, 8, };
static const int* CornerNeighborCursorsTable2D[4] = {
  CornerNeighborCursorsTable2D0,
  CornerNeighborCursorsTable2D1,
  CornerNeighborCursorsTable2D2,
  CornerNeighborCursorsTable2D3,
};
static const unsigned int CornerNeighborCursorsTable3D0[8] = {
   0, 1, 3, 4, 9, 10, 12, 13, };
static const unsigned int CornerNeighborCursorsTable3D1[8] = {
   1, 2, 4, 5, 10, 11, 13, 14, };
static const unsigned int CornerNeighborCursorsTable3D2[8] = {
   3, 4, 6, 7, 12, 13, 15, 16, };
static const unsigned int CornerNeighborCursorsTable3D3[8] = {
   4, 5, 7, 8, 13, 14, 16, 17, };
static const unsigned int CornerNeighborCursorsTable3D4[8] = {
   9, 10, 12, 13, 18, 19, 21, 22, };
static const unsigned int CornerNeighborCursorsTable3D5[8] = {
   10, 11, 13, 14, 19, 20, 22, 23, };
static const unsigned int CornerNeighborCursorsTable3D6[8] = {
   12, 13, 15, 16, 21, 22, 24, 25, };
static const unsigned int CornerNeighborCursorsTable3D7[8] = {
   13, 14, 16, 17, 22, 23, 25, 26, };
static const unsigned int* CornerNeighborCursorsTable3D[8] = {
  CornerNeighborCursorsTable3D0,
  CornerNeighborCursorsTable3D1,
  CornerNeighborCursorsTable3D2,
  CornerNeighborCursorsTable3D3,
  CornerNeighborCursorsTable3D4,
  CornerNeighborCursorsTable3D5,
  CornerNeighborCursorsTable3D6,
  CornerNeighborCursorsTable3D7,
};
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedMooreSuperCursor::Initialize(vtkHyperTreeGrid* grid, vtkIdType treeIndex, bool create)
{
  assert("pre: Non_valid_dimension" &&
          grid->GetDimension() >= 1 &&
          grid->GetDimension() <= 3);
  assert("pre: Non_valid_branchfactor" &&
          grid->GetBranchFactor() >= 2 &&
          grid->GetBranchFactor() <= 3);

  if (this->Grid == nullptr)
  {
    this->Grid = grid;
  }
  assert("pre: Non_same_grid" &&
          this->Grid == grid);

  // JB Initialize caracteristique
  switch (grid->GetNumberOfChildren())
  {
         case 2:
         {
           // dimension = 1, branch factor = 2
           this->IndiceCentralCursor = 1;
           this->NumberOfCursors = 3;
           this->ChildCursorToParentCursorTable
             = MooreChildCursorToParentCursorTable[0][0];
           this->ChildCursorToChildTable
             = MooreChildCursorToChildTable[0][0];
           break;
        }
        case 3:
        {
           // dimension = 1, branch factor = 3
           this->IndiceCentralCursor = 1;
           this->NumberOfCursors = 3;
           this->ChildCursorToParentCursorTable
             = MooreChildCursorToParentCursorTable[0][1];
           this->ChildCursorToChildTable
             = MooreChildCursorToChildTable[0][1];
           break;
        }
         case 4:
         {
           // dimension = 2, branch factor = 2
           this->IndiceCentralCursor = 4;
           this->NumberOfCursors = 9;
           this->ChildCursorToParentCursorTable
             = MooreChildCursorToParentCursorTable[1][0];
           this->ChildCursorToChildTable
             = MooreChildCursorToChildTable[1][0];
           break;
         }
         case 9:
         {
           // dimension = 2, branch factor = 3
           this->IndiceCentralCursor = 4;
           this->NumberOfCursors = 9;
           this->ChildCursorToParentCursorTable
             = MooreChildCursorToParentCursorTable[1][1];
           this->ChildCursorToChildTable
             = MooreChildCursorToChildTable[1][1];
           break;
         }
         case 8:
         {
           // dimension = 3, branch factor = 2
           this->IndiceCentralCursor = 13;
           this->NumberOfCursors = 27;
           this->ChildCursorToParentCursorTable
             = MooreChildCursorToParentCursorTable[2][0];
           this->ChildCursorToChildTable
             = MooreChildCursorToChildTable[2][0];
           break;
         }
         case 27:
         {
           // dimension = 3, branch factor = 3
           this->IndiceCentralCursor = 13;
           this->NumberOfCursors = 27;
           this->ChildCursorToParentCursorTable
             = MooreChildCursorToParentCursorTable[2][1];
           this->ChildCursorToChildTable
             = MooreChildCursorToChildTable[2][1];
           break;
         }
  } // switch Dimension

  // JB Pour le niveau zero tout est defini
  this->CentralCursor->Initialize(grid, treeIndex, create);
  //
  this->CurrentFirstNonValidEntryByLevel = 0;
  if (this->FirstNonValidEntryByLevel.size() <= this->CurrentFirstNonValidEntryByLevel + 1)
  {
    this->FirstNonValidEntryByLevel.resize(this->CurrentFirstNonValidEntryByLevel + 1);
  }
  this->FirstNonValidEntryByLevel[ this->CurrentFirstNonValidEntryByLevel ] = this->NumberOfCursors - 1;
  bool isOld = true;
  if (this->Entries.size() <= this->CurrentFirstNonValidEntryByLevel + 1)
  {
    isOld = false;
    this->Entries.resize(this->FirstNonValidEntryByLevel[ this->CurrentFirstNonValidEntryByLevel ]);
  }
  // JB Pour le niveau zero tout est reference
  this->FirstCurrentNeighboorReferenceEntry = 0;
  if (this->ReferenceEntries.size() <= this->FirstCurrentNeighboorReferenceEntry + this->NumberOfCursors - 1)
  {
    this->ReferenceEntries.resize(this->FirstCurrentNeighboorReferenceEntry + this->NumberOfCursors - 1);
  }
  std::vector< unsigned int >::iterator it = this->ReferenceEntries.begin();
  for (unsigned int icrt = this->FirstCurrentNeighboorReferenceEntry;
        it != this->ReferenceEntries.end();
        ++ it, ++ icrt)
  {
    (*it) = icrt;
  }

  // If dimension=d: center cursor is d
  //                 d-faces neighbor cursors are 0,...,2d except d
  unsigned int i, j, k;
  grid->GetLevelZeroCoordinatesFromIndex(treeIndex, i, j, k);
  unsigned int n[3];
  grid->GetCellDims(n);

  // JB Initialisation des cursors
  switch (grid->GetDimension())
  {
    case 1:
    {
        // dimension == 1
       if(i > 0)
       {
         // Cell has a neighbor to the left
         unsigned int r =
           grid->GetShiftedLevelZeroIndex(treeIndex, (unsigned int)-1, 0, 0);
         this->Entries[0].Initialize(grid, r);
       } else if (isOld) {
         this->Entries[0].Reset();
       }
       if(i + 1 < n[0])
       {
         // Cell has a neighbor to the right
         unsigned int r =
           grid->GetShiftedLevelZeroIndex(treeIndex, 1, 0, 0);
         this->Entries[1].Initialize(grid, r); // au lieu de 2
       } else if (isOld) {
         this->Entries[1].Reset();
       }
       break;
    }
    case 2:
    {
       // dimension == 2
       bool toW = (i > 0);
       bool toS = (j > 0);
       bool toE = (i + 1 < n[0]);
       bool toN = (j + 1 < n[1]);
       if (toS)
       {
         // Cell has a neighbor to the south
         unsigned int r =
           grid->GetShiftedLevelZeroIndex(treeIndex, 0, (unsigned int)-1, 0);
         this->Entries[1].Initialize(grid, r);
         if (toW)
         {
           // Cell has a neighbor to the southwest
           r =
             grid->GetShiftedLevelZeroIndex(treeIndex, (unsigned int)-1, (unsigned int)-1, 0);
           this->Entries[0].Initialize(grid, r);
         } else { // if (toW)
           this->Entries[0].Reset();
         }
         if (toE)
         {
           // Cell has a neighbor to the southeast
           r =
             grid->GetShiftedLevelZeroIndex(treeIndex, 1, (unsigned int)-1, 0);
           this->Entries[2].Initialize(grid, r);
         } else { // if (toE)
           this->Entries[2].Reset();
         }
       } else { // if (toS)
         this->Entries[0].Reset();
         this->Entries[1].Reset();
         this->Entries[2].Reset();
       }

       if (toW)
       {
         // Cell has a neighbor to the west
         unsigned int r =
           grid->GetShiftedLevelZeroIndex(treeIndex, (unsigned int)-1, 0, 0);
         this->Entries[3].Initialize(grid, r);
       } else { // if (toW)
         this->Entries[3].Reset();
       }
       if (toE)
       {
         // Cell has a neighbor to the east
         unsigned int r =
           grid->GetShiftedLevelZeroIndex(treeIndex, 1, 0, 0);
         this->Entries[4].Initialize(grid, r); // au lieu de 5
       } else { // if (toE)
         this->Entries[4].Reset();
       }
       if (toN)
       {
         // Cell has a neighbor to the north
         unsigned int r =
           grid->GetShiftedLevelZeroIndex(treeIndex, 0, 1, 0);
         this->Entries[6].Initialize(grid, r); // au lieu de 7
         if (toW)
         {
           // Cell has a neighbor to the northwest
           r =
             grid->GetShiftedLevelZeroIndex(treeIndex, (unsigned int)-1, 1, 0);
           this->Entries[5].Initialize(grid, r); // au lieu de 6
         } else { // if (toW)
           this->Entries[5].Reset();
         }
         if (toE)
         {
           // Cell has a neighbor to the northeast
           r =
             grid->GetShiftedLevelZeroIndex(treeIndex, 1, 1, 0);
           this->Entries[7].Initialize(grid, r); // au lieu de 8
         } else { // if (toW)
           this->Entries[7].Reset();
         }
       } else {// if (toN)
         this->Entries[5].Reset();
         this->Entries[6].Reset();
         this->Entries[7].Reset();
       }
       break;
    }
    case 3:
    {
       // Initialize all connectivity cursors
       for (unsigned int _c = 0; _c < this->NumberOfCursors - 1; ++ _c)
       {
         this->Entries[ _c ].Reset();
       } // _c

       // dimension == 3
       int minI = (i == 0) ? 0 : -1;
       int maxI = (i + 1 < n[0]) ? 2 : 1;
       int minJ = (j == 0) ? 0 : -1;
       int maxJ = (j + 1 < n[1]) ? 2 : 1;
       int minK = (k == 0) ? 0 : -1;
       int maxK = (k + 1 < n[2]) ? 2 : 1;

       // Initialize all connectivity cursors
       for (int _k = minK; _k < maxK; ++ _k)
       {
         for (int _j = minJ; _j < maxJ; ++ _j)
         {
           for (int _i = minI; _i < maxI; ++ _i)
           {
             unsigned int c = 13 + _i + 3 * _j + 9 * _k;
             if (c != this->IndiceCentralCursor)
             {
               unsigned int r =
                 grid->GetShiftedLevelZeroIndex(treeIndex, _i, _j, _k);
               if (c < this->IndiceCentralCursor)
               {
                 this->Entries[ c ].Initialize(grid, r);
               } else {
                 this->Entries[ c - 1 ].Initialize(grid, r);
               }
             }
           } // _i
         } // _j
       } // _k
       break;
    }
  } // switch Dimension
}

//-----------------------------------------------------------------------------
bool vtkHyperTreeGridNonOrientedMooreSuperCursor::GetCornerCursors(unsigned int c, unsigned int l, vtkIdList* leaves)
{
  unsigned int cursorIdx = 0;
  switch (this->GetDimension())
  {
    case 1:
      // dimension == 1
      cursorIdx = CornerNeighborCursorsTable1D[c][l];
      break;
    case 2:
      // dimension == 2
      cursorIdx = CornerNeighborCursorsTable2D[c][l];
      break;
    case 3:
      // dimension == 3
      cursorIdx = CornerNeighborCursorsTable3D[c][l];
      break;
    default:
      vtkErrorMacro("unexpected neighborhood");
      return false;
  } // switch (N)

  // Collect the cursor index for this leaf
  leaves->SetId(l, cursorIdx);

  // Determine ownership of corner
  bool owner = true;
  if(cursorIdx != this->IndiceCentralCursor)
  {
    vtkHyperTreeGridGeometryLevelEntry& cursor = this->Entries[ this->GetIndiceEntry(cursorIdx) ];
    if (! cursor.GetTree() || ! cursor.IsLeaf(this->Grid))
    {
      // If neighbor cell is out of bounds or has Non been
      // refined to a leaf, that leaf does Non own the corner
      owner = false;
    }
    else if (this->GetGrid()->HasMask()
               && this->GetGrid()->GetMask()->GetTuple1(cursor.GetGlobalNodeIndex()))
    {
      // If neighbor cell is masked, that leaf does Non own the corner
      owner = false;
    }
    else if (this->IndiceCentralCursor < cursorIdx &&
              cursor.GetLevel() == this->GetLevel())
    {
      // A level tie is broken in favor of the largest index
      owner = false;
    }
  } // if(cursorIdx! = this->IndiceCentralCursor)

  // Return ownership of corner by this node
  return owner;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridNonOrientedMooreSuperCursor::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "--vtkHyperTreeGridNonOrientedMooreSuperCursor--" << endl;
  vtkHyperTreeGridNonOrientedSuperCursor::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridNonOrientedMooreSuperCursor::~vtkHyperTreeGridNonOrientedMooreSuperCursor()
{
#ifndef NDEBUG
  std::cerr << "vtkHyperTreeGridNonOrientedMooreSuperCursor:" << std::endl;
#endif
}

//-----------------------------------------------------------------------------
