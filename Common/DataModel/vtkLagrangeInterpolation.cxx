/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLagrangeInterpolation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
#include "vtkLagrangeInterpolation.h"

#include "vtkLagrangeTriangle.h"
#include "vtkLagrangeWedge.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkVector.h"
#include "vtkDoubleArray.h"
#include "vtkVectorOperators.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkLagrangeInterpolation);

// -----------------------------------------------------------------------------
static const double hexCorner[8][3] = {
  {  0.,  0.,  0. },
  { +1.,  0.,  0. },
  { +1., +1.,  0. },
  {  0., +1.,  0. },
  {  0.,  0., +1. },
  { +1.,  0., +1. },
  { +1., +1., +1. },
  {  0., +1., +1. }
};

// Edges and faces are always oriented along quad/hexahedron axes,
// not any "cell-local" direction (i.e., faces do not all
// have inward-pointing normals).
static const int hexEdgeCorners[12][5] = {
  // e0 e1    varying-  fixed- parametric coordinate(s)
  { 0, 1,   0,        1, 2 },
  { 1, 2,   1,        0, 2 },
  { 3, 2,   0,        1, 2 },
  { 0, 3,   1,        0, 2 },
  { 4, 5,   0,        1, 2 },
  { 5, 6,   1,        0, 2 },
  { 7, 6,   0,        1, 2 },
  { 4, 7,   1,        0, 2 },
  { 0, 4,   2,        0, 1 },
  { 1, 5,   2,        0, 1 },
  { 3, 7,   2,        0, 1 },
  { 2, 6,   2,        0, 1 }
};

static const int hexFaceCorners[6][7] = {
  // c0 c1 c2 c3    varying- fixed-parametric coordinate(s)
  { 0, 3, 7, 4,   1, 2,    0 },
  { 1, 2, 6, 5,   1, 2,    0 },
  { 0, 1, 5, 4,   0, 2,    1 },
  { 3, 2, 6, 7,   0, 2,    1 },
  { 0, 1, 2, 3,   0, 1,    2 },
  { 4, 5, 6, 7,   0, 1,    2 },
};

static const int hexFaceEdges[6][4] = {
  // e0  e1  e2  e3
  {  3, 10,  7,  8 },
  {  1, 11,  5,  9 },
  {  0,  9,  4,  8 },
  {  2, 11,  6, 10 },
  {  0,  1,  2,  3 },
  {  4,  5,  6,  7 },
};
// -----------------------------------------------------------------------------
static const double wedgeCorner[6][3] = {
  {  0.,  0.,  0. },
  { +1.,  0.,  0. },
  {  0., +1.,  0. },
  {  0.,  0., +1. },
  { +1.,  0., +1. },
  {  0., +1., +1. }
};

// Edges and faces are always oriented along quad/hexahedron axes,
// not any "cell-local" direction (i.e., faces do not all
// have inward-pointing normals).
static const int wedgeEdgeCorners[9][5] = {
  // e0 e1    varying-  fixed- parametric coordinate(s)
  { 0, 1,   0,        1, 2 },
  { 1, 2,  -1,       -1, 2 },
  { 2, 0,   1,        0, 2 },

  { 3, 4,   0,        1, 2 },
  { 4, 5,  -1,       -1, 2 },
  { 5, 3,   1,        0, 2 },

  { 0, 3,   2,        0, 1 },
  { 1, 4,   2,        0, 1 },
  { 2, 5,   2,        0, 1 }
};

static const int wedgeFaceCorners[5][9] = {
  // c0  c1  c2  c3   varying-  fixed-param. coordinate(s)  orientation (0 is negative, 1 is positive)  fixed-param. value (-1=lo, +1=hi)
  { 0,  1,  2, -1,    0,  1,     2,                         0,                                         -1 },
  { 3,  4,  5, -1,    0,  1,     2,                         1,                                         +1 },

  { 0,  1,  4,  3,    0,  2,     1,                         1,                                         -1 },
  { 1,  2,  5,  4,   -1,  2,    -1,                         1,                                         -1 },
  { 0,  2,  5,  3,    1,  2,     0,                         0,                                         -1 },
};

static const int wedgeFaceEdges[5][5] = {
  // e0  e1  e2  e3    orientation (<- 1 when implied normal points in, not out)
  {  0,  1,  2, -1,   0 },
  {  3,  4,  5, -1,   1 },

  {  0,  7,  3,  6,   0 },
  {  1,  8,  4,  7,   0 },
  {  2,  8,  5,  6,   0 },
};
// -----------------------------------------------------------------------------

vtkLagrangeInterpolation::vtkLagrangeInterpolation()
{
  int maxOrder[3] = {
    MaxDegree,
    MaxDegree,
    MaxDegree
  };
  vtkLagrangeInterpolation::PrepareForOrder(maxOrder, 0);
}

vtkLagrangeInterpolation::~vtkLagrangeInterpolation() = default;

void vtkLagrangeInterpolation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

/// Evaluate 1-D shape functions for the given \a order at the given \a pcoord (in [0,1]).
void vtkLagrangeInterpolation::EvaluateShapeFunctions(int order, double pcoord, double* shape)
{
  int j, k;
  double v = order * pcoord;
  for (j = 0; j <= order; ++j)
  {
    shape[j] = 1.;
    for (k = 0; k <= order; ++k)
    {
      if (j != k)
      { // FIXME: (j - k) could be a register decremented inside the k loop:
        // and even better: normalization 1./(j - k) could be pre-computed and stored
        // somehow for each order that is actually used, removing division operations.
        shape[j] *= (v - k) / (j - k);
      }
    }
  }
}

/// Evaluate 1-D shape functions and their derivatives for the given \a order at the given \a pcoord (in [0,1]).
void vtkLagrangeInterpolation::EvaluateShapeAndGradient(int order, double pcoord, double* shape, double* deriv)
{
  int j, k, q;
  double dtmp;
  double v = order * pcoord;
  for (j = 0; j <= order; ++j)
  {
    //std::cout << "ShapeDeriv j = " << j << "  v = " << v << "\n";
    shape[j] = 1.;
    deriv[j] = 0.;
    for (k = 0; k <= order; ++k)
    {
      if (j != k)
      { // FIXME: (j - k) could be a register decremented inside the k loop:
        // and even better: normalization could be pre-computed and stored
        // somehow for each order that is actually used.
        shape[j] *= (v - k) / (j - k);

        // Now compute the derivative of shape[j]; we use the differentiation
        // rule d/dx(a * b) = a * d/dx(b) + b * d/dx(a) instead of faster
        // methods because it keeps the truncation error low(er):
        dtmp = 1.;
        //std::cout << "           k = " << k;
        for (q = 0; q <= order; ++q)
        {
          if (q == j)
          {
            continue;
          }
          dtmp *= (q == k ? 1. : (v - q)) / (j - q);
          //std::cout << "  q " << q << " dtmp *= " << ((q == k ? 1. : (v - q)) / (j - q));
        }
        //std::cout << "        dtmp = " << dtmp << "\n";
        deriv[j] += order*dtmp;
      }
    }
  }
}

int vtkLagrangeInterpolation::Tensor1ShapeFunctions(const int order[1], const double* pcoords, double* shape)
{
  vtkLagrangeInterpolation::EvaluateShapeFunctions(order[0], pcoords[0], shape);
  return order[0] + 1;
}

int vtkLagrangeInterpolation::Tensor1ShapeDerivatives(const int order[1], const double* pcoords, double* derivs)
{
  std::vector<double> dummy(order[0] + 1);
  vtkLagrangeInterpolation::EvaluateShapeAndGradient(order[0], pcoords[0], &dummy[0], derivs);
  return order[0] + 1;
}

