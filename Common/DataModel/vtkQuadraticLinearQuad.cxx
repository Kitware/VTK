// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Thanks to Soeren Gebbert  who developed this class and
// integrated it into VTK 5.0.

#include "vtkQuadraticLinearQuad.h"

#include "vtkDoubleArray.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkQuad.h"
#include "vtkQuadraticEdge.h"
#include <algorithm> //std::copy
#include <array>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkQuadraticLinearQuad);

//------------------------------------------------------------------------------
// Construct the quadratic linear quad with six points.
vtkQuadraticLinearQuad::vtkQuadraticLinearQuad()
{
  this->Edge = vtkQuadraticEdge::New();
  this->LinEdge = vtkLine::New();
  this->Quad = vtkQuad::New();
  this->Scalars = vtkDoubleArray::New();
  this->Scalars->SetNumberOfTuples(4); // vertices of a linear quad
  this->Points->SetNumberOfPoints(6);
  this->PointIds->SetNumberOfIds(6);
  for (int i = 0; i < 6; i++)
  {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
    this->PointIds->SetId(i, 0);
  }
}

//------------------------------------------------------------------------------
vtkQuadraticLinearQuad::~vtkQuadraticLinearQuad()
{
  this->Edge->Delete();
  this->LinEdge->Delete();
  this->Quad->Delete();
  this->Scalars->Delete();
}

//------------------------------------------------------------------------------
static int LinearQuads[2][4] = {
  { 0, 4, 5, 3 },
  { 4, 1, 2, 5 },
};

static int LinearQuadEdges[4][3] = {
  { 0, 1, 4 },
  { 1, 2, -1 },
  { 2, 3, 5 },
  { 3, 0, -1 },
};

//------------------------------------------------------------------------------
int* vtkQuadraticLinearQuad::GetEdgeArray(vtkIdType edgeId)
{
  return LinearQuadEdges[edgeId];
}

//------------------------------------------------------------------------------
vtkCell* vtkQuadraticLinearQuad::GetEdge(int edgeId)
{
  edgeId = (edgeId < 0 ? 0 : (edgeId > 3 ? 3 : edgeId));

  // We have 2 linear edges
  if (edgeId == 1 || edgeId == 3)
  {
    this->LinEdge->PointIds->SetId(0, this->PointIds->GetId(LinearQuadEdges[edgeId][0]));
    this->LinEdge->PointIds->SetId(1, this->PointIds->GetId(LinearQuadEdges[edgeId][1]));

    this->LinEdge->Points->SetPoint(0, this->Points->GetPoint(LinearQuadEdges[edgeId][0]));
    this->LinEdge->Points->SetPoint(1, this->Points->GetPoint(LinearQuadEdges[edgeId][1]));

    return this->LinEdge;
  }
  // and two quadratic edges
  else // (edgeId == 0 || edgeId == 2)
  {
    this->Edge->PointIds->SetId(0, this->PointIds->GetId(LinearQuadEdges[edgeId][0]));
    this->Edge->PointIds->SetId(1, this->PointIds->GetId(LinearQuadEdges[edgeId][1]));
    this->Edge->PointIds->SetId(2, this->PointIds->GetId(LinearQuadEdges[edgeId][2]));

    this->Edge->Points->SetPoint(0, this->Points->GetPoint(LinearQuadEdges[edgeId][0]));
    this->Edge->Points->SetPoint(1, this->Points->GetPoint(LinearQuadEdges[edgeId][1]));
    this->Edge->Points->SetPoint(2, this->Points->GetPoint(LinearQuadEdges[edgeId][2]));

    return this->Edge;
  }
}

