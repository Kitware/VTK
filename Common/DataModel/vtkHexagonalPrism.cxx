/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHexagonalPrism.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .SECTION Thanks
// Thanks to Philippe Guerville who developed this class. <br>
// Thanks to Charles Pignerol (CEA-DAM, France) who ported this class under
// VTK 4.<br>
// Thanks to Jean Favre (CSCS, Switzerland) who contributed to integrate this
// class in VTK.<br>
// Please address all comments to Jean Favre (jfavre at cscs.ch).

#include "vtkHexagonalPrism.h"

#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkPolygon.h"
#include "vtkQuad.h"

#include <cassert>
#ifndef VTK_LEGACY_REMOVE // needed temporarily in deprecated methods
#include <vector>
#endif

vtkStandardNewMacro(vtkHexagonalPrism);

static const double VTK_DIVERGED = 1.e6;

// You can recompute the value by doing:
// const double a = sqrt(3.0)/4.0 + 0.5;
#define EXPRA 0.933012701892219298

// You can recompute the value by doing:
// const double b = 0.5 - sqrt(3.0)/4.0;
// Thus EXPRA + EXPRB = 1.0
#define EXPRB 0.066987298107780702

//----------------------------------------------------------------------------
// Construct the prism with twelve points.
vtkHexagonalPrism::vtkHexagonalPrism()
{
  int i;
  this->Points->SetNumberOfPoints(12);
  this->PointIds->SetNumberOfIds(12);

  for (i = 0; i < 12; i++)
  {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
    this->PointIds->SetId(i, 0);
  }

  this->Line = vtkLine::New();
  this->Quad = vtkQuad::New();
  this->Polygon = vtkPolygon::New();
  this->Polygon->PointIds->SetNumberOfIds(6);
  this->Polygon->Points->SetNumberOfPoints(6);

  for (i = 0; i < 6; i++)
  {
    this->Polygon->Points->SetPoint(i, 0.0, 0.0, 0.0);
    this->Polygon->PointIds->SetId(i, 0);
  }
}

//----------------------------------------------------------------------------
vtkHexagonalPrism::~vtkHexagonalPrism()
{
  this->Line->Delete();
  this->Quad->Delete();
  this->Polygon->Delete();
}

//  Method to calculate parametric coordinates in an eight noded
//  linear hexahedron element from global coordinates.
//
static const int VTK_HEX_MAX_ITERATION = 10;
static const double VTK_HEX_CONVERGED = 1.e-03;

//----------------------------------------------------------------------------
int vtkHexagonalPrism::EvaluatePosition(const double x[3], double closestPoint[3], int& subId,
  double pcoords[3], double& dist2, double weights[])
{
  int iteration, converged;
  double params[3];
  double fcol[3], rcol[3], scol[3], tcol[3];
  int i, j;
  double d, pt[3];
  double derivs[36];

  //  set initial position for Newton's method
  subId = 0;
  pcoords[0] = pcoords[1] = pcoords[2] = params[0] = params[1] = params[2] = 0.5;

  //  enter iteration loop
  for (iteration = converged = 0; !converged && (iteration < VTK_HEX_MAX_ITERATION); iteration++)
  {
    //  calculate element interpolation functions and derivatives
    this->InterpolationFunctions(pcoords, weights);
    this->InterpolationDerivs(pcoords, derivs);

    //  calculate newton functions
    for (i = 0; i < 3; i++)
    {
      fcol[i] = rcol[i] = scol[i] = tcol[i] = 0.0;
    }
    for (i = 0; i < 12; i++)
    {
      this->Points->GetPoint(i, pt);
      for (j = 0; j < 3; j++)
      {
        fcol[j] += pt[j] * weights[i];
        rcol[j] += pt[j] * derivs[i];
        scol[j] += pt[j] * derivs[i + 12];
        tcol[j] += pt[j] * derivs[i + 24];
      }
    }

    for (i = 0; i < 3; i++)
    {
      fcol[i] -= x[i];
    }

    //  compute determinants and generate improvements
    d = vtkMath::Determinant3x3(rcol, scol, tcol);
    if (fabs(d) < 1.e-20)
    {
      vtkDebugMacro(<< "Determinant incorrect, iteration " << iteration);
      return -1;
    }

    pcoords[0] = params[0] - vtkMath::Determinant3x3(fcol, scol, tcol) / d;
    pcoords[1] = params[1] - vtkMath::Determinant3x3(rcol, fcol, tcol) / d;
    pcoords[2] = params[2] - vtkMath::Determinant3x3(rcol, scol, fcol) / d;

    //  check for convergence
    if (((fabs(pcoords[0] - params[0])) < VTK_HEX_CONVERGED) &&
      ((fabs(pcoords[1] - params[1])) < VTK_HEX_CONVERGED) &&
      ((fabs(pcoords[2] - params[2])) < VTK_HEX_CONVERGED))
    {
      converged = 1;
    }

    // Test for bad divergence (S.Hirschberg 11.12.2001)
    else if ((fabs(pcoords[0]) > VTK_DIVERGED) || (fabs(pcoords[1]) > VTK_DIVERGED) ||
      (fabs(pcoords[2]) > VTK_DIVERGED))
    {
      return -1;
    }

    //  if not converged, repeat
    else
    {
      params[0] = pcoords[0];
      params[1] = pcoords[1];
      params[2] = pcoords[2];
    }
  }

  //  if not converged, set the parametric coordinates to arbitrary values
  //  outside of element
  if (!converged)
  {
    return -1;
  }

  this->InterpolationFunctions(pcoords, weights);

  if (pcoords[0] >= -0.001 && pcoords[0] <= 1.001 && pcoords[1] >= -0.001 && pcoords[1] <= 1.001 &&
    pcoords[2] >= -0.001 && pcoords[2] <= 1.001)
  {
    if (closestPoint)
    {
      closestPoint[0] = x[0];
      closestPoint[1] = x[1];
      closestPoint[2] = x[2];
      dist2 = 0.0; // inside hexahedron
    }
    return 1;
  }
  else
  {
    double pc[3], w[12];
    if (closestPoint)
    {
      for (i = 0; i < 3; i++) // only approximate, not really true for warped hexa
      {
        if (pcoords[i] < 0.0)
        {
          pc[i] = 0.0;
        }
        else if (pcoords[i] > 1.0)
        {
          pc[i] = 1.0;
        }
        else
        {
          pc[i] = pcoords[i];
        }
      }
      this->EvaluateLocation(subId, pc, closestPoint, static_cast<double*>(w));
      dist2 = vtkMath::Distance2BetweenPoints(closestPoint, x);
    }
    return 0;
  }
}