/// Quadrilateral shape function computation
int vtkLagrangeInterpolation::Tensor2ShapeFunctions(const int order[2], const double pcoords[3], double* shape)
{
  // FIXME: Eventually needs to be varying length.
  double ll[2][vtkLagrangeInterpolation::MaxDegree + 1];
  int i, j;

  for (i = 0; i < 2; ++i)
  {
    vtkLagrangeInterpolation::EvaluateShapeFunctions(order[i], pcoords[i], ll[i]);
  }

  int sn = 0;

  // Corners
  shape[sn++] = ll[0][0]        * ll[1][0];
  shape[sn++] = ll[0][order[0]] * ll[1][0];
  shape[sn++] = ll[0][order[0]] * ll[1][order[1]];
  shape[sn++] = ll[0][0]        * ll[1][order[1]];

  int sn1 = sn + order[0] + order[1] - 2;
  for (i = 1; i < order[0]; ++i)
  {
    //cout << sn << ", " << sn1 << "\n";
    shape[sn++ ] = ll[0][i] * ll[1][0];               // Edge 0-1
    shape[sn1++] = ll[0][i] * ll[1][order[1]];        // Edge 2-3
  }

  for (i = 1; i < order[1]; ++i)
  {
    //cout << sn << ", " << sn1 << "\n";
    shape[sn++ ] = ll[0][order[0]] * ll[1][i];        // Edge 1-2
    shape[sn1++] = ll[0][0] * ll[1][i];               // Edge 3-0
  }
  sn = sn1; // Advance to the end of all edge DOFs.

  for (i = 1; i < order[1]; ++i)
  {
    for (j = 1; j < order[0]; ++j)
    {
      //cout << sn << "\n";
      shape[sn++ ] = ll[0][j] * ll[1][i];        // Face 0-1-2-3
    }
  }
  return sn;
}

// Quadrilateral shape-function derivatives
int vtkLagrangeInterpolation::Tensor2ShapeDerivatives(const int order[2], const double pcoords[2], double* deriv)
{
  // FIXME: Eventually needs to be varying length.
  double ll[2][vtkLagrangeInterpolation::MaxDegree + 1];
  double dd[2][vtkLagrangeInterpolation::MaxDegree + 1];
  int i, j;

  for (i = 0; i < 2; ++i)
  {
    vtkLagrangeInterpolation::EvaluateShapeAndGradient(
      order[i], pcoords[i], ll[i], dd[i]);
  }

  int sn = 0;

  // Corners
  deriv[sn++] = dd[0][0]        * ll[1][0];
  deriv[sn++] = ll[0][0]        * dd[1][0];

  deriv[sn++] = dd[0][order[0]] * ll[1][0];
  deriv[sn++] = ll[0][order[0]] * dd[1][0];

  deriv[sn++] = dd[0][order[0]] * ll[1][order[1]];
  deriv[sn++] = ll[0][order[0]] * dd[1][order[1]];

  deriv[sn++] = dd[0][0]        * ll[1][order[1]];
  deriv[sn++] = ll[0][0]        * dd[1][order[1]];

  int sn1 = sn + 2 * (order[0] + order[1] - 2);
  for (i = 1; i < order[0]; ++i)
  {
    //cout << sn << ", " << sn1 << "\n";
    deriv[sn++ ] = dd[0][i] * ll[1][0];               // Edge 0-1
    deriv[sn++ ] = ll[0][i] * dd[1][0];               // Edge 0-1

    deriv[sn1++] = dd[0][i] * ll[1][order[1]];        // Edge 2-3
    deriv[sn1++] = ll[0][i] * dd[1][order[1]];        // Edge 2-3
  }

  for (i = 1; i < order[1]; ++i)
  {
    //cout << sn << ", " << sn1 << "\n";
    deriv[sn++ ] = dd[0][order[0]] * ll[1][i];        // Edge 1-2
    deriv[sn++ ] = ll[0][order[0]] * dd[1][i];        // Edge 1-2

    deriv[sn1++] = dd[0][0] * ll[1][i];               // Edge 3-0
    deriv[sn1++] = ll[0][0] * dd[1][i];               // Edge 3-0
  }
  sn = sn1;
  for (i = 1; i < order[1]; ++i)
  {
    for (j = 1; j < order[0]; ++j)
    {
      //cout << sn << "\n";
      deriv[sn++ ] = dd[0][j] * ll[1][i];        // Face 0-1-2-3
      deriv[sn++ ] = ll[0][j] * dd[1][i];        // Face 0-1-2-3
    }
  }
  return sn;
}

/// Hexahedral shape function computation
int vtkLagrangeInterpolation::Tensor3ShapeFunctions(const int order[3], const double pcoords[3], double* shape)
{
  // FIXME: Eventually needs to be varying length.
  double ll[3][vtkLagrangeInterpolation::MaxDegree + 1];
  int i,j,k;

  for (i = 0; i < 3; ++i)
  {
    vtkLagrangeInterpolation::EvaluateShapeFunctions(order[i], pcoords[i], ll[i]);
  }

  int sn = 0;

  // Corners
  shape[sn++] = ll[0][0]        * ll[1][0]        * ll[2][0];
  shape[sn++] = ll[0][order[0]] * ll[1][0]        * ll[2][0];
  shape[sn++] = ll[0][order[0]] * ll[1][order[1]] * ll[2][0];
  shape[sn++] = ll[0][0]        * ll[1][order[1]] * ll[2][0];
  shape[sn++] = ll[0][0]        * ll[1][0]        * ll[2][order[2]];
  shape[sn++] = ll[0][order[0]] * ll[1][0]        * ll[2][order[2]];
  shape[sn++] = ll[0][order[0]] * ll[1][order[1]] * ll[2][order[2]];
  shape[sn++] = ll[0][0]        * ll[1][order[1]] * ll[2][order[2]];

  int sn1, sn2, sn3;
  sn1 = order[0] + order[1] - 2;
  sn2 = sn1 * 2;
  sn3 = sn + sn1 + sn2;
  sn1 += sn;
  sn2 += sn;
  for (i = 1; i < order[0]; ++i)
  {
    //cout << sn << ", " << sn1 << ", " << sn2 << ", " << sn3 << "\n";
    shape[sn++ ] = ll[0][i] * ll[1][0] * ll[2][0];               // Edge 0-1
    shape[sn1++] = ll[0][i] * ll[1][order[1]] * ll[2][0];        // Edge 2-3
    shape[sn2++] = ll[0][i] * ll[1][0] * ll[2][order[2]];        // Edge 4-5
    shape[sn3++] = ll[0][i] * ll[1][order[1]] * ll[2][order[2]]; // Edge 6-7
  }

  for (i = 1; i < order[1]; ++i)
  {
    //cout << sn << ", " << sn1 << ", " << sn2 << ", " << sn3 << "\n";
    shape[sn++ ] = ll[0][order[0]] * ll[1][i] * ll[2][0];        // Edge 1-2
    shape[sn1++] = ll[0][0] * ll[1][i] * ll[2][0];               // Edge 3-0
    shape[sn2++] = ll[0][order[0]] * ll[1][i] * ll[2][order[2]]; // Edge 5-6
    shape[sn3++] = ll[0][0] * ll[1][i] * ll[2][order[2]];        // Edge 7-4
  }
  sn = sn3;
  sn1 = order[2] - 1;
  sn2 = sn1 * 2;
  sn3 = sn + sn1 + sn2;
  sn1 += sn;
  sn2 += sn;
  for (i = 1; i < order[2]; ++i)
  {
    //cout << sn << ", " << sn1 << ", " << sn2 << ", " << sn3 << "\n";
    shape[sn++ ] = ll[0][0] * ll[1][0] * ll[2][i];                // Edge 0-4
    shape[sn1++] = ll[0][order[0]] * ll[1][0] * ll[2][i];         // Edge 1-5
    // Kitware insists on swapping edges 10 and 11 as follows:
    shape[sn3++] = ll[0][order[0]] * ll[1][order[1]] * ll[2][i];  // Edge 2-6
    shape[sn2++] = ll[0][0] * ll[1][order[1]] * ll[2][i];         // Edge 3-7
  }

  sn = sn3;
  sn1 = (order[1] - 1)*(order[2] - 1);
  sn2 = sn1 * 2;
  sn3 = sn + sn2 + (order[2] - 1)*(order[0] - 1);
  sn1 += sn;
  sn2 += sn;
  for (i = 1; i < order[2]; ++i)
  {
    for (j = 1; j < order[1]; ++j)
    {
      //cout << sn << ", " << sn1 << "\n";
      shape[sn++ ] = ll[0][0] * ll[1][j] * ll[2][i];        // Face 0-4-7-3
      shape[sn1++] = ll[0][order[0]] * ll[1][j] * ll[2][i]; // Face 1-2-6-5
    }
    for (j = 1; j < order[0]; ++j)
    {
      //cout << sn2 << ", " << sn3 << "\n";
      shape[sn2++] = ll[0][j] * ll[1][0] * ll[2][i];        // Face 0-1-5-4
      shape[sn3++] = ll[0][j] * ll[1][order[1]] * ll[2][i]; // Face 2-3-7-6
    }
  }
  sn = sn3;
  sn1 = sn + (order[0] - 1)*(order[1] - 1);
  for (i = 1; i < order[1]; ++i)
  {
    for (j = 1; j < order[0]; ++j)
    {
      //cout << sn << ", " << sn1 << "\n";
      shape[sn++ ] = ll[0][j] * ll[1][i] * ll[2][0];        // Face 0-1-2-3
      shape[sn1++] = ll[0][j] * ll[1][i] * ll[2][order[2]]; // Face 4-7-6-5
    }
  }
  sn = sn1;
  for (k = 1; k < order[2]; ++k)
  {
    for (j = 1; j < order[1]; ++j)
    {
      for (i = 1; i < order[0]; ++i)
      {
        //cout << sn << "\n";
        shape[sn++] = ll[0][i] * ll[1][j] * ll[2][k]; // Body
      }
    }
  }
  return sn;
}

