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

#include <vtkstd/queue>
#include <assert.h>

// format of the arrays LeftPoint, MidPoint, RightPoint is global, parametric,
// attributes: xyz rst [abc de...]
const int PARAMETRIC_OFFSET = 3;
const int ATTRIBUTES_OFFSET = 6;

// Pre computed table for the point to edge equivalence:
// [edge][point]
static int TRIANGLE_EDGES_TABLE[3][2] = {{0, 1}, {1, 2}, {2, 0}};

// Pre computed table for the tessellation of triangles
#define NO_TRIAN {-1,-1,-1}

// [case][triangle][vertex]
static signed char vtkTessellatorTriangleCases[9][4][3] = {
// Case no edge is split:  -> 0
{ NO_TRIAN, NO_TRIAN, NO_TRIAN, NO_TRIAN},
// Case edge 3 is split:  -> 1
{{0, 3, 2},{1, 2, 3}, NO_TRIAN, NO_TRIAN},
// Case edge 4 is split:  -> 2
{{0, 1, 4},{0, 4, 2}, NO_TRIAN, NO_TRIAN},
// Case edge 3 & 4 are split:  -> 3
{{0, 3, 2},{1, 4, 3},{3, 4, 2}, NO_TRIAN},
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
static int TETRA_EDGES_TABLE[6][2] = {
{0, 1}, {1, 2}, {2, 0}, {0, 3}, {1, 3}, {2, 3}
};

// Pre computed table for the tessellation of tetras
// There is two cases for the tessellation of a tetra, it is either oriented
// with the right hand rule or with the left hand rule
#define NO_TETRA {-1,-1,-1,-1}

// [case][tetra][vertex]
static signed char vtkTessellatorTetraCasesRight[65][8][4] = {
// Index = 0, Case were no edges are split
{{0,1,2,3}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 1, Case were edges: 4 are split
{{0,2,3,4},{1,2,4,3}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 2, Case were edges: 5 are split
{{0,1,5,3},{0,2,3,5}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 3, Case were edges: 4,5 are split
{{0,2,3,5},{0,3,4,5},{1,3,5,4}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 4, Case were edges: 6 are split
{{0,1,6,3},{1,2,6,3}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 5, Case were edges: 4,6 are split
{{0,3,4,6},{1,2,6,3},{1,3,6,4}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 6, Case were edges: 5,6 are split
{{0,1,5,3},{0,3,5,6},{2,3,6,5}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 7, Case were edges: 4,5,6 are split
{{0,3,4,6},{1,3,5,4},{2,3,6,5},{3,4,6,5}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 8, Case were edges: 7 are split
{{0,1,2,7},{1,2,7,3}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 9, Case were edges: 4,7 are split
{{0,2,7,4},{1,2,4,7},{1,2,7,3}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 10, Case were edges: 5,7 are split
{{0,1,5,7},{0,2,7,5},{1,3,5,7},{2,3,7,5}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 11, Case were edges: 4,5,7 are split
{{0,2,7,5},{0,4,5,7},{1,3,5,7},{1,4,7,5},{2,3,7,5}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 12, Case were edges: 6,7 are split
{{0,1,6,7},{1,2,6,7},{1,2,7,3}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 13, Case were edges: 4,6,7 are split
{{0,4,6,7},{1,2,6,7},{1,2,7,3},{1,4,7,6}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 14, Case were edges: 5,6,7 are split
{{0,1,5,7},{0,5,6,7},{1,3,5,7},{2,3,7,5},{2,5,7,6}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 15, Case were edges: 4,5,6,7 are split
{{0,4,6,7},{1,3,5,7},{1,4,7,5},{2,3,7,5},{2,5,7,6},{4,5,6,7}, NO_TETRA, NO_TETRA},
// Index = 16, Case were edges: 8 are split
{{0,1,2,8},{0,2,3,8}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 17, Case were edges: 4,8 are split
{{0,2,3,8},{0,2,8,4},{1,2,4,8}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 18, Case were edges: 5,8 are split
{{0,1,5,8},{0,2,3,8},{0,2,8,5}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 19, Case were edges: 4,5,8 are split
{{0,2,3,8},{0,2,8,5},{0,4,5,8},{1,4,8,5}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 20, Case were edges: 6,8 are split
{{0,1,6,8},{0,3,8,6},{1,2,6,8},{2,3,6,8}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 21, Case were edges: 4,6,8 are split
{{0,3,8,6},{0,4,6,8},{1,2,6,8},{1,4,8,6},{2,3,6,8}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 22, Case were edges: 5,6,8 are split
{{0,1,5,8},{0,3,8,6},{0,5,6,8},{2,3,6,8},{2,5,8,6}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 23, Case were edges: 4,5,6,8 are split
{{0,3,8,6},{0,4,6,8},{1,4,8,5},{2,3,6,8},{2,5,8,6},{4,5,6,8}, NO_TETRA, NO_TETRA},
// Index = 24, Case were edges: 7,8 are split
{{0,1,2,8},{0,2,7,8},{2,3,7,8}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 25, Case were edges: 4,7,8 are split
{{0,2,7,4},{1,2,4,8},{2,3,7,8},{2,4,8,7}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 26, Case were edges: 5,7,8 are split
{{0,1,5,8},{0,2,7,5},{0,5,7,8},{2,3,7,8},{2,5,8,7}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 27, Case were edges: 4,5,7,8 are split
{{0,2,7,5},{0,4,5,7},{1,4,8,5},{2,3,7,8},{2,5,8,7},{4,5,7,8}, NO_TETRA, NO_TETRA},
// Index = 28, Case were edges: 6,7,8 are split
{{0,1,6,8},{0,6,7,8},{1,2,6,8},{2,3,7,8},{2,6,8,7}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 29, Case were edges: 4,6,7,8 are split
{{0,4,6,7},{1,2,6,8},{1,4,8,6},{2,3,7,8},{2,6,8,7},{4,6,7,8}, NO_TETRA, NO_TETRA},
// Index = 30, Case were edges: 5,6,7,8 are split
{{0,1,5,8},{0,5,6,7},{0,5,7,8},{2,3,7,8},{2,5,7,6},{2,5,8,7}, NO_TETRA, NO_TETRA},
// Index = 31, Case were edges: 4,5,6,7,8 are split
{{0,4,6,7},{1,4,8,5},{2,3,7,8},{2,5,7,6},{2,5,8,7},{4,5,6,7},{4,5,7,8}, NO_TETRA},
// Index = 32, Case were edges: are split
{{0,1,2,9},{0,1,9,3}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 33, Case were edges: 4 are split
{{0,2,9,4},{0,3,4,9},{1,2,4,9},{1,3,9,4}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 34, Case were edges: 5 are split
{{0,1,5,9},{0,1,9,3},{0,2,9,5}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 35, Case were edges: 4,5 are split
{{0,2,9,5},{0,3,4,9},{0,4,5,9},{1,3,9,4},{1,4,9,5}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 36, Case were edges: 6 are split
{{0,1,6,9},{0,1,9,3},{1,2,6,9}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 37, Case were edges: 4,6 are split
{{0,3,4,9},{0,4,6,9},{1,2,6,9},{1,3,9,4},{1,4,9,6}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 38, Case were edges: 5,6 are split
{{0,1,5,9},{0,1,9,3},{0,5,6,9},{2,5,9,6}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 39, Case were edges: 4,5,6 are split
{{0,3,4,9},{0,4,6,9},{1,3,9,4},{1,4,9,5},{2,5,9,6},{4,5,6,9}, NO_TETRA, NO_TETRA},
// Index = 40, Case were edges: 7 are split
{{0,1,2,9},{0,1,9,7},{1,3,9,7}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 41, Case were edges: 4,7 are split
{{0,2,9,4},{0,4,9,7},{1,2,4,9},{1,3,9,7},{1,4,7,9}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 42, Case were edges: 5,7 are split
{{0,1,5,7},{0,2,9,5},{0,5,9,7},{1,3,9,7},{1,5,7,9}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 43, Case were edges: 4,5,7 are split
{{0,2,9,5},{0,4,5,7},{0,5,9,7},{1,3,9,7},{1,4,7,5},{1,5,7,9}, NO_TETRA, NO_TETRA},
// Index = 44, Case were edges: 6,7 are split
{{0,1,6,7},{1,2,6,9},{1,3,9,7},{1,6,7,9}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 45, Case were edges: 4,6,7 are split
{{0,4,6,7},{1,2,6,9},{1,3,9,7},{1,4,7,9},{1,4,9,6},{4,6,7,9}, NO_TETRA, NO_TETRA},
// Index = 46, Case were edges: 5,6,7 are split
{{0,1,5,7},{0,5,6,7},{1,3,9,7},{1,5,7,9},{2,5,9,6},{5,6,7,9}, NO_TETRA, NO_TETRA},
// Index = 47, Case were edges: 4,5,6,7 are split
{{0,4,6,7},{1,3,9,7},{1,4,7,5},{1,5,7,9},{2,5,9,6},{4,5,6,7},{5,6,7,9}, NO_TETRA},
// Index = 48, Case were edges: 8 are split
{{0,1,2,9},{0,1,9,8},{0,3,8,9}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 49, Case were edges: 4,8 are split
{{0,2,9,4},{0,3,8,9},{0,4,9,8},{1,2,4,9},{1,4,8,9}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 50, Case were edges: 5,8 are split
{{0,1,5,8},{0,2,9,5},{0,3,8,9},{0,5,9,8}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 51, Case were edges: 4,5,8 are split
{{0,2,9,5},{0,3,8,9},{0,4,5,9},{0,4,9,8},{1,4,8,5},{4,5,9,8}, NO_TETRA, NO_TETRA},
// Index = 52, Case were edges: 6,8 are split
{{0,1,6,8},{0,3,8,9},{0,6,9,8},{1,2,6,9},{1,6,8,9}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 53, Case were edges: 4,6,8 are split
{{0,3,8,9},{0,4,6,8},{0,6,9,8},{1,2,6,9},{1,4,8,6},{1,6,8,9}, NO_TETRA, NO_TETRA},
// Index = 54, Case were edges: 5,6,8 are split
{{0,1,5,8},{0,3,8,9},{0,5,6,8},{0,6,9,8},{2,5,9,6},{5,6,8,9}, NO_TETRA, NO_TETRA},
// Index = 55, Case were edges: 4,5,6,8 are split
{{0,3,8,9},{0,4,6,8},{0,6,9,8},{1,4,8,5},{2,5,9,6},{4,5,6,8},{5,6,8,9}, NO_TETRA},
// Index = 56, Case were edges: 7,8 are split
{{0,1,2,9},{0,1,9,8},{0,7,8,9},{3,7,9,8}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 57, Case were edges: 4,7,8 are split
{{0,2,9,4},{0,4,9,7},{1,2,4,9},{1,4,8,9},{3,7,9,8},{4,7,8,9}, NO_TETRA, NO_TETRA},
// Index = 58, Case were edges: 5,7,8 are split
{{0,1,5,8},{0,2,9,5},{0,5,7,8},{0,5,9,7},{3,7,9,8},{5,7,8,9}, NO_TETRA, NO_TETRA},
// Index = 59, Case were edges: 4,5,7,8 are split
{{0,2,9,5},{0,4,5,7},{0,5,9,7},{1,4,8,5},{3,7,9,8},{4,5,7,8},{5,7,8,9}, NO_TETRA},
// Index = 60, Case were edges: 6,7,8 are split
{{0,1,6,8},{0,6,7,8},{1,2,6,9},{1,6,8,9},{3,7,9,8},{6,7,8,9}, NO_TETRA, NO_TETRA},
// Index = 61, Case were edges: 4,6,7,8 are split
{{0,4,6,7},{1,2,6,9},{1,4,8,6},{1,6,8,9},{3,7,9,8},{4,6,7,8},{6,7,8,9}, NO_TETRA},
// Index = 62, Case were edges: 5,6,7,8 are split
{{0,1,5,8},{0,5,6,7},{0,5,7,8},{2,5,9,6},{3,7,9,8},{5,6,7,9},{5,7,8,9}, NO_TETRA},
// Index = 63, Case were edges: 4,5,6,7,8 are split
{{0,4,6,7},{1,4,8,5},{2,5,9,6},{3,7,9,8},{4,5,6,7},{4,5,7,8},{5,6,7,9},{5,7,8,9}},
//In case we reach outside the table
{ NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA}
};

//-----------------------------------------------------------------------------
//
// This table is for the case where the 'last edge' of the tetra could not be order
// properly, then we need a different case table
//
static signed char vtkTessellatorTetraCasesLeft[65][8][4] = {
// Index = 0, Case were no edges are split
{{0,1,2,3}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 1, Case were edges: 4 are split
{{0,2,3,4},{1,2,4,3}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 2, Case were edges: 5 are split
{{0,1,5,3},{0,2,3,5}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 3, Case were edges: 4,5 are split
{{0,2,3,5},{0,3,4,5},{1,3,5,4}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 4, Case were edges: 6 are split
{{0,1,6,3},{1,2,6,3}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 5, Case were edges: 4,6 are split
{{0,3,4,6},{1,2,6,3},{1,3,6,4}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 6, Case were edges: 5,6 are split
{{0,1,5,3},{0,3,5,6},{2,3,6,5}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 7, Case were edges: 4,5,6 are split
{{0,3,4,6},{1,3,5,4},{2,3,6,5},{3,4,6,5}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 8, Case were edges: 7 are split
{{0,1,2,7},{1,2,7,3}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 9, Case were edges: 4,7 are split
{{0,2,7,4},{1,2,4,7},{1,2,7,3}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 10, Case were edges: 5,7 are split
{{0,1,5,7},{0,2,7,5},{1,3,5,7},{2,3,7,5}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 11, Case were edges: 4,5,7 are split
{{0,2,7,5},{0,4,5,7},{1,3,5,7},{1,4,7,5},{2,3,7,5}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 12, Case were edges: 6,7 are split
{{0,1,6,7},{1,2,6,3},{1,3,6,7}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 13, Case were edges: 4,6,7 are split
{{0,4,6,7},{1,2,6,3},{1,3,6,7},{1,4,7,6}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 14, Case were edges: 5,6,7 are split
{{0,1,5,7},{0,5,6,7},{1,3,5,7},{2,3,6,5},{3,5,7,6}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 15, Case were edges: 4,5,6,7 are split
{{0,4,6,7},{1,3,5,7},{1,4,7,5},{2,3,6,5},{3,5,7,6},{4,5,6,7}, NO_TETRA, NO_TETRA},
// Index = 16, Case were edges: 8 are split
{{0,1,2,8},{0,2,3,8}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 17, Case were edges: 4,8 are split
{{0,2,3,8},{0,2,8,4},{1,2,4,8}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 18, Case were edges: 5,8 are split
{{0,1,5,8},{0,2,3,5},{0,3,8,5}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 19, Case were edges: 4,5,8 are split
{{0,2,3,5},{0,3,8,5},{0,4,5,8},{1,4,8,5}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 20, Case were edges: 6,8 are split
{{0,1,6,8},{0,3,8,6},{1,2,6,8},{2,3,6,8}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 21, Case were edges: 4,6,8 are split
{{0,3,8,6},{0,4,6,8},{1,2,6,8},{1,4,8,6},{2,3,6,8}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 22, Case were edges: 5,6,8 are split
{{0,1,5,8},{0,3,8,6},{0,5,6,8},{2,3,6,5},{3,5,8,6}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 23, Case were edges: 4,5,6,8 are split
{{0,3,8,6},{0,4,6,8},{1,4,8,5},{2,3,6,5},{3,5,8,6},{4,5,6,8}, NO_TETRA, NO_TETRA},
// Index = 24, Case were edges: 7,8 are split
{{0,1,2,8},{0,2,7,8},{2,3,7,8}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 25, Case were edges: 4,7,8 are split
{{0,2,7,4},{1,2,4,8},{2,3,7,8},{2,4,8,7}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 26, Case were edges: 5,7,8 are split
{{0,1,5,8},{0,2,7,5},{0,5,7,8},{2,3,7,5},{3,5,8,7}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 27, Case were edges: 4,5,7,8 are split
{{0,2,7,5},{0,4,5,7},{1,4,8,5},{2,3,7,5},{3,5,8,7},{4,5,7,8}, NO_TETRA, NO_TETRA},
// Index = 28, Case were edges: 6,7,8 are split
{{0,1,6,8},{0,6,7,8},{1,2,6,8},{2,3,6,8},{3,6,8,7}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 29, Case were edges: 4,6,7,8 are split
{{0,4,6,7},{1,2,6,8},{1,4,8,6},{2,3,6,8},{3,6,8,7},{4,6,7,8}, NO_TETRA, NO_TETRA},
// Index = 30, Case were edges: 5,6,7,8 are split
{{0,1,5,8},{0,5,6,7},{0,5,7,8},{2,3,6,5},{3,5,7,6},{3,5,8,7}, NO_TETRA, NO_TETRA},
// Index = 31, Case were edges: 4,5,6,7,8 are split
{{0,4,6,7},{1,4,8,5},{2,3,6,5},{3,5,7,6},{3,5,8,7},{4,5,6,7},{4,5,7,8}, NO_TETRA},
// Index = 32, Case were edges: are split
{{0,1,2,9},{0,1,9,3}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 33, Case were edges: 4 are split
{{0,2,9,4},{0,3,4,9},{1,2,4,9},{1,3,9,4}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 34, Case were edges: 5 are split
{{0,1,5,9},{0,1,9,3},{0,2,9,5}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 35, Case were edges: 4,5 are split
{{0,2,9,5},{0,3,4,9},{0,4,5,9},{1,3,9,4},{1,4,9,5}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 36, Case were edges: 6 are split
{{0,1,6,9},{0,1,9,3},{1,2,6,9}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 37, Case were edges: 4,6 are split
{{0,3,4,9},{0,4,6,9},{1,2,6,9},{1,3,9,4},{1,4,9,6}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 38, Case were edges: 5,6 are split
{{0,1,5,9},{0,1,9,3},{0,5,6,9},{2,5,9,6}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 39, Case were edges: 4,5,6 are split
{{0,3,4,9},{0,4,6,9},{1,3,9,4},{1,4,9,5},{2,5,9,6},{4,5,6,9}, NO_TETRA, NO_TETRA},
// Index = 40, Case were edges: 7 are split
{{0,1,2,9},{0,1,9,7},{1,3,9,7}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 41, Case were edges: 4,7 are split
{{0,2,9,4},{0,4,9,7},{1,2,4,9},{1,3,9,7},{1,4,7,9}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 42, Case were edges: 5,7 are split
{{0,1,5,7},{0,2,9,5},{0,5,9,7},{1,3,9,7},{1,5,7,9}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 43, Case were edges: 4,5,7 are split
{{0,2,9,5},{0,4,5,7},{0,5,9,7},{1,3,9,7},{1,4,7,5},{1,5,7,9}, NO_TETRA, NO_TETRA},
// Index = 44, Case were edges: 6,7 are split
{{0,1,6,7},{1,2,6,9},{1,3,9,7},{1,6,7,9}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 45, Case were edges: 4,6,7 are split
{{0,4,6,7},{1,2,6,9},{1,3,9,7},{1,4,7,9},{1,4,9,6},{4,6,7,9}, NO_TETRA, NO_TETRA},
// Index = 46, Case were edges: 5,6,7 are split
{{0,1,5,7},{0,5,6,7},{1,3,9,7},{1,5,7,9},{2,5,9,6},{5,6,7,9}, NO_TETRA, NO_TETRA},
// Index = 47, Case were edges: 4,5,6,7 are split
{{0,4,6,7},{1,3,9,7},{1,4,7,5},{1,5,7,9},{2,5,9,6},{4,5,6,7},{5,6,7,9}, NO_TETRA},
// Index = 48, Case were edges: 8 are split
{{0,1,2,9},{0,1,9,8},{0,3,8,9}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 49, Case were edges: 4,8 are split
{{0,2,9,4},{0,3,8,9},{0,4,9,8},{1,2,4,9},{1,4,8,9}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 50, Case were edges: 5,8 are split
{{0,1,5,8},{0,2,9,5},{0,3,8,9},{0,5,9,8}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 51, Case were edges: 4,5,8 are split
{{0,2,9,5},{0,3,8,9},{0,4,5,9},{0,4,9,8},{1,4,8,5},{4,5,9,8}, NO_TETRA, NO_TETRA},
// Index = 52, Case were edges: 6,8 are split
{{0,1,6,8},{0,3,8,9},{0,6,9,8},{1,2,6,9},{1,6,8,9}, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 53, Case were edges: 4,6,8 are split
{{0,3,8,9},{0,4,6,8},{0,6,9,8},{1,2,6,9},{1,4,8,6},{1,6,8,9}, NO_TETRA, NO_TETRA},
// Index = 54, Case were edges: 5,6,8 are split
{{0,1,5,8},{0,3,8,9},{0,5,6,8},{0,6,9,8},{2,5,9,6},{5,6,8,9}, NO_TETRA, NO_TETRA},
// Index = 55, Case were edges: 4,5,6,8 are split
{{0,3,8,9},{0,4,6,8},{0,6,9,8},{1,4,8,5},{2,5,9,6},{4,5,6,8},{5,6,8,9}, NO_TETRA},
// Index = 56, Case were edges: 7,8 are split
{{0,1,2,9},{0,1,9,8},{0,7,8,9},{3,7,9,8}, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
// Index = 57, Case were edges: 4,7,8 are split
{{0,2,9,4},{0,4,9,7},{1,2,4,9},{1,4,8,9},{3,7,9,8},{4,7,8,9}, NO_TETRA, NO_TETRA},
// Index = 58, Case were edges: 5,7,8 are split
{{0,1,5,8},{0,2,9,5},{0,5,7,8},{0,5,9,7},{3,7,9,8},{5,7,8,9}, NO_TETRA, NO_TETRA},
// Index = 59, Case were edges: 4,5,7,8 are split
{{0,2,9,5},{0,4,5,7},{0,5,9,7},{1,4,8,5},{3,7,9,8},{4,5,7,8},{5,7,8,9}, NO_TETRA},
// Index = 60, Case were edges: 6,7,8 are split
{{0,1,6,8},{0,6,7,8},{1,2,6,9},{1,6,8,9},{3,7,9,8},{6,7,8,9}, NO_TETRA, NO_TETRA},
// Index = 61, Case were edges: 4,6,7,8 are split
{{0,4,6,7},{1,2,6,9},{1,4,8,6},{1,6,8,9},{3,7,9,8},{4,6,7,8},{6,7,8,9}, NO_TETRA},
// Index = 62, Case were edges: 5,6,7,8 are split
{{0,1,5,8},{0,5,6,7},{0,5,7,8},{2,5,9,6},{3,7,9,8},{5,6,7,9},{5,7,8,9}, NO_TETRA},
// Index = 63, Case were edges: 4,5,6,7,8 are split
{{0,4,6,7},{1,4,8,5},{2,5,9,6},{3,7,9,8},{4,5,6,7},{4,5,7,8},{5,6,7,9},{5,7,8,9}},
//In case we reach outside the table
{ NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA, NO_TETRA},
};


vtkCxxRevisionMacro(vtkSimpleCellTessellator, "1.12");
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
  ~vtkTriangleTile() {};
  
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

  // can tile be split; if so, return TessellatePointsing tiles
  //  vtkTriangleTile res[4]
  int Refine( vtkSimpleCellTessellator* tess, vtkTriangleTile *res );

private:
  // Keep track of local coordinate in order to evaluate shape function
  double Vertex[3+3][3];  //3 points + 3 mid edge points
  vtkIdType PointId[3+3];
  int SubdivisionLevel;
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
  ~vtkTetraTile() {};
    
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

  // can tile be split; if so, return TessellatePointsing tiles
  // There can't be more than 8 tetras as it corresponds to the splitting
  // of all edges 
  // vtkTetraTile res[8]
  int Refine( vtkSimpleCellTessellator* tess, vtkTetraTile *res);

  // Description:
  // Return if the four first points of the tetra are all differents
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
  
private:
  // Need to keep track of local coordinate to evaluate shape functions
  // So all work is done in parametric coordinate

  double Vertex[4+6][3];  // 4 tetra points + 6 mid edge points
  vtkIdType PointId[4+6];
  int SubdivisionLevel;
};

//-----------------------------------------------------------------------------
int vtkTriangleTile::Refine(vtkSimpleCellTessellator* tess,
                            vtkTriangleTile *res ) // res[4]
{
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
          res[numTriangleCreated].SetPointId( j, this->PointId[cases[j]] );
          res[numTriangleCreated].SetVertex( j, this->Vertex[cases[j]] );
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
int vtkTetraTile::Refine( vtkSimpleCellTessellator* tess,
                          vtkTetraTile *res) // res[8]
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

  numTetraCreated = 0;
  
  if(this->SubdivisionLevel < tess->GetMaxSubdivisionLevel())
    {
    // loop over edges:
    for(i=0, index = 0;i<6;i++)
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
}

//-----------------------------------------------------------------------------
vtkSimpleCellTessellator::~vtkSimpleCellTessellator()
{
  this->EdgeTable->Delete();
  if(this->CellIterator)
    {
    this->CellIterator->Delete();
    }
  if(this->Scalars)
    {
    delete[] this->Scalars;
    }
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

      //Put everything in ths point hash table
      this->EdgeTable->InsertPointAndScalar(tri.GetPointId(j), global,
                                            this->Scalars);
      }
    }
}

//-----------------------------------------------------------------------------
// This function is supposed to be called only at toplevel (for passing data
// from third party to the hash point table)
void vtkSimpleCellTessellator::InsertPointsIntoEdgeTable(vtkTetraTile &tetra )
{
  double global[3];
 
  for(int j=0; j<4; j++)
    {
    // Need to check first if point is not already in the hash table
    // since EvaluateLocation / EvaluateTuple are expensive calls
    if( !this->EdgeTable->CheckPoint(tetra.GetPointId(j)) )
      {
      // its real space coordinate:
      this->GenericCell->EvaluateLocation(0,tetra.GetVertex(j), global);

      // Then scalar value associated with point:  
      this->GenericCell->InterpolateTuple(this->AttributeCollection,
                                          tetra.GetVertex(j), this->Scalars);

      //Put everything in the point hash table
      this->EdgeTable->InsertPointAndScalar(tetra.GetPointId(j), global,
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
      int localId;
      //int type = this->GenericCell->FindEdgeParent(left, right, localId);
      int type = FindEdgeParent2D(left, right, localId);

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

        // This means inside, this case should not happen
        // This only for consistency with the tetra version
        refCount = 1;
        }
      else if( type == 3)
        {
        // Inside:
        refCount = 1;
        }
      doSubdivision = tri.GetSubdivisionLevel() < this->GetMaxSubdivisionLevel();
      if(doSubdivision)
        {
        // global position and attributes at the left vertex
        this->EdgeTable->CheckPoint(leftId,leftPoint,
                                    leftPoint+ATTRIBUTES_OFFSET);
        // global position and attributes at the right vertex
        this->EdgeTable->CheckPoint(rightId,rightPoint,
                                    rightPoint+ATTRIBUTES_OFFSET);

        // parametric center of the edge
        local = midPoint+PARAMETRIC_OFFSET;
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
            doSubdivision = this->NeedEdgeSubdivision(leftPoint,midPoint,rightPoint,
                                                      alpha);
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

        //Put everything in ths point hash table
        this->EdgeTable->InsertPointAndScalar(ptId, midPoint,
                                              midPoint+ATTRIBUTES_OFFSET);
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
      doSubdivision = tetra.GetSubdivisionLevel() < this->GetMaxSubdivisionLevel();
      if(doSubdivision)
        {
        // global position and attributes at the left vertex
        this->EdgeTable->CheckPoint(leftId,leftPoint,
                                    leftPoint+ATTRIBUTES_OFFSET);
        // global position and attributes at the right vertex
        this->EdgeTable->CheckPoint(rightId,rightPoint,
                                    rightPoint+ATTRIBUTES_OFFSET);

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
            doSubdivision = this->NeedEdgeSubdivision(leftPoint,midPoint,rightPoint,
                                                      alpha);
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

        //Put everything in the point hash table
        this->EdgeTable->InsertPointAndScalar(ptId, midPoint,
                                              midPoint+ATTRIBUTES_OFFSET);
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
  for(i = 0; i<3; i++)
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
  for(i = 0; i<4; i++)
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
void vtkSimpleCellTessellator::InternalTessellateTriangle(vtkTriangleTile& root )
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
// Description:
// Return the internal edge table.
vtkGenericEdgeTable *vtkSimpleCellTessellator::GetEdgeTable()
{
  return this->EdgeTable;
}

//-----------------------------------------------------------------------------
void vtkSimpleCellTessellator::Tessellate(vtkGenericAdaptorCell *cell,
                                          vtkGenericAttributeCollection *att,
                                          vtkDoubleArray *points, 
                                          vtkCellArray *cellArray,
                                          vtkPointData *internalPd )
{
  int i;
 
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
#if 1
  // FIXME
  this->SetGenericCell( cell );
//  this->ErrorMetric->SetAttributeCollection( att );
#endif
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

  // Init the edge table
  
  this->EdgeTable->SetNumberOfComponents(internalPd->GetNumberOfComponents());
 
  this->PointOffset = internalPd->GetNumberOfComponents()+6;
  this->AllocateScalars(this->PointOffset*3);
 
  // Pass data to hash table:
  // FIXME some point are already in the hash table we shouldn't try
  // to calculate their associate point/scalar value then.
  this->InsertPointsIntoEdgeTable( root );

  //Prepare the hash table with the top-level edges:
  this->InsertEdgesIntoEdgeTable( root );

  //Start of the algorithm use a queue for now:
  vtkstd::queue<vtkTetraTile> work;
  work.push( root );
 
  vtkIdType count=0;
  while( !work.empty() ) 
    {
    vtkTetraTile piece[8];
    vtkTetraTile curr = work.front();
    work.pop();

    int n = curr.Refine( this, piece);

    for(i = 0; i<n; i++) 
      {
      work.push( piece[i] );
      }

    // We are done we should clean ourself from the hash table:
    this->RemoveEdgesFromEdgeTable( curr );
    count++;
    }

  // remove top level points
  for(i = 0; i<4; i++)
    {
    this->EdgeTable->RemovePoint( root.GetPointId(i) );
    }
  
//  this->EdgeTable->LoadFactor();
//  this->EdgeTable->DumpTable();

  // Okay we are done with this cell, dump the hash-table to check it is clean:
  // by clean I mean it only contains points on face/edge:
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
vtkSimpleCellTessellator::TessellateTriangleFace(vtkGenericAdaptorCell *cell, 
                                                  vtkGenericAttributeCollection *att,
                                                  vtkIdType index,
                                                  vtkDoubleArray *points,
                                                  vtkCellArray *cellArray,
                                                  vtkPointData *internalPd)
{
  assert("pre: cell_exixts" && cell!=0);
  assert("pre: valid_cell_type" && ((cell->GetType() == VTK_TETRA)
                                  ||(cell->GetType() == VTK_QUADRATIC_TETRA
                                  ||(cell->GetType() == VTK_HIGHER_ORDER_TETRAHEDRON))));
  assert("pre: valid_range_index" && index>=0 && index<=3 );

  int i=0;
  // Save parameter for later use
  this->TessellateCellArray = cellArray;
  this->TessellatePoints    = points;
  this->TessellatePointData = internalPd;

//  this->Reset(); // reset point cell and scalars

  this->GenericCell = cell;
  this->AttributeCollection = att;
  if(this->CellIterator==0)
    {
    this->CellIterator = cell->NewCellIterator();
    }
#if 1 
  // FIXME
  this->SetGenericCell( cell );
//  this->ErrorMetric->SetAttributeCollection( att );
#endif
  // Based on the id of the face we can find the points of the triangle:
  // When triangle found just tessellate it.
  vtkTriangleTile root;
  double *point;

  vtkIdType tetra[4];
  this->GenericCell->GetPointIds(tetra);
  int *indexTab = cell->GetFaceArray(index);

  for(i=0; i<3; i++)
    {
    point = this->GenericCell->GetParametricCoords() + 3*indexTab[i];
    root.SetVertex(i, point);

    root.SetPointId(i, tetra[indexTab[i]]);

    }

   // Init the edge table
  this->EdgeTable->SetNumberOfComponents(internalPd->GetNumberOfComponents());

  this->PointOffset = internalPd->GetNumberOfComponents() + 6;
  this->AllocateScalars(this->PointOffset*3);

  this->InsertPointsIntoEdgeTable( root );
  this->InsertEdgesIntoEdgeTable( root );
  this->InternalTessellateTriangle( root );
}

//-----------------------------------------------------------------------------
void vtkSimpleCellTessellator::Triangulate(vtkGenericAdaptorCell *cell, 
                                           vtkGenericAttributeCollection *att,
                                           vtkDoubleArray *points,
                                           vtkCellArray *cellArray, 
                                           vtkPointData *internalPd)
{
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
#if 1
  // FIXME
  this->SetGenericCell( cell );
//  this->ErrorMetric->SetAttributeCollection( att );
#endif
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

  // Init the edge table
  this->EdgeTable->SetNumberOfComponents(internalPd->GetNumberOfComponents());

  this->PointOffset = internalPd->GetNumberOfComponents() + 6;
  this->AllocateScalars(this->PointOffset*3);

  this->InsertPointsIntoEdgeTable( root );

  //Prepare the hash table with the top-level edges:
  this->InsertEdgesIntoEdgeTable( root );
  this->InternalTessellateTriangle( root );
}

//-----------------------------------------------------------------------------
// Is the edge defined by vertices (`p1',`p2') in parametric coordinates on
// some edge of the original triangle? If yes return on which edge it is,
// else return -1.
// \pre p1!=p2
// \pre p1 and p2 are in bounding box (0,0,0) (1,1,1)
// \post valid_result: (result==-1) || ( result>=0 && result<=2 )
int IsEdgeOnEdgeOfTriangle(double p1[3], double p2[3])
{
  assert("pre: points_differ" && (p1!=p2) && ((p1[0]!=p2[0]) || (p1[1]!=p2[1]) || (p1[2]!=p2[2])));

  assert("pre: p1_in_bounding_box" && p1[0]>=0 && p1[0]<=1 && p1[1]>=0 && p1[1]<=1 && p1[2]>=0 && p1[2]<=1 );
  assert("pre: p2_in_bounding_box" && p2[0]>=0 && p2[0]<=1 && p2[1]>=0 && p2[1]<=1 && p2[2]>=0 && p2[2]<=1 );
 
  // In fact, this test only check if the point are on an infinite line.

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
  return -1;
}

//-----------------------------------------------------------------------------
// Is the edge defined by vertices (`p1',`p2') in parametric coordinates on
// some edge of the original tetrahedron? If yes return on which edge it is,
// else return -1.
// \pre p1!=p2
// \pre p1 and p2 are in bounding box (0,0,0) (1,1,1)
// \post valid_result: (result==-1) || ( result>=0 && result<=5 )
int IsEdgeOnEdge(double p1[3], double p2[3])
{
  assert("pre: points_differ" && (p1!=p2) && ((p1[0]!=p2[0]) || (p1[1]!=p2[1]) || (p1[2]!=p2[2])));

  assert("pre: p1_in_bounding_box" && p1[0]>=0 && p1[0]<=1 && p1[1]>=0 && p1[1]<=1 && p1[2]>=0 && p1[2]<=1 );
  assert("pre: p2_in_bounding_box" && p2[0]>=0 && p2[0]<=1 && p2[1]>=0 && p2[1]<=1 && p2[2]>=0 && p2[2]<=1 );
 
  // In fact, this test only check if the point are on an infinite line.

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
// Is the edge defined by vertices (`p1',`p2') in parametric coordinates on
// some face of the original tetrahedron? If yes return on which face it is,
// else return -1.
// \pre p1!=p2
// \pre p1 and p2 are in bounding box (0,0,0) (1,1,1)
// \post valid_result: (result==-1) || ( result>=0 && result<=3 )
int vtkSimpleCellTessellator::IsEdgeOnFace(double p1[3], double p2[3])
{
  assert("pre: points_differ" && (p1!=p2) && ((p1[0]!=p2[0]) || (p1[1]!=p2[1]) || (p1[2]!=p2[2])));

  assert("pre: p1_in_bounding_box" && p1[0]>=0 && p1[0]<=1 && p1[1]>=0 && p1[1]<=1 && p1[2]>=0 && p1[2]<=1 );
  assert("pre: p2_in_bounding_box" && p2[0]>=0 && p2[0]<=1 && p2[1]>=0 && p2[1]<=1 && p2[2]>=0 && p2[2]<=1 );

  int result;
  int *indexTab;

  int face; // sum of the vertex id (trick to encode the face number).

   //Test on face 0:  (012)
  if(p1[2] == p2[2] && p2[2] == 0.0)
    {
    face = 3; // 0
    }
  else
    {
    //Test on face 1: (013)
    if(p1[1] == p2[1] && p2[1] == 0.0)
      {
      face = 4; // 1
      }
    else
      {
      //Test on face 2: (123)
      if((p1[0] + p1[1] + p1[2] == 1.0) && (p2[0] + p2[1] + p2[2] == 1.0))
        {
        face = 6; // 2
        }
      else
        {
        //Test on face 3: (023)
        if(p1[0] == p2[0] && p2[0] == 0.0)
          {
          face = 5; // 3;
          }
        else
          {
          face = -1;
          }
        }
      }
    }
  if(face != -1)
    {
    result = 0; // from this point, face is used as an end condition
    while( face != -1 && result < 4 )
      {
      indexTab = this->GenericCell->GetFaceArray(result);
      if(face != (indexTab[0] + indexTab[1] + indexTab[2]))
        {
        ++result;
        }
      else
        {
        face = -1;
        }
      }
    }
  else
    {
    result = -1;
    }
  assert("post: valid_result" && (result==-1) || ( result>=0 && result<=3 ));
  return result;
}

//-----------------------------------------------------------------------------
// Return 1 if the parent of edge defined by vertices (`p1',`p2') in parametric
// coordinates, is an edge; 3 if there is no parent (the edge is inside).
// If the parent is an edge, return its id in `localId'.
// \pre p1!=p2
// \pre p1 and p2 are in bounding box (0,0,0) (1,1,1)
// \post valid_result: (result==1)||(result==3)
int vtkSimpleCellTessellator::FindEdgeParent2D(double p1[3], double p2[3],
                                               int &localId)
{
  assert("pre: points_differ" && (p1!=p2) && ((p1[0]!=p2[0]) || (p1[1]!=p2[1]) || (p1[2]!=p2[2])));

  assert("pre: p1_in_bounding_box" && p1[0]>=0 && p1[0]<=1 && p1[1]>=0 && p1[1]<=1 && p1[2]>=0 && p1[2]<=1 );
  assert("pre: p2_in_bounding_box" && p2[0]>=0 && p2[0]<=1 && p2[1]>=0 && p2[1]<=1 && p2[2]>=0 && p2[2]<=1 );

  int result;

  if( ( localId = IsEdgeOnEdgeOfTriangle(p1, p2) ) != -1)
    {
    // On Edge
    // edge (p1,p2) is on the parent edge #id
    result = 1;
    }
  else
    {
    // Inside
    result = 3;
    }
  assert("post: valid_result" && ((result==1)||(result==3)));
  return result;
}

//-----------------------------------------------------------------------------
// Return 1 if the parent of edge defined by vertices (`p1',`p2') in parametric
// coordinates, is an edge; 2 if the parent is a face, 3 if there is no parent
// (the edge is inside). If the parent is an edge or a face, return its id in
// `localId'.
// \pre p1!=p2
// \pre p1 and p2 are in bounding box (0,0,0) (1,1,1)
// \post valid_result: result>=1 && result<=3
int vtkSimpleCellTessellator::FindEdgeParent(double p1[3], double p2[3],
                                             int &localId)
{
  assert("pre: points_differ1" && (p1!=p2));
  assert("pre: points_differ2" && ((p1[0]!=p2[0]) || (p1[1]!=p2[1]) || (p1[2]!=p2[2])));
  assert("pre: p1_in_bounding_box" && p1[0]>=0 && p1[0]<=1 && p1[1]>=0 && p1[1]<=1 && p1[2]>=0 && p1[2]<=1 );
  assert("pre: p2_in_bounding_box" && p2[0]>=0 && p2[0]<=1 && p2[1]>=0 && p2[1]<=1 && p2[2]>=0 && p2[2]<=1 );

  int result;

  if( ( localId = IsEdgeOnEdge(p1, p2) ) != -1)
    {
    // On Edge
    // edge (p1,p2) is on the parent edge #id
    result = 1;
    }
  else if( ( localId = IsEdgeOnFace(p1, p2) ) != -1)
    {
    // On Face
    // edge (p1,p2) is on the parent face #id
    result = 2;
    }
  else
    {
    // Inside
    result = 3;
    }
  assert("post: valid_result" && result>=1 && result<=3);
  return result;
}

//#define SLOW_API 1
//-----------------------------------------------------------------------------
// Return number of cells using edge #edgeId
int vtkSimpleCellTessellator::GetNumberOfCellsUsingEdge( int edgeId )
{
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
  int edgeSharing[6];
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