//----------------------------------------------------------------------------
//
// Compute iso-parametric interpolation functions
//
void vtkHexagonalPrism::InterpolationFunctions(const double pcoords[3], double sf[12])
{
  double r, s, t;
  r = pcoords[0];
  s = pcoords[1];
  t = pcoords[2];
  const double a = EXPRA;
  const double b = EXPRB;

  // clang-format off
  // First hexagon
  sf[0]  = -16./3. * (r - a  ) * (r - b) * (s - 1.0 ) * (t - 1.0);
  sf[1]  =  16./3. * (r - 0.5) * (r - b) * (s - 0.75) * (t - 1.0);
  sf[2]  = -16./3. * (r - 0.5) * (r - b) * (s - 0.25) * (t - 1.0);
  sf[3]  =  16./3. * (r - a  ) * (r - b) * (s - 0.0 ) * (t - 1.0);
  sf[4]  = -16./3. * (r - 0.5) * (r - a) * (s - 0.25) * (t - 1.0);
  sf[5]  =  16./3. * (r - 0.5) * (r - a) * (s - 0.75) * (t - 1.0);

  // Second hexagon
  sf[6]  =  16./3. * (r - a  ) * (r - b) * (s - 1.0 ) * (t - 0.0);
  sf[7]  = -16./3. * (r - 0.5) * (r - b) * (s - 0.75) * (t - 0.0);
  sf[8]  =  16./3. * (r - 0.5) * (r - b) * (s - 0.25) * (t - 0.0);
  sf[9]  = -16./3. * (r - a  ) * (r - b) * (s - 0.0 ) * (t - 0.0);
  sf[10] =  16./3. * (r - 0.5) * (r - a) * (s - 0.25) * (t - 0.0);
  sf[11] = -16./3. * (r - 0.5) * (r - a) * (s - 0.75) * (t - 0.0);
  // clang-format on
}

