/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericCellTessellator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGenericCellTessellator.h"
#include "vtkObjectFactory.h"

#include "vtkPoints.h"
#include "vtkIdList.h"
#include "vtkGenericAdaptorCell.h"
#include "vtkPointData.h"
#include "vtkDoubleArray.h"
#include "vtkMergePoints.h"
#include "vtkCellArray.h"
#include "vtkGenericEdgeTable.h"
#include "vtkGenericSubdivisionErrorMetric.h"
#include "vtkGenericAttribute.h"
#include "vtkGenericAttributeCollection.h"
#include "vtkGenericCellIterator.h"

#include <vtkstd/queue>
#include <assert.h>

// Pre computed table for the point to edge equivalence:
// [edge][point]
static int TRIANGLE_EDGES_TABLE[3][2] = {{0, 1}, {1, 2}, {0, 2}};


// Pre computed table for the tessellation of triangles
#define NO_TRIAN {-1,-1,-1}

static signed char vtkTessellatorTriangleCases[9][4][3] = {
// Case no edge is split:  -> 0
{ NO_TRIAN, NO_TRIAN, NO_TRIAN, NO_TRIAN},
// Case edge 3 is split:  -> 1
{{0, 3, 2},{1, 2, 3}, NO_TRIAN, NO_TRIAN},
// Case edge 4 is split:  -> 2
{{0, 1, 4},{0, 4, 2}, NO_TRIAN, NO_TRIAN},
// Case edge 3 & 4 are split:  -> 3
{{0, 3, 4},{1, 4, 3},{0, 4, 2}, NO_TRIAN},
// Case edge 5 is split:  -> 4
{{0, 1, 5},{1, 2, 5}, NO_TRIAN, NO_TRIAN},
// Case edge 3 & 5 are split:  -> 5
{{0, 3, 5},{1, 5, 3},{1, 2, 5}, NO_TRIAN},
// Case edge 4 & 5 are split:  -> 6
{{0, 4, 5},{0, 1, 4},{2, 5, 4}, NO_TRIAN},
// Case edge 4, 5 & 6 are split:  -> 7
{{0, 3, 5},{3, 4, 5},{1, 4, 3},{2, 5, 4}},
//In case we reach outside the table
{ NO_TRIAN, NO_TRIAN, NO_TRIAN, NO_TRIAN},
};


// Pre computed table for the point to edge equivalence:
// [edge][point]
static int TETRA_EDGES_TABLE[6][2] = {{0, 1}, {1, 2}, {0, 2}, {0, 3}, {1, 3},
                                      {2, 3}};

// Pre computed table for the tessellation of tetras
// There is two cases for the tessellation of a tetra, it is either oriented
// with the right hand rule or with the left hand rule
#define NO_TETRA {-1,-1,-1,-1}