//------------------------------------------------------------------------------
int vtkQuadraticLinearQuad::EvaluatePosition(const double x[3], double* closestPoint, int& subId,
  double pcoords[3], double& minDist2, double* weights)
{
  double pc[3], dist2;
  int ignoreId, i, returnStatus = 0, status;
  double tempWeights[4];
  double closest[3];

  // Efficient point access
  const auto pointsArray = vtkDoubleArray::FastDownCast(this->Points->GetData());
  if (!pointsArray)
  {
    vtkErrorMacro(<< "Points should be double type");
    return 0;
  }
  const double* pts = pointsArray->GetPointer(0);

  // two linear quads are used
  for (minDist2 = VTK_DOUBLE_MAX, i = 0; i < 2; i++)
  {
    this->Quad->Points->SetPoint(0, pts + 3 * LinearQuads[i][0]);
    this->Quad->Points->SetPoint(1, pts + 3 * LinearQuads[i][1]);
    this->Quad->Points->SetPoint(2, pts + 3 * LinearQuads[i][2]);
    this->Quad->Points->SetPoint(3, pts + 3 * LinearQuads[i][3]);

    status = this->Quad->EvaluatePosition(x, closest, ignoreId, pc, dist2, tempWeights);
    if (status != -1 && ((dist2 < minDist2) || ((dist2 == minDist2) && (returnStatus == 0))))
    {
      returnStatus = status;
      minDist2 = dist2;
      subId = i;
      pcoords[0] = pc[0];
      pcoords[1] = pc[1];
    }
  }

  // adjust parametric coordinates
  if (returnStatus != -1)
  {
    if (subId == 0)
    {
      pcoords[0] /= 2.0;
    }
    else if (subId == 1)
    {
      pcoords[0] = 0.5 + (pcoords[0] / 2.0);
    }
    pcoords[2] = 0.0;
    if (closestPoint != nullptr)
    {
      // Compute both closestPoint and weights
      this->EvaluateLocation(subId, pcoords, closestPoint, weights);
    }
    else
    {
      // Compute weights only
      vtkQuadraticLinearQuad::InterpolationFunctions(pcoords, weights);
    }
  }

  return returnStatus;
}

//------------------------------------------------------------------------------
void vtkQuadraticLinearQuad::EvaluateLocation(
  int& vtkNotUsed(subId), const double pcoords[3], double x[3], double* weights)
{
  int i, j;
  const double* pt;

  vtkQuadraticLinearQuad::InterpolationFunctions(pcoords, weights);

  // Efficient point access
  const auto pointsArray = vtkDoubleArray::FastDownCast(this->Points->GetData());
  if (!pointsArray)
  {
    vtkErrorMacro(<< "Points should be double type");
    return;
  }
  const double* pts = pointsArray->GetPointer(0);

  x[0] = x[1] = x[2] = 0.0;
  for (i = 0; i < 6; i++)
  {
    pt = pts + 3 * i;
    for (j = 0; j < 3; j++)
    {
      x[j] += pt[j] * weights[i];
    }
  }
}

//------------------------------------------------------------------------------
int vtkQuadraticLinearQuad::CellBoundary(int subId, const double pcoords[3], vtkIdList* pts)
{
  return this->Quad->CellBoundary(subId, pcoords, pts);
}

//------------------------------------------------------------------------------
void vtkQuadraticLinearQuad::Contour(double value, vtkDataArray* cellScalars,
  vtkIncrementalPointLocator* locator, vtkCellArray* verts, vtkCellArray* lines,
  vtkCellArray* polys, vtkPointData* inPd, vtkPointData* outPd, vtkCellData* inCd, vtkIdType cellId,
  vtkCellData* outCd)
{
  for (int i = 0; i < 2; i++)
  {
    for (int j = 0; j < 4; j++)
    {
      this->Quad->Points->SetPoint(j, this->Points->GetPoint(LinearQuads[i][j]));
      this->Quad->PointIds->SetId(j, this->PointIds->GetId(LinearQuads[i][j]));
      this->Scalars->SetValue(j, cellScalars->GetTuple1(LinearQuads[i][j]));
    }
    this->Quad->Contour(
      value, this->Scalars, locator, verts, lines, polys, inPd, outPd, inCd, cellId, outCd);
  }
}