//----------------------------------------------------------------------------
void vtkHexagonalPrism::InterpolationDerivs(const double pcoords[3], double derivs[36])
{
  double r, s, t;
  r = pcoords[0];
  s = pcoords[1];
  t = pcoords[2];
  const double a = EXPRA;
  const double b = EXPRB;
  // note: a+b=1.0

  // clang-format off
  // r-derivatives
  // First hexagon
  derivs[0]  = -16./3. * ( 2 * r - 1.0)     * (s - 1.0 ) * (t - 1.0);
  derivs[1]  =  16./3. * ( 2 * r - b - 0.5) * (s - 0.75) * (t - 1.0);
  derivs[2]  = -16./3. * ( 2 * r - b - 0.5) * (s - 0.25) * (t - 1.0);
  derivs[3]  =  16./3. * ( 2 * r - 1.0)     * (s - 0.0 ) * (t - 1.0);
  derivs[4]  = -16./3. * ( 2 * r - a - 0.5) * (s - 0.25) * (t - 1.0);
  derivs[5]  =  16./3. * ( 2 * r - a - 0.5) * (s - 0.75) * (t - 1.0);
  // Second hexagon
  derivs[6]  =  16./3. * ( 2 * r - 1.0)     * (s - 1.0 ) * (t - 0.0);
  derivs[7]  = -16./3. * ( 2 * r - b - 0.5) * (s - 0.75) * (t - 0.0);
  derivs[8]  =  16./3. * ( 2 * r - b - 0.5) * (s - 0.25) * (t - 0.0);
  derivs[9]  = -16./3. * ( 2 * r - 1.0)     * (s - 0.0 ) * (t - 0.0);
  derivs[10] =  16./3. * ( 2 * r - a - 0.5) * (s - 0.25) * (t - 0.0);
  derivs[11] = -16./3. * ( 2 * r - a - 0.5) * (s - 0.75) * (t - 0.0);

  // s-derivatives
  // First hexagon
  derivs[12] = -16./3. * (r - a  ) * (r - b) * (t - 1.0);
  derivs[13] =  16./3. * (r - 0.5) * (r - b) * (t - 1.0);
  derivs[14] = -16./3. * (r - 0.5) * (r - b) * (t - 1.0);
  derivs[15] =  16./3. * (r - a  ) * (r - b) * (t - 1.0);
  derivs[16] = -16./3. * (r - 0.5) * (r - a) * (t - 1.0);
  derivs[17] =  16./3. * (r - 0.5) * (r - a) * (t - 1.0);
  // Second hexagon
  derivs[18] =  16./3. * (r - a  ) * (r - b) * (t - 0.0);
  derivs[19] = -16./3. * (r - 0.5) * (r - b) * (t - 0.0);
  derivs[20] =  16./3. * (r - 0.5) * (r - b) * (t - 0.0);
  derivs[21] = -16./3. * (r - a  ) * (r - b) * (t - 0.0);
  derivs[22] =  16./3. * (r - 0.5) * (r - a) * (t - 0.0);
  derivs[23] = -16./3. * (r - 0.5) * (r - a) * (t - 0.0);

  // t-derivatives
  // First hexagon
  derivs[24] = -16./3. * (r - a  ) * (r - b) * (s - 1.0 );
  derivs[25] =  16./3. * (r - 0.5) * (r - b) * (s - 0.75);
  derivs[26] = -16./3. * (r - 0.5) * (r - b) * (s - 0.25);
  derivs[27] =  16./3. * (r - a  ) * (r - b) * (s - 0.0 );
  derivs[28] = -16./3. * (r - 0.5) * (r - a) * (s - 0.25);
  derivs[29] =  16./3. * (r - 0.5) * (r - a) * (s - 0.75);
  // Second hexagon
  derivs[30] =  16./3. * (r - a  ) * (r - b) * (s - 1.0 );
  derivs[31] = -16./3. * (r - 0.5) * (r - b) * (s - 0.75);
  derivs[32] =  16./3. * (r - 0.5) * (r - b) * (s - 0.25);
  derivs[33] = -16./3. * (r - a  ) * (r - b) * (s - 0.0 );
  derivs[34] =  16./3. * (r - 0.5) * (r - a) * (s - 0.25);
  derivs[35] = -16./3. * (r - 0.5) * (r - a) * (s - 0.75);
  // clang-format on
}

//----------------------------------------------------------------------------
void vtkHexagonalPrism::EvaluateLocation(
  int& vtkNotUsed(subId), const double pcoords[3], double x[3], double* weights)
{
  int i, j;
  double pt[3];

  this->InterpolationFunctions(pcoords, weights);

  x[0] = x[1] = x[2] = 0.0;
  for (i = 0; i < 12; i++)
  {
    this->Points->GetPoint(i, pt);
    for (j = 0; j < 3; j++)
    {
      x[j] += pt[j] * weights[i];
    }
  }
}