int vtkLagrangeInterpolation::Tensor3ShapeDerivatives(const int order[3], const double pcoords[3], double* deriv)
{
  // FIXME: Eventually needs to be varying length.
  double ll[3][vtkLagrangeInterpolation::MaxDegree + 1];
  double dd[3][vtkLagrangeInterpolation::MaxDegree + 1];
  int i, j, k;

  for (i = 0; i < 3; ++i)
  {
    vtkLagrangeInterpolation::EvaluateShapeAndGradient(
      order[i], pcoords[i], ll[i], dd[i]);
  }

  int sn = 0;

  // Corners
  deriv[sn++] = dd[0][0]        * ll[1][0]        * ll[2][0];
  deriv[sn++] = ll[0][0]        * dd[1][0]        * ll[2][0];
  deriv[sn++] = ll[0][0]        * ll[1][0]        * dd[2][0];

  deriv[sn++] = dd[0][order[0]] * ll[1][0]        * ll[2][0];
  deriv[sn++] = ll[0][order[0]] * dd[1][0]        * ll[2][0];
  deriv[sn++] = ll[0][order[0]] * ll[1][0]        * dd[2][0];

  deriv[sn++] = dd[0][order[0]] * ll[1][order[1]] * ll[2][0];
  deriv[sn++] = ll[0][order[0]] * dd[1][order[1]] * ll[2][0];
  deriv[sn++] = ll[0][order[0]] * ll[1][order[1]] * dd[2][0];

  deriv[sn++] = dd[0][0]        * ll[1][order[1]] * ll[2][0];
  deriv[sn++] = ll[0][0]        * dd[1][order[1]] * ll[2][0];
  deriv[sn++] = ll[0][0]        * ll[1][order[1]] * dd[2][0];

  deriv[sn++] = dd[0][0]        * ll[1][0]        * ll[2][order[2]];
  deriv[sn++] = ll[0][0]        * dd[1][0]        * ll[2][order[2]];
  deriv[sn++] = ll[0][0]        * ll[1][0]        * dd[2][order[2]];

  deriv[sn++] = dd[0][order[0]] * ll[1][0]        * ll[2][order[2]];
  deriv[sn++] = ll[0][order[0]] * dd[1][0]        * ll[2][order[2]];
  deriv[sn++] = ll[0][order[0]] * ll[1][0]        * dd[2][order[2]];

  deriv[sn++] = dd[0][order[0]] * ll[1][order[1]] * ll[2][order[2]];
  deriv[sn++] = ll[0][order[0]] * dd[1][order[1]] * ll[2][order[2]];
  deriv[sn++] = ll[0][order[0]] * ll[1][order[1]] * dd[2][order[2]];

  deriv[sn++] = dd[0][0]        * ll[1][order[1]] * ll[2][order[2]];
  deriv[sn++] = ll[0][0]        * dd[1][order[1]] * ll[2][order[2]];
  deriv[sn++] = ll[0][0]        * ll[1][order[1]] * dd[2][order[2]];


  int sn1, sn2, sn3;
  sn1 = 3 * (order[0] + order[1] - 2);
  sn2 = sn1 * 2;
  sn3 = sn + sn1 + sn2;
  sn1 += sn;
  sn2 += sn;
  for (i = 1; i < order[0]; ++i)
  {
    //cout << sn << ", " << sn1 << ", " << sn2 << ", " << sn3 << "\n";
    deriv[sn++ ] = dd[0][i] * ll[1][0] * ll[2][0];               // Edge 0-1
    deriv[sn++ ] = ll[0][i] * dd[1][0] * ll[2][0];               // Edge 0-1
    deriv[sn++ ] = ll[0][i] * ll[1][0] * dd[2][0];               // Edge 0-1

    deriv[sn1++] = dd[0][i] * ll[1][order[1]] * ll[2][0];        // Edge 2-3
    deriv[sn1++] = ll[0][i] * dd[1][order[1]] * ll[2][0];        // Edge 2-3
    deriv[sn1++] = ll[0][i] * ll[1][order[1]] * dd[2][0];        // Edge 2-3

    deriv[sn2++] = dd[0][i] * ll[1][0] * ll[2][order[2]];        // Edge 4-5
    deriv[sn2++] = ll[0][i] * dd[1][0] * ll[2][order[2]];        // Edge 4-5
    deriv[sn2++] = ll[0][i] * ll[1][0] * dd[2][order[2]];        // Edge 4-5

    deriv[sn3++] = dd[0][i] * ll[1][order[1]] * ll[2][order[2]]; // Edge 6-7
    deriv[sn3++] = ll[0][i] * dd[1][order[1]] * ll[2][order[2]]; // Edge 6-7
    deriv[sn3++] = ll[0][i] * ll[1][order[1]] * dd[2][order[2]]; // Edge 6-7
  }

  for (i = 1; i < order[1]; ++i)
  {
    //cout << sn << ", " << sn1 << ", " << sn2 << ", " << sn3 << "\n";
    deriv[sn++ ] = dd[0][order[0]] * ll[1][i] * ll[2][0];        // Edge 1-2
    deriv[sn++ ] = ll[0][order[0]] * dd[1][i] * ll[2][0];        // Edge 1-2
    deriv[sn++ ] = ll[0][order[0]] * ll[1][i] * dd[2][0];        // Edge 1-2

    deriv[sn1++] = dd[0][0] * ll[1][i] * ll[2][0];               // Edge 3-0
    deriv[sn1++] = ll[0][0] * dd[1][i] * ll[2][0];               // Edge 3-0
    deriv[sn1++] = ll[0][0] * ll[1][i] * dd[2][0];               // Edge 3-0

    deriv[sn2++] = dd[0][order[0]] * ll[1][i] * ll[2][order[2]]; // Edge 5-6
    deriv[sn2++] = ll[0][order[0]] * dd[1][i] * ll[2][order[2]]; // Edge 5-6
    deriv[sn2++] = ll[0][order[0]] * ll[1][i] * dd[2][order[2]]; // Edge 5-6

    deriv[sn3++] = dd[0][0] * ll[1][i] * ll[2][order[2]];        // Edge 7-4
    deriv[sn3++] = ll[0][0] * dd[1][i] * ll[2][order[2]];        // Edge 7-4
    deriv[sn3++] = ll[0][0] * ll[1][i] * dd[2][order[2]];        // Edge 7-4

  }
  sn = sn3;
  sn1 = 3 * (order[2] - 1);
  sn2 = sn1 * 2;
  sn3 = sn + sn1 + sn2;
  sn1 += sn;
  sn2 += sn;
  for (i = 1; i < order[2]; ++i)
  {
    //cout << sn << ", " << sn1 << ", " << sn2 << ", " << sn3 << "\n";
    deriv[sn++ ] = dd[0][0] * ll[1][0] * ll[2][i];                // Edge 0-4
    deriv[sn++ ] = ll[0][0] * dd[1][0] * ll[2][i];                // Edge 0-4
    deriv[sn++ ] = ll[0][0] * ll[1][0] * dd[2][i];                // Edge 0-4

    deriv[sn1++] = dd[0][order[0]] * ll[1][0] * ll[2][i];         // Edge 1-5
    deriv[sn1++] = ll[0][order[0]] * dd[1][0] * ll[2][i];         // Edge 1-5
    deriv[sn1++] = ll[0][order[0]] * ll[1][0] * dd[2][i];         // Edge 1-5

    // Kitware insists on swapping edges 10 and 11 as follows:
    deriv[sn3++] = dd[0][order[0]] * ll[1][order[1]] * ll[2][i];  // Edge 2-6
    deriv[sn3++] = ll[0][order[0]] * dd[1][order[1]] * ll[2][i];  // Edge 2-6
    deriv[sn3++] = ll[0][order[0]] * ll[1][order[1]] * dd[2][i];  // Edge 2-6

    deriv[sn2++] = dd[0][0] * ll[1][order[1]] * ll[2][i];         // Edge 3-7
    deriv[sn2++] = ll[0][0] * dd[1][order[1]] * ll[2][i];         // Edge 3-7
    deriv[sn2++] = ll[0][0] * ll[1][order[1]] * dd[2][i];         // Edge 3-7
  }

  sn = sn3;
  sn1 = 3 * (order[1] - 1) * (order[2] - 1);
  sn2 = sn1 * 2;
  sn3 = sn + sn2 + 3 * (order[2] - 1)*(order[0] - 1);
  sn1 += sn;
  sn2 += sn;
  for (i = 1; i < order[2]; ++i)
  {
    for (j = 1; j < order[1]; ++j)
    {
      //cout << sn << ", " << sn1 << "\n";
      deriv[sn++ ] = dd[0][0] * ll[1][j] * ll[2][i];        // Face 0-4-7-3
      deriv[sn++ ] = ll[0][0] * dd[1][j] * ll[2][i];        // Face 0-4-7-3
      deriv[sn++ ] = ll[0][0] * ll[1][j] * dd[2][i];        // Face 0-4-7-3

      deriv[sn1++] = dd[0][order[0]] * ll[1][j] * ll[2][i]; // Face 1-2-6-5
      deriv[sn1++] = ll[0][order[0]] * dd[1][j] * ll[2][i]; // Face 1-2-6-5
      deriv[sn1++] = ll[0][order[0]] * ll[1][j] * dd[2][i]; // Face 1-2-6-5
    }
    for (j = 1; j < order[0]; ++j)
    {
      //cout << sn2 << ", " << sn3 << "\n";
      deriv[sn2++] = dd[0][j] * ll[1][0] * ll[2][i];        // Face 0-1-5-4
      deriv[sn2++] = ll[0][j] * dd[1][0] * ll[2][i];        // Face 0-1-5-4
      deriv[sn2++] = ll[0][j] * ll[1][0] * dd[2][i];        // Face 0-1-5-4

      deriv[sn3++] = dd[0][j] * ll[1][order[1]] * ll[2][i]; // Face 2-3-7-6
      deriv[sn3++] = ll[0][j] * dd[1][order[1]] * ll[2][i]; // Face 2-3-7-6
      deriv[sn3++] = ll[0][j] * ll[1][order[1]] * dd[2][i]; // Face 2-3-7-6
    }
  }
  sn = sn3;
  sn1 = sn + 3 * (order[0] - 1) * (order[1] - 1);
  for (i = 1; i < order[1]; ++i)
  {
    for (j = 1; j < order[0]; ++j)
    {
      //cout << sn << ", " << sn1 << "\n";
      deriv[sn++ ] = dd[0][j] * ll[1][i] * ll[2][0];        // Face 0-1-2-3
      deriv[sn++ ] = ll[0][j] * dd[1][i] * ll[2][0];        // Face 0-1-2-3
      deriv[sn++ ] = ll[0][j] * ll[1][i] * dd[2][0];        // Face 0-1-2-3

      deriv[sn1++] = dd[0][j] * ll[1][i] * ll[2][order[2]]; // Face 4-7-6-5
      deriv[sn1++] = ll[0][j] * dd[1][i] * ll[2][order[2]]; // Face 4-7-6-5
      deriv[sn1++] = ll[0][j] * ll[1][i] * dd[2][order[2]]; // Face 4-7-6-5
    }
  }
  sn = sn1;
  for (k = 1; k < order[2]; ++k)
  {
    for (j = 1; j < order[1]; ++j)
    {
      for (i = 1; i < order[0]; ++i)
      {
        //cout << sn << "\n";
        deriv[sn++] = dd[0][i] * ll[1][j] * ll[2][k]; // Body
        deriv[sn++] = ll[0][i] * dd[1][j] * ll[2][k]; // Body
        deriv[sn++] = ll[0][i] * ll[1][j] * dd[2][k]; // Body
      }
    }
  }
  return sn;
}

