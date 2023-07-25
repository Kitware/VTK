// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkTriQuadraticPyramid.h"

#include "vtkBiQuadraticQuad.h"
#include "vtkBiQuadraticTriangle.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPyramid.h"
#include "vtkQuadraticEdge.h"
#include "vtkTetra.h"

#include <cstddef>

VTK_ABI_NAMESPACE_BEGIN
namespace
{
// defined constants used in interpolation functions and their partial derivatives
constexpr double TOL1M20 = 1e-20;
constexpr double ZERO = 0;
constexpr double HALF = 0.5;
constexpr double FOURTH = 0.25;
constexpr double EIGHTH = 0.125;
constexpr double NINTH = 1.0 / 9.0;
constexpr double FOURNINTHS = 4.0 / 9.0;
constexpr double THREESIXTYFOURTHS = 3.0 / 64.0;
constexpr double THREESIXTEENTHS = 3.0 / 16.0;
constexpr double NINESIXTEENTHS = 9.0 / 16.0;
constexpr double SIXTEENTHIRDS = 16.0 / 3.0;
constexpr double ONE = 1;
constexpr double TWO = 2;
constexpr double THREE = 3;
constexpr double FOUR = 4;
constexpr double EIGHT = 8;
constexpr double TWENTYSEVEN = 27;
constexpr double K1 = -FOURTH;
constexpr double K6 = -HALF;
constexpr double K10 = -ONE;
constexpr double K15A = TWENTYSEVEN / EIGHT;
constexpr double K15B = -TWENTYSEVEN / EIGHT;
constexpr double K19 = -SIXTEENTHIRDS;

constexpr vtkIdType PyramidFaces[5][9] = {
  { 0, 3, 2, 1, 8, 7, 6, 5, 13 },
  { 0, 1, 4, 5, 10, 9, 14, 0, 0 },
  { 1, 2, 4, 6, 11, 10, 15, 0, 0 },
  { 2, 3, 4, 7, 12, 11, 16, 0, 0 },
  { 3, 0, 4, 8, 9, 12, 17, 0, 0 },
};

constexpr vtkIdType PyramidEdges[8][3] = {
  { 0, 1, 5 },
  { 1, 2, 6 },
  { 2, 3, 7 },
  { 3, 0, 8 },
  { 0, 4, 9 },
  { 1, 4, 10 },
  { 2, 4, 11 },
  { 3, 4, 12 },
};

constexpr double VTK_DIVERGED = 1.e6;
constexpr int VTK_MAX_ITERATION = 20;
constexpr double VTK_CONVERGED = 1.e-03;

// LinearPyramids are used by Contour() and Clip() methods.
constexpr vtkIdType LinearPyramids[26][5] = {
  // 6 pyramids
  { 0, 5, 13, 8, 9 },
  { 5, 1, 6, 13, 10 },
  { 8, 13, 7, 3, 12 },
  { 13, 6, 2, 7, 11 },
  { 9, 10, 11, 12, 4 },
  { 9, 12, 11, 10, 18 },
  // 20 tetrahedra
  { 13, 6, 11, 15, 0 },
  { 5, 13, 9, 14, 0 },
  { 10, 13, 5, 14, 0 },
  { 7, 13, 11, 16, 0 },
  { 9, 13, 8, 17, 0 },
  { 6, 13, 10, 15, 0 },
  { 12, 13, 7, 16, 0 },
  { 13, 12, 8, 17, 0 },
  { 15, 13, 10, 18, 0 },
  { 13, 14, 10, 18, 0 },
  { 14, 13, 9, 18, 0 },
  { 15, 11, 13, 18, 0 },
  { 16, 13, 11, 18, 0 },
  { 13, 17, 9, 18, 0 },
  { 16, 12, 13, 18, 0 },
  { 17, 13, 12, 18, 0 },
  { 11, 15, 10, 18, 0 },
  { 10, 14, 9, 18, 0 },
  { 12, 16, 11, 18, 0 },
  { 9, 17, 12, 18, 0 },
};

constexpr vtkIdType triangulationPointIds[32][4] = {
  { 13, 6, 11, 15 },
  { 6, 11, 7, 13 },
  { 5, 13, 9, 14 },
  { 6, 5, 1, 10 },
  { 7, 6, 2, 11 },
  { 8, 7, 3, 12 },
  { 5, 8, 0, 9 },
  { 8, 9, 5, 13 },
  { 10, 13, 5, 14 },
  { 7, 12, 8, 13 },
  { 5, 10, 6, 13 },
  { 7, 13, 11, 16 },
  { 9, 13, 8, 17 },
  { 6, 13, 10, 15 },
  { 12, 13, 7, 16 },
  { 13, 12, 8, 17 },
  { 10, 14, 4, 18 },
  { 15, 13, 10, 18 },
  { 4, 15, 10, 18 },
  { 13, 14, 10, 18 },
  { 11, 15, 4, 18 },
  { 4, 14, 9, 18 },
  { 14, 13, 9, 18 },
  { 15, 11, 13, 18 },
  { 4, 16, 11, 18 },
  { 16, 13, 11, 18 },
  { 13, 17, 9, 18 },
  { 9, 17, 4, 18 },
  { 12, 16, 4, 18 },
  { 16, 12, 13, 18 },
  { 4, 17, 12, 18 },
  { 17, 13, 12, 18 },
};

// this array is used in shape functions/derivatives to swap values
constexpr int startSwapId[4] = { 0, 5, 9, 14 };

double vtkTQPyramidCellPCoords[57] = {
  // corner nodes
  0.0, 0.0, 0.5, //
  1.0, 0.0, 0.5, //
  1.0, 1.0, 0.5, //
  0.0, 1.0, 0.5, //
  0.5, 0.5, 1.0, //
  // mid-edge nodes
  0.5, 0.0, 0.5,    //
  1.0, 0.5, 0.5,    //
  0.5, 1.0, 0.5,    //
  0.0, 0.5, 0.5,    //
  0.25, 0.25, 0.75, //
  0.75, 0.25, 0.75, //
  0.75, 0.75, 0.75, //
  0.25, 0.75, 0.75, //
  // mid-face nodes
  0.5, 0.5, 0.5,             //
  0.5, 1.0 / 6.0, 4.0 / 6.0, //
  5.0 / 6.0, 0.5, 4.0 / 6.0, //
  0.5, 5.0 / 6.0, 4.0 / 6.0, //
  1.0 / 6.0, 0.5, 4.0 / 6.0, //
  // volumetric centroid node
  0.5, 0.5, 5.0 / 8.0 //
};
}