namespace
{
//
// Hexagonal prism topology:
//
//      4_____3
//     /\     /\.
//    /10\___/9 \.
//   /   /   \   \.
// 5/___/11  8\___\2
//  \   \     /   /
//   \   \___/   /
//    \ 6/   \7 /
//     \/_____\/
//      0     1
static constexpr vtkIdType edges[vtkHexagonalPrism::NumberOfEdges][2] = {
  { 0, 1 },   // 0
  { 1, 2 },   // 1
  { 2, 3 },   // 2
  { 3, 4 },   // 3
  { 4, 5 },   // 4
  { 5, 0 },   // 5
  { 6, 7 },   // 6
  { 7, 8 },   // 7
  { 8, 9 },   // 8
  { 9, 10 },  // 9
  { 10, 11 }, // 10
  { 11, 6 },  // 11
  { 0, 6 },   // 12
  { 1, 7 },   // 13
  { 2, 8 },   // 14
  { 3, 9 },   // 15
  { 4, 10 },  // 16
  { 5, 11 },  // 17
};

static constexpr vtkIdType
  faces[vtkHexagonalPrism::NumberOfFaces][vtkHexagonalPrism::MaximumFaceSize + 1] = {
    { 0, 5, 4, 3, 2, 1, -1 },     // 0
    { 6, 7, 8, 9, 10, 11, -1 },   // 1
    { 0, 1, 7, 6, -1, -1, -1 },   // 2
    { 1, 2, 8, 7, -1, -1, -1 },   // 3
    { 2, 3, 9, 8, -1, -1, -1 },   // 4
    { 3, 4, 10, 9, -1, -1, -1 },  // 5
    { 4, 5, 11, 10, -1, -1, -1 }, // 6
    { 5, 0, 6, 11, -1, -1, -1 },  // 7
  };

static constexpr vtkIdType edgeToAdjacentFaces[vtkHexagonalPrism::NumberOfEdges][2] = {
  { 0, 2 }, // 0
  { 0, 3 }, // 1
  { 0, 4 }, // 2
  { 0, 5 }, // 3
  { 0, 6 }, // 4
  { 0, 7 }, // 5
  { 1, 2 }, // 6
  { 1, 3 }, // 7
  { 1, 4 }, // 8
  { 1, 5 }, // 9
  { 1, 6 }, // 10
  { 1, 7 }, // 11
  { 2, 7 }, // 12
  { 2, 3 }, // 13
  { 3, 4 }, // 14
  { 4, 5 }, // 15
  { 5, 6 }, // 16
  { 6, 7 }, // 17
};

static constexpr vtkIdType
  faceToAdjacentFaces[vtkHexagonalPrism::NumberOfFaces][vtkHexagonalPrism::MaximumFaceSize] = {
    { 7, 6, 5, 4, 3, 2 },   // 0
    { 2, 3, 4, 5, 6, 7 },   // 1
    { 0, 3, 1, 7, -1, -1 }, // 2
    { 0, 4, 1, 2, -1, -1 }, // 3
    { 0, 5, 1, 3, -1, -1 }, // 4
    { 0, 6, 1, 4, -1, -1 }, // 5
    { 0, 7, 1, 5, -1, -1 }, // 6
    { 0, 2, 1, 6, -1, -1 }, // 7
  };

static constexpr vtkIdType
  pointToIncidentEdges[vtkHexagonalPrism::NumberOfPoints][vtkHexagonalPrism::MaximumValence] = {
    { 0, 12, 5 },   // 0
    { 0, 1, 13 },   // 1
    { 1, 2, 14 },   // 2
    { 2, 3, 15 },   // 3
    { 3, 4, 16 },   // 4
    { 4, 5, 17 },   // 5
    { 6, 11, 12 },  // 6
    { 6, 13, 7 },   // 7
    { 7, 14, 8 },   // 8
    { 8, 15, 9 },   // 9
    { 9, 16, 10 },  // 10
    { 10, 17, 11 }, // 11
  };

static constexpr vtkIdType
  pointToIncidentFaces[vtkHexagonalPrism::NumberOfPoints][vtkHexagonalPrism::MaximumValence] = {
    { 2, 7, 0 }, // 0
    { 0, 3, 2 }, // 1
    { 0, 4, 3 }, // 2
    { 0, 5, 4 }, // 3
    { 0, 6, 5 }, // 4
    { 0, 7, 6 }, // 5
    { 1, 7, 2 }, // 6
    { 2, 3, 1 }, // 7
    { 3, 4, 1 }, // 8
    { 4, 5, 1 }, // 9
    { 5, 6, 1 }, // 10
    { 6, 7, 1 }, // 11
  };

static constexpr vtkIdType
  pointToOneRingPoints[vtkHexagonalPrism::NumberOfPoints][vtkHexagonalPrism::MaximumValence] = {
    { 1, 6, 5 },  // 0
    { 0, 2, 7 },  // 1
    { 1, 3, 8 },  // 2
    { 2, 4, 9 },  // 3
    { 3, 5, 10 }, // 4
    { 4, 0, 11 }, // 5
    { 7, 11, 0 }, // 6
    { 6, 1, 8 },  // 7
    { 7, 2, 9 },  // 8
    { 8, 3, 10 }, // 9
    { 9, 4, 11 }, // 10
    { 10, 5, 6 }, // 11
  };

static constexpr vtkIdType numberOfPointsInFace[vtkHexagonalPrism::NumberOfFaces] = {
  6, // 0
  6, // 1
  4, // 2
  4, // 3
  4, // 4
  4, // 5
  4, // 6
  4  // 7
};

}

//----------------------------------------------------------------------------
bool vtkHexagonalPrism::GetCentroid(double centroid[3]) const
{
  return vtkHexagonalPrism::ComputeCentroid(this->Points, nullptr, centroid);
}

//----------------------------------------------------------------------------
bool vtkHexagonalPrism::ComputeCentroid(
  vtkPoints* points, const vtkIdType* pointIds, double centroid[3])
{
  double p[3];
  if (!pointIds)
  {
    vtkPolygon::ComputeCentroid(points, numberOfPointsInFace[0], faces[0], centroid);
    vtkPolygon::ComputeCentroid(points, numberOfPointsInFace[1], faces[1], p);
  }
  else
  {
    vtkIdType facePointsIds[6] = { pointIds[faces[0][0]], pointIds[faces[0][1]],
      pointIds[faces[0][2]], pointIds[faces[0][3]], pointIds[faces[0][4]], pointIds[faces[0][5]] };
    vtkPolygon::ComputeCentroid(points, numberOfPointsInFace[0], facePointsIds, centroid);
    facePointsIds[0] = pointIds[faces[1][0]];
    facePointsIds[1] = pointIds[faces[1][1]];
    facePointsIds[2] = pointIds[faces[1][2]];
    facePointsIds[3] = pointIds[faces[1][3]];
    facePointsIds[4] = pointIds[faces[1][4]];
    facePointsIds[5] = pointIds[faces[1][5]];
    vtkPolygon::ComputeCentroid(points, numberOfPointsInFace[1], facePointsIds, p);
  }
  centroid[0] += p[0];
  centroid[1] += p[1];
  centroid[2] += p[2];
  centroid[0] *= 0.5;
  centroid[1] *= 0.5;
  centroid[2] *= 0.5;
  return true;
}