void vtkLagrangeInterpolation::Tensor3EvaluateDerivative(
  const int order[3],
  const double* pcoords,
  vtkPoints* points,
  const double* fieldVals,
  int fieldDim,
  double* fieldDerivs)
{
  vtkIdType numberOfPoints = points->GetNumberOfPoints();
  this->PrepareForOrder(order, numberOfPoints);
  this->Tensor3ShapeDerivatives(order, pcoords, &this->DerivSpace[0]);

  // compute inverse Jacobian
  double *jI[3], j0[3], j1[3], j2[3];
  jI[0] = j0; jI[1] = j1; jI[2] = j2;
  if (this->JacobianInverse(points, this->DerivSpace.data(), jI) == 0)
  { // jacobian inverse computation failed
    return;
  }

  // now compute derivates of values provided
  for (int k=0; k < fieldDim; k++) //loop over values per vertex
  {
    double sum[3] = {0, 0, 0};
    for (vtkIdType i=0; i < numberOfPoints; i++) //loop over interp. function derivatives
    {
      // Note the subtle difference between the indexing of this->DerivSpace here and in WedgeEvaluateDerivative.
      double value = fieldVals[fieldDim*i + k];
      sum[0] += this->DerivSpace[3*i] * value;
      sum[1] += this->DerivSpace[3*i+1] * value;
      sum[2] += this->DerivSpace[3*i+2] * value;
    }

    for (int j=0; j < 3; j++) //loop over derivative directions
    {
      fieldDerivs[3*k + j] = sum[0]*jI[j][0] + sum[1]*jI[j][1] + sum[2]*jI[j][2];
    }
  }
}