vtkStandardNewMacro(vtkTriQuadraticPyramid);

//------------------------------------------------------------------------------
//
// Construct the pyramid with 19 points
//
vtkTriQuadraticPyramid::vtkTriQuadraticPyramid()
{
  this->PointIds->SetNumberOfIds(19);
  this->Points->SetNumberOfPoints(19);
  for (int i = 0; i < 19; i++)
  {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
    this->PointIds->SetId(i, 0);
  }

  this->Scalars->SetNumberOfTuples(5); // vertices of a linear pyramid
}

//------------------------------------------------------------------------------
vtkTriQuadraticPyramid::~vtkTriQuadraticPyramid() = default;

//------------------------------------------------------------------------------
const vtkIdType* vtkTriQuadraticPyramid::GetEdgeArray(vtkIdType edgeId)
{
  return PyramidEdges[edgeId];
}
//------------------------------------------------------------------------------
const vtkIdType* vtkTriQuadraticPyramid::GetFaceArray(vtkIdType faceId)
{
  return PyramidFaces[faceId];
}

//------------------------------------------------------------------------------
vtkCell* vtkTriQuadraticPyramid::GetEdge(int edgeId)
{
  edgeId = (edgeId < 0 ? 0 : (edgeId > 7 ? 7 : edgeId));

  for (int i = 0; i < 3; i++)
  {
    this->Edge->PointIds->SetId(i, this->PointIds->GetId(PyramidEdges[edgeId][i]));
    this->Edge->Points->SetPoint(i, this->Points->GetPoint(PyramidEdges[edgeId][i]));
  }

  return this->Edge;
}

//------------------------------------------------------------------------------
vtkCell* vtkTriQuadraticPyramid::GetFace(int faceId)
{
  faceId = (faceId < 0 ? 0 : (faceId > 4 ? 4 : faceId));

  // load point id's and coordinates
  // be careful with the first one:

  if (faceId > 0)
  {
    for (int i = 0; i < 7; i++)
    {
      this->TriangleFace->PointIds->SetId(i, this->PointIds->GetId(PyramidFaces[faceId][i]));
      this->TriangleFace->Points->SetPoint(i, this->Points->GetPoint(PyramidFaces[faceId][i]));
    }
    return this->TriangleFace;
  }
  else
  {
    for (int i = 0; i < 9; i++)
    {
      this->QuadFace->PointIds->SetId(i, this->PointIds->GetId(PyramidFaces[faceId][i]));
      this->QuadFace->Points->SetPoint(i, this->Points->GetPoint(PyramidFaces[faceId][i]));
    }
    return this->QuadFace;
  }
}