static signed char vtkTessellatorTetraCasesRight[65][8][4] = {
// Case no edge is split:  -> 0
{ NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edge 4 is split:  -> 1
{{0,2,3,4},{1,2,4,3}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edge 5 is split:  -> 2
{{0,2,3,5},{0,1,5,3}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edge 4 & 5 are split:  -> 3
{{0,2,3,5},{0,3,4,5},{1,3,5,4}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edge 6 is split:  -> 4
{{0,1,6,3},{1,2,6,3}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edge 4 & 6 are split:  -> 5
{{0,3,4,6},{1,2,6,3},{1,3,6,4}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edge 5 & 6 are split:  -> 6
{{0,1,5,3},{0,3,5,6},{2,3,6,5}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 4, 5 & 6 are split:  -> 7
{{0,3,4,6},{1,3,5,4},{2,3,6,5},{3,4,6,5}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edge 7 is split:  -> 8
{{1,2,7,3},{0,1,2,7}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edge 4 & 7 are split:  -> 9
{{1,2,7,3},{0,2,7,4},{1,2,4,7}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edge 5 & 7 are split:  -> 10
{{2,3,7,5},{1,3,5,7},{0,2,7,5},{0,1,5,7}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 4, 5 & 7 are split:  -> 11
{{2,3,7,5},{1,3,5,7},{0,2,7,5},{1,4,7,5},{0,4,5,7}, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edge 6 & 7 are split:  -> 12
{{1,2,7,3},{1,2,6,7},{0,1,6,7}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 4, 6 & 7 are split:  -> 13
{{1,2,7,3},{1,2,6,7},{1,4,7,6},{0,4,6,7}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 5, 6 & 7 are split:  -> 14
{{2,3,7,5},{1,3,5,7},{2,5,7,6},{0,1,5,7},{0,5,6,7}, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 4, 5, 6 & 7 are split:   -> 15
{{2,3,7,5},{1,3,5,7},{2,5,7,6},{1,4,7,5},{0,4,6,7},{4,5,6,7}, NO_TETRA, NO_TETRA},
// Case edge 8 is split:  -> 16
{{0,2,3,8},{0,1,2,8}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edge 4 & 8 are split:  -> 17
{{0,2,3,8},{0,2,8,4},{1,2,4,8}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edge 5 & 8 are split:  -> 18
{{0,2,3,8},{0,2,8,5},{0,1,5,8}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 4, 5 & 8 are split:  -> 19
{{0,2,3,8},{0,2,8,5},{1,4,8,5},{0,4,5,8}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edge 6 & 8 are split:  -> 20
{{2,3,6,8},{0,3,8,6},{1,2,6,8},{0,1,6,8}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 4, 6 & 8 are split:  -> 21
{{2,3,6,8},{0,3,8,6},{1,2,6,8},{1,4,8,6},{0,4,6,8}, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 5, 6 & 8 are split:  -> 22
{{2,3,6,8},{0,3,8,6},{2,5,8,6},{0,1,5,8},{0,5,6,8}, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 4, 5, 6 & 8 are split:  -> 23
{{2,3,6,8},{0,3,8,6},{2,5,8,6},{1,4,8,5},{0,4,6,8},{4,5,6,8}, NO_TETRA, NO_TETRA},
// Case edge 7 & 8 are split:  -> 24
{{2,3,7,8},{0,1,2,8},{0,2,7,8}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 4, 7 & 8 are split:  -> 25
{{2,3,7,8},{0,2,7,4},{2,4,8,7},{1,2,4,8}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 5, 7 & 8 are split:  -> 26
{{2,3,7,8},{2,5,8,7},{0,2,7,5},{0,1,5,8},{0,5,7,8}, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 4, 5, 7 & 8 are split:  -> 27
{{2,3,7,8},{2,5,8,7},{0,2,7,5},{1,4,8,5},{0,4,5,7},{4,5,7,8}, NO_TETRA, NO_TETRA},
// Case edges 6, 7 & 8 are split:  -> 28
{{2,3,7,8},{2,6,8,7},{1,2,6,8},{0,1,6,8},{0,6,7,8}, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 4, 6, 7 & 8 are split:  -> 29
{{2,3,7,8},{2,6,8,7},{1,2,6,8},{1,4,8,6},{0,4,6,7},{4,6,7,8}, NO_TETRA, NO_TETRA},
// Case edges 5, 6, 7 & 8 are split:  -> 30
{{2,3,7,8},{2,5,8,7},{2,5,7,6},{0,1,5,8},{0,5,7,8},{0,5,6,7}, NO_TETRA, NO_TETRA},
// Case edges 4, 5, 6, 7 & 8 are split:  -> 31
{{2,3,7,8},{2,5,8,7},{2,5,7,6},{1,4,8,5},{0,4,6,7},{4,5,7,8},{4,5,6,7}, NO_TETRA},
// Case edge 9 is split:  -> 32
{{0,1,9,3},{0,1,2,9}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edge 4 & 9 are split:  -> 33
{{1,3,9,4},{0,3,4,9},{1,2,4,9},{0,2,9,4}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edge 5 & 9 are split:  -> 34
{{0,1,9,3},{0,2,9,5},{0,1,5,9}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 4, 5 & 9 are split:  -> 35
{{1,3,9,4},{0,3,4,9},{0,2,9,5},{1,4,9,5},{0,4,5,9}, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edge 6 & 9 are split:  -> 36
{{0,1,9,3},{1,2,6,9},{0,1,6,9}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 4, 6 & 9 are split:  -> 37
{{1,3,9,4},{0,3,4,9},{1,2,6,9},{1,4,9,6},{0,4,6,9}, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 5, 6 & 9 are split:  -> 38
{{0,1,9,3},{2,5,9,6},{0,1,5,9},{0,5,6,9}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 4, 5, 6 & 9 are split:  -> 39
{{1,3,9,4},{0,3,4,9},{2,5,9,6},{1,4,9,5},{0,4,6,9},{4,5,6,9}, NO_TETRA, NO_TETRA},
// Case edge 7 & 9 are split:  -> 40
{{1,3,9,7},{0,1,2,9},{0,1,9,7}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 4, 7 & 9 are split:  -> 41
{{1,3,9,7},{1,2,4,9},{0,2,9,4},{1,4,7,9},{0,4,9,7}, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 5, 7 & 9 are split:  -> 42
{{1,3,9,7},{0,2,9,5},{1,5,7,9},{0,1,5,7},{0,5,9,7}, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 4, 5, 7 & 9 are split:  -> 43
{{1,3,9,7},{0,2,9,5},{1,5,7,9},{1,4,7,5},{0,5,9,7},{0,4,5,7}, NO_TETRA, NO_TETRA},
// Case edges 6, 7 & 9 are split:  -> 44
{{1,3,9,7},{1,2,6,9},{0,1,6,7},{1,6,7,9}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 4, 6, 7 & 9 are split:  -> 45
{{1,3,9,7},{1,2,6,9},{1,4,9,6},{1,4,7,9},{0,4,6,7},{4,6,7,9}, NO_TETRA, NO_TETRA},
// Case edges 5, 6, 7 & 9 are split:  -> 46
{{1,3,9,7},{2,5,9,6},{1,5,7,9},{0,1,5,7},{0,5,6,7},{5,6,7,9}, NO_TETRA, NO_TETRA},
// Case edges 4, 5, 6, 7 & 9 are split:  -> 47
{{1,3,9,7},{2,5,9,6},{1,5,7,9},{1,4,7,5},{0,4,6,7},{5,6,7,9},{4,5,6,7}, NO_TETRA},
// Case edge 8 & 9 are split:  -> 48
{{0,3,8,9},{0,1,2,9},{0,1,9,8}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 4, 8 & 9 are split:  -> 49
{{0,3,8,9},{1,2,4,9},{0,2,9,4},{1,4,8,9},{0,4,9,8}, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 5, 8 & 9 are split:  -> 50
{{0,3,8,9},{0,2,9,5},{0,1,5,8},{0,5,9,8}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 4, 5, 8 & 9 are split:  -> 51
{{0,3,8,9},{0,2,9,5},{1,4,8,5},{0,4,9,8},{0,4,5,9},{4,5,9,8}, NO_TETRA, NO_TETRA},
// Case edges 6, 8 & 9 are split:  -> 52
{{0,3,8,9},{1,2,6,9},{1,6,8,9},{0,1,6,8},{0,6,9,8}, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 4, 6, 8 & 9 are split:  -> 53
{{0,3,8,9},{1,2,6,9},{1,6,8,9},{1,4,8,6},{0,6,9,8},{0,4,6,8}, NO_TETRA, NO_TETRA},
// Case edges 5, 6, 8 & 9 are split:  -> 54
{{0,3,8,9},{2,5,9,6},{0,1,5,8},{0,5,6,8},{0,6,9,8},{5,6,8,9}, NO_TETRA, NO_TETRA},
// Case edges 4, 5, 6, 8 & 9 are split:  -> 55
{{0,3,8,9},{2,5,9,6},{1,4,8,5},{0,6,9,8},{0,4,6,8},{5,6,8,9},{4,5,6,8}, NO_TETRA},
// Case edges 7, 8 & 9 are split:  -> 56
{{3,7,9,8},{0,1,2,9},{0,1,9,8},{0,7,8,9}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 4, 7, 8 & 9 are split:  -> 57
{{3,7,9,8},{1,2,4,9},{0,2,9,4},{1,4,8,9},{0,4,9,7},{4,7,8,9}, NO_TETRA, NO_TETRA},
// Case edges 5, 7, 8 & 9 are split:  -> 58
{{3,7,9,8},{0,2,9,5},{0,1,5,8},{0,5,7,8},{0,5,9,7},{5,7,8,9}, NO_TETRA, NO_TETRA},
// Case edges 4, 5, 7, 8 & 9 are split:  -> 59
{{3,7,9,8},{0,2,9,5},{1,4,8,5},{0,5,9,7},{0,4,5,7},{5,7,8,9},{4,5,7,8}, NO_TETRA},
// Case edges 6, 7, 8 & 9 are split:  -> 60
{{3,7,9,8},{1,2,6,9},{1,6,8,9},{0,1,6,8},{0,6,7,8},{6,7,8,9}, NO_TETRA, NO_TETRA},
// Case edges 4, 6, 7, 8 & 9 are split:  -> 61
{{3,7,9,8},{1,2,6,9},{1,6,8,9},{1,4,8,6},{0,4,6,7},{6,7,8,9},{4,6,7,8}, NO_TETRA},
// Case edges 5, 6, 7, 8 & 9 are split:  -> 62
{{3,7,9,8},{2,5,9,6},{0,1,5,8},{0,5,7,8},{0,5,6,7},{5,6,7,9},{5,7,8,9}, NO_TETRA},
// Case were all 6 edges are subdivided -> 8 sub tetra / 1 possibility:  -> 63
{{3,7,9,8},{2,5,9,6},{1,4,8,5},{0,4,6,7},{5,6,7,9},{5,7,8,9},{4,5,7,8},{4,5,6,7}},
//In case we reach outside the table
{ NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA}
};

//-----------------------------------------------------------------------------
//
// This table is for the case where the 'last edge' of the tetra could not be order
// properly, then we need a different case table
//
static signed char vtkTessellatorTetraCasesLeft[65][8][4] = {
// Case no edge is split:  -> 0
{ NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edge 4 is split:  -> 1
{{0,2,3,4},{1,2,4,3}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edge 5 is split:  -> 2
{{0,2,3,5},{0,1,5,3}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edge 4 & 5 are split:  -> 3
{{0,2,3,5},{0,3,4,5},{1,3,5,4}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edge 6 is split:  -> 4
{{1,2,6,3},{0,1,6,3}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edge 4 & 6 are split:  -> 5
{{1,2,6,3},{0,3,4,6},{1,3,6,4}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edge 5 & 6 are split:  -> 6
{{2,3,6,5},{0,1,5,3},{0,3,5,6}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 4, 5 & 6 are split:  -> 7
{{2,3,6,5},{0,3,4,6},{1,3,5,4},{3,4,6,5}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edge 7 is split:  -> 8
{{0,1,2,7},{1,2,7,3}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edge 4 & 7 are split:  -> 9
{{0,2,7,4},{1,2,7,3},{1,2,4,7}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edge 5 & 7 are split:  -> 10
{{2,3,7,5},{0,2,7,5},{1,3,5,7},{0,1,5,7}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 4, 5 & 7 are split:  -> 11
{{2,3,7,5},{0,2,7,5},{1,3,5,7},{1,4,7,5},{0,4,5,7}, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edge 6 & 7 are split:  -> 12
{{1,2,6,3},{1,3,6,7},{0,1,6,7}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 4, 6 & 7 are split:  -> 13
{{1,2,6,3},{1,3,6,7},{1,4,7,6},{0,4,6,7}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 5, 6 & 7 are split:  -> 14
{{2,3,6,5},{3,5,7,6},{1,3,5,7},{0,1,5,7},{0,5,6,7}, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 4, 5, 6 & 7 are split:   -> 15
{{2,3,6,5},{3,5,7,6},{1,3,5,7},{1,4,7,5},{0,4,6,7},{4,5,6,7}, NO_TETRA, NO_TETRA},
// Case edge 8 is split:  -> 16
{{0,1,2,8},{0,2,3,8}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edge 4 & 8 are split:  -> 17
{{0,2,3,8},{0,2,8,4},{1,2,4,8}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edge 5 & 8 are split:  -> 18
{{0,2,3,5},{0,3,8,5},{0,1,5,8}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 4, 5 & 8 are split:  -> 19
{{0,2,3,5},{0,3,8,5},{1,4,8,5},{0,4,5,8}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edge 6 & 8 are split:  -> 20
{{2,3,6,8},{1,2,6,8},{0,3,8,6},{0,1,6,8}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 4, 6 & 8 are split:  -> 21
{{2,3,6,8},{1,2,6,8},{0,3,8,6},{1,4,8,6},{0,4,6,8}, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 5, 6 & 8 are split:  -> 22
{{2,3,6,5},{3,5,8,6},{0,3,8,6},{0,1,5,8},{0,5,6,8}, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 4, 5, 6 & 8 are split:  -> 23
{{2,3,6,5},{3,5,8,6},{0,3,8,6},{1,4,8,5},{0,4,6,8},{4,5,6,8}, NO_TETRA, NO_TETRA},
// Case edge 7 & 8 are split:  -> 24
{{0,1,2,8},{0,2,7,8},{2,3,7,8}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 4, 7 & 8 are split:  -> 25
{{0,2,7,4},{2,3,7,8},{2,4,8,7},{1,2,4,8}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 5, 7 & 8 are split:  -> 26
{{2,3,7,5},{0,2,7,5},{3,5,8,7},{0,1,5,8},{0,5,7,8}, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 4, 5, 7 & 8 are split:  -> 27
{{2,3,7,5},{0,2,7,5},{3,5,8,7},{1,4,8,5},{0,4,5,7},{4,5,7,8}, NO_TETRA, NO_TETRA},
// Case edges 6, 7 & 8 are split:  -> 28
{{2,3,6,8},{1,2,6,8},{3,6,8,7},{0,1,6,8},{0,6,7,8}, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 4, 6, 7 & 8 are split:  -> 29
{{2,3,6,8},{1,2,6,8},{3,6,8,7},{1,4,8,6},{0,4,6,7},{4,6,7,8}, NO_TETRA, NO_TETRA},
// Case edges 5, 6, 7 & 8 are split:  -> 30
{{2,3,6,5},{3,5,7,6},{3,5,8,7},{0,1,5,8},{0,5,7,8},{0,5,6,7}, NO_TETRA, NO_TETRA},
// Case edges 4, 5, 6, 7 & 8 are split:  -> 31
{{2,3,6,5},{3,5,7,6},{3,5,8,7},{1,4,8,5},{0,4,6,7},{4,5,7,8},{4,5,6,7}, NO_TETRA},
// Case edge 9 is split:  -> 32
{{0,1,2,9},{0,1,9,3}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edge 4 & 9 are split:  -> 33
{{1,2,4,9},{0,2,9,4},{1,3,9,4},{0,3,4,9}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edge 5 & 9 are split:  -> 34
{{0,2,9,5},{0,1,9,3},{0,1,5,9}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 4, 5 & 9 are split:  -> 35
{{0,2,9,5},{1,3,9,4},{0,3,4,9},{1,4,9,5},{0,4,5,9}, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edge 6 & 9 are split:  -> 36
{{1,2,6,9},{0,1,9,3},{0,1,6,9}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 4, 6 & 9 are split:  -> 37
{{1,2,6,9},{1,3,9,4},{0,3,4,9},{1,4,9,6},{0,4,6,9}, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 5, 6 & 9 are split:  -> 38
{{2,5,9,6},{0,1,9,3},{0,1,5,9},{0,5,6,9}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 4, 5, 6 & 9 are split:  -> 39
{{2,5,9,6},{1,3,9,4},{0,3,4,9},{1,4,9,5},{0,4,6,9},{4,5,6,9}, NO_TETRA, NO_TETRA},
// Case edge 7 & 9 are split:  -> 40
{{0,1,2,9},{1,3,9,7},{0,1,9,7}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 4, 7 & 9 are split:  -> 41
{{1,2,4,9},{0,2,9,4},{1,3,9,7},{1,4,7,9},{0,4,9,7}, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 5, 7 & 9 are split:  -> 42
{{0,2,9,5},{1,3,9,7},{1,5,7,9},{0,1,5,7},{0,5,9,7}, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 4, 5, 7 & 9 are split:  -> 43
{{0,2,9,5},{1,3,9,7},{1,5,7,9},{1,4,7,5},{0,5,9,7},{0,4,5,7}, NO_TETRA, NO_TETRA},
// Case edges 6, 7 & 9 are split:  -> 44
{{1,2,6,9},{1,3,9,7},{0,1,6,7},{1,6,7,9}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 4, 6, 7 & 9 are split:  -> 45
{{1,2,6,9},{1,3,9,7},{1,4,9,6},{1,4,7,9},{0,4,6,7},{4,6,7,9}, NO_TETRA, NO_TETRA},
// Case edges 5, 6, 7 & 9 are split:  -> 46
{{2,5,9,6},{1,3,9,7},{1,5,7,9},{0,1,5,7},{0,5,6,7},{5,6,7,9}, NO_TETRA, NO_TETRA},
// Case edges 4, 5, 6, 7 & 9 are split:  -> 47
{{2,5,9,6},{1,3,9,7},{1,5,7,9},{1,4,7,5},{0,4,6,7},{5,6,7,9},{4,5,6,7}, NO_TETRA},
// Case edge 8 & 9 are split:  -> 48
{{0,1,2,9},{0,3,8,9},{0,1,9,8}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 4, 8 & 9 are split:  -> 49
{{1,2,4,9},{0,2,9,4},{0,3,8,9},{1,4,8,9},{0,4,9,8}, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 5, 8 & 9 are split:  -> 50
{{0,2,9,5},{0,3,8,9},{0,1,5,8},{0,5,9,8}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 4, 5, 8 & 9 are split:  -> 51
{{0,2,9,5},{0,3,8,9},{1,4,8,5},{0,4,9,8},{0,4,5,9},{4,5,9,8}, NO_TETRA, NO_TETRA},
// Case edges 6, 8 & 9 are split:  -> 52
{{1,2,6,9},{0,3,8,9},{1,6,8,9},{0,1,6,8},{0,6,9,8}, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 4, 6, 8 & 9 are split:  -> 53
{{1,2,6,9},{0,3,8,9},{1,6,8,9},{1,4,8,6},{0,6,9,8},{0,4,6,8}, NO_TETRA, NO_TETRA},
// Case edges 5, 6, 8 & 9 are split:  -> 54
{{2,5,9,6},{0,3,8,9},{0,1,5,8},{0,5,6,8},{0,6,9,8},{5,6,8,9}, NO_TETRA, NO_TETRA},
// Case edges 4, 5, 6, 8 & 9 are split:  -> 55
{{2,5,9,6},{0,3,8,9},{1,4,8,5},{0,6,9,8},{0,4,6,8},{5,6,8,9},{4,5,6,8}, NO_TETRA},
// Case edges 7, 8 & 9 are split:  -> 56
{{0,1,2,9},{3,7,9,8},{0,1,9,8},{0,7,8,9}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Case edges 4, 7, 8 & 9 are split:  -> 57
{{1,2,4,9},{0,2,9,4},{3,7,9,8},{1,4,8,9},{0,4,9,7},{4,7,8,9}, NO_TETRA, NO_TETRA},
// Case edges 5, 7, 8 & 9 are split:  -> 58
{{0,2,9,5},{3,7,9,8},{0,1,5,8},{0,5,7,8},{0,5,9,7},{5,7,8,9}, NO_TETRA, NO_TETRA},
// Case edges 4, 5, 7, 8 & 9 are split:  -> 59
{{0,2,9,5},{3,7,9,8},{1,4,8,5},{0,5,9,7},{0,4,5,7},{5,7,8,9},{4,5,7,8}, NO_TETRA},
// Case edges 6, 7, 8 & 9 are split:  -> 60
{{1,2,6,9},{3,7,9,8},{1,6,8,9},{0,1,6,8},{0,6,7,8},{6,7,8,9}, NO_TETRA, NO_TETRA},
// Case edges 4, 6, 7, 8 & 9 are split:  -> 61
{{1,2,6,9},{3,7,9,8},{1,6,8,9},{1,4,8,6},{0,4,6,7},{6,7,8,9},{4,6,7,8}, NO_TETRA},
// Case edges 5, 6, 7, 8 & 9 are split:  -> 62
{{2,5,9,6},{3,7,9,8},{0,1,5,8},{0,5,7,8},{0,5,6,7},{5,6,7,9},{5,7,8,9}, NO_TETRA},
// Case were all 6 edges are subdivided -> 8 sub tetra / 1 possibility:  -> 63
{{2,5,9,6},{3,7,9,8},{1,4,8,5},{0,4,6,7},{5,6,7,9},{5,7,8,9},{4,5,7,8},{4,5,6,7}},
//In case we reach outside the table
{ NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
};


vtkCxxRevisionMacro(vtkGenericCellTessellator, "1.2");
vtkStandardNewMacro(vtkGenericCellTessellator);
vtkCxxSetObjectMacro(vtkGenericCellTessellator, ErrorMetric, vtkGenericSubdivisionErrorMetric);

int FindEdgeParent(double p1[3], double p2[3], int &localId); //FIXME

//-----------------------------------------------------------------------------
//
// vtkTriangleTile
// 

class vtkTriangleTile;

class vtkTriangleTile 
{
public:
  vtkTriangleTile()
    {
    for(int i=0;i<6;i++)
      {
      this->PointId[i] = -1;
      this->Vertex[i][0] = -100;
      this->Vertex[i][1] = -100;
      this->Vertex[i][2] = -100;
      }
    }
  ~vtkTriangleTile() {}

  void SetVertex( int i , double v[3] ) 
  {
    this->Vertex[i][0] = v[0];
    this->Vertex[i][1] = v[1];
    this->Vertex[i][2] = v[2];
  }

  void SetPointId(int i, vtkIdType id) {this->PointId[i] = id;}

  void SetPointIds(vtkIdType id[3]) 
    {
    this->PointId[0] = id[0];
    this->PointId[1] = id[1];
    this->PointId[2] = id[2];
    }

  double *GetVertex( int i ) 
    {
    return this->Vertex[i];
    }

  vtkIdType GetPointId( int i ) 
    {
    return this->PointId[i];
    }

  // Return true if (e1, e2) is an edge of the tri:
  int IsAnEdge(vtkIdType e1, vtkIdType e2)
    {
    int sum = 0;
    for(int i=0; i<3; i++)
      {
      if(e1 == this->PointId[i] || e2 == this->PointId[i])
        {
        sum++;
        }
      }
    return sum == 2;
    }

  // can tile be split; if so, return TessellatePointsing tiles
  //  vtkTriangleTile res[4]
  int Refine( vtkGenericCellTessellator* tess, vtkTriangleTile *res );

private:
  // Keep track of local coordinate in order to evaluate shape function
  double Vertex[3+3][3];  //3 points + 3 mid edge points
  vtkIdType PointId[3+3];
};
//-----------------------------------------------------------------------------
//
// vtkTetraTile
// 

class vtkTetraTile;

class vtkTetraTile
{
public:
  vtkTetraTile()
    {
    for(int i=0;i<10;i++)
      {
      this->PointId[i] = -1;
      this->Vertex[i][0] = -100;
      this->Vertex[i][1] = -100;
      this->Vertex[i][2] = -100;
      }
    }
  ~vtkTetraTile() {}
    
  void SetVertex( int i , double v[3] ) 
  {
    Vertex[i][0] = v[0];
    Vertex[i][1] = v[1];
    Vertex[i][2] = v[2];
  }

  void SetPointId(int i, vtkIdType id) {this->PointId[i] = id;}

  void SetPointIds(vtkIdType id[4]) 
    {
    this->PointId[0] = id[0];
    this->PointId[1] = id[1];
    this->PointId[2] = id[2];
    this->PointId[3] = id[3];
    }

  void GetVertex( int i, double pt[3] ) 
    {
    pt[0] = this->Vertex[i][0];
    pt[1] = this->Vertex[i][1];
    pt[2] = this->Vertex[i][2];
    }

  double *GetVertex( int i ) {return Vertex[i];}
  
  vtkIdType GetPointId( int i ) {return this->PointId[i];}

  // Return true if (e1, e2) is an edge of the tetra:
  int IsAnEdge(vtkIdType e1, vtkIdType e2)
    {
    int sum = 0;
    for(int i=0; i<4; i++)
      {
      if(e1 == this->PointId[i] || e2 == this->PointId[i])
        {
        sum++;
        }
      }
    return sum == 2;
    }

  // can tile be split; if so, return TessellatePointsing tiles
  // There can't be more than 8 tetras as it corresponds to the splitting
  // of all edges 
  // vtkTetraTile res[8]
  int Refine( vtkGenericCellTessellator* tess, vtkTetraTile *res );

private:
  // Need to keep track of local coordinate to evaluate shape functions
  // So all work is done in parametric coordinate

  double Vertex[4+6][3];  // 4 tetra points + 6 mid edge points
  vtkIdType PointId[4+6];
};

//-----------------------------------------------------------------------------
int vtkTriangleTile::Refine(vtkGenericCellTessellator* tess,
                            vtkTriangleTile *res ) // res[4]
{
  int i, index;
  int numTriangleCreated;

  double edgeSplitList[3];
  vtkIdType ptId = 0;
  int l, r;

  for(i=0, index=0;i<3;i++)
    {
    // we have to calculate mid point between edge TRIANGLE_EDGES_TABLE[i][0] and
    // TRIANGLE_EDGES_TABLE[i][1]
    l = TRIANGLE_EDGES_TABLE[i][0];
    r = TRIANGLE_EDGES_TABLE[i][1];

    edgeSplitList[i] = tess->EdgeTable->CheckEdge(this->PointId[l],
      this->PointId[r], ptId);

    if( edgeSplitList[i] == -1 )
      {
      // we should not be here because on previous step we made sure to prepare
      // the hash table
      assert(0); // we should not be here
      }

    // Build the case table
    if (edgeSplitList[i])
      {
      index |= 1 << i;
      }
    }

  if( index )
    {
    // That mean at least one edge was split and thus index != 0
    signed char *cases;
    cases = **(vtkTessellatorTriangleCases + index);

    for(numTriangleCreated = 0; cases[0]> -1; cases+=3)
      {
      for(int j=0;j<3;j++)
        {
        res[numTriangleCreated].SetPointId( j, this->PointId[cases[j]] );
        res[numTriangleCreated].SetVertex( j, this->Vertex[cases[j]] );
        }
      //Insert edges from new triangle into hash table:
      tess->InsertEdgesIntoEdgeTable( res[numTriangleCreated] );

      //update number of triangles
      numTriangleCreated++;
      }
    }
  else
    {
    // no edge were split so recursion is done
    // add the cell array to the list
    numTriangleCreated = 0;
    tess->TessellateCellArray->InsertNextCell(3, this->PointId);

    double point[3], scalar[3];
    for(int j=0;j<3;j++)
      {
      tess->EdgeTable->CheckPoint(this->PointId[j], point, scalar);

      // There will some be duplicate points during a while but
      // this is the cost for speed:
      tess->TessellatePoints->InsertNextTuple( point );
      tess->TessellateScalars->InsertNextTuple( scalar );
      }
    }

  return numTriangleCreated;
}

//-----------------------------------------------------------------------------
static void Reorder(vtkIdType in[4], vtkIdType order[4]) 
{
  // Input: in[4] contains pointId of a tetra in right hand rule.
  // Output: this function reorders so that:
  // out[0] < out[1]
  // out[0] < out[2]
  // out[0] < out[3]
  // out[1] < out[2]
  // out[1] < out[3]
  // and still respect the right hand rule for tetra:


  vtkIdType min1 = in[0];
  vtkIdType min2 = in[1];
  vtkIdType idx1 = 0;
  vtkIdType idx2 = 1;
  for(int i=1;i<4;i++)
    {
    if(min1 > in[i])
      {
      min2 = min1;
      idx2 = idx1;
      min1 = in[i];
      idx1 = i;
      }
    else if(min2 > in[i])
      {
      min2 = in[i];
      idx2 = i;
      }
    }

  // For debug:
  // order[0] = order[1] = order[2] = order[3] = -1;
  order[0]=idx1;
  order[1]=idx2;
  
  if(idx1 == 0)
    {
    if(idx2 == 1)
      {
      order[2] = 2;
      order[3] = 3;
      }
    else if(idx2 == 2)
      {
      order[2] = 3;
      order[3] = 1;
      }
    else if(idx2 == 3)
      {
      order[2] = 1;
      order[3] = 2;
      }
    }
  else if(idx1 == 1)
    {
    if(idx2 == 0)
      {
      order[2] = 3;
      order[3] = 2;
      }
    else if(idx2 == 2)
      {
      order[2] = 0;
      order[3] = 3;
      }
    else if(idx2 == 3)
      {
      order[2] = 2;
      order[3] = 0;
      }
    }
  else if(idx1 == 2)
    {
    if(idx2 == 0)
      {
      order[2] = 1;
      order[3] = 3;
      }
    else if(idx2 == 1)
      {
      order[2] = 3;
      order[3] = 0;
      }
    else if(idx2 == 3)
      {
      order[2] = 0;
      order[3] = 1;
      }
    }
  else if(idx1 == 3)
    {
    if(idx2 == 0)
      {
      order[2] = 2;
      order[3] = 1;
      }
    else if(idx2 == 1)
      {
      order[2] = 0;
      order[3] = 2;
      }
    else if(idx2 == 2)
      {
      order[2] = 1;
      order[3] = 0;
      }
    }    
}

//-----------------------------------------------------------------------------
int vtkTetraTile::Refine( vtkGenericCellTessellator* tess,
                          vtkTetraTile *res ) // res[8]
{
  int i, index;
  int numTetraCreated;

  // We need to order the point by lower id first
  // this will create an edge ordering and based on that we can
  // find which edge is split this gives us a mask for the tessellation

  // There is only 6 edges in a tetra we need this structure to quickly
  // determine in which case we are to tessellate the tetra.
  int edgeSplitList[6]; //FIXME
  vtkIdType ptId = 0;
  int l, r;

  // loop over edges:
  for(i=0, index = 0;i<6;i++)
    {
    // we have to calculate mid point between edge TETRA_EDGES_TABLE[i][0] and
    // TETRA_EDGES_TABLE[i][1]
    l = TETRA_EDGES_TABLE[i][0];
    r = TETRA_EDGES_TABLE[i][1];

    edgeSplitList[i] = tess->EdgeTable->CheckEdge(this->PointId[l],
      this->PointId[r], ptId);

    if( edgeSplitList[i] == -1 )
      {
      // we should not be here because on previous step we made sure to prepare
      // the hash table
      assert(0); //we should not be here
      }

    // Build the case table
    if (edgeSplitList[i])
      {
      index |= 1 << i;
      }
    }

  if( index )
    {
    // That mean at least one edge was split and thus index != 0
    vtkIdType tetra[4], order[4];
    signed char *cases;

    // we compare right away PointId[2] to PointId[3] because we assume 
    // input tetra is already ordered properly (cf. Reorder previous step)
    if(this->PointId[2] < this->PointId[3])
      {
      cases = **(vtkTessellatorTetraCasesRight + index);
      }
    else
      {
      cases = **(vtkTessellatorTetraCasesLeft + index);
      }

    // For each sub-tetra, increment number of tetra created
    // And check each of its edges if its in the hash table
    for(numTetraCreated = 0; cases[0]> -1; cases+=4)
      {
      for(int k=0;k<4;k++)
        {
        tetra[k] = this->PointId[cases[k]];
        }

      // The whole purpose of Reorder is really to classify the tetra, the 
      // reordering is only useful for quick testing. The tet will either 
      // classify as Right ordered or Left ordered
      Reorder(tetra, order);
      
      // Set the tetras point for the next recursion     
      for(int j=0;j<4;j++)
        {
        res[numTetraCreated].SetPointId( j, this->PointId[cases[order[j]]] );
        res[numTetraCreated].SetVertex( j, this->Vertex[cases[order[j]]] );
        }

      //Insert edges from new tetra into hash table:
      tess->InsertEdgesIntoEdgeTable( res[numTetraCreated] );

      //update number of tetras
      numTetraCreated++;
      }
    }
  else
    {
    // no edge were split so recursion is done
    // add the cell array to the list
    numTetraCreated = 0;
    tess->TessellateCellArray->InsertNextCell(4, this->PointId);
    
    double point[3], scalar[3];
    for(int j=0;j<4;j++)
      {
      tess->EdgeTable->CheckPoint(this->PointId[j], point, scalar);

      // There will some be duplicate points during a while but
      // this is the cost for speed:
      tess->TessellatePoints->InsertNextTuple( point );
      tess->TessellateScalars->InsertNextTuple( scalar );
      }
    }

  return numTetraCreated;
}

//-----------------------------------------------------------------------------
// Create the tessellator helper with a default of 0.25 for threshold
//
vtkGenericCellTessellator::vtkGenericCellTessellator()
{
  this->GenericCell = NULL;
  
  this->TessellatePoints = NULL;
  this->TessellateCellArray = NULL;
  this->TessellateScalars = NULL;

  this->EdgeTable = vtkGenericEdgeTable::New();
  //this->EdgeTable->DebugOn();
  //this->DebugOn();
  
  this->AttributeCollection = NULL;
  this->ErrorMetric = vtkGenericSubdivisionErrorMetric::New();
  //this->ErrorMetric->DebugOn();
  
  this->CellIterator = 0;
}

//-----------------------------------------------------------------------------
vtkGenericCellTessellator::~vtkGenericCellTessellator()
{
  if ( this->GenericCell )
    {
    this->GenericCell->Delete();
    }
  if( this->AttributeCollection )
    {
    this->AttributeCollection->Delete();
    }
  this->EdgeTable->Delete();
  this->SetErrorMetric( 0 );
  if(this->CellIterator)
    {
    this->CellIterator->Delete();
    }
}

//-----------------------------------------------------------------------------
// This function is supposed to be called only at toplevel (for passing data
// from third party to the hash point table)
void vtkGenericCellTessellator::InsertPointsIntoEdgeTable( vtkTriangleTile &tri )
{
  double global[3];
  double scalar[10]; //FIXME: magic number
  // FB: scalar should be an argument of InsertPointsIntoEdgeTable.

  for(int j=0;j<3;j++)
    {
    // Need to check first if point is not already in the hash table
    // since EvaluateLocation / EvaluateTuple are expensive calls
    if( !this->EdgeTable->CheckPoint(tri.GetPointId(j)) )
      {
      // it's real space coordinate:
      this->GenericCell->EvaluateLocation(0,tri.GetVertex(j), global);
  
      // Then scalar value associated with point:  
      //this->GenericCell->EvaluateShapeFunction(tri.GetVertex(j), scalar);
//      this->AttributeCollection->EvaluateTuple(this->GenericCell,tri.GetVertex(j), scalar);
      this->GenericCell->InterpolateTuple(this->AttributeCollection,
                                          tri.GetVertex(j), scalar);
      //Put everything in ths point hash table
      this->EdgeTable->InsertPointAndScalar(tri.GetPointId(j), global, scalar);
      }
    }
}

//-----------------------------------------------------------------------------
// This function is supposed to be called only at toplevel (for passing data
// from third party to the hash point table)
void vtkGenericCellTessellator::InsertPointsIntoEdgeTable(vtkTetraTile &tetra )
{
  double global[3];
  double scalar[10]; //FIXME: magic number
  // FB: see remark in  InsertPointsIntoEdgeTable(vtkTetraTile &)

  for(int j=0;j<4;j++)
    {
    // Need to check first if point is not already in the hash table
    // since EvaluateLocation / EvaluateTuple are expensive calls
    if( !this->EdgeTable->CheckPoint(tetra.GetPointId(j)) )
      {
      // it's real space coordinate:
      this->GenericCell->EvaluateLocation(0,tetra.GetVertex(j), global);
  
      // Then scalar value associated with point:  
      //this->GenericCell->EvaluateShapeFunction(tetra.GetVertex(j), scalar);
//      this->AttributeCollection->EvaluateTuple(this->GenericCell,tetra.GetVertex(j), scalar);
      this->GenericCell->InterpolateTuple(this->AttributeCollection,
                                          tetra.GetVertex(j), scalar);
      
      //Put everything in ths point hash table
      this->EdgeTable->InsertPointAndScalar(tetra.GetPointId(j), global,
                                            scalar);
      }
    }
}

//-----------------------------------------------------------------------------
// 
void vtkGenericCellTessellator::InsertEdgesIntoEdgeTable(vtkTriangleTile &tri )
{
  double local[3];
  double global[3];
  double scalar[10]; //FIXME: magic number
  // FB: see remark in  InsertPointsIntoEdgeTable(vtkTetraTile &)
  vtkIdType l, r;
  vtkIdType cellId = this->GenericCell->GetId();

  //First setup the point reference count:
  for(int i = 0; i<3; i++)
    {
    this->EdgeTable->IncrementPointReferenceCount( tri.GetPointId(i));
    }

  // Loop over all edges:
  // For each edge:
  //    if in hash table: incr ref 
  //    else:             evaluate & put in table ref = 1
  for(int j=0;j<3;j++)
    {
    l = TRIANGLE_EDGES_TABLE[j][0];
    r = TRIANGLE_EDGES_TABLE[j][1];
        
    double *left = tri.GetVertex(l);
    double *right = tri.GetVertex(r);

    vtkIdType leftId = tri.GetPointId(l);
    vtkIdType rightId = tri.GetPointId(r);

    //Check first in the hash table
    vtkIdType ptId = -1;
    int refCount = 1;

    //vtkDebugMacro( << "InsertEdgesIntoEdgeTable:" << leftId << "," << rightId );

    // To calculate the edge ref count, we either:
    // - find it in the hash table
    // - calculate from higher order cell:

    if( this->EdgeTable->CheckEdge(leftId, rightId, ptId) == -1)
      {
      // The edge was not found in the hash table, that mean we have to 
      // determine it's reference counting from the higher order cell:
      int localId;
      //int type = this->GenericCell->FindEdgeParent(left, right, localId);
      int type = FindEdgeParent(left, right, localId);
      
      if( type == 1)
        {
        // On edge:
        refCount = this->GetNumberOfCellsUsingEdge( localId );
        }
      else if( type == 2)
        {
        //On face:
        //refCount = this->GenericCell->GetNumberOfCellsUsingFace( localId );
        //assert("check: " && 0 );
        
        //This means inside FIXME FIXME FIXME FIXME FIXME FIXME 
        refCount = 1;
        }
      else if( type == 3)
        {
        // Inside:
        refCount = 1;
        }

      // Now, separate case were the edge is split or not:

      if( this->ErrorMetric->EvaluateEdge(left, right))
        {
        this->EdgeTable->InsertEdge(leftId, rightId, cellId, refCount, ptId);
        assert("check: id exists" && ptId != -1 );

        // And also the value we'll have to put to avoid recomputing them later:
        //First find the point local coordinates:
        local[0] = (tri.GetVertex(l)[0] + tri.GetVertex(r)[0])/2.;
        local[1] = (tri.GetVertex(l)[1] + tri.GetVertex(r)[1])/2.;
        local[2] = (tri.GetVertex(l)[2] + tri.GetVertex(r)[2])/2.;

        //Save mid point:
        tri.SetVertex(j+3, local);
        tri.SetPointId(j+3, ptId);

        // Then it's real space coordinate:
        this->GenericCell->EvaluateLocation(0,local, global);

        // Then scalar value associated with point:  
        //this->GenericCell->EvaluateShapeFunction(local, scalar);
        //this->AttributeCollection->EvaluateTuple(this->GenericCell, local, scalar);
        this->GenericCell->InterpolateTuple(this->AttributeCollection, local, scalar);
        //Put everything in ths point hash table
        this->EdgeTable->InsertPointAndScalar(ptId, global, scalar);
        }
      else
        {
        // The edge does not need to be split simply insert it
        this->EdgeTable->InsertEdge(leftId, rightId, cellId, refCount);
        }
      }
    else
      {
      // else the edge is in the table we need to increment its ref count.
      // This becomes tricky when we are incrementing an edge shared across
      // cell, we should not increment edge ref count when first time in a cell
      // Precondition third package have unique cellId.
      this->EdgeTable->IncrementEdgeReferenceCount(leftId, rightId, cellId);

      //vtkDebugMacro( << "IncrementEdgeReferenceCount:" << ptId );
      
      if(ptId != -1)
        {
        tri.SetPointId(j+3, ptId);
        
        local[0] = (tri.GetVertex(l)[0] + tri.GetVertex(r)[0])/2.;
        local[1] = (tri.GetVertex(l)[1] + tri.GetVertex(r)[1])/2.;
        local[2] = (tri.GetVertex(l)[2] + tri.GetVertex(r)[2])/2.;

        tri.SetVertex(j+3, local);
        }
      }
    }
}

//-----------------------------------------------------------------------------
// 
void vtkGenericCellTessellator::InsertEdgesIntoEdgeTable( vtkTetraTile &tetra )
{
  double local[3];
  double global[3];
  double scalar[10]; //FIXME: magic number
  // FB: see remark in  InsertPointsIntoEdgeTable(vtkTetraTile &)
  vtkIdType l, r;
  const vtkIdType cellId = this->GenericCell->GetId();

  //First setup the point reference count:
  for(int i=0; i<4; i++)
    {
    this->EdgeTable->IncrementPointReferenceCount(tetra.GetPointId(i));
    }

  // Loop over all edges:
  // For each edge:
  //    if in hash table: incr ref 
  //    else:             evaluate & put in table ref = 1
  for(int j=0;j<6;j++)
    {
    l = TETRA_EDGES_TABLE[j][0];
    r = TETRA_EDGES_TABLE[j][1];
        
    double *left = tetra.GetVertex(l);
    double *right = tetra.GetVertex(r);

    vtkIdType leftId = tetra.GetPointId(l);
    vtkIdType rightId = tetra.GetPointId(r);

    //Check first in the hash table
    vtkIdType ptId = -1;
    int refCount = 1;

    //vtkDebugMacro( << "InsertEdgesIntoEdgeTable:" << leftId << "," << rightId );

    // To calculate the edge ref count, we either:
    // - find it in the hash table
    // - calculate from higher order cell:

    if( this->EdgeTable->CheckEdge(leftId, rightId, ptId) == -1)
      {
      // The edge was not found in the hash table, that mean we have to 
      // determine it's reference counting from the higher order cell:
      int localId;
      int type = FindEdgeParent(left, right, localId);
      
      if(type == 1)
        {
        // On edge:
        refCount = this->GetNumberOfCellsUsingEdge( localId );
        }
      else if(type == 2)
        {
        //On face:
        refCount = this->GetNumberOfCellsUsingFace( localId );
        }
      else if(type == 3)
        {
        // Inside:
        refCount = 1;
        }

      // Now, separate case were the edge is split or not:
      if( this->ErrorMetric->EvaluateEdge(left, right) )
        {
        this->EdgeTable->InsertEdge(leftId, rightId, cellId, refCount, ptId);
        assert("check: id exists" && ptId != -1 );

        // And also the value we'll have to put to avoid recomputing them later:
        //First find the point local coordinates:
        local[0] = (tetra.GetVertex(l)[0] + tetra.GetVertex(r)[0])/2.;
        local[1] = (tetra.GetVertex(l)[1] + tetra.GetVertex(r)[1])/2.;
        local[2] = (tetra.GetVertex(l)[2] + tetra.GetVertex(r)[2])/2.;

        //Save mid point:
        tetra.SetVertex(j+4, local);
        tetra.SetPointId(j+4, ptId);

        // Then its real space coordinate:
        this->GenericCell->EvaluateLocation(0,local,global);

        // Then scalar value associated with point:  
        this->GenericCell->InterpolateTuple(this->AttributeCollection, local,
                                            scalar);
        //Put everything in the point hash table
        this->EdgeTable->InsertPointAndScalar(ptId, global, scalar);
        }
      else
        {
        // The edge does not need to be split simply insert it
        this->EdgeTable->InsertEdge(leftId, rightId, cellId, refCount);
        }
      }
    else
      {
      // else the edge is in the table we need to increment its ref count.
      // This becomes tricky when we are incrementing an edge shared across
      // cell, we should not increment edge ref count when first time in a cell
      // Precondition third package have unique cellId.
      this->EdgeTable->IncrementEdgeReferenceCount(leftId, rightId, cellId);

      //vtkDebugMacro( << "IncrementEdgeReferenceCount:" << ptId );
      
      if(ptId != -1)
        {
        tetra.SetPointId(j+4, ptId);
        
        local[0] = (tetra.GetVertex(l)[0] + tetra.GetVertex(r)[0])/2.;
        local[1] = (tetra.GetVertex(l)[1] + tetra.GetVertex(r)[1])/2.;
        local[2] = (tetra.GetVertex(l)[2] + tetra.GetVertex(r)[2])/2.;

        tetra.SetVertex(j+4, local);
        }
      }
    }
}

//-----------------------------------------------------------------------------
void vtkGenericCellTessellator::RemoveEdgesFromEdgeTable( vtkTriangleTile &tri )
{
  vtkIdType l,r;
  int i;
  
  // First setup the point reference count:
  for(i = 0; i<3; i++)
    {
    this->EdgeTable->RemovePoint( tri.GetPointId(i));
    }

  // Clean the hash table by removing all edges from this tet, loop over edges:
  for(i=0;i<3;i++)
    {
    l = TRIANGLE_EDGES_TABLE[i][0];
    r = TRIANGLE_EDGES_TABLE[i][1];

    this->EdgeTable->RemoveEdge(tri.GetPointId(l), tri.GetPointId(r));
    }
}
//-----------------------------------------------------------------------------
void vtkGenericCellTessellator::RemoveEdgesFromEdgeTable( vtkTetraTile &tetra )
{
  vtkIdType l,r;
  int i;
  
  // First setup the point reference count:
  for(i = 0; i<4; i++)
    {
    this->EdgeTable->RemovePoint( tetra.GetPointId(i));
    }

  // Clean the hash table by removing all edges from this tet, loop over edges:
  for(i=0;i<6;i++)
    {
    l = TETRA_EDGES_TABLE[i][0];
    r = TETRA_EDGES_TABLE[i][1];

    this->EdgeTable->RemoveEdge(tetra.GetPointId(l), tetra.GetPointId(r));
    }
}
//-----------------------------------------------------------------------------
void vtkGenericCellTessellator::InternalTessellateTriangle(vtkTriangleTile& root )
{
  // use a queue instead of a list to speed things up 
  vtkstd::queue< vtkTriangleTile > work;
  vtkTriangleTile begin = vtkTriangleTile(root);
  work.push( begin );

  while( !work.empty() ) 
    {
    vtkTriangleTile piece[4];
    vtkTriangleTile curr = work.front();
    work.pop();

    int n = curr.Refine( this, piece );

    for( int i = 0; i<n; i++) 
      {
      work.push( piece[i] );
      }
    // We are done we should clean ourself from the hash table:
    this->RemoveEdgesFromEdgeTable( curr );
    }

  // remove top level points
  for(int i = 0; i<3; i++)
    {
    this->EdgeTable->RemovePoint( root.GetPointId(i) );
    }

  //this->EdgeTable->LoadFactor();
  //this->EdgeTable->DumpTable();

}

//-----------------------------------------------------------------------------
void vtkGenericCellTessellator::Reset()
{
  // No memory deletion should happen here, as one cell to another there
  // should be the same amount of points to tessellate
  this->TessellatePoints->Reset();
  this->TessellateCellArray->Reset();
  this->TessellateScalars->Reset();
}

//-----------------------------------------------------------------------------
void vtkGenericCellTessellator::Initialize(vtkIdType numPts)
{
  this->EdgeTable->Initialize( numPts );
}

//-----------------------------------------------------------------------------
void vtkGenericCellTessellator::Tessellate(vtkGenericAdaptorCell *cell,
                                           vtkGenericAttributeCollection *att,
                                           vtkDoubleArray *points, 
                                           vtkCellArray *cellArray,
                                           vtkDoubleArray *scalars  )
{
  int i;
  
  // Save parameter for later use
  this->GenericCell = cell;
  this->TessellatePoints = points;
  this->TessellateCellArray = cellArray;
  this->TessellateScalars = scalars;
  this->AttributeCollection = att;
  if(this->CellIterator==0)
    {
    this->CellIterator = cell->NewCellIterator();
    }
  
  // FIXME
  this->ErrorMetric->SetGenericCell( cell );
  this->ErrorMetric->SetAttributeCollection( att );

  // Here we need to be very cautious to create the first level of tetra
  // We need to pre-order the tetra, meaning classify it either as a right hand
  // or left hand order
  // At the same time we evaluate the interpolation function to find the scalar
  // value associated with the points
  vtkTetraTile root;
  double *point;
  vtkIdType tetra[4], order[4] = {-1,-1,-1,-1};

  assert("check: is a tetra" && 
         this->GenericCell->GetNumberOfBoundaries(0) == 4);

  // Using third package ptId: It has to be consitent (assuming)
  // Therefore if no tessellation occurs the pointId should match
  this->GenericCell->GetPointIds(tetra);
  Reorder(tetra, order);

  for(i=0; i<4; i++)
    {
    //this->GenericCell->GetParametricCoords(order[i], point);
    point = this->GenericCell->GetParametricCoords() + 3*order[i];
    root.SetVertex(i, point);
    root.SetPointId(i, tetra[order[i]]);
    //vtkDebugMacro( << "tetra[order[i]]:" << tetra[order[i]] );
    }

  // Pass data to hash table:
  // FIXME some point are already in the hash table we shouldn't try
  // to calculate their associate point/scalar value then.
  this->InsertPointsIntoEdgeTable( root );

  //Prepare the hash table with the top-level edges:
  this->InsertEdgesIntoEdgeTable( root );

  //Start of the algorithm use a queue for now:
  vtkstd::queue<vtkTetraTile> work;
  work.push( root );

  //vtkDebugMacro( << "New tet being tessellated" );

  while( !work.empty() ) 
    {
    vtkTetraTile piece[8];
    vtkTetraTile curr = work.front();
    work.pop();

    int n = curr.Refine( this, piece );
    
    for(i = 0; i<n; i++) 
      {
      work.push( piece[i] );
      }

    // We are done we should clean ourself from the hash table:
    this->RemoveEdgesFromEdgeTable( curr );
    }

  // remove top level points
  for(i = 0; i<4; i++)
    {
    this->EdgeTable->RemovePoint( root.GetPointId(i) );
    }
  
  //this->EdgeTable->LoadFactor();
  //this->EdgeTable->DumpTable();

  // Okay we are done with this cell, dump the hash-table to check it is clean:
  // by clean I mean it only contains points on face/edge:
}

//-----------------------------------------------------------------------------
void vtkGenericCellTessellator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "GenericCell: " << this->GenericCell << endl;
  os << indent << "TessellateScalars: " 
     << this->TessellateScalars << endl;
  os << indent << "TessellateCellArray: " 
     << this->TessellateCellArray << endl;
  os << indent << "TessellatePoints: " 
     << this->TessellatePoints << endl;

  os << indent << "ErrorMetric: " 
     << this->ErrorMetric << endl;
}

//-----------------------------------------------------------------------------
void vtkGenericCellTessellator
::TessellateTriangleFace(vtkGenericAdaptorCell *cell, 
                         vtkGenericAttributeCollection *att,
                         vtkIdType index, vtkDoubleArray *points,
                         vtkCellArray *cellArray, vtkDoubleArray *scalars)
{
  int i=0;
  // Save parameter for later use
  this->TessellateCellArray = cellArray;
  this->TessellatePoints = points;
  this->TessellateScalars = scalars;
  this->GenericCell = cell;
  this->AttributeCollection = att;
  if(this->CellIterator==0)
    {
    this->CellIterator = cell->NewCellIterator();
    }
  
  // FIXME
  this->ErrorMetric->SetGenericCell( cell );
  this->ErrorMetric->SetAttributeCollection( att );

  // Based on the id of the face we can find the points of the triangle:
  // When triangle found just tessellate it.
  vtkTriangleTile root;
  double *point;

  vtkIdType tetra[4];
  this->GenericCell->GetPointIds(tetra);

  assert("check: is a tetra" &&
         this->GenericCell->GetNumberOfBoundaries(0) == 4);

  int indexTab[3];
  switch(index)
    {
    case 0:
      indexTab[0] = 0;
      indexTab[1] = 2;
      indexTab[2] = 1;
      for(i=0; i<3; i++)
        {
        point = this->GenericCell->GetParametricCoords() + 3*indexTab[i];
        root.SetVertex(i, point);
        root.SetPointId(i, tetra[indexTab[i]]);
        }
      this->InsertPointsIntoEdgeTable( root );
      this->InsertEdgesIntoEdgeTable( root );
      this->InternalTessellateTriangle( root );
      break;
    case 1:
      indexTab[0] = 0;
      indexTab[1] = 1;
      indexTab[2] = 3;
      for(i=0; i<3; i++)
        {
        point = this->GenericCell->GetParametricCoords() + 3*indexTab[i];
        root.SetVertex(i, point);
        root.SetPointId(i, tetra[indexTab[i]]);
        }
      this->InsertPointsIntoEdgeTable( root );
      this->InsertEdgesIntoEdgeTable( root );
      this->InternalTessellateTriangle( root );
      break;
    case 2:
      indexTab[0] = 1;
      indexTab[1] = 2;
      indexTab[2] = 3;
      for(i=0; i<3; i++)
        {
        point = this->GenericCell->GetParametricCoords() + 3*indexTab[i];
        root.SetVertex(i, point);
        root.SetPointId(i, tetra[indexTab[i]]);
        }
      this->InsertPointsIntoEdgeTable( root );
      this->InsertEdgesIntoEdgeTable( root );
      this->InternalTessellateTriangle( root );
      break;
    case 3:
      indexTab[0] = 2;
      indexTab[1] = 0;
      indexTab[2] = 3;
      for(i=0; i<3; i++)
        {
        point = this->GenericCell->GetParametricCoords() + 3*indexTab[i];
        root.SetVertex(i, point);
        root.SetPointId(i, tetra[indexTab[i]]);
        }
      this->InsertPointsIntoEdgeTable( root );
      this->InsertEdgesIntoEdgeTable( root );
      this->InternalTessellateTriangle( root );
      break;
    }
}

//-----------------------------------------------------------------------------
void vtkGenericCellTessellator::Triangulate(vtkGenericAdaptorCell *cell, 
                                            vtkGenericAttributeCollection *att,
                                            vtkDoubleArray *points,
                                            vtkCellArray *cellArray, 
                                            vtkDoubleArray *scalars)
{
  // Save parameter for later use
  this->GenericCell = cell;
  this->TessellatePoints = points;
  this->TessellateCellArray = cellArray;
  this->TessellateScalars = scalars;
  this->AttributeCollection = att;
  if(this->CellIterator==0)
    {
    this->CellIterator = cell->NewCellIterator();
    }
  
  // FIXME
  this->ErrorMetric->SetGenericCell( cell );
  this->ErrorMetric->SetAttributeCollection( att );
  vtkTriangleTile root;
  double *point;
  vtkIdType tri[3];
  this->GenericCell->GetPointIds(tri);

  for(int i=0; i<3; i++)
    {
    point = this->GenericCell->GetParametricCoords() + 3*i;
    root.SetVertex(i, point);
    root.SetPointId(i, tri[i]);
    }

  this->InsertPointsIntoEdgeTable( root );

  //Prepare the hash table with the top-level edges:
  this->InsertEdgesIntoEdgeTable( root );
  this->InternalTessellateTriangle( root );
}

//-----------------------------------------------------------------------------
// if yes return on which edge it is, else return -1
// \pre p1!=p2
// \pre p1 and p2 are in bounding box (0,0,0) (1,1,1)
int IsEdgeOnEdge(double p1[3], double p2[3])
{
  // We assume p1 and p2 are differents, and within the bounding box. In fact,
  // this test only check if the point are on an infinite line.

  //Test on edge 0:
  if((p1[1] == p2[1]) && (p2[1] == 0.0) && (p1[2] == p2[2]) && (p2[2] == 0.0))
    {
    return 0;
    }
  //Test on edge 1:
  else if( (p1[0] + p1[1] == 1.0) && (p2[0] + p2[1] == 1.0) && 
           (p1[2] == p2[2]) && (p2[2] == 0.0))
    {
    return 1;
    }
  //Test on edge 2:
  else if((p1[0] == p2[0]) && (p2[0] == 0.0) && (p1[2] == p2[2]) && 
          (p2[2] == 0.0))
    {
    return 2;
    }
  //Test on edge 3:
  else if((p1[0] == p2[0]) && (p2[0] == 0.0) && (p1[1] == p2[1]) && 
          (p2[1] == 0.0))
    {
    return 3;
    }
  //Test on edge 4:
  else if((p1[1] == p2[1]) && (p2[1] == 0.0) && (p1[0] + p1[2] == 1.0) && 
          (p2[0] + p2[2] == 1.0))
    {
    return 4;
    }
  //Test on edge 5:
  else if((p1[0] == p2[0]) && (p2[0] == 0.0) && (p1[1] + p1[2] == 1.0) && 
          (p2[1] + p2[2] == 1.0))
    {
    return 5;
    }
  return -1;
}

//-----------------------------------------------------------------------------
// if yes return on which face it is, else return -1
// \pre p1!=p2
// \pre p1 and p2 are in bounding box (0,0,0) (1,1,1)
int IsEdgeOnFace(double p1[3], double p2[3])
{
  //Test on face 0:  (012)
  if((p1[2] == p2[2]) && (p2[2] == 0.0))
    {
    return 0;
    }
  //Test on face 1: (013)
  if((p1[1] == p2[1]) && (p2[1] == 0.0))
    {
    return 1;
    }
  //Test on face 2: (123)
  if((p1[0] + p1[1] + p1[2] == 1.0) && (p2[0] + p2[1] + p2[2] == 1.0))
    {
    return 2;
    }
  //Test on face 3: (023)
  if((p1[0] == p2[0]) && (p2[0] == 0.0))
    {
    return 3;
    }

  return -1;
}
//-----------------------------------------------------------------------------
// Return 1 if the parent of edge (`p1',`p2'), is an edge; 2 if the parent is
// a face, 3 if there is no parent (the edge is inside).
// If the parent is an edge or a face, return its id in `localId'.
int FindEdgeParent(double p1[3], double p2[3], int &localId)
{

  if( ( localId = IsEdgeOnEdge(p1, p2) ) != -1)
    {
    // On Edge
    // edge (p1,p2) is on the parent edge #id
    return 1;
    }
  else if( ( localId = IsEdgeOnFace(p1, p2) ) != -1)
    {
    // On Face
    // edge (p1,p2) is on the parent face #id
    return 2;
    }
  else
    {
    // Inside
    return 3;
    }

}

#if 0
//-----------------------------------------------------------------------------
// There is a special case when the edge is either on the edge or the face of
// the parent cell (the third party cell) the ref count should take that into
// account
// Parameters are local coordinate
int vtkGenericCellTessellator::FindEdgeReferenceCount(double p1[3], double p2[3],
  vtkIdType &e1, vtkIdType &e2)
{
  //int count = 0;
  //How many times this edges is being shared across cells
  int face = -1;
  int edge = -1;

  vtkDebugMacro( << "FindEdgeReferenceCount:" << 
    p1[0] << "," << p1[1] << "," <<  p1[2] << "\n" << 
    p2[0] << "," << p2[1] << "," <<  p2[2]  );

  edge = IsEdgeOnEdge(p1, p2);
  // If edge is on edge we could also check it is on the right face as s sanity
  // check:
  // IsEdgeOnFace(p1, p2);

  if( edge == -1 )
    {
    vtkDebugMacro( << "FindEdgeReferenceCount: Edge is not on edge" );

    // this edge could still be on a face:
    face = IsEdgeOnFace(p1, p2);
    if(face == -1)
      {
      vtkDebugMacro( << "FindEdgeReferenceCount: Edge is not on face" );
      // ok this edge is neither on an edge or a face. So its ref count is only 1
      // But we return -1 so that we know the edge is not special
      // We could return 0 but this is dangerous, for example :
      // an edge on the limit of a plane would also have a reference of 1 face
      // so result would be 1 - 1 = 0
      return -1;
      }
    //Test if cell is on boundary
    if( this->GenericCell->IsFaceOnBoundary( face ) )
      {
      // So no other cell is using it:
      return 1;
      }
    //else this face is used by another cell
    return 2;
    }

  //else this edge is on a bigger edge:
  int edgeSharing[6];
  this->GenericCell->CountEdgeNeighbors(edgeSharing);

  vtkDebugMacro( << "GetEdgesSharing:" << edge <<  " \n" << 
    edgeSharing[0] << "," << edgeSharing[1] << "," <<  edgeSharing[2] << "\n" << 
    edgeSharing[3] << "," << edgeSharing[4] << "," <<  edgeSharing[5]  );

  for(int i=0; i<6;i++)
    {
    assert("check: edge is shared" && edgeSharing[i] != 0 );
    }
  
  int edge_table[6][2] = {{0, 1}, {1, 2}, {0, 2}, {0, 3}, {1, 3}, {2, 3}};

  vtkIdType ids[4];
  this->GenericCell->GetPointIds( ids );

  e1 = ids[edge_table[edge][0]];
  e2 = ids[edge_table[edge][1]];

  return edgeSharing[edge];
}
#endif


#define SLOW_API 1
//-----------------------------------------------------------------------------
// Return number of cells using edge #edgeId
int vtkGenericCellTessellator::GetNumberOfCellsUsingEdge( int edgeId )
{
#if SLOW_API
  int result = 0;
  this->GenericCell->GetBoundaryIterator(this->CellIterator, 1);
  this->CellIterator->Begin();

  int i=0;
  while(!this->CellIterator->IsAtEnd() && (i < edgeId) )
    {
    this->CellIterator->Next();
    ++i;
    }

  assert("check: cell_found" && i==edgeId);
  // +1 because CountNeighbors does not include the cell itself.
  result = this->GenericCell->CountNeighbors(this->CellIterator->GetCell())+1;
  return result;
#else
  int edgeSharing[6];
  this->GenericCell->CountEdgeNeighbors( edgeSharing );
  
  return edgeSharing[edgeId];
#endif
}

//-----------------------------------------------------------------------------
// Return number of cells using face #faceId
int vtkGenericCellTessellator::GetNumberOfCellsUsingFace( int faceId )
{
#if SLOW_API
  int result=0;
  this->GenericCell->GetBoundaryIterator(this->CellIterator, 2);
  this->CellIterator->Begin();

  int i=0;
  while(!this->CellIterator->IsAtEnd() && ( i < faceId) )
    {
    this->CellIterator->Next();
    ++i;
    }

  assert("check: cell_found" && i==faceId);
  // +1 because CountNeighbors does not include the cell itself.
  result = this->GenericCell->CountNeighbors(this->CellIterator->GetCell())+1;

  return result;
#else
  // basically 1 or 2:
  //Test if cell is on boundary
  if( this->GenericCell->IsFaceOnBoundary( faceId ) )
    {
    // So no other cell is using it:
    return 1;
    }

  //else this face is used by another cell
  return 2;
#endif
}