/// Wedge shape function computation
void vtkLagrangeInterpolation::WedgeShapeFunctions(const int order[3], const vtkIdType numberOfPoints, const double pcoords[3], double* shape)
{
  static vtkNew<vtkLagrangeTriangle> tri;

  if (order[0] != order[1])
  {
    vtkGenericWarningMacro(
      "Orders 0 and 1 (parametric coordinates of triangle, " << order[0] << " and " << order[1] << ") must match.");
    return;
  }

  int rsOrder = order[0];
  int tOrder = order[2];
  if (
    rsOrder > vtkLagrangeInterpolation::MaxDegree ||
    tOrder > vtkLagrangeInterpolation::MaxDegree)
  {
    vtkGenericWarningMacro(
      "vtkLagrangeInterpolation::MaxDegree exceeded by "
      << order[0] << ", " << order[1] << ", " << order[2]);
    return;
  }

#ifdef VTK_21_POINT_WEDGE
  if (numberOfPoints == 21 && order[0] == 2)
  {
    const double r = pcoords[0];
    const double s = pcoords[1];
    // the parametric space along this axis is [-1,1] for these calculations
    const double t = 2 * pcoords[2] - 1.;
    const double rsm = 1. - r - s;
    const double rs = r * s;
    const double tp = 1. + t;
    const double tm = 1. - t;

    shape[ 0] = -0.5 * t * tm * rsm * (1.0 - 2.0 * (r + s) + 3.0 * rs);
    shape[ 1] = -0.5 * t * tm * (r - 2.0 * (rsm * r + rs) + 3.0 * rsm * rs);
    shape[ 2] = -0.5 * t * tm * (s - 2.0 * (rsm * s + rs) + 3.0 * rsm * rs);
    shape[ 3] =  0.5 * t * tp * rsm * (1.0 - 2.0 * (r + s) + 3.0 * rs);
    shape[ 4] =  0.5 * t * tp * (r - 2.0 * (rsm * r + rs) + 3.0 * rsm * rs);
    shape[ 5] =  0.5 * t * tp * (s - 2.0 * (rsm * s + rs) + 3.0 * rsm * rs);
    shape[ 6] = -0.5 * t * tm * rsm * (4.0 * r - 12.0 * rs);
    shape[ 7] = -0.5 * t * tm * (4.0 * rs - 12.0 * rsm * rs);
    shape[ 8] = -0.5 * t * tm * rsm * (4.0 * s - 12.0 * rs);
    shape[ 9] =  0.5 * t * tp * rsm * (4.0 * r - 12.0 * rs);
    shape[10] =  0.5 * t * tp * (4.0 * rs - 12.0 * rsm * rs);
    shape[11] =  0.5 * t * tp * rsm * (4.0 * s - 12.0 * rs);
    shape[12] =  tp * tm * rsm * (1.0 - 2.0 * (r + s) + 3.0 * rs);
    shape[13] =  tp * tm * (r - 2.0 * (rsm * r + rs) + 3.0 * rsm * rs);
    shape[14] =  tp * tm * (s - 2.0 * (rsm * s + rs) + 3.0 * rsm * rs);
    shape[15] = -0.5 * 27.0 * t * tm * rsm * rs;
    shape[16] =  0.5 * 27.0 * t * tp * rsm * rs;
    shape[17] =  tp * tm * rsm * (4.0 * r - 12.0 * rs);
    shape[18] =  tp * tm * (4.0 * rs - 12.0 * rsm * rs);
    shape[19] =  tp * tm * rsm * (4.0 * s - 12.0 * rs);
    shape[20] =  27.0 * tp * tm * rsm * rs;
    return;
  }
#endif

  // FIXME: Eventually needs to be varying length.
  double ll[vtkLagrangeInterpolation::MaxDegree + 1];
  double tt[(vtkLagrangeInterpolation::MaxDegree + 1) * (vtkLagrangeInterpolation::MaxDegree + 2) / 2];
  vtkLagrangeInterpolation::EvaluateShapeFunctions(tOrder, pcoords[2], ll);
  vtkVector3d triP(pcoords);
  triP[2] = 0;
  int numtripts = (rsOrder + 1) * (rsOrder + 2) / 2;
  tri->GetPoints()->SetNumberOfPoints(numtripts);
  tri->GetPointIds()->SetNumberOfIds(numtripts);
  tri->Initialize();
  tri->InterpolateFunctions(triP.GetData(), tt);

  int sn;
  //int numPts = numtripts * (tOrder + 1);
  vtkIdType ijk[3];
  for (int kk = 0; kk <= tOrder; ++kk)
  {
    for (int jj = 0; jj <= rsOrder; ++jj)
    {
      ijk[1] = jj;
      for (int ii = 0; ii <= rsOrder - jj; ++ii)
      {
        ijk[0] = ii;
        sn = vtkLagrangeWedge::PointIndexFromIJK(ii, jj, kk, order);
        if (sn >= 0)
        {
          ijk[2] = rsOrder - ii - jj;
          int tOff = vtkLagrangeTriangle::Index(ijk, rsOrder);
          shape[sn] = ll[kk] * tt[tOff];
        }
      }
    }
  }
}