int vtkTriQuadraticPyramid::EvaluatePosition(const double* x, double closestPoint[3], int& subId,
  double pcoords[3], double& dist2, double weights[])
{
  subId = 0;
  // Efficient point access
  const auto pointsArray = vtkDoubleArray::FastDownCast(this->Points->GetData());
  if (!pointsArray)
  {
    vtkErrorMacro(<< "Points should be double type");
    return 0;
  }
  const double* pts = pointsArray->GetPointer(0);

  // There are problems searching for the apex point, so we check if
  // we are there first before doing the full parametric inversion.
  const double* apexPoint;
  apexPoint = pts + 3 * 4;
  dist2 = vtkMath::Distance2BetweenPoints(apexPoint, x);
  double baseMidpoint[3] = { pts[0], pts[1], pts[2] };
  for (int i = 1; i < 4; i++)
  {
    const double* tmp;
    tmp = pts + 3 * i;
    for (int j = 0; j < 3; j++)
    {
      baseMidpoint[j] += tmp[j];
    }
  }
  for (int i = 0; i < 3; i++)
  {
    baseMidpoint[i] /= 4.;
  }
  double length2 = vtkMath::Distance2BetweenPoints(apexPoint, baseMidpoint);

  // we use .001 as the relative tolerance here since that is the same
  // that is used for the interior cell check below, but we need to
  // square it here because we're looking at dist2^2.
  if (dist2 == 0. || (length2 != 0. && dist2 / length2 < 1.e-6))
  {
    pcoords[0] = vtkTQPyramidCellPCoords[3 * 4 + 0]; // apex-X
    pcoords[1] = vtkTQPyramidCellPCoords[3 * 4 + 1]; // apex-Y
    pcoords[2] = vtkTQPyramidCellPCoords[3 * 4 + 2]; // apex-Z
    vtkTriQuadraticPyramid::InterpolationFunctions(pcoords, weights);
    if (closestPoint)
    {
      memcpy(closestPoint, x, 3 * sizeof(double));
      dist2 = 0.;
    }
    return 1;
  }

  double derivs[3 * 19];

  // compute a bound on the volume to get a scale for an acceptable determinant
  double longestEdge = 0;
  const double *pt0, *pt1;
  for (int i = 0; i < 8; i++)
  {
    pt0 = pts + 3 * PyramidEdges[i][0];
    pt1 = pts + 3 * PyramidEdges[i][1];
    longestEdge = std::max(longestEdge, vtkMath::Distance2BetweenPoints(pt0, pt1));
  }
  // longestEdge value is already squared
  double volumeBound = longestEdge * std::sqrt(longestEdge);
  double determinantTolerance = std::min(1e-20, 0.00001 * volumeBound);

  //  set initial position for Newton's method
  double params[3] = { 0.5, 0.5, 0.5 };
  pcoords[0] = pcoords[1] = pcoords[2] = params[0];

  //  enter iteration loop
  int converged = 0;
  for (int iteration = 0; !converged && (iteration < VTK_MAX_ITERATION); iteration++)
  {
    //  calculate element interpolation functions and derivatives
    vtkTriQuadraticPyramid::InterpolationFunctions(pcoords, weights);
    vtkTriQuadraticPyramid::InterpolationDerivs(pcoords, derivs);

    //  calculate newton functions
    double fcol[3] = { 0, 0, 0 }, rcol[3] = { 0, 0, 0 }, scol[3] = { 0, 0, 0 },
           tcol[3] = { 0, 0, 0 };
    const double* pt;
    for (int i = 0; i < 19; i++)
    {
      pt = pts + 3 * i;
      for (int j = 0; j < 3; j++)
      {
        fcol[j] += pt[j] * weights[i];
        rcol[j] += pt[j] * derivs[i];
        scol[j] += pt[j] * derivs[i + 19];
        tcol[j] += pt[j] * derivs[i + 38];
      }
    }

    for (int i = 0; i < 3; i++)
    {
      fcol[i] -= x[i];
    }

    //  compute determinants and generate improvements
    double d = vtkMath::Determinant3x3(rcol, scol, tcol);
    if (std::abs(d) < determinantTolerance)
    {
      vtkDebugMacro(<< "Determinant incorrect, iteration " << iteration);
      return -1;
    }

    pcoords[0] = params[0] - 0.5 * vtkMath::Determinant3x3(fcol, scol, tcol) / d;
    pcoords[1] = params[1] - 0.5 * vtkMath::Determinant3x3(rcol, fcol, tcol) / d;
    pcoords[2] = params[2] - 0.5 * vtkMath::Determinant3x3(rcol, scol, fcol) / d;

    //  check for convergence
    if (((std::abs(pcoords[0] - params[0])) < VTK_CONVERGED) &&
      ((std::abs(pcoords[1] - params[1])) < VTK_CONVERGED) &&
      ((std::abs(pcoords[2] - params[2])) < VTK_CONVERGED))
    {
      converged = 1;
    }
    // Test for bad divergence (S.Hirschberg 11.12.2001)
    else if ((std::abs(pcoords[0]) > VTK_DIVERGED) || (std::abs(pcoords[1]) > VTK_DIVERGED) ||
      (std::abs(pcoords[2]) > VTK_DIVERGED))
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

  //  If not converged, do not modify the parametric coordinates
  if (!converged)
  {
    return -1;
  }

  vtkTriQuadraticPyramid::InterpolationFunctions(pcoords, weights);

  // This is correct in that the XY parametric coordinate plane "shrinks"
  // while Z (which is between 0.5 and 1) increases and X and Y always are between 0 and 1.
  if (pcoords[0] >= -0.001 && pcoords[0] <= 1.001 && pcoords[1] >= -0.001 && pcoords[1] <= 1.001 &&
    pcoords[2] >= 0.499 && pcoords[2] <= 1.001)
  {
    if (closestPoint)
    {
      closestPoint[0] = x[0];
      closestPoint[1] = x[1];
      closestPoint[2] = x[2];
      dist2 = 0.0; // inside pyramid
    }

    return 1;
  }
  else
  {
    double pc[3], w[19];
    // only approximate
    if (closestPoint)
    {
      for (int i = 0; i < 3; i++)
      {
        if (i < 2 && pcoords[i] < 0.0) // this is for X and Y
        {
          pc[i] = 0.0;
        }
        else if (i == 2 && pcoords[i] < 0.5) // this is for Z
        {
          pc[i] = 0.5;
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

//------------------------------------------------------------------------------
void vtkTriQuadraticPyramid::EvaluateLocation(
  int& vtkNotUsed(subId), const double pcoords[3], double x[3], double* weights)
{
  int i, j;
  const double* pt;

  vtkTriQuadraticPyramid::InterpolationFunctions(pcoords, weights);

  // Efficient point access
  const auto pointsArray = vtkDoubleArray::FastDownCast(this->Points->GetData());
  if (!pointsArray)
  {
    vtkErrorMacro(<< "Points should be double type");
    return;
  }
  const double* pts = pointsArray->GetPointer(0);

  x[0] = x[1] = x[2] = 0.0;
  for (i = 0; i < 19; i++)
  {
    pt = pts + 3 * i;
    for (j = 0; j < 3; j++)
    {
      x[j] += pt[j] * weights[i];
    }
  }
}

//------------------------------------------------------------------------------
int vtkTriQuadraticPyramid::CellBoundary(int subId, const double pcoords[3], vtkIdList* pts)
{
  // The parametric coordinates of the 5 corners of vtkTriQuadraticPyramid and vtkPyramid differ.
  // Therefore, there is a need for conversion. So we will create a vtkPyramid using the parametric
  // coordinates of the 5 corners of vtkTriQuadraticPyramid and use EvaluatePosition to extract the
  // correct parametric coordinates.

  for (int j = 0; j < 5; j++) // for each point of pyramid
  {
    this->Pyramid->Points->SetPoint(j, vtkTQPyramidCellPCoords + (static_cast<ptrdiff_t>(3 * j)));
    this->Pyramid->PointIds->SetId(j, j);
  }

  int subIdInterior;
  double pcoordsInterior[3], dist2, weights[5];
  this->Pyramid->EvaluatePosition(pcoords, nullptr, subIdInterior, pcoordsInterior, dist2, weights);

  return this->Pyramid->CellBoundary(subId, pcoordsInterior, pts);
}

//------------------------------------------------------------------------------
void vtkTriQuadraticPyramid::Contour(double value, vtkDataArray* cellScalars,
  vtkIncrementalPointLocator* locator, vtkCellArray* verts, vtkCellArray* lines,
  vtkCellArray* polys, vtkPointData* inPd, vtkPointData* outPd, vtkCellData* inCd,
  vtkIdType vtkNotUsed(cellId), vtkCellData* outCd)
{
  // subdivide into 6 linear pyramids + 20 tetrahedra

  // contour each linear pyramid separately
  this->Scalars->SetNumberOfTuples(5); // num of vertices
  for (int i = 0; i < 6; i++)          // for each pyramid
  {
    for (int j = 0; j < 5; j++) // for each point of pyramid
    {
      this->Pyramid->Points->SetPoint(j, this->Points->GetPoint(LinearPyramids[i][j]));
      this->Pyramid->PointIds->SetId(j, LinearPyramids[i][j]);
      this->Scalars->SetValue(j, cellScalars->GetTuple1(LinearPyramids[i][j]));
    }
    this->Pyramid->Contour(
      value, this->Scalars, locator, verts, lines, polys, inPd, outPd, inCd, i, outCd);
  }

  // contour each linear tetra separately
  this->Scalars->SetNumberOfTuples(4); // num of vertices
  for (int i = 6; i < 26; i++)         // for each tetra
  {
    for (int j = 0; j < 4; j++) // for each point of tetra
    {
      this->Tetra->Points->SetPoint(j, this->Points->GetPoint(LinearPyramids[i][j]));
      this->Tetra->PointIds->SetId(j, LinearPyramids[i][j]);
      this->Scalars->SetValue(j, cellScalars->GetTuple1(LinearPyramids[i][j]));
    }
    this->Tetra->Contour(
      value, this->Scalars, locator, verts, lines, polys, inPd, outPd, inCd, i, outCd);
  }
}

//------------------------------------------------------------------------------
// Line-pyramid intersection. Intersection has to occur within [0,1] parametric
// coordinates and with specified tolerance.
//
int vtkTriQuadraticPyramid::IntersectWithLine(
  const double* p1, const double* p2, double tol, double& t, double* x, double* pcoords, int& subId)
{
  int intersection = 0;
  double tTemp;
  double pc[3], xTemp[3], triFaceWeights[7], point[3];
  int inter, subIdTriFace;

  t = VTK_DOUBLE_MAX;
  for (int faceNum = 0; faceNum < 5; faceNum++)
  {
    // We have 9 nodes on rect face
    // and 7 on triangle faces
    if (faceNum > 0)
    {
      for (int i = 0; i < 7; i++)
      {
        // global points
        this->Points->GetPoint(PyramidFaces[faceNum][i], point);
        this->TriangleFace->Points->SetPoint(i, point);
        this->TriangleFace->PointIds->SetId(i, this->PointIds->GetId(PyramidFaces[faceNum][i]));
        // parametric points
        this->TriangleFace2->Points->SetPoint(
          i, vtkTQPyramidCellPCoords + static_cast<ptrdiff_t>(3 * PyramidFaces[faceNum][i]));
        this->TriangleFace2->PointIds->SetId(i, this->PointIds->GetId(PyramidFaces[faceNum][i]));
      }
      inter = this->TriangleFace->IntersectWithLine(p1, p2, tol, tTemp, xTemp, pc, subId);
    }
    else
    {
      for (int i = 0; i < 9; i++)
      {
        this->Points->GetPoint(PyramidFaces[faceNum][i], point);
        this->QuadFace->Points->SetPoint(i, point);
        this->QuadFace->PointIds->SetId(i, this->PointIds->GetId(PyramidFaces[faceNum][i]));
      }
      inter = this->QuadFace->IntersectWithLine(p1, p2, tol, tTemp, xTemp, pc, subId);
    }
    if (inter)
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
            // The QuadFace is simple because its parametric coordinates have always the same Z,
            // and for this pyramid, the minimum Z is 0.5.
            pcoords[0] = pc[0];
            pcoords[1] = pc[1];
            pcoords[2] = 0.5;
            break;

          default:
            // The TriangleFaces are trickier. We create a TriangleFace2 that has as points
            // the parametric points of the checked face of this pyramid. Using
            // the parametric coordinates (pc) that we got as a result by using
            // TriangleFace->IntersectWithLine, we can acquire the `global` coordinates
            // which are the parametric coordinates of this pyramid.
            this->TriangleFace2->EvaluateLocation(subIdTriFace, pc, pcoords, triFaceWeights);
            break;
        }
      }
    }
  }
  return intersection;
}

//------------------------------------------------------------------------------
int vtkTriQuadraticPyramid::Triangulate(int vtkNotUsed(index), vtkIdList* ptIds, vtkPoints* pts)
{
  // split into 32 tetrahedra
  static constexpr vtkIdType totalTetrahedra = 32;
  static constexpr vtkIdType tetrahedronPoints = 4;
  pts->SetNumberOfPoints(totalTetrahedra * tetrahedronPoints);
  ptIds->SetNumberOfIds(totalTetrahedra * tetrahedronPoints);

  vtkIdType counter = 0;
  for (int i = 0; i < totalTetrahedra; i++)
  {
    for (int j = 0; j < tetrahedronPoints; j++)
    {
      ptIds->SetId(counter, this->PointIds->GetId(triangulationPointIds[i][j]));
      pts->SetPoint(counter, this->Points->GetPoint(triangulationPointIds[i][j]));
      counter++;
    }
  }

  return 1;
}

//------------------------------------------------------------------------------
// Given parametric coordinates compute inverse Jacobian transformation
// matrix. Returns 9 elements of 3x3 inverse Jacobian plus interpolation
// function derivatives.
//
void vtkTriQuadraticPyramid::JacobianInverse(
  const double pcoords[3], double** inverse, double derivs[57])
{
  int i, j;
  double *m[3], m0[3], m1[3], m2[3];
  double x[3];

  // compute interpolation function derivatives
  vtkTriQuadraticPyramid::InterpolationDerivs(pcoords, derivs);

  // create Jacobian matrix
  m[0] = m0;
  m[1] = m1;
  m[2] = m2;
  for (i = 0; i < 3; i++) // initialize matrix
  {
    m0[i] = m1[i] = m2[i] = 0.0;
  }

  for (j = 0; j < 19; j++)
  {
    this->Points->GetPoint(j, x);
    for (i = 0; i < 3; i++)
    {
      m0[i] += x[i] * derivs[j];
      m1[i] += x[i] * derivs[19 + j];
      m2[i] += x[i] * derivs[38 + j];
    }
  }

  // now find the inverse
  if (vtkMath::InvertMatrix(m, inverse, 3) == 0)
  {
    vtkErrorMacro(<< "Jacobian inverse not found");
    return;
  }
}

//------------------------------------------------------------------------------
void vtkTriQuadraticPyramid::Derivatives(
  int vtkNotUsed(subId), const double pcoords[3], const double* values, int dim, double* derivs)
{
  double *jI[3], j0[3], j1[3], j2[3];
  double functionDerivs[3 * 19], sum[3];
  int i, j, k;

  // compute inverse Jacobian and interpolation function derivatives
  jI[0] = j0;
  jI[1] = j1;
  jI[2] = j2;
  this->JacobianInverse(pcoords, jI, functionDerivs);

  // now compute derivatives of values provided
  for (k = 0; k < dim; k++) // loop over values per vertex
  {
    sum[0] = sum[1] = sum[2] = 0.0;
    for (i = 0; i < 19; i++) // loop over interp. function derivatives
    {
      sum[0] += functionDerivs[i] * values[dim * i + k];
      sum[1] += functionDerivs[19 + i] * values[dim * i + k];
      sum[2] += functionDerivs[38 + i] * values[dim * i + k];
    }
    for (j = 0; j < 3; j++) // loop over derivative directions
    {
      derivs[3 * k + j] = sum[0] * jI[j][0] + sum[1] * jI[j][1] + sum[2] * jI[j][2];
    }
  }
}

//------------------------------------------------------------------------------
// Clip this quadratic pyramid using scalar value provided. Like contouring,
// except that it cuts the pyramid to produce tetrahedra.
//
void vtkTriQuadraticPyramid::Clip(double value, vtkDataArray* cellScalars,
  vtkIncrementalPointLocator* locator, vtkCellArray* tets, vtkPointData* inPd, vtkPointData* outPd,
  vtkCellData* inCd, vtkIdType vtkNotUsed(cellId), vtkCellData* outCd, int insideOut)
{
  // subdivide into 6 linear pyramids + 20 tetrahedra

  // contour each linear pyramid separately
  this->Scalars->SetNumberOfTuples(5); // num of vertices
  for (int i = 0; i < 6; i++)          // for each subdivided pyramid
  {
    for (int j = 0; j < 5; j++) // for each of the five vertices of the pyramid
    {
      this->Pyramid->Points->SetPoint(j, this->Points->GetPoint(LinearPyramids[i][j]));
      this->Pyramid->PointIds->SetId(j, LinearPyramids[i][j]);
      this->Scalars->SetValue(j, cellScalars->GetTuple1(LinearPyramids[i][j]));
    }
    this->Pyramid->Clip(
      value, this->Scalars, locator, tets, inPd, outPd, inCd, i, outCd, insideOut);
  }

  this->Scalars->SetNumberOfTuples(4); // num of vertices
  for (int i = 6; i < 26; i++)         // for each subdivided tetra
  {
    for (int j = 0; j < 4; j++) // for each of the four vertices of the tetra
    {
      this->Tetra->Points->SetPoint(j, this->Points->GetPoint(LinearPyramids[i][j]));
      this->Tetra->PointIds->SetId(j, LinearPyramids[i][j]);
      this->Scalars->SetValue(j, cellScalars->GetTuple1(LinearPyramids[i][j]));
    }
    this->Tetra->Clip(value, this->Scalars, locator, tets, inPd, outPd, inCd, i, outCd, insideOut);
  }
}

//------------------------------------------------------------------------------
// Compute interpolation functions for the nineteen nodes.
//
void vtkTriQuadraticPyramid::InterpolationFunctions(const double pcoords[3], double weights[19])
{
  // VTK needs parametric coordinates to be between (0,1). Isoparametric
  // shape functions are formulated between (-1,1). Here we do a
  // coordinate system conversion from (0,1) to (-1,1).
  const double R = 2.0 * (pcoords[0] - 0.5);
  const double S = 2.0 * (pcoords[1] - 0.5);
  const double T = 2.0 * (pcoords[2] - 0.5);

  // Evaluate common R,S,T multiplication terms
  const double RR = R * R;
  const double SS = S * S;
  const double TT = T * T;
  const double RS = R * S;
  const double RT = R * T;
  const double ST = S * T;
  const double RRS = RR * S;
  const double RRT = RR * T;
  const double RSS = R * SS;
  const double SST = SS * T;
  const double RTT = R * TT;
  const double STT = S * TT;
  const double RRSS = RR * SS;

  // Evaluate numerical values of (T-1), (T-1)^2, (T-1)^3
  const double TM = T - 1.0;
  const double TMM = TM * TM;
  const double TMMM = TMM * TM;

  // Evaluate numerical value of 1/(T-1)
  const double MT = std::abs(TM) > TOL1M20 ? ONE / TM : ZERO;
  // Evaluate numerical value of 1/(T-1)^2
  const double MMT = std::abs(TMM) > TOL1M20 ? ONE / TMM : ZERO;
  // Evaluate numerical value of 1/(T-1)^3
  const double MMMT = std::abs(TMMM) > TOL1M20 ? ONE / TMMM : ZERO;

  // Baseline interpolation weights
  weights[0] = K1 * (R + S - ONE) * (RS * MT - R - S + T - ONE);
  weights[1] = K1 * (-R + S - ONE) * (-RS * MT + R - S + T - ONE);
  weights[2] = K1 * (-R - S - ONE) * (RS * MT + R + S + T - ONE);
  weights[3] = K1 * (R - S - ONE) * (-RS * MT - R + S + T - ONE);
  weights[4] = -T + TWO * TT;
  weights[5] = K6 * (-RRS * MT + RR + ST - S - TMM);
  weights[6] = K6 * (RSS * MT - RT + R + SS - TMM);
  weights[7] = K6 * (RRS * MT + RR - ST + S - TMM);
  weights[8] = K6 * (-RSS * MT + RT - R + SS - TMM);
  weights[9] = K10 * (RS + RS * MT - RT - ST - T + TT);
  weights[10] = K10 * (-RS - RS * MT + RT - ST - T + TT);
  weights[11] = K10 * (RS + RS * MT + RT + ST - T + TT);
  weights[12] = K10 * (-RS - RS * MT - RT + ST - T + TT);
  weights[13] = RRSS * MMT - RR - SS + TMM;
  weights[14] = K15A * (RRS + RRS * MT - RRSS * MT - RRSS * MMT + ST - STT + SST);
  weights[15] = K15B * (RSS + RSS * MT + RT - RTT + RRSS * MT + RRSS * MMT - RRT);
  weights[16] = K15B * (RRS + RRS * MT + RRSS * MT + RRSS * MMT + ST - STT - SST);
  weights[17] = K15A * (RSS + RSS * MT + RT - RTT - RRSS * MT - RRSS * MMT + RRT);
  weights[18] = K19 * (RRSS * MMT + RRSS * MMMT - RR - RR * MT - SS - SS * MT - T + TT);

  // Modifications to interpolation weights
  weights[0] = weights[0] + FOURTH * weights[13] + NINTH * (weights[14] + weights[17]) +
    weights[18] * THREESIXTYFOURTHS;
  weights[1] = weights[1] + FOURTH * weights[13] + NINTH * (weights[14] + weights[15]) +
    weights[18] * THREESIXTYFOURTHS;
  weights[2] = weights[2] + FOURTH * weights[13] + NINTH * (weights[15] + weights[16]) +
    weights[18] * THREESIXTYFOURTHS;
  weights[3] = weights[3] + FOURTH * weights[13] + NINTH * (weights[16] + weights[17]) +
    weights[18] * THREESIXTYFOURTHS;
  weights[4] = weights[4] + EIGHTH * weights[18] +
    NINTH * (weights[14] + weights[15] + weights[16] + weights[17]);
  weights[5] = weights[5] - FOURNINTHS * weights[14] - weights[13] * HALF;
  weights[6] = weights[6] - FOURNINTHS * weights[15] - weights[13] * HALF;
  weights[7] = weights[7] - FOURNINTHS * weights[16] - weights[13] * HALF;
  weights[8] = weights[8] - FOURNINTHS * weights[17] - weights[13] * HALF;
  weights[9] =
    weights[9] - FOURNINTHS * (weights[14] + weights[17]) - weights[18] * THREESIXTEENTHS;
  weights[10] =
    weights[10] - FOURNINTHS * (weights[14] + weights[15]) - weights[18] * THREESIXTEENTHS;
  weights[11] =
    weights[11] - FOURNINTHS * (weights[15] + weights[16]) - weights[18] * THREESIXTEENTHS;
  weights[12] =
    weights[12] - FOURNINTHS * (weights[16] + weights[17]) - weights[18] * THREESIXTEENTHS;
  weights[13] = weights[13] - weights[18] * NINESIXTEENTHS;

  // It should be noted that the order of points is slightly different to what Browning described,
  // still CCW though, so that the first parametric node can have x = 0, and y = 0,
  // e.g. P0 -> P2, P1-> P3, P2 -> P0, P3 -> P1. For code-clarity purposes and 1-1 relation with
  // his dissertation, it's better to swap the values at the end.

  for (int i = 0; i < 4; ++i)
  {
    const double temp[4] = { weights[startSwapId[i] + 0], weights[startSwapId[i] + 1],
      weights[startSwapId[i] + 2], weights[startSwapId[i] + 3] };

    weights[startSwapId[i] + 0] = temp[2];
    weights[startSwapId[i] + 1] = temp[3];
    weights[startSwapId[i] + 2] = temp[0];
    weights[startSwapId[i] + 3] = temp[1];
  }
}

//------------------------------------------------------------------------------
// Derivatives in parametric space.
//
void vtkTriQuadraticPyramid::InterpolationDerivs(const double pcoords[3], double derivs[57])
{
  // VTK needs parametric coordinates to be between (0,1). Isoparametric
  // shape functions are formulated between (-1,1). Here we do a
  // coordinate system conversion from (0,1) to (-1,1).
  const double R = 2.0 * (pcoords[0] - 0.5);
  const double S = 2.0 * (pcoords[1] - 0.5);
  const double T = 2.0 * (pcoords[2] - 0.5);

  // Evaluate common R,S,T multiplication terms
  const double RR = R * R;
  const double SS = S * S;
  const double TT = T * T;
  const double RS = R * S;
  const double RT = R * T;
  const double ST = S * T;
  const double RRS = RR * S;
  const double RSS = R * SS;
  const double RRSS = RR * SS;

  // Evaluate numerical values of (T-1), (T-1)^2, (T-1)^3, and (T-1)^4
  const double TM = T - 1.0;
  const double TMM = TM * TM;
  const double TMMM = TMM * TM;
  const double TMMMM = TMMM * TM;

  // Evaluate numerical value of 1/(T-1)
  const double MT = std::abs(TM) > TOL1M20 ? ONE / TM : ZERO;
  // Evaluate numerical value of 1/(T-1)^2
  const double MMT = std::abs(TMM) > TOL1M20 ? ONE / TMM : ZERO;
  // Evaluate numerical value of 1/(T-1)^3
  const double MMMT = std::abs(TMMM) > TOL1M20 ? ONE / TMMM : ZERO;
  // Evaluate numerical value of 1/(T-1)^4
  const double MMMMT = std::abs(TMMMM) > TOL1M20 ? ONE / TMMMM : ZERO;

  // Baseline partial derivative of interpolation functions based on R
  derivs[0] = K1 * (RS * MT - R - S + T + (S * MT - ONE) * (R + S - ONE) - ONE);
  derivs[1] = K1 * (RS * MT - R + S - T + (-S * MT + ONE) * (-R + S - ONE) + ONE);
  derivs[2] = K1 * (-RS * MT - R - S - T + (S * MT + ONE) * (-R - S - ONE) + ONE);
  derivs[3] = K1 * (-RS * MT - R + S + T + (-S * MT - ONE) * (R - S - ONE) - ONE);
  derivs[4] = ZERO;
  derivs[5] = K6 * (-TWO * RS * MT + TWO * R);
  derivs[6] = K6 * (SS * MT - T + ONE);
  derivs[7] = K6 * (TWO * RS * MT + TWO * R);
  derivs[8] = K6 * (-SS * MT + T - ONE);
  derivs[9] = K10 * (S + S * MT - T);
  derivs[10] = K10 * (-S - S * MT + T);
  derivs[11] = K10 * (S + S * MT + T);
  derivs[12] = K10 * (-S - S * MT - T);
  derivs[13] = TWO * RSS * MMT - TWO * R;
  derivs[14] = K15A * (TWO * RS + TWO * RS * MT - TWO * RSS * MT - TWO * RSS * MMT);
  derivs[15] = K15B * (TWO * RSS * MT + TWO * RSS * MMT - TWO * RT + SS + SS * MT + T - TT);
  derivs[16] = K15B * (TWO * RS + TWO * RS * MT + TWO * RSS * MT + TWO * RSS * MMT);
  derivs[17] = K15A * (-TWO * RSS * MT - TWO * RSS * MMT + TWO * RT + SS + SS * MT + T - TT);
  derivs[18] = K19 * (TWO * RSS * MMT + TWO * RSS * MMMT - TWO * R - TWO * R * MT);

  // Baseline partial derivative of interpolation functions based on S
  derivs[19] = K1 * (RS * MT - R - S + T + (R * MT - ONE) * (R + S - ONE) - ONE);
  derivs[20] = K1 * (-RS * MT + R - S + T + (-R * MT - ONE) * (-R + S - ONE) - ONE);
  derivs[21] = K1 * (-RS * MT - R - S - T + (R * MT + ONE) * (-R - S - ONE) + ONE);
  derivs[22] = K1 * (RS * MT + R - S - T + (-R * MT + ONE) * (R - S - ONE) + ONE);
  derivs[23] = ZERO;
  derivs[24] = K6 * (-RR * MT + T - ONE);
  derivs[25] = K6 * (TWO * RS * MT + TWO * S);
  derivs[26] = K6 * (RR * MT - T + ONE);
  derivs[27] = K6 * (-TWO * RS * MT + TWO * S);
  derivs[28] = K10 * (R + R * MT - T);
  derivs[29] = K10 * (-R - R * MT - T);
  derivs[30] = K10 * (R + R * MT + T);
  derivs[31] = K10 * (-R - R * MT + T);
  derivs[32] = TWO * RRS * MMT - TWO * S;
  derivs[33] = K15A * (-TWO * RRS * MT - TWO * RRS * MMT + RR + RR * MT + TWO * ST + T - TT);
  derivs[34] = K15B * (TWO * RS + TWO * RS * MT + TWO * RRS * MT + TWO * RRS * MMT);
  derivs[35] = K15B * (TWO * RRS * MT + TWO * RRS * MMT + RR + RR * MT - TWO * ST + T - TT);
  derivs[36] = K15A * (TWO * RS + TWO * RS * MT - TWO * RRS * MT - TWO * RRS * MMT);
  derivs[37] = K19 * (TWO * RRS * MMT + TWO * RRS * MMMT - TWO * S - TWO * S * MT);

  // Baseline partial derivative of interpolation functions based on T
  derivs[38] = K1 * (-RS * MMT + ONE) * (R + S - ONE);
  derivs[39] = K1 * (RS * MMT + ONE) * (-R + S - ONE);
  derivs[40] = K1 * (-RS * MMT + ONE) * (-R - S - ONE);
  derivs[41] = K1 * (RS * MMT + ONE) * (R - S - ONE);
  derivs[42] = FOUR * T - ONE;
  derivs[43] = K6 * (RRS * MMT + S - TWO * T + TWO);
  derivs[44] = K6 * (-RSS * MMT - R - TWO * T + TWO);
  derivs[45] = K6 * (-RRS * MMT - S - TWO * T + TWO);
  derivs[46] = K6 * (RSS * MMT + R - TWO * T + TWO);
  derivs[47] = K10 * (-RS * MMT - R - S + TWO * T - ONE);
  derivs[48] = K10 * (RS * MMT + R - S + TWO * T - ONE);
  derivs[49] = K10 * (-RS * MMT + R + S + TWO * T - ONE);
  derivs[50] = K10 * (RS * MMT - R + S + TWO * T - ONE);
  derivs[51] = -TWO * RRSS * MMMT + TWO * T - TWO;
  derivs[52] =
    K15A * (-RRS * MMT - RRSS * (-TWO * T + TWO) * MMMMT + RRSS * MMT - TWO * ST + S + SS);
  derivs[53] =
    K15B * (-RSS * MMT - TWO * RT + R + RRSS * (-TWO * T + TWO) * MMMMT - RRSS * MMT - RR);
  derivs[54] =
    K15B * (-RRS * MMT + RRSS * (-TWO * T + TWO) * MMMMT - RRSS * MMT - TWO * ST + S - SS);
  derivs[55] =
    K15A * (-RSS * MMT - TWO * RT + R - RRSS * (-TWO * T + TWO) * MMMMT + RRSS * MMT + RR);
  derivs[56] =
    K19 * (-TWO * RRSS * MMMT - THREE * RRSS * MMMMT + RR * MMT + SS * MMT + TWO * T - ONE);

  // Modifications to partial derivatives of interpolation functions based on R
  derivs[0] = derivs[0] + derivs[13] * FOURTH + derivs[18] * THREESIXTYFOURTHS +
    NINTH * (derivs[14] + derivs[17]);
  derivs[1] = derivs[1] + derivs[13] * FOURTH + derivs[18] * THREESIXTYFOURTHS +
    NINTH * (derivs[14] + derivs[15]);
  derivs[2] = derivs[2] + derivs[13] * FOURTH + derivs[18] * THREESIXTYFOURTHS +
    NINTH * (derivs[15] + derivs[16]);
  derivs[3] = derivs[3] + derivs[13] * FOURTH + derivs[18] * THREESIXTYFOURTHS +
    NINTH * (derivs[16] + derivs[17]);
  derivs[4] =
    derivs[4] + derivs[18] * EIGHTH + NINTH * (derivs[14] + derivs[15] + derivs[16] + derivs[17]);
  derivs[5] = derivs[5] - derivs[13] * HALF - derivs[14] * FOURNINTHS;
  derivs[6] = derivs[6] - derivs[13] * HALF - derivs[15] * FOURNINTHS;
  derivs[7] = derivs[7] - derivs[13] * HALF - derivs[16] * FOURNINTHS;
  derivs[8] = derivs[8] - derivs[13] * HALF - derivs[17] * FOURNINTHS;
  derivs[9] = derivs[9] - derivs[18] * THREESIXTEENTHS + FOURNINTHS * (-derivs[14] - derivs[17]);
  derivs[10] = derivs[10] - derivs[18] * THREESIXTEENTHS + FOURNINTHS * (-derivs[14] - derivs[15]);
  derivs[11] = derivs[11] - derivs[18] * THREESIXTEENTHS + FOURNINTHS * (-derivs[15] - derivs[16]);
  derivs[12] = derivs[12] - derivs[18] * THREESIXTEENTHS + FOURNINTHS * (-derivs[16] - derivs[17]);
  derivs[13] = derivs[13] - derivs[18] * NINESIXTEENTHS;

  // Modifications to partial derivatives of interpolation functions based on S
  derivs[19] = derivs[19] + derivs[32] * FOURTH + derivs[37] * THREESIXTYFOURTHS +
    NINTH * (derivs[33] + derivs[36]);
  derivs[20] = derivs[20] + derivs[32] * FOURTH + derivs[37] * THREESIXTYFOURTHS +
    NINTH * (derivs[33] + derivs[34]);
  derivs[21] = derivs[21] + derivs[32] * FOURTH + derivs[37] * THREESIXTYFOURTHS +
    NINTH * (derivs[34] + derivs[35]);
  derivs[22] = derivs[22] + derivs[32] * FOURTH + derivs[37] * THREESIXTYFOURTHS +
    NINTH * (derivs[35] + derivs[36]);
  derivs[23] =
    derivs[23] + derivs[37] * EIGHTH + NINTH * (derivs[33] + derivs[34] + derivs[35] + derivs[36]);
  derivs[24] = derivs[24] - derivs[32] * HALF - derivs[33] * FOURNINTHS;
  derivs[25] = derivs[25] - derivs[32] * HALF - derivs[34] * FOURNINTHS;
  derivs[26] = derivs[26] - derivs[32] * HALF - derivs[35] * FOURNINTHS;
  derivs[27] = derivs[27] - derivs[32] * HALF - derivs[36] * FOURNINTHS;
  derivs[28] = derivs[28] - derivs[37] * THREESIXTEENTHS + FOURNINTHS * (-derivs[33] - derivs[36]);
  derivs[29] = derivs[29] - derivs[37] * THREESIXTEENTHS + FOURNINTHS * (-derivs[33] - derivs[34]);
  derivs[30] = derivs[30] - derivs[37] * THREESIXTEENTHS + FOURNINTHS * (-derivs[34] - derivs[35]);
  derivs[31] = derivs[31] - derivs[37] * THREESIXTEENTHS + FOURNINTHS * (-derivs[35] - derivs[36]);
  derivs[32] = derivs[32] - derivs[37] * NINESIXTEENTHS;

  // Modifications to partial derivatives of interpolation functions based on T
  derivs[38] = derivs[38] + derivs[51] * FOURTH + derivs[56] * THREESIXTYFOURTHS +
    NINTH * (derivs[52] + derivs[55]);
  derivs[39] = derivs[39] + derivs[51] * FOURTH + derivs[56] * THREESIXTYFOURTHS +
    NINTH * (derivs[52] + derivs[53]);
  derivs[40] = derivs[40] + derivs[51] * FOURTH + derivs[56] * THREESIXTYFOURTHS +
    NINTH * (derivs[53] + derivs[54]);
  derivs[41] = derivs[41] + derivs[51] * FOURTH + derivs[56] * THREESIXTYFOURTHS +
    NINTH * (derivs[54] + derivs[55]);
  derivs[42] =
    derivs[42] + derivs[56] * EIGHTH + NINTH * (derivs[52] + derivs[53] + derivs[54] + derivs[55]);
  derivs[43] = derivs[43] - derivs[51] * HALF - derivs[52] * FOURNINTHS;
  derivs[44] = derivs[44] - derivs[51] * HALF - derivs[53] * FOURNINTHS;
  derivs[45] = derivs[45] - derivs[51] * HALF - derivs[54] * FOURNINTHS;
  derivs[46] = derivs[46] - derivs[51] * HALF - derivs[55] * FOURNINTHS;
  derivs[47] = derivs[47] - derivs[56] * THREESIXTEENTHS + FOURNINTHS * (-derivs[52] - derivs[55]);
  derivs[48] = derivs[48] - derivs[56] * THREESIXTEENTHS + FOURNINTHS * (-derivs[52] - derivs[53]);
  derivs[49] = derivs[49] - derivs[56] * THREESIXTEENTHS + FOURNINTHS * (-derivs[53] - derivs[54]);
  derivs[50] = derivs[50] - derivs[56] * THREESIXTEENTHS + FOURNINTHS * (-derivs[54] - derivs[55]);
  derivs[51] = derivs[51] - derivs[56] * NINESIXTEENTHS;

  // It should be noted that the order of points is slightly different to what Browning described,
  // still CCW though, so that the first parametric node can have x = 0, and y = 0,
  // e.g. P0 -> P2, P1-> P3, P2 -> P0, P3 -> P1. For code-clarity purposes and 1-1 relation with
  // his dissertation, it's better to swap the values at the end.

  for (int i = 0; i < 4; ++i)
  {
    for (int j = 0; j < 57; j += 19)
    {
      const double temp[4] = { derivs[j + startSwapId[i] + 0], derivs[j + startSwapId[i] + 1],
        derivs[j + startSwapId[i] + 2], derivs[j + startSwapId[i] + 3] };

      derivs[j + startSwapId[i] + 0] = temp[2];
      derivs[j + startSwapId[i] + 1] = temp[3];
      derivs[j + startSwapId[i] + 2] = temp[0];
      derivs[j + startSwapId[i] + 3] = temp[1];
    }
  }

  // we compute derivatives in [-1; 1] but we need them in [ 0; 1]
  for (int i = 0; i < 57; i++)
  {
    derivs[i] *= 2;
  }
}

//------------------------------------------------------------------------------
double* vtkTriQuadraticPyramid::GetParametricCoords()
{
  return vtkTQPyramidCellPCoords;
}

//------------------------------------------------------------------------------
// This method is the same as the vtkCell but handles the Z axes differently
// because the parametric coordinates on the Z axis are between 0.5 and 1.
double vtkTriQuadraticPyramid::GetParametricDistance(const double pcoords[3])
{
  int i;
  double pDist, pDistMax = 0.0;

  for (i = 0; i < 3; i++)
  {
    if (i < 2 && pcoords[i] < 0.0) // this is for X and Y
    {
      pDist = 0.0 - pcoords[i];
    }
    else if (i == 2 && pcoords[i] < 0.5) // this is for Z
    {
      pDist = 0.5 - pcoords[i];
    }
    else if (pcoords[i] > 1.0)
    {
      pDist = pcoords[i] - 1.0;
    }
    else // inside the cell in the parametric direction
    {
      pDist = 0.0;
    }
    if (pDist > pDistMax)
    {
      pDistMax = pDist;
    }
  }
  return pDistMax;
}

//------------------------------------------------------------------------------
void vtkTriQuadraticPyramid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Edge:\n";
  this->Edge->PrintSelf(os, indent.GetNextIndent());
  os << indent << "TriangleFace:\n";
  this->TriangleFace->PrintSelf(os, indent.GetNextIndent());
  os << indent << "TriangleFace2:\n";
  this->TriangleFace2->PrintSelf(os, indent.GetNextIndent());
  os << indent << "QuadFace:\n";
  this->QuadFace->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Tetra:\n";
  this->Tetra->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Pyramid:\n";
  this->Pyramid->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Scalars:\n";
  this->Scalars->PrintSelf(os, indent.GetNextIndent());
}
VTK_ABI_NAMESPACE_END