//------------------------------------------------------------------------------
// Clip this quadratic quad using scalar value provided. Like contouring,
// except that it cuts the quad to produce other quads and triangles.
void vtkQuadraticLinearQuad::Clip(double value, vtkDataArray* cellScalars,
  vtkIncrementalPointLocator* locator, vtkCellArray* polys, vtkPointData* inPd, vtkPointData* outPd,
  vtkCellData* inCd, vtkIdType cellId, vtkCellData* outCd, int insideOut)
{
  for (int i = 0; i < 2; i++)
  {
    for (int j = 0; j < 4; j++) // for each of the four vertices of the linear quad
    {
      this->Quad->Points->SetPoint(j, this->Points->GetPoint(LinearQuads[i][j]));
      this->Quad->PointIds->SetId(j, this->PointIds->GetId(LinearQuads[i][j]));
      this->Scalars->SetTuple(j, cellScalars->GetTuple(LinearQuads[i][j]));
    }
    this->Quad->Clip(
      value, this->Scalars, locator, polys, inPd, outPd, inCd, cellId, outCd, insideOut);
  }
}

//------------------------------------------------------------------------------
// Line-line intersection. Intersection has to occur within [0,1] parametric
// coordinates and with specified tolerance.
int vtkQuadraticLinearQuad::IntersectWithLine(
  const double* p1, const double* p2, double tol, double& t, double* x, double* pcoords, int& subId)
{
  int subTest, i;
  subId = 0;

  // intersect the two linear quads
  for (i = 0; i < 2; i++)
  {
    this->Quad->Points->SetPoint(0, this->Points->GetPoint(LinearQuads[i][0]));
    this->Quad->Points->SetPoint(1, this->Points->GetPoint(LinearQuads[i][1]));
    this->Quad->Points->SetPoint(2, this->Points->GetPoint(LinearQuads[i][2]));
    this->Quad->Points->SetPoint(3, this->Points->GetPoint(LinearQuads[i][3]));

    if (this->Quad->IntersectWithLine(p1, p2, tol, t, x, pcoords, subTest))
    {
      return 1;
    }
  }

  return 0;
}

//------------------------------------------------------------------------------
int vtkQuadraticLinearQuad::TriangulateLocalIds(int vtkNotUsed(index), vtkIdList* ptIds)
{
  // Create four linear triangles:
  // Choose the triangulation that minimizes the edge length
  // across the cell.
  ptIds->SetNumberOfIds(12);
  double d1 = vtkMath::Distance2BetweenPoints(this->Points->GetPoint(0), this->Points->GetPoint(5));
  double d2 = vtkMath::Distance2BetweenPoints(this->Points->GetPoint(3), this->Points->GetPoint(4));
  double d3 = vtkMath::Distance2BetweenPoints(this->Points->GetPoint(4), this->Points->GetPoint(2));
  double d4 = vtkMath::Distance2BetweenPoints(this->Points->GetPoint(5), this->Points->GetPoint(1));

  if (d1 <= d2)
  {
    constexpr std::array<vtkIdType, 6> localPtIds1{ 0, 4, 5, 0, 5, 3 };
    std::copy(localPtIds1.begin(), localPtIds1.end(), ptIds->begin());
  }
  else
  {
    constexpr std::array<vtkIdType, 6> localPtIds2{ 0, 4, 3, 4, 5, 3 };
    std::copy(localPtIds2.begin(), localPtIds2.end(), ptIds->begin());
  }

  if (d3 <= d4)
  {
    constexpr std::array<vtkIdType, 6> localPtIds3{ 4, 1, 2, 4, 2, 5 };
    std::copy(localPtIds3.begin(), localPtIds3.end(), ptIds->begin() + 6);
  }
  else
  {
    constexpr std::array<vtkIdType, 6> localPtIds4{ 4, 1, 5, 1, 2, 5 };
    std::copy(localPtIds4.begin(), localPtIds4.end(), ptIds->begin() + 6);
  }
  return 1;
}