//----------------------------------------------------------------------------
bool vtkHexagonalPrism::IsInsideOut()
{
  double n0[3], n1[3];
  vtkPolygon::ComputeNormal(this->Points, numberOfPointsInFace[0], faces[0], n0);
  vtkPolygon::ComputeNormal(this->Points, numberOfPointsInFace[1], faces[1], n1);
  return vtkMath::Dot(n0, n1) > 0.0;
}

//----------------------------------------------------------------------------
// Returns the closest face to the point specified. Closeness is measured
// parametrically.
int vtkHexagonalPrism::CellBoundary(int subId, const double pcoords[3], vtkIdList* pts)
{
  // load coordinates
  double* points = this->GetParametricCoords();
  for (int i = 0; i < 6; i++)
  {
    this->Polygon->PointIds->SetId(i, i);
    this->Polygon->Points->SetPoint(i, &points[3 * i]);
  }

  this->Polygon->CellBoundary(subId, pcoords, pts);

  int min = vtkMath::Min(pts->GetId(0), pts->GetId(1));
  int max = vtkMath::Max(pts->GetId(0), pts->GetId(1));

  // Base on the edge find the quad that correspond:
  int index;
  if ((index = (max - min)) > 1)
  {
    index = 7;
  }
  else
  {
    index += min + 1;
  }

  double a[3], b[3], u[3], v[3];
  this->Polygon->Points->GetPoint(pts->GetId(0), a);
  this->Polygon->Points->GetPoint(pts->GetId(1), b);
  u[0] = b[0] - a[0];
  u[1] = b[1] - a[1];
  v[0] = pcoords[0] - a[0];
  v[1] = pcoords[1] - a[1];

  double dot = vtkMath::Dot2D(v, u);
  double uNorm = vtkMath::Norm2D(u);
  if (uNorm != 0.0)
  {
    dot /= uNorm;
  }
  dot = (v[0] * v[0] + v[1] * v[1]) - dot * dot;
  // mathematically dot must be >= zero but, surprise surprise, it can actually
  // be negative
  if (dot > 0)
  {
    dot = sqrt(dot);
  }
  else
  {
    dot = 0;
  }
  const vtkIdType* verts;

  if (pcoords[2] < 0.5)
  {
    // could be closer to face 1
    // compare that distance to the distance to the quad.

    if (dot < pcoords[2])
    {
      // We are closer to the quad face
      verts = faces[index];
      for (int i = 0; i < 4; i++)
      {
        pts->InsertId(i, verts[i]);
      }
    }
    else
    {
      // we are closer to the hexa face 1
      for (int i = 0; i < 6; i++)
      {
        pts->InsertId(i, faces[0][i]);
      }
    }
  }
  else
  {
    // could be closer to face 2
    // compare that distance to the distance to the quad.

    if (dot < (1. - pcoords[2]))
    {
      // We are closer to the quad face
      verts = faces[index];
      for (int i = 0; i < 4; i++)
      {
        pts->InsertId(i, verts[i]);
      }
    }
    else
    {
      // we are closer to the hexa face 2
      for (int i = 0; i < 6; i++)
      {
        pts->InsertId(i, faces[1][i]);
      }
    }
  }

  // determine whether point is inside of hexagon
  if (pcoords[0] < 0.0 || pcoords[0] > 1.0 || pcoords[1] < 0.0 || pcoords[1] > 1.0 ||
    pcoords[2] < 0.0 || pcoords[2] > 1.0)
  {
    return 0;
  }
  else
  {
    return 1;
  }
}

//----------------------------------------------------------------------------
const vtkIdType* vtkHexagonalPrism::GetEdgeToAdjacentFacesArray(vtkIdType edgeId)
{
  assert(edgeId < vtkHexagonalPrism::NumberOfEdges && "edgeId too large");
  return edgeToAdjacentFaces[edgeId];
}

//----------------------------------------------------------------------------
const vtkIdType* vtkHexagonalPrism::GetFaceToAdjacentFacesArray(vtkIdType faceId)
{
  assert(faceId < vtkHexagonalPrism::NumberOfFaces && "faceId too large");
  return faceToAdjacentFaces[faceId];
}

//----------------------------------------------------------------------------
const vtkIdType* vtkHexagonalPrism::GetPointToIncidentEdgesArray(vtkIdType pointId)
{
  assert(pointId < vtkHexagonalPrism::NumberOfPoints && "pointId too large");
  return pointToIncidentEdges[pointId];
}