/// Wedge shape-function derivative evaluation
void vtkLagrangeInterpolation::WedgeShapeDerivatives(const int order[3], const vtkIdType numberOfPoints, const double pcoords[3], double* derivs)
{
  static vtkNew<vtkLagrangeTriangle> tri;

  if (order[0] != order[1])
  {
    vtkGenericWarningMacro(
      "Orders 0 and 1 (parametric coordinates of triangle, " << order[0] << " and " << order[1] << ") must match.");
    return;
  }

  int rsOrder = order[0];
  int tOrder = order[2];
  if (
    rsOrder > vtkLagrangeInterpolation::MaxDegree ||
    tOrder > vtkLagrangeInterpolation::MaxDegree)
  {
    vtkGenericWarningMacro(
      "vtkLagrangeInterpolation::MaxDegree exceeded by "
      << order[0] << ", " << order[1] << ", " << order[2]);
    return;
  }

  // FIXME: Eventually needs to be varying length.
  double ll[vtkLagrangeInterpolation::MaxDegree + 1];
  double ld[vtkLagrangeInterpolation::MaxDegree + 1];
  double tt[(vtkLagrangeInterpolation::MaxDegree + 1) * (vtkLagrangeInterpolation::MaxDegree + 2) / 2];
  double td[(vtkLagrangeInterpolation::MaxDegree + 1) * (vtkLagrangeInterpolation::MaxDegree + 2)];
  vtkLagrangeInterpolation::EvaluateShapeAndGradient(tOrder, pcoords[2], ll, ld);
  vtkVector3d triP(pcoords);
  triP[2] = 0;
  int numtripts = (rsOrder + 1) * (rsOrder + 2) / 2;
  tri->GetPoints()->SetNumberOfPoints(numtripts);
  tri->GetPointIds()->SetNumberOfIds(numtripts);
  tri->Initialize();
  tri->InterpolateFunctions(triP.GetData(), tt);
  tri->InterpolateDerivs(triP.GetData(), td);

  int numPts = numtripts * (tOrder + 1);
#ifdef VTK_21_POINT_WEDGE
  if (numberOfPoints == 21 && order[0] == 2)
  {
    const double r = pcoords[0];
    const double s = pcoords[1];
    // the parametric space along this axis is [-1,1] for these calculations
    const double t = 2 * pcoords[2] - 1.;
    const double tm = t - 1.;
    const double tp = t + 1.;
    const double rsm = 1. - r - s;
    const double rs = r * s;

    // dN/dr
    derivs[ 0] = 0.5*t*tm*(-3.0*rs + 2.0*r + 2.0*s + (3.0*s - 2.0)*rsm - 1.0);
    derivs[ 1] = -0.5*t*tm*(3.0*rs - 4.0*r - 3.0*s*rsm + 1.0);
    derivs[ 2] = -1.5*s*t*tm*(2*r + s - 1);
    derivs[ 3] = 0.5*t*tp*(-3.0*rs + 2.0*r + 2.0*s + (3.0*s - 2.0)*rsm - 1.0);
    derivs[ 4] = -0.5*t*tp*(3.0*rs - 4.0*r - 3.0*s*rsm + 1.0);
    derivs[ 5] = -1.5*s*t*tp*(2*r + s - 1);
    derivs[ 6] = 0.5*t*(12.0*s - 4.0)*tm*(2*r + s - 1);
    derivs[ 7] = 0.5*s*t*tm*(24.0*r + 12.0*s - 8.0);
    derivs[ 8] = s*t*tm*(12.0*r + 6.0*s - 8.0);
    derivs[ 9] = 0.5*t*(12.0*s - 4.0)*tp*(2*r + s - 1);
    derivs[10] = 0.5*s*t*tp*(24.0*r + 12.0*s - 8.0);
    derivs[11] = s*t*tp*(12.0*r + 6.0*s - 8.0);
    derivs[12] = tm*tp*(3.0*rs - 2.0*r - 2.0*s - (3.0*s - 2.0)*rsm + 1.0);
    derivs[13] = tm*tp*(3.0*rs - 4.0*r - 3.0*s*rsm + 1.0);
    derivs[14] = 3.0*s*tm*tp*(2*r + s - 1);
    derivs[15] = 13.5*s*t*tm*(-2*r - s + 1);
    derivs[16] = 13.5*s*t*tp*(-2*r - s + 1);
    derivs[17] = (12.0*s - 4.0)*tm*tp*(-2*r - s + 1);
    derivs[18] = -s*tm*tp*(24.0*r + 12.0*s - 8.0);
    derivs[19] = s*tm*tp*(-24.0*r - 12.0*s + 16.0);
    derivs[20] = 27.0*s*tm*tp*(2*r + s - 1);

    // dN/ds
    derivs[21] = 0.5*t*tm*(-3.0*rs + 2.0*r + 2.0*s + (3.0*r - 2.0)*rsm - 1.0);
    derivs[22] =  -1.5*r*t*tm*(r + 2*s - 1);
    derivs[23] =  -0.5*t*tm*(3.0*rs - 3.0*r*rsm - 4.0*s + 1.0);
    derivs[24] =  0.5*t*tp*(-3.0*rs + 2.0*r + 2.0*s + (3.0*r - 2.0)*rsm - 1.0);
    derivs[25] =  -1.5*r*t*tp*(r + 2*s - 1);
    derivs[26] =  -0.5*t*tp*(3.0*rs - 3.0*r*rsm - 4.0*s + 1.0);
    derivs[27] =  r*t*tm*(6.0*r + 12.0*s - 8.0);
    derivs[28] =  0.5*r*t*tm*(12.0*r + 24.0*s - 8.0);
    derivs[29] =  0.5*t*(12.0*r - 4.0)*tm*(r + 2*s - 1);
    derivs[30] =  r*t*tp*(6.0*r + 12.0*s - 8.0);
    derivs[31] =  0.5*r*t*tp*(12.0*r + 24.0*s - 8.0);
    derivs[32] =  0.5*t*(12.0*r - 4.0)*tp*(r + 2*s - 1);
    derivs[33] =  tm*tp*(3.0*rs - 2.0*r - 2.0*s - (3.0*r - 2.0)*rsm + 1.0);
    derivs[34] =  3.0*r*tm*tp*(r + 2*s - 1);
    derivs[35] =  tm*tp*(3.0*rs - 3.0*r*rsm - 4.0*s + 1.0);
    derivs[36] =  13.5*r*t*tm*(-r - 2*s + 1);
    derivs[37] =  13.5*r*t*tp*(-r - 2*s + 1);
    derivs[38] =  r*tm*tp*(-12.0*r - 24.0*s + 16.0);
    derivs[39] =  -r*tm*tp*(12.0*r + 24.0*s - 8.0);
    derivs[40] =  (12.0*r - 4.0)*tm*tp*(-r - 2*s + 1);
    derivs[41] =  27.0*r*tm*tp*(r + 2*s - 1);

    // dN/dt
    derivs[42] = (2*t - 1)*rsm*(3.0*rs - 2.0*r - 2.0*s + 1.0);
    derivs[43] =  r*(-2*t + 1)*(-2.0*r - 3.0*s*rsm + 1.0);
    derivs[44] =  s*(-2*t + 1)*(-3.0*r*rsm - 2.0*s + 1.0);
    derivs[45] =  (2*t + 1)*rsm*(3.0*rs - 2.0*r - 2.0*s + 1.0);
    derivs[46] =  -r*(2*t + 1)*(-2.0*r - 3.0*s*rsm + 1.0);
    derivs[47] =  -s*(2*t + 1)*(-3.0*r*rsm - 2.0*s + 1.0);
    derivs[48] =  -r*(12.0*s - 4.0)*(2*t - 1)*rsm;
    derivs[49] =  rs*(2*t - 1)*(12.0*r + 12.0*s - 8.0);
    derivs[50] =  -s*(12.0*r - 4.0)*(2*t - 1)*rsm;
    derivs[51] =  -r*(12.0*s - 4.0)*(2*t + 1)*rsm;
    derivs[52] =  rs*(2*t + 1)*(12.0*r + 12.0*s - 8.0);
    derivs[53] =  -s*(12.0*r - 4.0)*(2*t + 1)*rsm;
    derivs[54] =  -4*t*rsm*(3.0*rs - 2.0*r - 2.0*s + 1.0);
    derivs[55] =  4.*r*(1. - 3.*s + 3.*s*s + r*(-2. + 3.*s))*t;
    derivs[56] =  4.*s*t*(-3.0*r*rsm - 2.0*s + 1.0);
    derivs[57] =  -27.*rs*(-2*t + 1)*rsm;
    derivs[58] =  27.*rs*(2*t + 1)*rsm;
    derivs[59] =  4.*r*t*(12.0*s - 4.0)*rsm;
    derivs[60] =  2.*rs*t*(-24.0*r - 24.0*s + 16.0);
    derivs[61] =  4.*s*t*(12.0*r - 4.0)*rsm;
    derivs[62] =  -108.*rs*t*rsm;

    return;
  }
#endif
  vtkIdType ijk[3];
  for (int kk = 0; kk <= tOrder; ++kk)
  {
    for (int jj = 0; jj <= rsOrder; ++jj)
    {
      ijk[1] = jj;
      for (int ii = 0; ii <= rsOrder - jj; ++ii)
      {
        ijk[0] = ii;
        int sn = vtkLagrangeWedge::PointIndexFromIJK(ii, jj, kk, order);
        if (sn >= 0)
        {
          ijk[2] = rsOrder - ii - jj;
          int tOff = vtkLagrangeTriangle::Index(ijk, rsOrder);
          derivs[sn] = td[tOff] * ll[kk];
          derivs[sn + numPts] = td[tOff+numtripts] * ll[kk];
          derivs[sn + 2 * numPts] = ld[kk] * tt[tOff];
        }
      }
    }
  }
}

#define VTK_MAX_WARNS 6
int vtkLagrangeInterpolation::JacobianInverse(
  vtkPoints* points,
  const double* derivs,
  double** inverse)
{
  double *m[3], m0[3], m1[3], m2[3];
  double x[3];

  // create Jacobian matrix
  m[0] = m0; m[1] = m1; m[2] = m2;
  for (int i=0; i < 3; i++) //initialize matrix
  {
    m0[i] = m1[i] = m2[i] = 0.0;
  }

  vtkIdType numberOfPoints = points->GetNumberOfPoints();
  for (vtkIdType j=0; j < numberOfPoints; j++ )
  {
    points->GetPoint(j, x);
    for (int i=0; i < 3; i++ )
    {
      m0[i] += x[i] * derivs[3*j];
      m1[i] += x[i] * derivs[3*j+1];
      m2[i] += x[i] * derivs[3*j+2];
    }
  }

  // now find the inverse
  if ( vtkMath::InvertMatrix(m,inverse,3) == 0 )
  {
    static int numWarns=0;
    if ( numWarns++ < VTK_MAX_WARNS )
    {
      vtkErrorMacro(<<"Jacobian inverse not found");
      vtkErrorMacro(<<"Matrix:" << m[0][0] << " " << m[0][1] << " " << m[0][2] << " "
                    << m[1][0] << " " << m[1][1] << " " << m[1][2] << " "
                    << m[2][0] << " " << m[2][1] << " " << m[2][2] );
      return 0;
    }
  }

  return 1;
}

int vtkLagrangeInterpolation::JacobianInverseWedge(
  vtkPoints* points,
  const double* derivs,
  double** inverse)
{
  double *m[3], m0[3], m1[3], m2[3];
  double x[3];

  // create Jacobian matrix
  m[0] = m0; m[1] = m1; m[2] = m2;
  for (int i=0; i < 3; i++) //initialize matrix
  {
    m0[i] = m1[i] = m2[i] = 0.0;
  }

  vtkIdType numberOfPoints = points->GetNumberOfPoints();
  for (vtkIdType j=0; j < numberOfPoints; j++ )
  {
    points->GetPoint(j, x);
    for (int i=0; i < 3; i++ )
    {
      m0[i] += x[i] * derivs[j];
      m1[i] += x[i] * derivs[numberOfPoints+j];
      m2[i] += x[i] * derivs[2*numberOfPoints+j];
    }
  }

  // now find the inverse
  if ( vtkMath::InvertMatrix(m,inverse,3) == 0 )
  {
    static int numWarns=0;
    if ( numWarns++ < VTK_MAX_WARNS )
    {
      vtkErrorMacro(<<"Jacobian inverse not found");
      vtkErrorMacro(<<"Matrix:" << m[0][0] << " " << m[0][1] << " " << m[0][2] << " "
                    << m[1][0] << " " << m[1][1] << " " << m[1][2] << " "
                    << m[2][0] << " " << m[2][1] << " " << m[2][2] );
      return 0;
    }
  }

  return 1;
}

