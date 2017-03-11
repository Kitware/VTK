/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSimpleCellTessellator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSimpleCellTessellator.h"
#include "vtkObjectFactory.h"

#include "vtkCellArray.h"
#include "vtkDoubleArray.h"
#include "vtkGenericAdaptorCell.h"
#include "vtkGenericAttributeCollection.h"
#include "vtkGenericAttribute.h"
#include "vtkGenericCellIterator.h"
#include "vtkGenericDataSet.h"
#include "vtkGenericEdgeTable.h"
#include "vtkGenericSubdivisionErrorMetric.h"
#include "vtkPointData.h"
#include "vtkPoints.h"

#include "vtkOrderedTriangulator.h"
#include "vtkPolygon.h"
#include "vtkTetra.h"

#include <queue>
#include <stack>
#include <cassert>

// format of the arrays LeftPoint, MidPoint, RightPoint is global, parametric,
// attributes: xyz rst [abc de...]
const int PARAMETRIC_OFFSET = 3;
const int ATTRIBUTES_OFFSET = 6;

// Pre computed table for the point to edge equivalence:
// [edge][point]
static int TRIANGLE_EDGES_TABLE[3][2] = {{0, 1}, {1, 2}, {2, 0}};


// Pre computed table for the tessellation of triangles
#define NO_TRIAN {-1,-1,-1}

// Each edge can either be split or not therefore there is
// 2^3 = 8 differents cases of tessellation
// The last case is only a sentinel to avoid stepping out of table
// If we consider edge 3 the first edge, 4 the second and 5 the last one
// 'Index' can be computed by the decimal evaluation of the binary representing
// which is is split ex: 3 and 5 are split is noted:
// {1, 0, 1} = 1*2^0 + 0*2^1 + 1*2^2 = 5
// [case][triangle][vertex]
static signed char vtkTessellatorTriangleCases[9][4][3] = {
// Index = 0, Case where no edges are split
{ NO_TRIAN, NO_TRIAN, NO_TRIAN, NO_TRIAN},
// Index = 1, Case where edges 3 are split
{{0, 3, 2},{1, 2, 3}, NO_TRIAN, NO_TRIAN},
// Index = 2, Case where edges 4 are split
{{0, 1, 4},{0, 4, 2}, NO_TRIAN, NO_TRIAN},
// Index = 3, Case where edges 3,4 are split
{{0, 3, 2},{1, 4, 3},{3, 4, 2}, NO_TRIAN},
// Index = 4, Case where edges 5 are split
{{0, 1, 5},{1, 2, 5}, NO_TRIAN, NO_TRIAN},
// Index = 5, Case where edges 3,5 are split
{{0, 3, 5},{1, 5, 3},{1, 2, 5}, NO_TRIAN},
// Index = 6, Case where edges 4,5 are split
{{0, 4, 5},{0, 1, 4},{2, 5, 4}, NO_TRIAN},
// Index = 7, Case where edges 4,5,6 are split
{{0, 3, 5},{3, 4, 5},{1, 4, 3},{2, 5, 4}},
// In case we reach outside the table
{ NO_TRIAN, NO_TRIAN, NO_TRIAN, NO_TRIAN},
};

// Pre computed table for the point to edge equivalence:
// [edge][point]
static int TETRA_EDGES_TABLE[6][2] = {
{0, 1}, {1, 2}, {2, 0}, {0, 3}, {1, 3}, {2, 3}
};

// Pre computed table for the tessellation of tetras
// There is two cases for the tessellation of a tetra, it is either oriented
// with the right hand rule or with the left hand rule
#define NO_TETRA {-1,-1,-1,-1}