//----------------------------------------------------------------------------
const vtkIdType* vtkHexagonalPrism::GetPointToIncidentFacesArray(vtkIdType pointId)
{
  assert(pointId < vtkHexagonalPrism::NumberOfPoints && "pointId too large");
  return pointToIncidentFaces[pointId];
}

//----------------------------------------------------------------------------
const vtkIdType* vtkHexagonalPrism::GetPointToOneRingPointsArray(vtkIdType pointId)
{
  assert(pointId < vtkHexagonalPrism::NumberOfPoints && "pointId too large");
  return pointToOneRingPoints[pointId];
}

//----------------------------------------------------------------------------
const vtkIdType* vtkHexagonalPrism::GetEdgeArray(vtkIdType edgeId)
{
  assert(edgeId < vtkHexagonalPrism::NumberOfEdges && "edgeId too large");
  return edges[edgeId];
}

#ifndef VTK_LEGACY_REMOVE
//----------------------------------------------------------------------------
void vtkHexagonalPrism::GetEdgePoints(int edgeId, int*& pts)
{
  VTK_LEGACY_REPLACED_BODY(vtkHexagonalPrism::GetEdgePoints(int, int*&), "VTK 9.0",
    vtkHexagonalPrism::GetEdgePoints(vtkIdType, const vtkIdType*&));
  static std::vector<int> tmp(std::begin(faces[edgeId]), std::end(faces[edgeId]));
  pts = tmp.data();
}

//----------------------------------------------------------------------------
void vtkHexagonalPrism::GetFacePoints(int faceId, int*& pts)
{
  VTK_LEGACY_REPLACED_BODY(vtkHexagonalPrism::GetFacePoints(int, int*&), "VTK 9.0",
    vtkHexagonalPrism::GetFacePoints(vtkIdType, const vtkIdType*&));
  static std::vector<int> tmp(std::begin(faces[faceId]), std::end(faces[faceId]));
  pts = tmp.data();
}
#endif

//----------------------------------------------------------------------------
vtkCell* vtkHexagonalPrism::GetEdge(int edgeId)
{
  const vtkIdType* verts;

  verts = edges[edgeId];

  // load point id's
  this->Line->PointIds->SetId(0, this->PointIds->GetId(verts[0]));
  this->Line->PointIds->SetId(1, this->PointIds->GetId(verts[1]));

  // load coordinates
  this->Line->Points->SetPoint(0, this->Points->GetPoint(verts[0]));
  this->Line->Points->SetPoint(1, this->Points->GetPoint(verts[1]));

  return this->Line;
}

//----------------------------------------------------------------------------
const vtkIdType* vtkHexagonalPrism::GetFaceArray(vtkIdType faceId)
{
  assert(faceId < vtkHexagonalPrism::NumberOfFaces && "faceId too large");
  return faces[faceId];
}