void vtkLagrangeInterpolation::WedgeEvaluate(
  const int order[3],
  const vtkIdType numberOfPoints,
  const double* pcoords,
  double* fieldVals,
  int fieldDim,
  double* fieldAtPCoords)
{
  this->PrepareForOrder(order, numberOfPoints);
  this->WedgeShapeFunctions(order, numberOfPoints, pcoords, &this->ShapeSpace[0]);
  // Loop over components of the field:
  for (int cc = 0; cc < fieldDim; ++cc)
  {
    fieldAtPCoords[cc] = 0.;
    // Loop over shape functions (per-DOF values of the cell):
    for (vtkIdType pp = 0; pp < numberOfPoints; ++pp)
    {
      fieldAtPCoords[cc] += this->ShapeSpace[pp] * fieldVals[fieldDim * pp + cc];
    }
  }
}

void vtkLagrangeInterpolation::WedgeEvaluateDerivative(
  const int order[3],
  const double* pcoords,
  vtkPoints* points,
  const double* fieldVals,
  int fieldDim,
  double* fieldDerivs)
{
  vtkIdType numberOfPoints = points->GetNumberOfPoints();
  this->PrepareForOrder(order, numberOfPoints);
  this->WedgeShapeDerivatives(order, numberOfPoints, pcoords, &this->DerivSpace[0]);

  // compute inverse Jacobian
  double *jI[3], j0[3], j1[3], j2[3];
  jI[0] = j0; jI[1] = j1; jI[2] = j2;
  if (this->JacobianInverseWedge(points, this->DerivSpace.data(), jI) == 0)
  { // jacobian inverse computation failed
    return;
  }

  // now compute derivates of values provided
  for (int k=0; k < fieldDim; k++) //loop over values per vertex
  {
    double sum[3] = {0, 0, 0};
    for (vtkIdType i=0; i < numberOfPoints; i++) //loop over interp. function derivatives
    {
      double value = fieldVals[fieldDim*i + k];
      sum[0] += this->DerivSpace[i] * value;
      sum[1] += this->DerivSpace[numberOfPoints + i] * value;
      sum[2] += this->DerivSpace[2*numberOfPoints + i] * value;
    }

    for (int j=0; j < 3; j++) //loop over derivative directions
    {
      fieldDerivs[3*k + j] = sum[0]*jI[j][0] + sum[1]*jI[j][1] + sum[2]*jI[j][2];
    }
  }
}

vtkVector3d vtkLagrangeInterpolation::GetParametricHexCoordinates(int vertexId)
{
  return vtkVector3d(hexCorner[vertexId]);
}

vtkVector2i vtkLagrangeInterpolation::GetPointIndicesBoundingHexEdge(int edgeId)
{
  return vtkVector2i(hexEdgeCorners[edgeId][0], hexEdgeCorners[edgeId][1]);
}

int vtkLagrangeInterpolation::GetVaryingParameterOfHexEdge(int edgeId)
{
  return hexEdgeCorners[edgeId][2];
}

vtkVector2i vtkLagrangeInterpolation::GetFixedParametersOfHexEdge(int edgeId)
{
  return vtkVector2i(hexEdgeCorners[edgeId][3], hexEdgeCorners[edgeId][4]);
}

const int* vtkLagrangeInterpolation::GetPointIndicesBoundingHexFace(int faceId)
{
  return hexFaceCorners[faceId];
}

const int* vtkLagrangeInterpolation::GetEdgeIndicesBoundingHexFace(int faceId)
{
  return hexFaceEdges[faceId];
}

vtkVector2i vtkLagrangeInterpolation::GetVaryingParametersOfHexFace(int faceId)
{
  return vtkVector2i(hexFaceCorners[faceId][4], hexFaceCorners[faceId][5]);
}

int vtkLagrangeInterpolation::GetFixedParameterOfHexFace(int faceId)
{
  return hexFaceCorners[faceId][6];
}

vtkVector3d vtkLagrangeInterpolation::GetParametricWedgeCoordinates(int vertexId)
{
  return vtkVector3d(wedgeCorner[vertexId]);
}

vtkVector2i vtkLagrangeInterpolation::GetPointIndicesBoundingWedgeEdge(int edgeId)
{
  return vtkVector2i(wedgeEdgeCorners[edgeId][0], wedgeEdgeCorners[edgeId][1]);
}

int vtkLagrangeInterpolation::GetVaryingParameterOfWedgeEdge(int edgeId)
{
  return wedgeEdgeCorners[edgeId][2];
}

vtkVector2i vtkLagrangeInterpolation::GetFixedParametersOfWedgeEdge(int edgeId)
{
  return vtkVector2i(wedgeEdgeCorners[edgeId][3], wedgeEdgeCorners[edgeId][4]);
}

const int* vtkLagrangeInterpolation::GetPointIndicesBoundingWedgeFace(int faceId)
{
  return wedgeFaceCorners[faceId];
}

/// Return 4 edge ids bounding face (with -1 as last id for triangles) plus a face orientation as the 5th number.
const int* vtkLagrangeInterpolation::GetEdgeIndicesBoundingWedgeFace(int faceId)
{
  return wedgeFaceEdges[faceId];
}

vtkVector2i vtkLagrangeInterpolation::GetVaryingParametersOfWedgeFace(int faceId)
{
  return vtkVector2i(wedgeFaceCorners[faceId][4], wedgeFaceCorners[faceId][5]);
}

int vtkLagrangeInterpolation::GetFixedParameterOfWedgeFace(int faceId)
{
  return wedgeFaceCorners[faceId][6];
}

void vtkLagrangeInterpolation::AppendCurveCollocationPoints(vtkSmartPointer<vtkPoints>& pts, const int order[1])
{
  if (!pts)
  {
    pts = vtkSmartPointer<vtkPoints>::New();
  }

  vtkIdType existing = pts->GetNumberOfPoints();
  vtkIdType np = order[0] + 1;
  pts->SetNumberOfPoints(existing + np);
  vtkVector3d e0( 0., 0., 0.);
  vtkVector3d e1(+1., 0., 0.);

  // Insert corner points
  vtkIdType sn = existing;
  pts->SetPoint(sn++, e0.GetData());
  pts->SetPoint(sn++, e1.GetData());

  // Insert edge points
  for (int ii = 1; ii < order[0]; ++ii)
  {
    pts->SetPoint(sn++,
                  ii / static_cast<double>(order[0]), 0.0, 0.0 // Force quad to z = 0 plane
      );
  }
}

void vtkLagrangeInterpolation::AppendQuadrilateralCollocationPoints(vtkSmartPointer<vtkPoints>& pts, const int order[2])
{
  if (!pts)
  {
    pts = vtkSmartPointer<vtkPoints>::New();
  }

  vtkIdType existing = pts->GetNumberOfPoints();
  vtkIdType np = (order[0] + 1) * (order[1] + 1);
  pts->SetNumberOfPoints(existing + np);
  // Insert corner points
  vtkIdType sn = existing;
  for (int ii = 0; ii < 4; ++ii)
  {
    vtkVector3d cc(hexCorner[ii]);
    cc[2] = 0.0; // Force quad to z = 0 plane
    pts->SetPoint(sn++, cc.GetData());
  }

  // Insert edge points
  for (int ii = 0; ii < 4; ++ii)
  {
    vtkVector3d e0(hexCorner[hexEdgeCorners[ii][0]]);
    vtkVector3d e1(hexCorner[hexEdgeCorners[ii][1]]);
    for (int jj = 1; jj < order[hexEdgeCorners[ii][2]]; ++jj)
    {
      double rr = jj / static_cast<double>(order[hexEdgeCorners[ii][2]]);
      vtkVector3d vv = ((1. - rr) * e0 + rr * e1);
      vv[2] = 0.0; // Force quad to z = 0 plane
      pts->SetPoint(sn++, vv.GetData());
    }
  }

  // Insert face points
  for (int jj = 1; jj < order[1]; ++jj)
  {
    for (int ii = 1; ii < order[0]; ++ii)
    {
      pts->SetPoint(sn++,
                    ii / static_cast<double>(order[0]),
                    jj / static_cast<double>(order[1]),
                    0.0 // Force quad to z = 0 plane
        );
    }
  }
}