//------------------------------------------------------------------------------
void vtkQuadraticLinearQuad::Derivatives(
  int vtkNotUsed(subId), const double pcoords[3], const double* values, int dim, double* derivs)
{
  double x0[3], x1[3], x2[3], deltaX[3], weights[6];
  int i, j;
  double functionDerivs[12];

  this->Points->GetPoint(0, x0);
  this->Points->GetPoint(1, x1);
  this->Points->GetPoint(2, x2);

  vtkQuadraticLinearQuad::InterpolationFunctions(pcoords, weights);
  vtkQuadraticLinearQuad::InterpolationDerivs(pcoords, functionDerivs);

  for (i = 0; i < 3; i++)
  {
    deltaX[i] = x1[i] - x0[i] - x2[i];
  }
  for (i = 0; i < dim; i++)
  {
    for (j = 0; j < 3; j++)
    {
      if (deltaX[j] != 0)
      {
        derivs[3 * i + j] = (values[2 * i + 1] - values[2 * i]) / deltaX[j];
      }
      else
      {
        derivs[3 * i + j] = 0;
      }
    }
  }
}

//------------------------------------------------------------------------------
// Compute interpolation functions. The first four nodes are the corner
// vertices; the others are mid-edge nodes.
void vtkQuadraticLinearQuad::InterpolationFunctions(const double pcoords[3], double weights[6])
{
  double x = pcoords[0];
  double y = pcoords[1];

  // corners
  weights[0] = -1.0 * (2.0 * x - 1.0) * (x - 1.0) * (y - 1.0);
  weights[1] = -1.0 * (2.0 * x - 1.0) * (x) * (y - 1.0);
  weights[2] = (2.0 * x - 1.0) * (x) * (y);
  weights[3] = (2.0 * x - 1.0) * (x - 1.0) * (y);

  // Edge middle nodes
  weights[4] = 4.0 * (x) * (1.0 - x) * (1.0 - y);
  weights[5] = 4.0 * (x) * (1.0 - x) * (y);
}

//------------------------------------------------------------------------------
// Derivatives in parametric space.
void vtkQuadraticLinearQuad::InterpolationDerivs(const double pcoords[3], double derivs[12])
{
  double x = pcoords[0];
  double y = pcoords[1];

  // Derivatives in the x-direction
  // corners
  derivs[0] = -1.0 * (4.0 * x - 3.0) * (y - 1.0);
  derivs[1] = -1.0 * (4.0 * x - 1.0) * (y - 1.0);
  derivs[2] = (4.0 * x - 1.0) * (y);
  derivs[3] = (4.0 * x - 3.0) * (y);
  // Edge middle nodes
  derivs[4] = 4.0 * (1.0 - 2.0 * x) * (1.0 - y);
  derivs[5] = 4.0 * (1.0 - 2.0 * x) * (y);

  // Derivatives in the y-direction
  // corners
  derivs[6] = -1.0 * (2.0 * x - 1.0) * (x - 1.0);
  derivs[7] = -1.0 * (2.0 * x - 1.0) * (x);
  derivs[8] = (2.0 * x - 1.0) * (x);
  derivs[9] = (2.0 * x - 1.0) * (x - 1.0);
  // Edge middle nodes
  derivs[10] = -4.0 * (x) * (1.0 - x);
  derivs[11] = 4.0 * (x) * (1.0 - x);
}

//------------------------------------------------------------------------------
static double vtkQLinQuadCellPCoords[18] = {
  0.0, 0.0, 0.0, //
  1.0, 0.0, 0.0, //
  1.0, 1.0, 0.0, //
  0.0, 1.0, 0.0, //
  0.5, 0.0, 0.0, //
  0.5, 1.0, 0.0  //
};

double* vtkQuadraticLinearQuad::GetParametricCoords()
{
  return vtkQLinQuadCellPCoords;
}

//------------------------------------------------------------------------------
void vtkQuadraticLinearQuad::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Edge:\n";
  this->Edge->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Quad:\n";
  this->Quad->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Scalars:\n";
  this->Scalars->PrintSelf(os, indent.GetNextIndent());
}
VTK_ABI_NAMESPACE_END