//----------------------------------------------------------------------------
vtkCell* vtkHexagonalPrism::GetFace(int faceId)
{
  const vtkIdType* verts;

  verts = faces[faceId];

  if (verts[4] != -1) // polys cell
  {
    // load point id's
    this->Polygon->PointIds->SetId(0, this->PointIds->GetId(verts[0]));
    this->Polygon->PointIds->SetId(1, this->PointIds->GetId(verts[1]));
    this->Polygon->PointIds->SetId(2, this->PointIds->GetId(verts[2]));
    this->Polygon->PointIds->SetId(3, this->PointIds->GetId(verts[3]));
    this->Polygon->PointIds->SetId(4, this->PointIds->GetId(verts[4]));
    this->Polygon->PointIds->SetId(5, this->PointIds->GetId(verts[5]));

    // load coordinates
    this->Polygon->Points->SetPoint(0, this->Points->GetPoint(verts[0]));
    this->Polygon->Points->SetPoint(1, this->Points->GetPoint(verts[1]));
    this->Polygon->Points->SetPoint(2, this->Points->GetPoint(verts[2]));
    this->Polygon->Points->SetPoint(3, this->Points->GetPoint(verts[3]));
    this->Polygon->Points->SetPoint(4, this->Points->GetPoint(verts[4]));
    this->Polygon->Points->SetPoint(5, this->Points->GetPoint(verts[5]));

    return this->Polygon;
  }
  else
  {
    // load point id's
    this->Quad->PointIds->SetId(0, this->PointIds->GetId(verts[0]));
    this->Quad->PointIds->SetId(1, this->PointIds->GetId(verts[1]));
    this->Quad->PointIds->SetId(2, this->PointIds->GetId(verts[2]));
    this->Quad->PointIds->SetId(3, this->PointIds->GetId(verts[3]));

    // load coordinates
    this->Quad->Points->SetPoint(0, this->Points->GetPoint(verts[0]));
    this->Quad->Points->SetPoint(1, this->Points->GetPoint(verts[1]));
    this->Quad->Points->SetPoint(2, this->Points->GetPoint(verts[2]));
    this->Quad->Points->SetPoint(3, this->Points->GetPoint(verts[3]));

    return this->Quad;
  }
}
//----------------------------------------------------------------------------
//
// Intersect prism faces against line. Each prism face is a quadrilateral.
//
int vtkHexagonalPrism::IntersectWithLine(const double p1[3], const double p2[3], double tol,
  double& t, double x[3], double pcoords[3], int& subId)
{
  int intersection = 0;
  double pt1[3], pt2[3], pt3[3], pt4[3], pt5[3], pt6[3];
  double tTemp;
  double pc[3], xTemp[3], dist2, weights[12];
  int faceNum;

  t = VTK_DOUBLE_MAX;

  // first intersect the penta faces
  for (faceNum = 0; faceNum < 2; faceNum++)
  {
    this->Points->GetPoint(faces[faceNum][0], pt1);
    this->Points->GetPoint(faces[faceNum][1], pt2);
    this->Points->GetPoint(faces[faceNum][2], pt3);
    this->Points->GetPoint(faces[faceNum][3], pt4);
    this->Points->GetPoint(faces[faceNum][4], pt5);
    this->Points->GetPoint(faces[faceNum][5], pt6);

    this->Quad->Points->SetPoint(0, pt1);
    this->Quad->Points->SetPoint(1, pt2);
    this->Quad->Points->SetPoint(2, pt3);
    this->Quad->Points->SetPoint(3, pt4);
    intersection = this->Quad->IntersectWithLine(p1, p2, tol, tTemp, xTemp, pc, subId);

    if (!intersection)
    {
      this->Quad->Points->SetPoint(0, pt4);
      this->Quad->Points->SetPoint(1, pt5);
      this->Quad->Points->SetPoint(2, pt6);
      this->Quad->Points->SetPoint(3, pt1);
      intersection = this->Quad->IntersectWithLine(p1, p2, tol, tTemp, xTemp, pc, subId);
    }

    if (intersection)
    {
      intersection = 1;
      if (tTemp < t)
      {
        t = tTemp;
        x[0] = xTemp[0];
        x[1] = xTemp[1];
        x[2] = xTemp[2];
        switch (faceNum)
        {
          case 0:
            pcoords[0] = pc[0];
            pcoords[1] = pc[1];
            pcoords[2] = 0.0;
            break;

          case 1:
            pcoords[0] = pc[0];
            pcoords[1] = pc[1];
            pcoords[2] = 1.0;
            break;
        }
      }
    }
  }

  // now intersect the quad faces
  for (faceNum = 2; faceNum < 8; faceNum++)
  {
    this->Points->GetPoint(faces[faceNum][0], pt1);
    this->Points->GetPoint(faces[faceNum][1], pt2);
    this->Points->GetPoint(faces[faceNum][2], pt3);
    this->Points->GetPoint(faces[faceNum][3], pt4);

    this->Quad->Points->SetPoint(0, pt1);
    this->Quad->Points->SetPoint(1, pt2);
    this->Quad->Points->SetPoint(2, pt3);
    this->Quad->Points->SetPoint(3, pt4);

    if (this->Quad->IntersectWithLine(p1, p2, tol, tTemp, xTemp, pc, subId))
    {
      intersection = 1;
      if (tTemp < t)
      {
        t = tTemp;
        x[0] = xTemp[0];
        x[1] = xTemp[1];
        x[2] = xTemp[2];
        this->EvaluatePosition(x, xTemp, subId, pcoords, dist2, weights);
      }
    }
  }

  return intersection;
}
//----------------------------------------------------------------------------
int vtkHexagonalPrism::Triangulate(int vtkNotUsed(index), vtkIdList* ptIds, vtkPoints* pts)
{
  ptIds->Reset();
  pts->Reset();

  for (int i = 0; i < 4; i++)
  {
    ptIds->InsertId(i, this->PointIds->GetId(i));
    pts->InsertPoint(i, this->Points->GetPoint(i));
  }

  return 1;
}
//----------------------------------------------------------------------------
//
// Compute derivatives in x-y-z directions. Use chain rule in combination
// with interpolation function derivatives.
//
void vtkHexagonalPrism::Derivatives(
  int vtkNotUsed(subId), const double pcoords[3], const double* values, int dim, double* derivs)
{
  double *jI[3], j0[3], j1[3], j2[3];
  double functionDerivs[36], sum[3], value;
  int i, j, k;

  // compute inverse Jacobian and interpolation function derivatives
  jI[0] = j0;
  jI[1] = j1;
  jI[2] = j2;
  this->JacobianInverse(pcoords, jI, functionDerivs);

  // now compute derivates of values provided
  for (k = 0; k < dim; k++) // loop over values per point
  {
    sum[0] = sum[1] = sum[2] = 0.0;
    for (i = 0; i < 12; i++) // loop over interp. function derivatives
    {
      value = values[dim * i + k];
      sum[0] += functionDerivs[i] * value;
      sum[1] += functionDerivs[12 + i] * value;
      sum[2] += functionDerivs[24 + i] * value;
    }

    for (j = 0; j < 3; j++) // loop over derivative directions
    {
      derivs[3 * k + j] = sum[0] * jI[j][0] + sum[1] * jI[j][1] + sum[2] * jI[j][2];
    }
  }
}
//----------------------------------------------------------------------------
// Given parametric coordinates compute inverse Jacobian transformation
// matrix. Returns 9 elements of 3x3 inverse Jacobian plus interpolation
// function derivatives.
void vtkHexagonalPrism::JacobianInverse(
  const double pcoords[3], double** inverse, double derivs[36])
{
  int i, j;
  double *m[3], m0[3], m1[3], m2[3];
  double x[3];

  // compute interpolation function derivatives
  this->InterpolationDerivs(pcoords, derivs);

  // create Jacobian matrix
  m[0] = m0;
  m[1] = m1;
  m[2] = m2;
  for (i = 0; i < 3; i++) // initialize matrix
  {
    m0[i] = m1[i] = m2[i] = 0.0;
  }

  for (j = 0; j < 12; j++)
  {
    this->Points->GetPoint(j, x);
    for (i = 0; i < 3; i++)
    {
      m0[i] += x[i] * derivs[j];
      m1[i] += x[i] * derivs[12 + j];
      m2[i] += x[i] * derivs[24 + j];
    }
  }

  // now find the inverse
  if (vtkMath::InvertMatrix(m, inverse, 3) == 0)
  {
    vtkErrorMacro(<< "Jacobian inverse not found");
    return;
  }
}