void vtkLagrangeInterpolation::AppendHexahedronCollocationPoints(vtkSmartPointer<vtkPoints>& pts, const int order[3])
{
  if (!pts)
  {
    pts = vtkSmartPointer<vtkPoints>::New();
  }

  vtkIdType existing = pts->GetNumberOfPoints();
  vtkIdType np = (order[0] + 1) * (order[1] + 1) * (order[2] + 1);
  pts->SetNumberOfPoints(existing + np);
  // Insert corner points
  vtkIdType sn = existing;
  for (int ii = 0; ii < 8; ++ii)
  {
    pts->SetPoint(sn++, hexCorner[ii]);
  }

  // Insert edge points
  for (unsigned ii = 0; ii < sizeof(hexEdgeCorners) / sizeof(hexEdgeCorners[0]); ++ii)
  {
    vtkVector3d e0(hexCorner[hexEdgeCorners[ii][0]]);
    vtkVector3d e1(hexCorner[hexEdgeCorners[ii][1]]);
    for (int jj = 1; jj < order[hexEdgeCorners[ii][2]]; ++jj)
    {
      double rr = jj / static_cast<double>(order[hexEdgeCorners[ii][2]]);
      vtkVector3d vv = ((1. - rr) * e0 + rr * e1);
      pts->SetPoint(sn++, vv.GetData());
    }
  }

  // Insert face points
  for (unsigned kk = 0; kk < sizeof(hexFaceCorners) / sizeof(hexFaceCorners[0]); ++kk)
  {
    vtkVector3d f0(hexCorner[hexFaceCorners[kk][0]]);
    vtkVector3d f1(hexCorner[hexFaceCorners[kk][1]]);
    vtkVector3d f2(hexCorner[hexFaceCorners[kk][2]]);
    vtkVector3d f3(hexCorner[hexFaceCorners[kk][3]]);
    for (int jj = 1; jj < order[hexFaceCorners[kk][5]]; ++jj)
    {
      double ss = jj / static_cast<double>(order[hexFaceCorners[kk][5]]);
      for (int ii = 1; ii < order[hexFaceCorners[kk][4]]; ++ii)
      {
        double rr = ii / static_cast<double>(order[hexFaceCorners[kk][4]]);
        vtkVector3d vv =
          (1. - ss) * ((1. - rr) * f0 + rr * f1) +
          ss        * ((1. - rr) * f3 + rr * f2);
        pts->SetPoint(sn++, vv.GetData());
      }
    }
  }

  // Insert body points
  for (int kk = 1; kk < order[2]; ++kk)
  {
    for (int jj = 1; jj < order[1]; ++jj)
    {
      for (int ii = 1; ii < order[0]; ++ii)
      {
        pts->SetPoint(sn++,
                      ii / static_cast<double>(order[0]),
                      jj / static_cast<double>(order[1]),
                      kk / static_cast<double>(order[2]));
      }
    }
  }
}

void vtkLagrangeInterpolation::AppendWedgeCollocationPoints(vtkSmartPointer<vtkPoints>& pts, const int order[3])
{
  if (!pts)
  {
    pts = vtkSmartPointer<vtkPoints>::New();
  }

  vtkIdType existing = pts->GetNumberOfPoints();
  vtkIdType np = (order[0] + 1) * (order[1] + 2) * (order[2] + 1) / 2; // NB: assert(order[0] == order[1])
  pts->SetNumberOfPoints(existing + np);
  // Insert corner points
  vtkIdType sn = existing;
  for (int ii = 0; ii < 6; ++ii)
  {
    pts->SetPoint(sn++, wedgeCorner[ii]);
  }

  int rsOrder = order[0]; // assert(order[0] == order[1])
  int tOrder = order[2];

  // Insert edge points
  for (unsigned ii = 0; ii < sizeof(wedgeEdgeCorners) / sizeof(wedgeEdgeCorners[0]); ++ii)
  {
    vtkVector3d e0(wedgeCorner[wedgeEdgeCorners[ii][0]]);
    vtkVector3d e1(wedgeCorner[wedgeEdgeCorners[ii][1]]);
    int varyingParam = wedgeEdgeCorners[ii][2];
    int edgeOrder = varyingParam >= 0 ? order[varyingParam] : rsOrder;
    for (unsigned jj = 1; jj < static_cast<unsigned>(edgeOrder); ++jj)
    {
      double rr = jj / static_cast<double>(edgeOrder);
      vtkVector3d vv = ((1. - rr) * e0 + rr * e1);
      pts->SetPoint(sn++, vv.GetData());
    }
  }

  // Insert face points
  unsigned nn;
  for (nn = 0; nn < 2; ++nn)
  { // Triangular faces
    vtkVector3d f0(wedgeCorner[wedgeFaceCorners[nn][0]]);
    vtkVector3d f1(wedgeCorner[wedgeFaceCorners[nn][1]]);
    // Note funky f3/f2 numbering here matches quadrilateral/hex code
    // where points are in CCW loop:
    vtkVector3d f3(wedgeCorner[wedgeFaceCorners[nn][2]]);
    vtkVector3d f2 = f0 + (f1 - f0) + (f3 - f0);

    for (int jj = 1; jj < rsOrder; ++jj)
    {
      double ss = jj / static_cast<double>(rsOrder);
      for (int ii = 1; ii < rsOrder - jj; ++ii)
      {
        double rr = ii / static_cast<double>(rsOrder);
        vtkVector3d vv =
          (1. - ss) * ((1. - rr) * f0 + rr * f1) +
          ss        * ((1. - rr) * f3 + rr * f2);
        pts->SetPoint(sn++, vv.GetData());
      }
    }
  }

  for (; nn < sizeof(wedgeFaceCorners) / sizeof(wedgeFaceCorners[0]); ++nn)
  { // Quadrilateral faces
    vtkVector3d f0(wedgeCorner[wedgeFaceCorners[nn][0]]);
    vtkVector3d f1(wedgeCorner[wedgeFaceCorners[nn][1]]);
    vtkVector3d f2(wedgeCorner[wedgeFaceCorners[nn][2]]);
    vtkVector3d f3(wedgeCorner[wedgeFaceCorners[nn][3]]);

    for (int jj = 1; jj < tOrder; ++jj)
    {
      double ss = jj / static_cast<double>(tOrder);
      for (int ii = 1; ii < rsOrder; ++ii)
      {
        double rr = ii / static_cast<double>(rsOrder);
        vtkVector3d vv =
          (1. - ss) * ((1. - rr) * f0 + rr * f1) +
          ss        * ((1. - rr) * f3 + rr * f2);
        pts->SetPoint(sn++, vv.GetData());
      }
    }
  }

  // Insert body points
  for (int kk = 1; kk < tOrder; ++kk)
  {
    for (int jj = 1; jj < rsOrder; ++jj)
    {
      for (int ii = 1; ii < rsOrder - jj; ++ii)
      {
        pts->SetPoint(sn++,
                      ii / static_cast<double>(rsOrder),
                      jj / static_cast<double>(rsOrder),
                      kk / static_cast<double>(tOrder));
      }
    }
  }
}

#if 0
void vtkLagrangeInterpolation::AppendWedgeCollocationPoints(vtkPoints* pts, int o1, int o2, int o3)
{
  int o[3] = {o1, o2, o3};
  vtkSmartPointer<vtkPoints> spts(pts);
  vtkLagrangeInterpolation::AppendWedgeCollocationPoints(spts, o);
}
#endif // 0

void vtkLagrangeInterpolation::PrepareForOrder(const int order[3], const vtkIdType numberOfPoints)
{
  // Ensure some scratch space is allocated for templated evaluation methods.
  std::size_t maxShape = numberOfPoints > 0 ? numberOfPoints : ((order[0] + 1) * (order[1] + 1) * (order[2] + 1));
  std::size_t maxDeriv = maxShape * 3;
  if (this->ShapeSpace.size() < maxShape)
  {
    this->ShapeSpace.resize(maxShape);
  }
  if (this->DerivSpace.size() < maxDeriv)
  {
    this->DerivSpace.resize(maxDeriv);
  }
}