// Each edge can either be split or not therefore there is
// 2^6 = 64 differents cases of tessellation
// The last case is only a sentinel to avoid stepping out of table
// [case][tetra][vertex]
static signed char vtkTessellatorTetraCasesRight[65][8][4] = {
// Index = 0, Case where no edges are split
{{0,1,2,3}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 1, Case where edges: 4 are split
{{0,2,3,4},{1,2,4,3}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 2, Case where edges: 5 are split
{{0,1,5,3},{0,2,3,5}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 3, Case where edges: 4,5 are split
{{0,2,3,5},{0,3,4,5},{1,3,5,4}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 4, Case where edges: 6 are split
{{0,1,6,3},{1,2,6,3}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 5, Case where edges: 4,6 are split
{{0,3,4,6},{1,2,6,3},{1,3,6,4}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 6, Case where edges: 5,6 are split
{{0,1,5,3},{0,3,5,6},{2,3,6,5}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 7, Case where edges: 4,5,6 are split
{{0,3,4,6},{1,3,5,4},{2,3,6,5},{3,4,6,5}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 8, Case where edges: 7 are split
{{0,1,2,7},{1,2,7,3}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 9, Case where edges: 4,7 are split
{{0,2,7,4},{1,2,4,7},{1,2,7,3}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 10, Case where edges: 5,7 are split
{{0,1,5,7},{0,2,7,5},{1,3,5,7},{2,3,7,5}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 11, Case where edges: 4,5,7 are split
{{0,2,7,5},{0,4,5,7},{1,3,5,7},{1,4,7,5},{2,3,7,5}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 12, Case where edges: 6,7 are split
{{0,1,6,7},{1,2,6,7},{1,2,7,3}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 13, Case where edges: 4,6,7 are split
{{0,4,6,7},{1,2,6,7},{1,2,7,3},{1,4,7,6}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 14, Case where edges: 5,6,7 are split
{{0,1,5,7},{0,5,6,7},{1,3,5,7},{2,3,7,5},{2,5,7,6}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 15, Case where edges: 4,5,6,7 are split
{{0,4,6,7},{1,3,5,7},{1,4,7,5},{2,3,7,5},{2,5,7,6},{4,5,6,7}, NO_TETRA, NO_TETRA},
// Index = 16, Case where edges: 8 are split
{{0,1,2,8},{0,2,3,8}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 17, Case where edges: 4,8 are split
{{0,2,3,8},{0,2,8,4},{1,2,4,8}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 18, Case where edges: 5,8 are split
{{0,1,5,8},{0,2,3,8},{0,2,8,5}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 19, Case where edges: 4,5,8 are split
{{0,2,3,8},{0,2,8,5},{0,4,5,8},{1,4,8,5}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 20, Case where edges: 6,8 are split
{{0,1,6,8},{0,3,8,6},{1,2,6,8},{2,3,6,8}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 21, Case where edges: 4,6,8 are split
{{0,3,8,6},{0,4,6,8},{1,2,6,8},{1,4,8,6},{2,3,6,8}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 22, Case where edges: 5,6,8 are split
{{0,1,5,8},{0,3,8,6},{0,5,6,8},{2,3,6,8},{2,5,8,6}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 23, Case where edges: 4,5,6,8 are split
{{0,3,8,6},{0,4,6,8},{1,4,8,5},{2,3,6,8},{2,5,8,6},{4,5,6,8}, NO_TETRA, NO_TETRA},
// Index = 24, Case where edges: 7,8 are split
{{0,1,2,8},{0,2,7,8},{2,3,7,8}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 25, Case where edges: 4,7,8 are split
{{0,2,7,4},{1,2,4,8},{2,3,7,8},{2,4,8,7}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 26, Case where edges: 5,7,8 are split
{{0,1,5,8},{0,2,7,5},{0,5,7,8},{2,3,7,8},{2,5,8,7}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 27, Case where edges: 4,5,7,8 are split
{{0,2,7,5},{0,4,5,7},{1,4,8,5},{2,3,7,8},{2,5,8,7},{4,5,7,8}, NO_TETRA, NO_TETRA},
// Index = 28, Case where edges: 6,7,8 are split
{{0,1,6,8},{0,6,7,8},{1,2,6,8},{2,3,7,8},{2,6,8,7}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 29, Case where edges: 4,6,7,8 are split
{{0,4,6,7},{1,2,6,8},{1,4,8,6},{2,3,7,8},{2,6,8,7},{4,6,7,8}, NO_TETRA, NO_TETRA},
// Index = 30, Case where edges: 5,6,7,8 are split
{{0,1,5,8},{0,5,6,7},{0,5,7,8},{2,3,7,8},{2,5,7,6},{2,5,8,7}, NO_TETRA, NO_TETRA},
// Index = 31, Case where edges: 4,5,6,7,8 are split
{{0,4,6,7},{1,4,8,5},{2,3,7,8},{2,5,7,6},{2,5,8,7},{4,5,6,7},{4,5,7,8}, NO_TETRA},
// Index = 32, Case where edges: are split
{{0,1,2,9},{0,1,9,3}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 33, Case where edges: 4 are split
{{0,2,9,4},{0,3,4,9},{1,2,4,9},{1,3,9,4}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 34, Case where edges: 5 are split
{{0,1,5,9},{0,1,9,3},{0,2,9,5}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 35, Case where edges: 4,5 are split
{{0,2,9,5},{0,3,4,9},{0,4,5,9},{1,3,9,4},{1,4,9,5}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 36, Case where edges: 6 are split
{{0,1,6,9},{0,1,9,3},{1,2,6,9}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 37, Case where edges: 4,6 are split
{{0,3,4,9},{0,4,6,9},{1,2,6,9},{1,3,9,4},{1,4,9,6}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 38, Case where edges: 5,6 are split
{{0,1,5,9},{0,1,9,3},{0,5,6,9},{2,5,9,6}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 39, Case where edges: 4,5,6 are split
{{0,3,4,9},{0,4,6,9},{1,3,9,4},{1,4,9,5},{2,5,9,6},{4,5,6,9}, NO_TETRA, NO_TETRA},
// Index = 40, Case where edges: 7 are split
{{0,1,2,9},{0,1,9,7},{1,3,9,7}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 41, Case where edges: 4,7 are split
{{0,2,9,4},{0,4,9,7},{1,2,4,9},{1,3,9,7},{1,4,7,9}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 42, Case where edges: 5,7 are split
{{0,1,5,7},{0,2,9,5},{0,5,9,7},{1,3,9,7},{1,5,7,9}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 43, Case where edges: 4,5,7 are split
{{0,2,9,5},{0,4,5,7},{0,5,9,7},{1,3,9,7},{1,4,7,5},{1,5,7,9}, NO_TETRA, NO_TETRA},
// Index = 44, Case where edges: 6,7 are split
{{0,1,6,7},{1,2,6,9},{1,3,9,7},{1,6,7,9}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 45, Case where edges: 4,6,7 are split
{{0,4,6,7},{1,2,6,9},{1,3,9,7},{1,4,7,9},{1,4,9,6},{4,6,7,9}, NO_TETRA, NO_TETRA},
// Index = 46, Case where edges: 5,6,7 are split
{{0,1,5,7},{0,5,6,7},{1,3,9,7},{1,5,7,9},{2,5,9,6},{5,6,7,9}, NO_TETRA, NO_TETRA},
// Index = 47, Case where edges: 4,5,6,7 are split
{{0,4,6,7},{1,3,9,7},{1,4,7,5},{1,5,7,9},{2,5,9,6},{4,5,6,7},{5,6,7,9}, NO_TETRA},
// Index = 48, Case where edges: 8 are split
{{0,1,2,9},{0,1,9,8},{0,3,8,9}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 49, Case where edges: 4,8 are split
{{0,2,9,4},{0,3,8,9},{0,4,9,8},{1,2,4,9},{1,4,8,9}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 50, Case where edges: 5,8 are split
{{0,1,5,8},{0,2,9,5},{0,3,8,9},{0,5,9,8}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 51, Case where edges: 4,5,8 are split
{{0,2,9,5},{0,3,8,9},{0,4,5,9},{0,4,9,8},{1,4,8,5},{4,5,9,8}, NO_TETRA, NO_TETRA},
// Index = 52, Case where edges: 6,8 are split
{{0,1,6,8},{0,3,8,9},{0,6,9,8},{1,2,6,9},{1,6,8,9}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 53, Case where edges: 4,6,8 are split
{{0,3,8,9},{0,4,6,8},{0,6,9,8},{1,2,6,9},{1,4,8,6},{1,6,8,9}, NO_TETRA, NO_TETRA},
// Index = 54, Case where edges: 5,6,8 are split
{{0,1,5,8},{0,3,8,9},{0,5,6,8},{0,6,9,8},{2,5,9,6},{5,6,8,9}, NO_TETRA, NO_TETRA},
// Index = 55, Case where edges: 4,5,6,8 are split
{{0,3,8,9},{0,4,6,8},{0,6,9,8},{1,4,8,5},{2,5,9,6},{4,5,6,8},{5,6,8,9}, NO_TETRA},
// Index = 56, Case where edges: 7,8 are split
{{0,1,2,9},{0,1,9,8},{0,7,8,9},{3,7,9,8}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 57, Case where edges: 4,7,8 are split
{{0,2,9,4},{0,4,9,7},{1,2,4,9},{1,4,8,9},{3,7,9,8},{4,7,8,9}, NO_TETRA, NO_TETRA},
// Index = 58, Case where edges: 5,7,8 are split
{{0,1,5,8},{0,2,9,5},{0,5,7,8},{0,5,9,7},{3,7,9,8},{5,7,8,9}, NO_TETRA, NO_TETRA},
// Index = 59, Case where edges: 4,5,7,8 are split
{{0,2,9,5},{0,4,5,7},{0,5,9,7},{1,4,8,5},{3,7,9,8},{4,5,7,8},{5,7,8,9}, NO_TETRA},
// Index = 60, Case where edges: 6,7,8 are split
{{0,1,6,8},{0,6,7,8},{1,2,6,9},{1,6,8,9},{3,7,9,8},{6,7,8,9}, NO_TETRA, NO_TETRA},
// Index = 61, Case where edges: 4,6,7,8 are split
{{0,4,6,7},{1,2,6,9},{1,4,8,6},{1,6,8,9},{3,7,9,8},{4,6,7,8},{6,7,8,9}, NO_TETRA},
// Index = 62, Case where edges: 5,6,7,8 are split
{{0,1,5,8},{0,5,6,7},{0,5,7,8},{2,5,9,6},{3,7,9,8},{5,6,7,9},{5,7,8,9}, NO_TETRA},
// Index = 63, Case where edges: 4,5,6,7,8 are split
{{0,4,6,7},{1,4,8,5},{2,5,9,6},{3,7,9,8},{4,5,6,7},{4,5,7,8},{5,6,7,9},{5,7,8,9}},
// In case we reach outside the table
{ NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA}
};

//-----------------------------------------------------------------------------
//
// This table is for the case where the 'last edge' of the tetra could not be order
// properly, then we need a different case table
//
static signed char vtkTessellatorTetraCasesLeft[65][8][4] = {
// Index = 0, Case where no edges are split
{{0,1,2,3}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 1, Case where edges: 4 are split
{{0,2,3,4},{1,2,4,3}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 2, Case where edges: 5 are split
{{0,1,5,3},{0,2,3,5}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 3, Case where edges: 4,5 are split
{{0,2,3,5},{0,3,4,5},{1,3,5,4}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 4, Case where edges: 6 are split
{{0,1,6,3},{1,2,6,3}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 5, Case where edges: 4,6 are split
{{0,3,4,6},{1,2,6,3},{1,3,6,4}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 6, Case where edges: 5,6 are split
{{0,1,5,3},{0,3,5,6},{2,3,6,5}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 7, Case where edges: 4,5,6 are split
{{0,3,4,6},{1,3,5,4},{2,3,6,5},{3,4,6,5}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 8, Case where edges: 7 are split
{{0,1,2,7},{1,2,7,3}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 9, Case where edges: 4,7 are split
{{0,2,7,4},{1,2,4,7},{1,2,7,3}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 10, Case where edges: 5,7 are split
{{0,1,5,7},{0,2,7,5},{1,3,5,7},{2,3,7,5}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 11, Case where edges: 4,5,7 are split
{{0,2,7,5},{0,4,5,7},{1,3,5,7},{1,4,7,5},{2,3,7,5}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 12, Case where edges: 6,7 are split
{{0,1,6,7},{1,2,6,3},{1,3,6,7}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 13, Case where edges: 4,6,7 are split
{{0,4,6,7},{1,2,6,3},{1,3,6,7},{1,4,7,6}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 14, Case where edges: 5,6,7 are split
{{0,1,5,7},{0,5,6,7},{1,3,5,7},{2,3,6,5},{3,5,7,6}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 15, Case where edges: 4,5,6,7 are split
{{0,4,6,7},{1,3,5,7},{1,4,7,5},{2,3,6,5},{3,5,7,6},{4,5,6,7}, NO_TETRA, NO_TETRA},
// Index = 16, Case where edges: 8 are split
{{0,1,2,8},{0,2,3,8}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 17, Case where edges: 4,8 are split
{{0,2,3,8},{0,2,8,4},{1,2,4,8}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 18, Case where edges: 5,8 are split
{{0,1,5,8},{0,2,3,5},{0,3,8,5}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 19, Case where edges: 4,5,8 are split
{{0,2,3,5},{0,3,8,5},{0,4,5,8},{1,4,8,5}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 20, Case where edges: 6,8 are split
{{0,1,6,8},{0,3,8,6},{1,2,6,8},{2,3,6,8}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 21, Case where edges: 4,6,8 are split
{{0,3,8,6},{0,4,6,8},{1,2,6,8},{1,4,8,6},{2,3,6,8}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 22, Case where edges: 5,6,8 are split
{{0,1,5,8},{0,3,8,6},{0,5,6,8},{2,3,6,5},{3,5,8,6}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 23, Case where edges: 4,5,6,8 are split
{{0,3,8,6},{0,4,6,8},{1,4,8,5},{2,3,6,5},{3,5,8,6},{4,5,6,8}, NO_TETRA, NO_TETRA},
// Index = 24, Case where edges: 7,8 are split
{{0,1,2,8},{0,2,7,8},{2,3,7,8}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 25, Case where edges: 4,7,8 are split
{{0,2,7,4},{1,2,4,8},{2,3,7,8},{2,4,8,7}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 26, Case where edges: 5,7,8 are split
{{0,1,5,8},{0,2,7,5},{0,5,7,8},{2,3,7,5},{3,5,8,7}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 27, Case where edges: 4,5,7,8 are split
{{0,2,7,5},{0,4,5,7},{1,4,8,5},{2,3,7,5},{3,5,8,7},{4,5,7,8}, NO_TETRA, NO_TETRA},
// Index = 28, Case where edges: 6,7,8 are split
{{0,1,6,8},{0,6,7,8},{1,2,6,8},{2,3,6,8},{3,6,8,7}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 29, Case where edges: 4,6,7,8 are split
{{0,4,6,7},{1,2,6,8},{1,4,8,6},{2,3,6,8},{3,6,8,7},{4,6,7,8}, NO_TETRA, NO_TETRA},
// Index = 30, Case where edges: 5,6,7,8 are split
{{0,1,5,8},{0,5,6,7},{0,5,7,8},{2,3,6,5},{3,5,7,6},{3,5,8,7}, NO_TETRA, NO_TETRA},
// Index = 31, Case where edges: 4,5,6,7,8 are split
{{0,4,6,7},{1,4,8,5},{2,3,6,5},{3,5,7,6},{3,5,8,7},{4,5,6,7},{4,5,7,8}, NO_TETRA},
// Index = 32, Case where edges: are split
{{0,1,2,9},{0,1,9,3}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 33, Case where edges: 4 are split
{{0,2,9,4},{0,3,4,9},{1,2,4,9},{1,3,9,4}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 34, Case where edges: 5 are split
{{0,1,5,9},{0,1,9,3},{0,2,9,5}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 35, Case where edges: 4,5 are split
{{0,2,9,5},{0,3,4,9},{0,4,5,9},{1,3,9,4},{1,4,9,5}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 36, Case where edges: 6 are split
{{0,1,6,9},{0,1,9,3},{1,2,6,9}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 37, Case where edges: 4,6 are split
{{0,3,4,9},{0,4,6,9},{1,2,6,9},{1,3,9,4},{1,4,9,6}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 38, Case where edges: 5,6 are split
{{0,1,5,9},{0,1,9,3},{0,5,6,9},{2,5,9,6}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 39, Case where edges: 4,5,6 are split
{{0,3,4,9},{0,4,6,9},{1,3,9,4},{1,4,9,5},{2,5,9,6},{4,5,6,9}, NO_TETRA, NO_TETRA},
// Index = 40, Case where edges: 7 are split
{{0,1,2,9},{0,1,9,7},{1,3,9,7}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 41, Case where edges: 4,7 are split
{{0,2,9,4},{0,4,9,7},{1,2,4,9},{1,3,9,7},{1,4,7,9}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 42, Case where edges: 5,7 are split
{{0,1,5,7},{0,2,9,5},{0,5,9,7},{1,3,9,7},{1,5,7,9}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 43, Case where edges: 4,5,7 are split
{{0,2,9,5},{0,4,5,7},{0,5,9,7},{1,3,9,7},{1,4,7,5},{1,5,7,9}, NO_TETRA, NO_TETRA},
// Index = 44, Case where edges: 6,7 are split
{{0,1,6,7},{1,2,6,9},{1,3,9,7},{1,6,7,9}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 45, Case where edges: 4,6,7 are split
{{0,4,6,7},{1,2,6,9},{1,3,9,7},{1,4,7,9},{1,4,9,6},{4,6,7,9}, NO_TETRA, NO_TETRA},
// Index = 46, Case where edges: 5,6,7 are split
{{0,1,5,7},{0,5,6,7},{1,3,9,7},{1,5,7,9},{2,5,9,6},{5,6,7,9}, NO_TETRA, NO_TETRA},
// Index = 47, Case where edges: 4,5,6,7 are split
{{0,4,6,7},{1,3,9,7},{1,4,7,5},{1,5,7,9},{2,5,9,6},{4,5,6,7},{5,6,7,9}, NO_TETRA},
// Index = 48, Case where edges: 8 are split
{{0,1,2,9},{0,1,9,8},{0,3,8,9}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 49, Case where edges: 4,8 are split
{{0,2,9,4},{0,3,8,9},{0,4,9,8},{1,2,4,9},{1,4,8,9}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 50, Case where edges: 5,8 are split
{{0,1,5,8},{0,2,9,5},{0,3,8,9},{0,5,9,8}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 51, Case where edges: 4,5,8 are split
{{0,2,9,5},{0,3,8,9},{0,4,5,9},{0,4,9,8},{1,4,8,5},{4,5,9,8}, NO_TETRA, NO_TETRA},
// Index = 52, Case where edges: 6,8 are split
{{0,1,6,8},{0,3,8,9},{0,6,9,8},{1,2,6,9},{1,6,8,9}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 53, Case where edges: 4,6,8 are split
{{0,3,8,9},{0,4,6,8},{0,6,9,8},{1,2,6,9},{1,4,8,6},{1,6,8,9}, NO_TETRA, NO_TETRA},
// Index = 54, Case where edges: 5,6,8 are split
{{0,1,5,8},{0,3,8,9},{0,5,6,8},{0,6,9,8},{2,5,9,6},{5,6,8,9}, NO_TETRA, NO_TETRA},
// Index = 55, Case where edges: 4,5,6,8 are split
{{0,3,8,9},{0,4,6,8},{0,6,9,8},{1,4,8,5},{2,5,9,6},{4,5,6,8},{5,6,8,9}, NO_TETRA},
// Index = 56, Case where edges: 7,8 are split
{{0,1,2,9},{0,1,9,8},{0,7,8,9},{3,7,9,8}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 57, Case where edges: 4,7,8 are split
{{0,2,9,4},{0,4,9,7},{1,2,4,9},{1,4,8,9},{3,7,9,8},{4,7,8,9}, NO_TETRA, NO_TETRA},
// Index = 58, Case where edges: 5,7,8 are split
{{0,1,5,8},{0,2,9,5},{0,5,7,8},{0,5,9,7},{3,7,9,8},{5,7,8,9}, NO_TETRA, NO_TETRA},
// Index = 59, Case where edges: 4,5,7,8 are split
{{0,2,9,5},{0,4,5,7},{0,5,9,7},{1,4,8,5},{3,7,9,8},{4,5,7,8},{5,7,8,9}, NO_TETRA},
// Index = 60, Case where edges: 6,7,8 are split
{{0,1,6,8},{0,6,7,8},{1,2,6,9},{1,6,8,9},{3,7,9,8},{6,7,8,9}, NO_TETRA, NO_TETRA},
// Index = 61, Case where edges: 4,6,7,8 are split
{{0,4,6,7},{1,2,6,9},{1,4,8,6},{1,6,8,9},{3,7,9,8},{4,6,7,8},{6,7,8,9}, NO_TETRA},
// Index = 62, Case where edges: 5,6,7,8 are split
{{0,1,5,8},{0,5,6,7},{0,5,7,8},{2,5,9,6},{3,7,9,8},{5,6,7,9},{5,7,8,9}, NO_TETRA},
// Index = 63, Case where edges: 4,5,6,7,8 are split
{{0,4,6,7},{1,4,8,5},{2,5,9,6},{3,7,9,8},{4,5,6,7},{4,5,7,8},{5,6,7,9},{5,7,8,9}},
// In case we reach outside the table
{ NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
};

// Return the classification state for each original vertex.
// TRIANGLE_VERTEX_STATE[originalvertex]
//                                    edge: 2 1 0
static int TRIANGLE_VERTEX_STATE[3]={5,  // 1 0 1
                                     3,  // 0 1 1
                                     6}; // 1 1 0

vtkStandardNewMacro(vtkSimpleCellTessellator);
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
#ifndef NDEBUG
    for(int i=0;i<6;i++)
    {
      this->PointId[i] = -1;
      this->Vertex[i][0] = -100;
      this->Vertex[i][1] = -100;
      this->Vertex[i][2] = -100;
    }
#endif
    this->SubdivisionLevel = 0;
    assert("inv: " && this->ClassInvariant());
  }

#if 0
  int DifferentFromOriginals(double local[3])
  {
    int result = 1;
    int k = 0;
    while(k<3 && result)
    {
      result = !((local[0] == this->Vertex[k][0])
              && (local[1] == this->Vertex[k][1])
              && (local[2] == this->Vertex[k][2]));
      ++k;
    }
    return result;
  }
#endif

#ifndef NDEBUG
  int ClassInvariant()
  {
    // Mid point are different from all original points.
    int isValid = 1;
    int j = 3;
    int k;
    while(j<6 && isValid)
    {
      // Don't even look at original points if the mid-point is not
      // initialized
      isValid = (this->Vertex[j][0] == -100)
             && (this->Vertex[j][1] == -100)
             && (this->Vertex[j][2] == -100);
      if(!isValid)
      {
        k = 0;
        isValid = 1;
        while(k<3 && isValid)
        {
          isValid = !((this->Vertex[j][0] == this->Vertex[k][0])
                   && (this->Vertex[j][1] == this->Vertex[k][1])
                   && (this->Vertex[j][2] == this->Vertex[k][2]));
          ++k;
        }
      }
      ++j;
    }
    return isValid;
  }
#endif

  void SetSubdivisionLevel(int level)
  {
    assert("pre: positive_level" && level>=0);
    this->SubdivisionLevel = level;
  }

  int GetSubdivisionLevel()
  {
    return this->SubdivisionLevel;
  }

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

  // Description:
  // Copy point j of source into point i of the current tile.
  void CopyPoint(int i,
                 vtkTriangleTile *source,
                 int j)
  {
      assert("pre: primary_i" && i>=0 && i<=2);
      assert("pre: source_exists" && source!=0);
      assert("pre: valid_j" && j>=0 && j<=5);

      this->PointId[i] = source->PointId[j];

      this->Vertex[i][0] = source->Vertex[j][0];
      this->Vertex[i][1] = source->Vertex[j][1];
      this->Vertex[i][2] = source->Vertex[j][2];

      this->ClassificationState[i]=source->ClassificationState[j];

      assert("inv: " && this->ClassInvariant());
  }



  // can tile be split; if so, return TessellatePointsing tiles
  //  vtkTriangleTile res[4]
  int Refine( vtkSimpleCellTessellator* tess, vtkTriangleTile *res );

  // Description:
  // Initialize the Edges array as for a root triangle
  void SetOriginal()
  {
      this->ClassificationState[0]=TRIANGLE_VERTEX_STATE[0];
      this->ClassificationState[1]=TRIANGLE_VERTEX_STATE[1];
      this->ClassificationState[2]=TRIANGLE_VERTEX_STATE[2];
  }

  // Description:
  // Find the parent (if any) of the edge defined by the local point ids i and
  // j. Return the local id of the parent edge, -1 otherwise.
  signed char FindEdgeParent(int p1,
                             int p2)
  {
      assert("pre: primary point" && p1>=0 && p1<=2 && p2>=0 && p2<=2);
      signed char result=-1;

      int midPointState=this->ClassificationState[p1]&this->ClassificationState[p2];
      if(midPointState==0)
      {
        result=-1; // no parent edge
      }
      else
      {
        if((midPointState&1)!=0)
        {
          result=0;
        }
        else
        {
          if((midPointState&2)!=0)
          {
            result=1;
          }
          else
          {
            result=2;
          }
        }
      }
      return result;
  }

  // Description:
  // Set the edge parent of mid as parentEdge.
  void SetEdgeParent(int mid,
                     int p1,
                     int p2)
  {
      assert("pre: mid-point" && mid>=3 && mid<=5);
      assert("pre: primary point" && p1>=0 && p1<=2 && p2>=0 && p2<=2);
      this->ClassificationState[mid]=this->ClassificationState[p1]&this->ClassificationState[p2];
  }

private:
  // Keep track of local coordinate in order to evaluate shape function
  double Vertex[3+3][3];  //3 points + 3 mid edge points
  vtkIdType PointId[3+3];
  int SubdivisionLevel;

  // bit i (0 to 3) tells if point p (0 to 5) is laying on original edge i.
  unsigned char ClassificationState[6];
};
//-----------------------------------------------------------------------------
//
// vtkTetraTile
//

class vtkTetraTile;

// For each of the 4 original vertices, list of the 3 edges it belongs to
// each sub-array is in increasing order.
// [vertex][edge]
static int VERTEX_EDGES[4][3]={{0,2,3},{0,1,4},{1,2,5},{3,4,5}};
// For each of the 4 original vertices, list of the 3 faces it belongs to
// each sub-array is in increasing order.
// [vertex][face]
static int VERTEX_FACES[4][3]={{0,2,3},{0,1,3},{1,2,3},{0,1,2}};

// Return the classification state for each original vertex.
// TETRA_VERTEX_STATE[originalvertex]
//                                           f3 f2 f1 f0 e5 e4 e3 e2 e1 e0
static int TETRA_VERTEX_STATE[4]={0x34d,  // 1  1  0  1  0  0  1  1  0  1
                                  0x2d3,  // 1  0  1  1  0  1  0  0  1  1
                                  0x3a6,  // 1  1  1  0  1  0  0  1  1  0
                                  0x1f8}; // 0  1  1  1  1  1  1  0  0  0

class vtkTetraTile
{
public:
  vtkTetraTile()
  {
#ifndef NDEBUG
    for(int i=0;i<10;i++)
    {
      this->PointId[i] = -1;
      this->Vertex[i][0] = -100;
      this->Vertex[i][1] = -100;
      this->Vertex[i][2] = -100;
    }
#endif
    this->SubdivisionLevel = 0;
    assert("inv: " && this->ClassInvariant());
  }

#if 0
  int DifferentFromOriginals(double local[3])
  {
    int result=1;
    int k=0;
    while(k<4 && result)
    {
      result=!((local[0] ==this->Vertex[k][0]) &&
               (local[1] == this->Vertex[k][1])
               && (local[2] == this->Vertex[k][2]));
      ++k;
    }
    return result;
  }
#endif

#ifndef NDEBUG
  int ClassInvariant()
  {
    // Mid point are different from all original points.
    int isValid = 1;
    int j = 4;
    int k;
    while(j<10 && isValid)
    {
      // Don't even look at original points if the mid-point is not
      // initialized
      isValid = (this->Vertex[j][0] == -100)
             && (this->Vertex[j][1] == -100)
             && (this->Vertex[j][2] == -100);
      if(!isValid)
      {
        k = 0;
        isValid = 1;
        while(k<4 && isValid)
        {
          isValid = !((this->Vertex[j][0] == this->Vertex[k][0])
                   && (this->Vertex[j][1] == this->Vertex[k][1])
                   && (this->Vertex[j][2] == this->Vertex[k][2]));
          ++k;
        }
      }
      ++j;
    }
    return isValid;
  }
#endif

  void SetSubdivisionLevel(int level)
  {
    assert("pre: positive_level" && level>=0);
    this->SubdivisionLevel=level;
  }

  int GetSubdivisionLevel()
  {
    return this->SubdivisionLevel;
  }

  void SetVertex( int i, double v[3] )
  {
    this->Vertex[i][0] = v[0];
    this->Vertex[i][1] = v[1];
    this->Vertex[i][2] = v[2];
    assert("inv: " && this->ClassInvariant());
  }

  void SetPointId(int i, vtkIdType id) { this->PointId[i] = id; }

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

  double *GetVertex( int i ) { return Vertex[i]; }

  vtkIdType GetPointId( int i ) { return this->PointId[i]; }

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

  // Description:
  // Copy point j of source into point i of the current tile.
  void CopyPoint(int i,
                 vtkTetraTile *source,
                 int j)
  {
      assert("pre: primary_i" && i>=0 && i<=3);
      assert("pre: source_exists" && source!=0);
      assert("pre: valid_j" && j>=0 && j<=9);

      this->PointId[i] = source->PointId[j];

      this->Vertex[i][0] = source->Vertex[j][0];
      this->Vertex[i][1] = source->Vertex[j][1];
      this->Vertex[i][2] = source->Vertex[j][2];

      this->ClassificationState[i]=source->ClassificationState[j];

      assert("inv: " && this->ClassInvariant());
  }

  // Description:
  // Copy the pointer to the Edge and Face Ids on the
  // top-level sub-tetrahedron.
  void CopyEdgeAndFaceIds(vtkTetraTile *source)
  {
      assert("pre: source_exists" && source!=0);
      this->EdgeIds= source->EdgeIds;
      this->FaceIds= source->FaceIds;
  }

  // Description:
  // Return the local edge id the complex cell from the local edge id
  // of the top-level subtetra
  int GetEdgeIds(int idx)
  {
      assert("pre:" && idx>=0); // <=number of edges on a complex cell
      return this->EdgeIds[idx];
  }

  // Description:
  // Return the local face id the complex cell from the local face id
  // of the top-level subtetra
  int GetFaceIds(int idx)
  {
      assert("pre:" && idx>=0);// <=number of faces on a complex cell
      return this->FaceIds[idx];
  }

  // can tile be split; if so, return TessellatePointsing tiles
  // There can't be more than 8 tetras as it corresponds to the splitting
  // of all edges
  // vtkTetraTile res[8]
  int Refine( vtkSimpleCellTessellator* tess, vtkTetraTile *res);

  // Description:
  // Initialize the Edges and Faces arrays as for a root tetrahedron
  void SetOriginal(vtkIdType order[4],
                   int *edgeIds, //6
                   int *faceIds) // 4
  {
      this->EdgeIds=edgeIds;
      this->FaceIds=faceIds;

      int i=0;
      while(i<4) // for each vertex
      {
        int j=order[i];
        this->ClassificationState[i]=TETRA_VERTEX_STATE[j];

        int n=0;
        int tmp;
        unsigned short mask;
        while(n<3) // copy each edge
        {
          tmp=VERTEX_EDGES[j][n];
          if(edgeIds[tmp]==-1)
          {
            mask=~(1<<tmp);
            this->ClassificationState[i]=this->ClassificationState[i]&mask;
          }
          tmp=VERTEX_FACES[j][n];
          if(faceIds[tmp]==-1)
          {
            mask=~(1<<(tmp+6));
            this->ClassificationState[i]=this->ClassificationState[i]&mask;
          }
          ++n;
        }
        ++i;
      }
  }

  // Description:
  // Find the parent (if any) of the edge defined by the local point ids i and
  // j. Return the local id of the parent edge, -1 otherwise.
  int FindEdgeParent(int p1,
                     int p2,
                     signed char &parentId)
  {
      assert("pre: primary point" && p1>=0 && p1<=3 && p2>=0 && p2<=3);

      unsigned short midPointState=this->ClassificationState[p1]&this->ClassificationState[p2];

      int result;
      if(midPointState==0)
      {
        result=3;
        parentId=-1;
      }
      else
      {
        if(midPointState&(0x3f))
        {
          result=1; // on edge
          parentId=0; // TODO
          unsigned short mask=1;
          int found=0;
          while(parentId<5 && !found)
          {
            found=(midPointState&mask)!=0;
            if(!found)
            {
              mask<<=1;
              ++parentId;
            }
          }
        }
        else
        {
          result=2; // on face
          parentId=0; // TODO

          unsigned short mask=0x40; // first face bit
          int found=0;
          while(parentId<4 && !found)
          {
            found=(midPointState&mask)!=0;
            if(!found)
            {
              mask<<=1;
              ++parentId;
            }
          }

        }
      }
      return result;
  }


  // Description:
  // Set the edge parent of mid as parentEdge.
  void SetParent(int mid,
                 int p1,
                 int p2)
  {
      assert("pre: mid-point" && mid>=4 && mid<=9);
      assert("pre: primary point" && p1>=0 && p1<=3 && p2>=0 && p2<=3);

      this->ClassificationState[mid]=this->ClassificationState[p1]&this->ClassificationState[p2];
  }

  // Description:
  // Return if the four corner points of the tetra are all differents
#ifndef NDEBUG
  int PointsDifferents()
  {
    int result=1;
    int i;
    int j;
    int k;

    i = 0;
    while(i<3 && result)
    {
      j = i+1;
      while(j<4 && result)
      {
        result = this->PointId[i] != this->PointId[j];
        ++j;
      }
      ++i;
    }
    if(result) // point id are ok, now test the coordinates
    {
      i = 0;
      while(i<3 && result)
      {
        j = i+1;
        while(j<4 && result)
        {
          k = 0;
          result = 0;
          while(k<3)
          {
            result = result || (this->Vertex[i][k] != this->Vertex[j][k]);
            ++k;
          }
          ++j;
        }
        ++i;
      }
    }

    return result;
  }
#endif

private:
  // Need to keep track of local coordinate to evaluate shape functions
  // So all work is done in parametric coordinate

  double Vertex[4+6][3];  // 4 tetra points + 6 mid edge points
  vtkIdType PointId[4+6];
  int SubdivisionLevel;

  // bit i (0 to 5) tells if point p (0 to 9) is laying on original edge i.
  // bit j (6 to 9) tells if point p (0 to 9) is laying on original face j.
  unsigned short ClassificationState[4+6];

  int *EdgeIds;
  int *FaceIds;
};

//-----------------------------------------------------------------------------
int vtkTriangleTile::Refine(vtkSimpleCellTessellator* tess,
                            vtkTriangleTile *res )
{
  // The output will contain a maximum of 4 vtkTriangleTiles
  int i, index;
  int numTriangleCreated = 0;

  double edgeSplitList[3];
  vtkIdType ptId = 0;
  int l, r;

  if(this->SubdivisionLevel < tess->GetMaxSubdivisionLevel())
  {
    // loop over edges
    for(i=0, index=0; i<3; i++)
    {
      // we have to calculate mid point between edge TRIANGLE_EDGES_TABLE[i][0]
      // and TRIANGLE_EDGES_TABLE[i][1]
      l = TRIANGLE_EDGES_TABLE[i][0];
      r = TRIANGLE_EDGES_TABLE[i][1];

      edgeSplitList[i] = tess->EdgeTable->CheckEdge(this->PointId[l],
                                                    this->PointId[r], ptId);

      // On previous step we made sure to prepare the hash table
      assert("check: edge table prepared" && edgeSplitList[i] != -1);

      // Build the case table
      if (edgeSplitList[i])
      {
        index |= 1 << i;
      }
    }

    if( index )
    {
      // That mean at least one edge was split and thus index != 0
      signed char *cases = **(vtkTessellatorTriangleCases + index);

      for(; cases[0] > -1; cases+=3)
      {
        for(int j=0; j<3; j++)
        {
          res[numTriangleCreated].CopyPoint(j,this,cases[j]);
//          res[numTriangleCreated].SetPointId( j, this->PointId[cases[j]] );
//          res[numTriangleCreated].SetVertex( j, this->Vertex[cases[j]] );
        }
        //update number of triangles
        numTriangleCreated++;
      }
      //Insert edges from new triangle into hash table:
      for(int k=0; k < numTriangleCreated; k++)
      {
        res[k].SubdivisionLevel = this->SubdivisionLevel + 1;
        tess->InsertEdgesIntoEdgeTable( res[k] );
      }
    }
  }

  if(numTriangleCreated == 0)
  {
    // no edge were split so recursion is done
    // add the cell array to the list
    tess->TessellateCellArray->InsertNextCell(3, this->PointId);

    for(int j=0; j<3; j++)
    {
      tess->CopyPoint(this->PointId[j]);
    }
  }

  return numTriangleCreated;
}

//-----------------------------------------------------------------------------
// Description:
// Extract point `pointId' from the edge table to the output point and output
// point data.
void vtkSimpleCellTessellator::CopyPoint(vtkIdType pointId)
{
  double point[3];
  double *p = this->Scalars;

  this->EdgeTable->CheckPoint(pointId, point, p);
  // There will some be duplicate points during a while but
  // this is the cost for speed:
  this->TessellatePoints->InsertNextTuple( point );
//  this->TessellatePointData->InsertNextTuple( tess->Scalars );

  int c = this->TessellatePointData->GetNumberOfArrays();
  vtkDataArray *attribute;

  for(int i=0; i<c; i++)
  {
    attribute = this->TessellatePointData->GetArray(i);
    attribute->InsertNextTuple(p);
    p += attribute->GetNumberOfComponents();
  }
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
  order[0] = idx1;
  order[1] = idx2;

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
int vtkTetraTile::Refine(vtkSimpleCellTessellator* tess,
                         vtkTetraTile *res)
{
  // The output will contains a maximum of 8 vtkTetraTiles
  int i, index;
  int numTetraCreated = 0;

  // We need to order the point by lower id first
  // this will create an edge ordering and based on that we can
  // find which edge is split this gives us a mask for the tessellation

  // There is only 6 edges in a tetra we need this structure to quickly
  // determine in which case we are to tessellate the tetra.
  int edgeSplitList[6];
  vtkIdType ptId = 0;
  int l, r;

  if(this->SubdivisionLevel < tess->GetMaxSubdivisionLevel())
  {
    // loop over edges:
    for(i=0, index=0; i<6; i++)
    {
      // we have to calculate mid point between edge TETRA_EDGES_TABLE[i][0] and
      // TETRA_EDGES_TABLE[i][1]
      l = TETRA_EDGES_TABLE[i][0];
      r = TETRA_EDGES_TABLE[i][1];

      edgeSplitList[i] = tess->EdgeTable->CheckEdge(this->PointId[l],
                                                    this->PointId[r], ptId);

      // On previous step we made sure to prepare the hash table
      assert("check: edge table prepared" && edgeSplitList[i] != -1);

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
      int k;

      for(; cases[0]> -1; cases+=4)
      {
        for(k=0; k<4; k++)
        {
          // This is the line that makes Visual Studio 7.0 to fail compiling
          // a valid code in release mode. If we add the following line before
          // the actual use of cases[k], everything works fine...
          // line that fixes Visual Studio: cout<<cases[k]<<endl;
          tetra[k] = this->PointId[cases[k]];
        }

        // The whole purpose of Reorder is really to classify the tetra, the
        // reordering is only useful for quick testing. The tet will either
        // classify as Right ordered or Left ordered
        Reorder(tetra, order);

        // Set the tetras point for the next recursion
        for(int j=0;j<4;j++)
        {
          res[numTetraCreated].CopyPoint(j,this,cases[order[j]]);
        }
        res[numTetraCreated].CopyEdgeAndFaceIds(this);
        numTetraCreated++;
      }
      k = 0;
      while(k < numTetraCreated)
      {
        res[k].SubdivisionLevel = this->SubdivisionLevel + 1;
        tess->InsertEdgesIntoEdgeTable( res[k] );
        ++k;
      }
    }
  }

  if(numTetraCreated == 0)
  {
    // no edge were split so recursion is done
    // add the cell array to the list
    tess->TessellateCellArray->InsertNextCell(4, this->PointId);

    for(int j=0; j<4; j++)
    {
      tess->CopyPoint(this->PointId[j]);
    }
  }

  return numTetraCreated;
}

//-----------------------------------------------------------------------------
// Create the tessellator helper with a default of 0.25 for threshold
//
vtkSimpleCellTessellator::vtkSimpleCellTessellator()
{
  this->GenericCell = NULL;

  this->TessellatePoints    = NULL;
  this->TessellateCellArray = NULL;
  this->TessellatePointData = NULL;

  this->EdgeTable = vtkGenericEdgeTable::New();

  this->AttributeCollection = NULL;

  this->CellIterator    = 0;
  this->Scalars         = 0;
  this->ScalarsCapacity = 0;
  this->PointOffset     = 0;

  this->DataSet         = 0;

  this->FixedSubdivisions       = 0; // 0 means no fixed subdivision
  this->MaxSubdivisionLevel     = 0; // 0 means no subdivision at all
  this->CurrentSubdivisionLevel = 0;


  this->Triangulator=vtkOrderedTriangulator::New();
  this->Triangulator->UseTemplatesOn();

  this->PointIds=0;
  this->PointIdsCapacity=0;

  this->Connectivity=vtkCellArray::New();
  this->Polygon=vtkPolygon::New();
  this->TriangleIds=vtkIdList::New();
  this->TriangleIds->Allocate(VTK_CELL_SIZE);
}

//-----------------------------------------------------------------------------
vtkSimpleCellTessellator::~vtkSimpleCellTessellator()
{
  this->EdgeTable->Delete();
  if(this->CellIterator)
  {
    this->CellIterator->Delete();
  }
  delete[] this->Scalars;

  this->Triangulator->Delete();
  delete[] this->PointIds;

  this->Connectivity->Delete();
  this->Polygon->Delete();
  this->TriangleIds->Delete();
}

//-----------------------------------------------------------------------------
// This function is supposed to be called only at toplevel (for passing data
// from third party to the hash point table)
void vtkSimpleCellTessellator::InsertPointsIntoEdgeTable(vtkTriangleTile &tri)
{
  double global[3];

  for(int j=0; j<3; j++)
  {
    // Need to check first if point is not already in the hash table
    // since EvaluateLocation / EvaluateTuple are expensive calls
    if( !this->EdgeTable->CheckPoint(tri.GetPointId(j)) )
    {
      // it's real space coordinate:
      this->GenericCell->EvaluateLocation(0,tri.GetVertex(j), global);

      // Then scalar value associated with point:
      this->GenericCell->InterpolateTuple(this->AttributeCollection,
                                          tri.GetVertex(j), this->Scalars);

      //Put everything in this point hash table
      this->EdgeTable->InsertPointAndScalar(tri.GetPointId(j), global,
                                            this->Scalars);
    }
  }
}

//-----------------------------------------------------------------------------
//
void vtkSimpleCellTessellator::InsertEdgesIntoEdgeTable(vtkTriangleTile &tri )
{
  double *local = 0;
  vtkIdType tmp;
  vtkIdType l, r;
  vtkIdType cellId = this->GenericCell->GetId();

  const double alpha = 0.5;
  assert("check: normalized alpha" && alpha>0 && alpha<1);

  //First setup the point reference count:
  for(int i = 0; i<3; i++)
  {
    this->EdgeTable->IncrementPointReferenceCount(tri.GetPointId(i));
  }

  double *leftPoint  = this->Scalars;
  double *midPoint   = this->Scalars + this->PointOffset;
  double *rightPoint = midPoint + this->PointOffset;


  // Loop over all edges:
  // For each edge:
  //    if in hash table: incr ref
  //    else:             evaluate & put in table ref = 1
  for(int j=0; j<3; j++)
  {
    l = TRIANGLE_EDGES_TABLE[j][0];
    r = TRIANGLE_EDGES_TABLE[j][1];

    vtkIdType leftId  = tri.GetPointId(l);
    vtkIdType rightId = tri.GetPointId(r);

    if(leftId > rightId)
    {
      // ensure that the left point has the smallest id
      // hence, evaluation occurs in the same direction in any case
      // the computations of error and interpolation will not suffer from
      // numerical precision.
      tmp     = leftId;
      leftId  = rightId;
      rightId = tmp;

      tmp = l;
      l   = r;
      r   = tmp;
    }

    double *left  = tri.GetVertex(l);
    double *right = tri.GetVertex(r);

    memcpy(leftPoint  + PARAMETRIC_OFFSET, left,  sizeof(double)*3);
    memcpy(rightPoint + PARAMETRIC_OFFSET, right, sizeof(double)*3);

    //Check first in the hash table
    vtkIdType ptId = -1;

    // To calculate the edge ref count, we either:
    // - find it in the hash table
    // - calculate from higher order cell:

    int toSplit = this->EdgeTable->CheckEdge(leftId, rightId, ptId);
    int doSubdivision;

    if( toSplit == -1)
    {
      // The edge was not found in the hash table, that mean we have to
      // determine it's reference counting from the higher order cell:

      signed char parentEdge=tri.FindEdgeParent(l,r);
      int refCount;
      if(parentEdge==-1)
      {
        // no parent
        refCount = 1;
      }
      else
      {
        refCount = this->GetNumberOfCellsUsingEdge(parentEdge);
      }

      doSubdivision = tri.GetSubdivisionLevel() < this->GetMaxSubdivisionLevel();

      //
      // For measurement of the quality of a fixed subdivision.
      //
      if(!doSubdivision) // done
      {
        if(this->GetMaxSubdivisionLevel()==this->GetFixedSubdivisions())
        {
          // fixed subdivision only
          if(this->GetMeasurement())
          {
            // global position and attributes at the left vertex
            this->EdgeTable->CheckPoint(leftId,leftPoint,
                                        leftPoint + ATTRIBUTES_OFFSET);
            // global position and attributes at the right vertex
            this->EdgeTable->CheckPoint(rightId,rightPoint,
                                        rightPoint + ATTRIBUTES_OFFSET);

            // parametric center of the edge
            local = midPoint + PARAMETRIC_OFFSET;

            for(int i=0; i<3; i++)
            {
              local[i] = left[i] + alpha*(right[i] - left[i]);
            }
            // global position of the center
            this->GenericCell->EvaluateLocation(0,local,midPoint);

            // attributes at the center
            this->GenericCell->InterpolateTuple(this->AttributeCollection,
                                                local,
                                                midPoint+ATTRIBUTES_OFFSET);
            this->UpdateMaxError(leftPoint,midPoint,rightPoint,alpha);
          }
        }
      }
      //
      //
      //


      if(doSubdivision)
      {
        // global position and attributes at the left vertex
        this->EdgeTable->CheckPoint(leftId, leftPoint,
                                    leftPoint + ATTRIBUTES_OFFSET);
        // global position and attributes at the right vertex
        this->EdgeTable->CheckPoint(rightId, rightPoint,
                                    rightPoint + ATTRIBUTES_OFFSET);

        // parametric center of the edge
        local = midPoint + PARAMETRIC_OFFSET;
        for(int i=0; i < 3; i++)
        {
          local[i] = left[i] + alpha*(right[i] - left[i]);
        }
        // is the mid point different from both the left and right point?
        // if not, we do not subdivide, it is a degenerated case.
        //doSubdivision = tri.DifferentFromOriginals(local);
        doSubdivision = (alpha != 0.0 && alpha != 1.0);

        if(doSubdivision)
        {
          // global position of the center
          this->GenericCell->EvaluateLocation(0,local,midPoint);

          // attributes at the center
          this->GenericCell->InterpolateTuple(this->AttributeCollection, local,
                                              midPoint + ATTRIBUTES_OFFSET);

          doSubdivision = tri.GetSubdivisionLevel() < this->GetFixedSubdivisions();
          if(!doSubdivision) // fixed subdivision is done, need adaptive one?
          {
            doSubdivision = this->RequiresEdgeSubdivision(leftPoint,midPoint,
                                                          rightPoint,alpha);
          }
        }
      } // first doSubdivision

      if(doSubdivision)
      {
        this->EdgeTable->InsertEdge(leftId, rightId, cellId, refCount, ptId);
        assert("check: id exists" && ptId != -1 );

        // And also the value we'll have to put to avoid recomputing them later:

        //Save mid point:
        tri.SetVertex(j+3, local);
        tri.SetPointId(j+3, ptId);
        tri.SetEdgeParent(j+3,l,r); //parentEdge);

        //Put everything in this point hash table
        this->EdgeTable->InsertPointAndScalar(ptId, midPoint,
                                              midPoint + ATTRIBUTES_OFFSET);
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

      if(toSplit == 1) // we cannot just right if(toSplit) because it can be -1
      {
        tri.SetPointId(j+3, ptId);

        double pcoords[3];
        pcoords[0] = tri.GetVertex(l)[0] + alpha*(tri.GetVertex(r)[0] - tri.GetVertex(l)[0]);
        pcoords[1] = tri.GetVertex(l)[1] + alpha*(tri.GetVertex(r)[1] - tri.GetVertex(l)[1]);
        pcoords[2] = tri.GetVertex(l)[2] + alpha*(tri.GetVertex(r)[2] - tri.GetVertex(l)[2]);

        tri.SetVertex(j+3, pcoords);
        // note we don't need to call SetEdgeParent() because
        // if the edge is already in the hashtable it means that
        // it is already tessellated. All other point using this
        // edge will come from either inside the triangle either from
        // and another edge. For sur the resulting edge will be inside (-1)

        tri.SetEdgeParent(j+3,l,r);

      }
    }
  }
}

//-----------------------------------------------------------------------------
//
void vtkSimpleCellTessellator::InsertEdgesIntoEdgeTable( vtkTetraTile &tetra )
{
  double *local = 0;

  vtkIdType tmp;
  vtkIdType l, r;
  const vtkIdType cellId = this->GenericCell->GetId();

//  double alpha=0.5+0.02*(rand()/(RAND_MAX+1.0)-0.5);
  const double alpha = 0.5;
  assert("check: normalized alpha" && alpha>0 && alpha<1);

  //First setup the point reference count:
  for(int i=0; i<4; i++)
  {
    this->EdgeTable->IncrementPointReferenceCount(tetra.GetPointId(i));
  }

  double *leftPoint  = this->Scalars;
  double *midPoint   = this->Scalars + this->PointOffset;
  double *rightPoint = midPoint + this->PointOffset;

  // Loop over all edges:
  // For each edge:
  //    if in hash table: incr ref
  //    else:             evaluate & put in table ref = 1
  for(int j=0; j<6; j++)
  {
    l = TETRA_EDGES_TABLE[j][0];
    r = TETRA_EDGES_TABLE[j][1];

    vtkIdType leftId  = tetra.GetPointId(l);
    vtkIdType rightId = tetra.GetPointId(r);

    if(leftId > rightId)
    {
      // ensure that the left point has the smallest id
      // hence, evaluation occurs in the same direction in any case
      // the computations of error and interpolation will not suffer from
      // numerical precision.
      tmp     = leftId;
      leftId  = rightId;
      rightId = tmp;

      tmp = l;
      l   = r;
      r   = tmp;
    }

    double *left  = tetra.GetVertex(l);
    double *right = tetra.GetVertex(r);

    memcpy(leftPoint  + PARAMETRIC_OFFSET, left,  sizeof(double)*3);
    memcpy(rightPoint + PARAMETRIC_OFFSET, right, sizeof(double)*3);

    //Check first in the hash table
    vtkIdType ptId = -1;
    int refCount = 1;

    //vtkDebugMacro( << "InsertEdgesIntoEdgeTable:" << leftId << "," << rightId );

    // To calculate the edge ref count, we either:
    // - find it in the hash table
    // - calculate from higher order cell:

    int toSplit = this->EdgeTable->CheckEdge(leftId, rightId, ptId);
    int doSubdivision;

    if( toSplit == -1)
    {
      // The edge was not found in the hash table, that mean we have to
      // determine it's reference counting from the higher order cell:


      signed char parentId;
      int type=tetra.FindEdgeParent(l,r,parentId);
      if(type == 1)
      {
        // On edge:
        refCount = this->GetNumberOfCellsUsingEdge( tetra.GetEdgeIds(parentId) );
      }
      else if(type == 2)
      {
        //On face:
        refCount = this->GetNumberOfCellsUsingFace( tetra.GetFaceIds(parentId) );
      }
      else if(type == 3)
      {
        // Inside:
        refCount = 1;
      }

      doSubdivision = tetra.GetSubdivisionLevel() < this->GetMaxSubdivisionLevel();

      //
      // For measurement of the quality of a fixed subdivision.
      //
      if(!doSubdivision) // done
      {
        if(this->GetMaxSubdivisionLevel()==this->GetFixedSubdivisions())
        {
          // fixed subdivision only
          if(this->GetMeasurement())
          {
            // global position and attributes at the left vertex
            this->EdgeTable->CheckPoint(leftId,leftPoint,
                                        leftPoint + ATTRIBUTES_OFFSET);
            // global position and attributes at the right vertex
            this->EdgeTable->CheckPoint(rightId,rightPoint,
                                        rightPoint + ATTRIBUTES_OFFSET);

            // parametric center of the edge
            local = midPoint + PARAMETRIC_OFFSET;

            for(int i=0; i<3; i++)
            {
              local[i] = left[i] + alpha*(right[i] - left[i]);
            }
            // global position of the center
            this->GenericCell->EvaluateLocation(0,local,midPoint);

            // attributes at the center
            this->GenericCell->InterpolateTuple(this->AttributeCollection,
                                                local,
                                                midPoint+ATTRIBUTES_OFFSET);
            this->UpdateMaxError(leftPoint,midPoint,rightPoint,alpha);
          }
        }
      }
      //
      //
      //

      if(doSubdivision)
      {
        // global position and attributes at the left vertex
        this->EdgeTable->CheckPoint(leftId,leftPoint,
                                    leftPoint + ATTRIBUTES_OFFSET);
        // global position and attributes at the right vertex
        this->EdgeTable->CheckPoint(rightId,rightPoint,
                                    rightPoint + ATTRIBUTES_OFFSET);

        // parametric center of the edge
        local = midPoint + PARAMETRIC_OFFSET;

        for(int i=0; i<3; i++)
        {
          local[i] = left[i] + alpha*(right[i] - left[i]);
        }
        // is the mid point different from both the left and right point?
        // if not, we do not subdivide, it is a degenerated case.
        //doSubdivision=tetra.DifferentFromOriginals(local);
        doSubdivision = (alpha != 0.0 && alpha != 1.0);

        if(doSubdivision)
        {
          // global position of the center
          this->GenericCell->EvaluateLocation(0,local,midPoint);

          // attributes at the center
          this->GenericCell->InterpolateTuple(this->AttributeCollection, local,
                                              midPoint + ATTRIBUTES_OFFSET);

          doSubdivision = tetra.GetSubdivisionLevel() < this->GetFixedSubdivisions();
          if(!doSubdivision) // fixed subdivision is done, need adaptive one?
          {
            doSubdivision = this->RequiresEdgeSubdivision(leftPoint,midPoint,
                                                          rightPoint,alpha);
          }
        }

      } // first doSubdivision

      if(doSubdivision)
      {
        this->EdgeTable->InsertEdge(leftId, rightId, cellId, refCount, ptId);
        assert("check: id exists" && ptId != -1 );

        // And also the value we'll have to put to avoid recomputing them later:
        //Save mid point:
        tetra.SetVertex(j+4, local);
        tetra.SetPointId(j+4, ptId);
        tetra.SetParent(j+4,l,r);

//        tetra.SetParent(j+4,parentId,type);

        //Put everything in the point hash table
        this->EdgeTable->InsertPointAndScalar(ptId, midPoint,
                                              midPoint + ATTRIBUTES_OFFSET);
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

      if(toSplit == 1) // we cannot just right if(toSplit) because it can be -1
      {
        tetra.SetPointId(j+4, ptId);

        double pcoords[3];
        pcoords[0] = tetra.GetVertex(l)[0]+ alpha*(tetra.GetVertex(r)[0] - tetra.GetVertex(l)[0]);
        pcoords[1] = tetra.GetVertex(l)[1]+ alpha*(tetra.GetVertex(r)[1] - tetra.GetVertex(l)[1]);
        pcoords[2] = tetra.GetVertex(l)[2]+ alpha*(tetra.GetVertex(r)[2] - tetra.GetVertex(l)[2]);
        assert("not degenerated" &&  !(((left[0]  == pcoords[0])
                                     && (left[1]  == pcoords[1])
                                     && (left[2]  == pcoords[2]))
                                    || ((right[0] == pcoords[0])
                                     && (right[1] == pcoords[1])
                                     && (right[2] == pcoords[2]))));

        tetra.SetVertex(j+4, pcoords);

//        signed char parentId;
//        int type=tetra.FindEdgeParent(l,r,parentId);

//        tetra.SetParent(j+4,parentId,type); //tetra.SetParent(j+4,-1,3);

        tetra.SetParent(j+4,l,r);
      }
    }
  }
}

//-----------------------------------------------------------------------------
void vtkSimpleCellTessellator::RemoveEdgesFromEdgeTable( vtkTriangleTile &tri )
{
  vtkIdType l,r;
  int i;

  // First setup the point reference count:
  for(i=0; i<3; i++)
  {
    this->EdgeTable->RemovePoint( tri.GetPointId(i));
  }

  // Clean the hash table by removing all edges from this tet, loop over edges:
  for(i=0; i<3; i++)
  {
    l = TRIANGLE_EDGES_TABLE[i][0];
    r = TRIANGLE_EDGES_TABLE[i][1];

    this->EdgeTable->RemoveEdge(tri.GetPointId(l), tri.GetPointId(r));
  }
}
//-----------------------------------------------------------------------------
void vtkSimpleCellTessellator::RemoveEdgesFromEdgeTable( vtkTetraTile &tetra )
{
  vtkIdType l,r;
  int i;

  // First setup the point reference count:
  for(i=0; i<4; i++)
  {
    this->EdgeTable->RemovePoint( tetra.GetPointId(i));
  }

  // Clean the hash table by removing all edges from this tet, loop over edges:
  for(i=0; i<6; i++)
  {
    l = TETRA_EDGES_TABLE[i][0];
    r = TETRA_EDGES_TABLE[i][1];

    vtkIdType ll = tetra.GetPointId(l);
    vtkIdType rr = tetra.GetPointId(r);

    this->EdgeTable->RemoveEdge(ll, rr);
  }
}

//-----------------------------------------------------------------------------
void vtkSimpleCellTessellator::Reset()
{
  // No memory deletion should happen here, as one cell to another there
  // should be the same amount of points to tessellate
  this->TessellatePoints->Reset();
  this->TessellateCellArray->Reset();
}

//-----------------------------------------------------------------------------
// Description:
// Initialize the tessellator with a data set `ds'.
void vtkSimpleCellTessellator::Initialize(vtkGenericDataSet *ds)
{
  this->DataSet = ds;

  if(this->DataSet)
  {
    this->NumberOfPoints = this->DataSet->GetNumberOfPoints();
    this->EdgeTable->Initialize(this->NumberOfPoints);
  }
}

//-----------------------------------------------------------------------------
void vtkSimpleCellTessellator::Tessellate(vtkGenericAdaptorCell *cell,
                                          vtkGenericAttributeCollection *att,
                                          vtkDoubleArray *points,
                                          vtkCellArray *cellArray,
                                          vtkPointData *internalPd )
{
  assert("pre: cell_exists" && cell!=0);
  assert("pre: valid_dimension" && cell->GetDimension()==3);
  assert("pre: att_exists" && att!=0);
  assert("pre: points_exists" && points!=0);
  assert("pre: cellArray_exists" && cellArray!=0);
  assert("pre: internalPd_exists" && internalPd!=0);

  int j;
  int numVertices;

  // Save parameter for later use
  this->GenericCell = cell;
  this->TessellatePoints = points;
  this->TessellateCellArray = cellArray;
  this->TessellatePointData = internalPd;
  this->AttributeCollection = att;
  if(this->CellIterator==0)
  {
    this->CellIterator = cell->NewCellIterator();
  }

  // send the cell to the error metrics
  this->SetGenericCell( cell );

  int complexCell=cell->GetType()!=VTK_HIGHER_ORDER_TETRAHEDRON;

  if(complexCell)
  {
    numVertices=cell->GetNumberOfBoundaries(0);
  }
  else
  {
    numVertices=4;
  }

  this->AllocatePointIds(numVertices);
  cell->GetPointIds(this->PointIds);


  // Init the edge table
  this->EdgeTable->SetNumberOfComponents(internalPd->GetNumberOfComponents());
  this->PointOffset = internalPd->GetNumberOfComponents()+6;
  this->AllocateScalars(this->PointOffset*3);

  // Insert the points of the complex cell into the hashtable
  double global[3];
  for(j=0; j<numVertices; j++)
  {
    // Need to check first if point is not already in the hash table
    // since EvaluateLocation / EvaluateTuple are expensive calls
    if( !this->EdgeTable->CheckPoint(this->PointIds[j]) )
    {
      double *pcoords=cell->GetParametricCoords() + 3*j;
      // its real space coordinate:
      cell->EvaluateLocation(0,pcoords, global);

      // Then scalar value associated with point:
      cell->InterpolateTuple(this->AttributeCollection,
                             pcoords, this->Scalars);

      //Put everything in the point hash table
      this->EdgeTable->InsertPointAndScalar(this->PointIds[j], global,
                                            this->Scalars);
    }
  }

  std::queue<vtkTetraTile> work;
  vtkTetraTile roots[10]; // up to 10 top-level sub-tetra

  // Here, declare the edges and faces outside the if/else, as the pointer to
  // that variable will no longer be valid when out of scope.
  int edgesIdsArray[6*10]; // 6 edges per sub-tetra, max of 10 sub-tetra
  int faceIdsArray[4*10]; // 4 faces per sub-tetra,  max of 10 sub-tetra
  int *edgeIds=edgesIdsArray;
  int *faceIds=faceIdsArray;

  // Put the top-levels subtetra in the work queue.

  if(complexCell)
  {
    this->Triangulator->PreSortedOff();
    this->Triangulator->InitTriangulation(0,1,0,1,0,1,numVertices);
    int i=0;
    double *pcoords=cell->GetParametricCoords();
    while(i<numVertices)
    {
      // we feed the triangulator with dummy global coordinates
      // because we just care about the connectivity
      this->Triangulator->InsertPoint(i,pcoords,pcoords,0); // 2
      ++i;
      pcoords+=3;
    }
    this->Triangulator->Triangulate();
    this->Connectivity->Reset();
    this->Triangulator->AddTetras(0,this->Connectivity); // 1
    this->Connectivity->InitTraversal();
    vtkIdType npts=0;
    vtkIdType *pts=0;
    vtkIdType ids[4];

    int numEdges=cell->GetNumberOfBoundaries(1);
    int numFaces=cell->GetNumberOfBoundaries(2);

    int tetraId=0;
    while(this->Connectivity->GetNextCell(npts,pts))
    {
      assert("check: is a tetra" && npts==4);
      // Get the point Ids (global)
      j=0;
      while(j<4)
      {
        ids[j]=this->PointIds[pts[j]];
        ++j;
      }
      // Get the edges Ids (local)
//      int edgeIds[6];
      int *originalEdge;
      int edge[2];
      j=0;
      while(j<6)
      {
        edge[0]=pts[vtkTetra::GetEdgeArray(j)[0]];
        edge[1]=pts[vtkTetra::GetEdgeArray(j)[1]];
        int k=0;
        edgeIds[j]=-1;
        while(k<numEdges&&(edgeIds[j]==-1))
        {
          originalEdge=cell->GetEdgeArray(k);
          if((originalEdge[0]==edge[0]&&originalEdge[1]==edge[1])||
             (originalEdge[0]==edge[1]&&originalEdge[1]==edge[0]))
          {
            edgeIds[j]=k;
          }
          ++k;
        }
        ++j;
      }

      // Get the face Ids (local)
//      int faceIds[4];
      int *originalFace;
      int face[3];
      j=0;
      while(j<4)
      {
        face[0]=pts[vtkTetra::GetFaceArray(j)[0]];
        face[1]=pts[vtkTetra::GetFaceArray(j)[1]];
        face[2]=pts[vtkTetra::GetFaceArray(j)[2]];
        int k=0;
        faceIds[j]=-1;
        while(k<numFaces&&(faceIds[j]==-1))
        {
          originalFace=cell->GetFaceArray(k);

          if(this->FacesAreEqual(originalFace,face))
          {
            faceIds[j]=k;
          }
          ++k;
        }
        ++j;
      }

      this->InitTetraTile(roots[tetraId],pts,ids,edgeIds, faceIds);
      work.push(roots[tetraId]);

      edgeIds=edgeIds+6;
      faceIds=faceIds+4;
      ++tetraId;
    } // while(connectivity)
  }
  else
  {
    vtkIdType pts[4]={0,1,2,3}; // from sub-tetra tessellation

    //
    // Get the edges Ids (local)
    int *originalEdge;
    int edge[2];
    j=0;
    while(j<6)
    {
      edge[0]=vtkTetra::GetEdgeArray(j)[0]; // faster that edge[0]=pts[vtkTetra::GetEdgeArray(j)[0]]
      edge[1]=vtkTetra::GetEdgeArray(j)[1];
      int k=0;
      edgeIds[j]=-1;
      while(edgeIds[j]==-1)
      {
        originalEdge=cell->GetEdgeArray(k);
        if((originalEdge[0]==edge[0]&&originalEdge[1]==edge[1])||
           (originalEdge[0]==edge[1]&&originalEdge[1]==edge[0]))
        {
          edgeIds[j]=k;
        }
        ++k;
      }
      ++j;
    }

    // Get the face Ids (local)
    int *originalFace;
    int face[3];
    int numFaces=cell->GetNumberOfBoundaries(2);
    j=0;
    while(j<4)
    {
      face[0]=pts[vtkTetra::GetFaceArray(j)[0]];
      face[1]=pts[vtkTetra::GetFaceArray(j)[1]];
      face[2]=pts[vtkTetra::GetFaceArray(j)[2]];
      int k=0;
      faceIds[j]=-1;
      // k<this->GetNumberOfBoundaries(2) is not required because with no tessellation
      // all the faceIds array has to match with the original faces
      while(k<numFaces&&(faceIds[j]==-1))
      {
        originalFace=cell->GetFaceArray(k);
        if(this->FacesAreEqual(originalFace,face))
        {
          faceIds[j]=k;
        }
        ++k;
      }
      ++j;
    }
    this->InitTetraTile(roots[0],pts,this->PointIds,edgeIds, faceIds);
    work.push(roots[0]);
  }

  // refine loop
  int count=0;
  while( !work.empty() )
  {
    vtkTetraTile piece[8];
    vtkTetraTile curr = work.front();
    work.pop();

    int n = curr.Refine( this, piece);

    for(int i = 0; i<n; i++)
    {
      work.push( piece[i] );
    }

    // We are done we should clean ourself from the hash table:
    this->RemoveEdgesFromEdgeTable( curr );
    ++count;
  }

  // remove the points of the complex cell from the hashtable
  for(j=0; j<numVertices; j++)
  {
    this->EdgeTable->RemovePoint(this->PointIds[j]);
  }
}

//-----------------------------------------------------------------------------
void vtkSimpleCellTessellator::InitTetraTile(vtkTetraTile &root,
                                             vtkIdType *localIds,
                                             vtkIdType *ids,
                                             int *edgeIds,
                                             int *faceIds)
{
  assert("pre: cell_exists" && this->GenericCell!=0);
  assert("pre: localIds_exists" && localIds!=0);
  assert("pre: ids_exists" && ids!=0);
  assert("pre: edgeIds_exists" && edgeIds!=0);
  assert("pre: faceIds_exists" && faceIds!=0);

#ifndef NDEBUG
  vtkIdType order[4] = {-1,-1,-1,-1};
#else
  vtkIdType order[4];
#endif
  int i;
  double *point;

  Reorder(ids, order);
  for(i=0; i<4; i++)
  {
    point = this->GenericCell->GetParametricCoords() + 3*localIds[order[i]];
    root.SetVertex(i, point);
    root.SetPointId(i, ids[order[i]]);
  }
  root.SetOriginal(order,edgeIds,faceIds);

   //Prepare the hash table with the top-level edges:
  this->InsertEdgesIntoEdgeTable( root );
}
//-----------------------------------------------------------------------------
void vtkSimpleCellTessellator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "GenericCell: " << this->GenericCell << endl;
  os << indent << "TessellatePointData: "
     << this->TessellatePointData << endl;
  os << indent << "TessellateCellArray: "
     << this->TessellateCellArray << endl;
  os << indent << "TessellatePoints: "
     << this->TessellatePoints << endl;
}

//-----------------------------------------------------------------------------
void
vtkSimpleCellTessellator::TessellateFace(vtkGenericAdaptorCell *cell,
                                         vtkGenericAttributeCollection *att,
                                         vtkIdType index,
                                         vtkDoubleArray *points,
                                         vtkCellArray *cellArray,
                                         vtkPointData *internalPd)
{
  assert("pre: cell_exists" && cell!=0);
  assert("pre: valid_dimension" && cell->GetDimension()==3);
  assert("pre: valid_index_range" && (index>=0) && (index<cell->GetNumberOfBoundaries(2)));
  assert("pre: att_exists" && att!=0);
  assert("pre: points_exists" && points!=0);
  assert("pre: cellArray_exists" && cellArray!=0);
  assert("pre: internalPd_exists" && internalPd!=0);

  int j;
  if(cell->GetType()!=VTK_HIGHER_ORDER_TETRAHEDRON)
  {
    // build a linear polygon, call tessellate() on it and iterate over each triangle
    // by sending it to the tessellator

    int *faceVerts=cell->GetFaceArray(index);
    int numVerts=cell->GetNumberOfVerticesOnFace(index);
    this->Polygon->PointIds->SetNumberOfIds(numVerts);
    this->Polygon->Points->SetNumberOfPoints(numVerts);

    this->AllocatePointIds(cell->GetNumberOfBoundaries(0));
    cell->GetPointIds(this->PointIds);
    double *pcoords=cell->GetParametricCoords();

    int i=0;
    while(i<numVerts)
    {
      this->Polygon->PointIds->SetId(i,i); // this->PointIds[i]
      this->Polygon->Points->SetPoint(i, pcoords+3*faceVerts[i]); // should be global?
      ++i;
    }

    this->Polygon->Triangulate(this->TriangleIds);

    // now iterate over any sub-triangle and call triangulateface on it
    vtkIdType pts[3];
    vtkIdType ids[3];
    int c=this->TriangleIds->GetNumberOfIds();
    i=0;
    while(i<c)
    {
      // Build the next sub-triangle
      j=0;
      while(j<3)
      {
        pts[j]=faceVerts[this->TriangleIds->GetId(i)];
        // Get the point Ids (global)
        ids[j]=this->PointIds[pts[j]];
        ++j;
        ++i;
      }

      //
      // Get the edges Ids (local)
      int edgeIds[3];
      int *originalEdge;
      int edge[2];
      j=0;
      int numEdges=cell->GetNumberOfBoundaries(1);

      while(j<3)
      {
        edge[0]=pts[TRIANGLE_EDGES_TABLE[j][0]];
        edge[1]=pts[TRIANGLE_EDGES_TABLE[j][1]];
        int k=0;
        edgeIds[j]=-1;
        while(k<numEdges&&(edgeIds[j]==-1))
        {
          originalEdge=cell->GetEdgeArray(k);
          if((originalEdge[0]==edge[0]&&originalEdge[1]==edge[1])||
             (originalEdge[0]==edge[1]&&originalEdge[1]==edge[0]))
          {
            edgeIds[j]=k;
          }
          ++k;
        }
        ++j;
      }

      // index is not used in the tessellator.
      this->TriangulateTriangle(cell, pts,ids,edgeIds,att,points,cellArray, internalPd);
    }

  }
  else
  {
    vtkIdType pts[3]; // from sub-tetra tessellation

    this->AllocatePointIds(4); // tetra
    cell->GetPointIds(this->PointIds);

    int *facepts = cell->GetFaceArray(index);
    // we know we are using a tetra.
    pts[0]=facepts[0];
    pts[1]=facepts[1];
    pts[2]=facepts[2];

    vtkIdType ids[3];
    // Get the point Ids (global)
    j=0;
    while(j<3)
    {
      ids[j]=this->PointIds[pts[j]];
      ++j;
    }

      //
    // Get the edges Ids (local)
    int edgeIds[3];
    int *originalEdge;
    int edge[2];
    j=0;
    while(j<3)
    {
      edge[0]=pts[TRIANGLE_EDGES_TABLE[j][0]];
      edge[1]=pts[TRIANGLE_EDGES_TABLE[j][1]];
      int k=0;
      edgeIds[j]=-1;
      while(edgeIds[j]==-1)
      {
        originalEdge=cell->GetEdgeArray(k);
        if((originalEdge[0]==edge[0]&&originalEdge[1]==edge[1])||
           (originalEdge[0]==edge[1]&&originalEdge[1]==edge[0]))
        {
          edgeIds[j]=k;
        }
        ++k;
      }
      ++j;
    }

    // index is not used in the tessellator.
    this->TriangulateTriangle(cell, pts,ids,edgeIds,att,points,cellArray, internalPd);
  }
}

//-----------------------------------------------------------------------------
void vtkSimpleCellTessellator::Triangulate(vtkGenericAdaptorCell *cell,
                                           vtkGenericAttributeCollection *att,
                                           vtkDoubleArray *points,
                                           vtkCellArray *cellArray,
                                           vtkPointData *internalPd)
{
  assert("pre: cell_exists" && cell!=0);
  assert("pre: valid_dimension" && cell->GetDimension()==2);
  assert("pre: att_exists" && att!=0);
  assert("pre: points_exists" && points!=0);
  assert("pre: cellArray_exists" && cellArray!=0);
  assert("pre: internalPd_exists" && internalPd!=0);

  int j;

  if(cell->GetType()!=VTK_HIGHER_ORDER_TRIANGLE)
  {
     // build a linear polygon, call tessellate() on it and iterate over each triangle
    // by sending it to the tessellator

//    int *faceVerts=cell->GetFaceArray(index); // implicit
//    int numVerts=cell->GetNumberOfVerticesOnFace(index);
    int numVerts=cell->GetNumberOfBoundaries(0);

    this->Polygon->PointIds->SetNumberOfIds(numVerts);
    this->Polygon->Points->SetNumberOfPoints(numVerts);

    this->AllocatePointIds(cell->GetNumberOfBoundaries(0));
    cell->GetPointIds(this->PointIds);
    double *pcoords=cell->GetParametricCoords();

    int i=0;
    while(i<numVerts)
    {
      this->Polygon->PointIds->SetId(i,i); // this->PointIds[i]
      this->Polygon->Points->SetPoint(i, pcoords+3*i); // should be global?
      ++i;
    }

    this->Polygon->Triangulate(this->TriangleIds);

    // now iterate over any sub-triangle and call triangulateface on it
    vtkIdType pts[3];
    vtkIdType ids[3];
    int c=this->TriangleIds->GetNumberOfIds();
    i=0;
    while(i<c)
    {
      // Build the next sub-triangle
      j=0;
      while(j<3)
      {
        pts[j]=this->TriangleIds->GetId(i);
        // Get the point Ids (global)
        ids[j]=this->PointIds[pts[j]];
        ++j;
        ++i;
      }

      //
      // Get the edges Ids (local)
      int edgeIds[3];
      int *originalEdge;
      int edge[2];
      j=0;
      int numEdges=cell->GetNumberOfBoundaries(1);

      while(j<3)
      {
        edge[0]=pts[TRIANGLE_EDGES_TABLE[j][0]];
        edge[1]=pts[TRIANGLE_EDGES_TABLE[j][1]];
        int k=0;
        edgeIds[j]=-1;
        while(k<numEdges&&(edgeIds[j]==-1))
        {
          originalEdge=cell->GetEdgeArray(k);
          if((originalEdge[0]==edge[0]&&originalEdge[1]==edge[1])||
             (originalEdge[0]==edge[1]&&originalEdge[1]==edge[0]))
          {
            edgeIds[j]=k;
          }
          ++k;
        }
        ++j;
      }

      // index is not used in the tessellator.
      this->TriangulateTriangle(cell, pts,ids,edgeIds,att,points,cellArray, internalPd);
    }
  }
  else
  {
    vtkIdType pts[3]={0,1,2};
    int edgeIds[3]={0,1,2};
    this->AllocatePointIds(cell->GetNumberOfBoundaries(0));
    cell->GetPointIds(this->PointIds);
    this->TriangulateTriangle(cell, pts, this->PointIds, edgeIds, att,
                              points, cellArray,internalPd);

  }
}

//-----------------------------------------------------------------------------
void vtkSimpleCellTessellator::TriangulateTriangle(vtkGenericAdaptorCell *cell,
                                                   vtkIdType *localIds,
                                                   vtkIdType *ids,
                                                   int *edgeIds,
                                                   vtkGenericAttributeCollection *att,
                                                   vtkDoubleArray *points,
                                                   vtkCellArray *cellArray,
                                                   vtkPointData *internalPd)
{
  assert("pre: cell_exixts" && cell!=0);
  assert("pre: localIds_exists" && localIds!=0);
  assert("pre: ids_exists" && ids!=0);
  assert("pre: edgeIds_exists" && edgeIds!=0);

  // Save parameter for later use
  this->GenericCell = cell;

  this->TessellatePoints    = points;
  this->TessellateCellArray = cellArray;
  this->TessellatePointData = internalPd;

  this->AttributeCollection = att;

  if(this->CellIterator==0)
  {
    this->CellIterator = cell->NewCellIterator();
  }
  this->EdgeIds=edgeIds;

  this->SetGenericCell( cell );

  vtkTriangleTile root;
  double *point;

  int i;
  for(i=0; i<3; i++)
  {
    point = this->GenericCell->GetParametricCoords() + 3*localIds[i];
    root.SetVertex(i, point);
    root.SetPointId(i, ids[i]);
  }
  root.SetOriginal();

  // Init the edge table
  this->EdgeTable->SetNumberOfComponents(internalPd->GetNumberOfComponents());

  this->PointOffset = internalPd->GetNumberOfComponents() + 6;
  this->AllocateScalars(this->PointOffset*3);

  this->InsertPointsIntoEdgeTable( root );

  //Prepare the hash table with the top-level edges:
  this->InsertEdgesIntoEdgeTable( root );

  std::queue< vtkTriangleTile > work;
  vtkTriangleTile begin = vtkTriangleTile(root);
  work.push( begin );

  while( !work.empty() )
  {
    vtkTriangleTile piece[4];
    vtkTriangleTile curr = work.front();
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
  for(i = 0; i<3; i++)
  {
    this->EdgeTable->RemovePoint( root.GetPointId(i) );
  }

  //this->EdgeTable->LoadFactor();
  //this->EdgeTable->DumpTable();
}

//#define SLOW_API 1
//-----------------------------------------------------------------------------
// Return number of cells using edge #edgeId
int vtkSimpleCellTessellator::GetNumberOfCellsUsingEdge( int edgeId )
{
  assert("pre: valid_range" && edgeId>=0 ); // && edgeId<=5);
#if SLOW_API
  int result = 0;
  this->GenericCell->GetBoundaryIterator(this->CellIterator, 1);
  this->CellIterator->Begin();

  int i = 0;
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
  // The cell with the greatest number of edges is the hexagonal prism
  // 6*2+6
  int edgeSharing[18];
  this->GenericCell->CountEdgeNeighbors( edgeSharing );
  return edgeSharing[edgeId]+1;
#endif
}

//-----------------------------------------------------------------------------
// Return number of cells using face #faceId
int vtkSimpleCellTessellator::GetNumberOfCellsUsingFace( int faceId )
{
#if SLOW_API
  int result=0;
  this->GenericCell->GetBoundaryIterator(this->CellIterator, 2);
  this->CellIterator->Begin();

  int i = 0;
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
  if( this->GenericCell->IsFaceOnBoundary( faceId ) )
  {
    // So no other cell is using it:
    return 1;
  }

  //else this face is used by another cell
  return 2;
#endif
}

//-----------------------------------------------------------------------------
// Description:
// Allocate some memory if Scalars does not exists or is smaller than size.
// \pre positive_size: size>0
void vtkSimpleCellTessellator::AllocateScalars(int size)
{
  assert("pre: positive_size" && size > 0);

  if(this->Scalars == 0)
  {
    this->Scalars = new double[size];
    this->ScalarsCapacity = size;
  }
  else
  {
    if(this->ScalarsCapacity < size)
    {
      delete[] this->Scalars;
      this->Scalars = new double[size];
      this->ScalarsCapacity = size;
    }
  }
}


//-----------------------------------------------------------------------------
// Description:
// Return the number of fixed subdivisions. It is used to prevent from
// infinite loop in degenerated cases. For order 3 or higher, if the
// inflection point is exactly on the mid-point, error metric will not
// detect that a subdivision is required. 0 means no fixed subdivision:
// there will be only adaptive subdivisions.
//
// The algorithm first performs `GetFixedSubdivisions' non adaptive
// subdivisions followed by at most `GetMaxAdaptiveSubdivisions' adaptive
// subdivisions. Hence, there are at most `GetMaxSubdivisionLevel'
// subdivisions.
// \post positive_result: result>=0 && result<=GetMaxSubdivisionLevel()
int vtkSimpleCellTessellator::GetFixedSubdivisions()
{
  assert("post: positive_result" && this->FixedSubdivisions >= 0 && this->FixedSubdivisions <= this->MaxSubdivisionLevel);
  return this->FixedSubdivisions;
}

//-----------------------------------------------------------------------------
// Description:
// Return the maximum level of subdivision. It is used to prevent from
// infinite loop in degenerated cases. For order 3 or higher, if the
// inflection point is exactly on the mid-point, error metric will not
// detect that a subdivision is required. 0 means no subdivision,
// neither fixed nor adaptive.
// \post positive_result: result>=GetFixedSubdivisions()
int vtkSimpleCellTessellator::GetMaxSubdivisionLevel()
{
  assert("post: positive_result" && this->MaxSubdivisionLevel >= this->FixedSubdivisions);
  return this->MaxSubdivisionLevel;
}

//-----------------------------------------------------------------------------
// Description:
// Return the maximum number of adaptive subdivisions.
// \post valid_result: result==GetMaxSubdivisionLevel()-GetFixedSubdivisions()
int vtkSimpleCellTessellator::GetMaxAdaptiveSubdivisions()
{
  return this->MaxSubdivisionLevel - this->FixedSubdivisions;
}

//-----------------------------------------------------------------------------
// Description:
// Set the number of fixed subdivisions. See GetFixedSubdivisions() for
// more explanations.
// \pre positive_level: level>=0 && level<=GetMaxSubdivisionLevel()
// \post is_set: GetFixedSubdivisions()==level
void vtkSimpleCellTessellator::SetFixedSubdivisions(int level)
{
  assert("pre: positive_level" && level >= 0 && level <= this->GetMaxSubdivisionLevel());
  this->FixedSubdivisions = level;
}

//-----------------------------------------------------------------------------
// Description:
// Set the maximum level of subdivision. See GetMaxSubdivisionLevel() for
// more explanations.
// \pre positive_level: level>=GetFixedSubdivisions()
// \post is_set: level==GetMaxSubdivisionLevel()
void vtkSimpleCellTessellator::SetMaxSubdivisionLevel(int level)
{
  assert("pre: positive_level" && level >= this->GetFixedSubdivisions());
  this->MaxSubdivisionLevel = level;
}

//-----------------------------------------------------------------------------
// Description:
// Set both the number of fixed subdivisions and the maximum level of
// subdivisions. See GetFixedSubdivisions(), GetMaxSubdivisionLevel() and
// GetMaxAdaptiveSubdivisions() for more explanations.
// \pre positive_fixed: fixed>=0
// \pre valid_range: fixed<=maxLevel
// \post fixed_is_set: fixed==GetFixedSubdivisions()
// \post maxLevel_is_set: maxLevel==GetMaxSubdivisionLevel()
void vtkSimpleCellTessellator::SetSubdivisionLevels(int fixed,
                                                    int maxLevel)
{
  assert("pre: positive_fixed" && fixed >= 0);
  assert("pre: valid_range" && fixed <= maxLevel);
  this->FixedSubdivisions = fixed;
  this->MaxSubdivisionLevel = maxLevel;
}

//----------------------------------------------------------------------------
// Description:
// Allocate some memory if PointIds does not exist or is smaller than size.
// \pre positive_size: size>0
void vtkSimpleCellTessellator::AllocatePointIds(int size)
{
  assert("pre: positive_size" && size>0);

  if(this->PointIdsCapacity<size)
  {
    delete[] this->PointIds;
    this->PointIds=new vtkIdType[size];
    this->PointIdsCapacity=size;
  }
}

//----------------------------------------------------------------------------
// Description:
// Are the faces `originalFace' and `face' equal?
// The result is independent from any order or orientation.
// \pre originalFace_exists: originalFace!=0
int vtkSimpleCellTessellator::FacesAreEqual(int *originalFace,
                                            int face[3])
{
  assert("pre: originalFace_exists" && originalFace!=0);

  int result=0;
  int i=0;
  int j=1;
  int k=2;
  while(!result && i<3)
  {
    // counterclockwise
    result=originalFace[0]==face[i]
      && originalFace[1]==face[j]
      && originalFace[2]==face[k];
    // clockwise
    if(!result)
    {
      result=originalFace[0]==face[i]
        && originalFace[2]==face[j]
        && originalFace[1]==face[k];
    }
    ++i;
    ++j;
    ++k;

    if(j>2)
    {
      j=0;
    }
    else
    {
      if(k>2)
      {
        k=0;
      }
    }
  }
  return result;
}