//----------------------------------------------------------------------------
vtkIdType vtkHexagonalPrism::GetPointToOneRingPoints(vtkIdType pointId, const vtkIdType*& pts)
{
  assert(pointId < vtkHexagonalPrism::NumberOfPoints && "pointId too large");
  pts = pointToOneRingPoints[pointId];
  return vtkHexagonalPrism::MaximumValence;
}

//----------------------------------------------------------------------------
vtkIdType vtkHexagonalPrism::GetPointToIncidentFaces(vtkIdType pointId, const vtkIdType*& faceIds)
{
  assert(pointId < vtkHexagonalPrism::NumberOfPoints && "pointId too large");
  faceIds = pointToIncidentFaces[pointId];
  return vtkHexagonalPrism::MaximumValence;
}

//----------------------------------------------------------------------------
vtkIdType vtkHexagonalPrism::GetPointToIncidentEdges(vtkIdType pointId, const vtkIdType*& edgeIds)
{
  assert(pointId < vtkHexagonalPrism::NumberOfPoints && "pointId too large");
  edgeIds = pointToIncidentEdges[pointId];
  return vtkHexagonalPrism::MaximumValence;
}

//----------------------------------------------------------------------------
vtkIdType vtkHexagonalPrism::GetFaceToAdjacentFaces(vtkIdType faceId, const vtkIdType*& faceIds)
{
  assert(faceId < vtkHexagonalPrism::NumberOfFaces && "faceId too large");
  faceIds = faceToAdjacentFaces[faceId];
  return numberOfPointsInFace[faceId];
}

//----------------------------------------------------------------------------
void vtkHexagonalPrism::GetEdgeToAdjacentFaces(vtkIdType edgeId, const vtkIdType*& pts)
{
  assert(edgeId < vtkHexagonalPrism::NumberOfEdges && "edgeId too large");
  pts = edgeToAdjacentFaces[edgeId];
}

//----------------------------------------------------------------------------
void vtkHexagonalPrism::GetEdgePoints(vtkIdType edgeId, const vtkIdType*& pts)
{
  assert(edgeId < vtkHexagonalPrism::NumberOfEdges && "edgeId too large");
  pts = this->GetEdgeArray(edgeId);
}

//----------------------------------------------------------------------------
vtkIdType vtkHexagonalPrism::GetFacePoints(vtkIdType faceId, const vtkIdType*& pts)
{
  assert(faceId < vtkHexagonalPrism::NumberOfFaces && "faceId too large");
  pts = this->GetFaceArray(faceId);
  return numberOfPointsInFace[faceId];
}

static double vtkHexagonalPrismCellPCoords[36] = {
  0.5, 0.0, 0.0,    //
  EXPRA, 0.25, 0.0, //
  EXPRA, 0.75, 0.0, //
  0.5, 1.0, 0.0,    //
  EXPRB, 0.75, 0.0, //
  EXPRB, 0.25, 0.0, //
  0.5, 0.0, 1.0,    //
  EXPRA, 0.25, 1.0, //
  EXPRA, 0.75, 1.0, //
  0.5, 1.0, 1.0,    //
  EXPRB, 0.75, 1.0, //
  EXPRB, 0.25, 1.0, //
};

//----------------------------------------------------------------------------
double* vtkHexagonalPrism::GetParametricCoords()
{
  return vtkHexagonalPrismCellPCoords;
}

//----------------------------------------------------------------------------
void vtkHexagonalPrism::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Line:\n";
  this->Line->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Quad:\n";
  this->Quad->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Polygon:\n";
  this->Polygon->PrintSelf(os, indent.GetNextIndent());
}
