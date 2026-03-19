// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkQuadraticTriangle.h"

#include "vtkDoubleArray.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkQuadraticEdge.h"
#include "vtkTriangle.h"
#include <algorithm> //std::copy
#include <array>

namespace
{
//------------------------------------------------------------------------------
[[maybe_unused]] constexpr const char* Topology = R"(
   QuadraticTriangle topology:

              2
             / \
            /   \
           5     4
          /       \
         /         \
        0-----3-----1
)";

//------------------------------------------------------------------------------
double ParametricCoords[18] = {
  0.0, 0.0, 0.0, //
  1.0, 0.0, 0.0, //
  0.0, 1.0, 0.0, //
  0.5, 0.0, 0.0, //
  0.5, 0.5, 0.0, //
  0.0, 0.5, 0.0  //
};

constexpr vtkIdType Edges[3][3] = {
  { 0, 1, 3 }, // edge 0, midpoint 3
  { 1, 2, 4 }, // edge 1, midpoint 4
  { 2, 0, 5 }, // edge 2, midpoint 5
};

//------------------------------------------------------------------------------
// order picked carefully for parametric coordinate conversion
vtkIdType LinearCells[4][3] = {
  { 0, 3, 5 },
  { 3, 1, 4 },
  { 5, 4, 2 },
  { 4, 5, 3 },
};
}

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkQuadraticTriangle);

//------------------------------------------------------------------------------
// Construct the line with two points.
vtkQuadraticTriangle::vtkQuadraticTriangle()
{
  this->Edge = vtkSmartPointer<vtkQuadraticEdge>::New();
  this->Face = vtkSmartPointer<vtkTriangle>::New();
  this->Scalars = vtkSmartPointer<vtkDoubleArray>::New();
  this->Scalars->SetNumberOfTuples(3);
  this->Points->SetNumberOfPoints(6);
  this->PointIds->SetNumberOfIds(6);
  for (int i = 0; i < 6; i++)
  {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
    this->PointIds->SetId(i, 0);
  }
}

//------------------------------------------------------------------------------
vtkCell* vtkQuadraticTriangle::GetEdge(int edgeId)
{
  edgeId = std::clamp(edgeId, 0, 2);
  const vtkIdType* verts = Edges[edgeId];

  // load point id's
  this->Edge->PointIds->SetId(0, this->PointIds->GetId(verts[0]));
  this->Edge->PointIds->SetId(1, this->PointIds->GetId(verts[1]));
  this->Edge->PointIds->SetId(2, this->PointIds->GetId(verts[2]));

  // load coordinates
  this->Edge->Points->SetPoint(0, this->Points->GetPoint(verts[0]));
  this->Edge->Points->SetPoint(1, this->Points->GetPoint(verts[1]));
  this->Edge->Points->SetPoint(2, this->Points->GetPoint(verts[2]));

  return this->Edge;
}

//------------------------------------------------------------------------------
int vtkQuadraticTriangle::EvaluatePosition(const double* x, double closestPoint[3], int& subId,
  double pcoords[3], double& minDist2, double weights[])
{
  double pc[3], dist2;
  int ignoreId, returnStatus = 0;
  double tempWeights[3];
  double closest[3];

  // Efficient point access
  const auto pointsArray = vtkDoubleArray::FastDownCast(this->Points->GetData());
  if (!pointsArray)
  {
    vtkErrorMacro(<< "Points should be double type");
    return 0;
  }
  const double* pts = pointsArray->GetPointer(0);

  // four linear triangles are used
  minDist2 = VTK_DOUBLE_MAX;
  for (int i = 0; i < 4; i++)
  {
    this->Face->Points->SetPoint(0, pts + 3 * LinearCells[i][0]);
    this->Face->Points->SetPoint(1, pts + 3 * LinearCells[i][1]);
    this->Face->Points->SetPoint(2, pts + 3 * LinearCells[i][2]);

    int status = this->Face->EvaluatePosition(x, closest, ignoreId, pc, dist2, tempWeights);
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
      pcoords[1] /= 2.0;
    }
    else if (subId == 1)
    {
      pcoords[0] = 0.5 + (pcoords[0] / 2.0);
      pcoords[1] /= 2.0;
    }
    else if (subId == 2)
    {
      pcoords[0] /= 2.0;
      pcoords[1] = 0.5 + (pcoords[1] / 2.0);
    }
    else
    {
      pcoords[0] = 0.5 - pcoords[0] / 2.0;
      pcoords[1] = 0.5 - pcoords[1] / 2.0;
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
      vtkQuadraticTriangle::InterpolationFunctions(pcoords, weights);
    }
  }

  return returnStatus;
}

//------------------------------------------------------------------------------
void vtkQuadraticTriangle::EvaluateLocation(
  int& vtkNotUsed(subId), const double pcoords[3], double x[3], double* weights)
{
  // Efficient point access
  const auto pointsArray = vtkDoubleArray::FastDownCast(this->Points->GetData());
  if (!pointsArray)
  {
    vtkErrorMacro(<< "Points should be double type");
    return;
  }
  const double* pts = pointsArray->GetPointer(0);

  const double* a0 = pts;
  const double* a1 = pts + 3;
  const double* a2 = pts + 6;
  const double* a3 = pts + 9;
  const double* a4 = pts + 12;
  const double* a5 = pts + 15;

  vtkQuadraticTriangle::InterpolationFunctions(pcoords, weights);

  for (int i = 0; i < 3; i++)
  {
    x[i] = a0[i] * weights[0] + a1[i] * weights[1] + a2[i] * weights[2] + a3[i] * weights[3] +
      a4[i] * weights[4] + a5[i] * weights[5];
  }
}

//------------------------------------------------------------------------------
int vtkQuadraticTriangle::CellBoundary(int subId, const double pcoords[3], vtkIdList* pts)
{
  return this->Face->CellBoundary(subId, pcoords, pts);
}

//------------------------------------------------------------------------------
void vtkQuadraticTriangle::Contour(double value, vtkDataArray* cellScalars,
  vtkIncrementalPointLocator* locator, vtkCellArray* verts, vtkCellArray* lines,
  vtkCellArray* polys, vtkPointData* inPd, vtkPointData* outPd, vtkCellData* inCd, vtkIdType cellId,
  vtkCellData* outCd)
{
  for (int i = 0; i < 4; i++)
  {
    this->Face->Points->SetPoint(0, this->Points->GetPoint(LinearCells[i][0]));
    this->Face->Points->SetPoint(1, this->Points->GetPoint(LinearCells[i][1]));
    this->Face->Points->SetPoint(2, this->Points->GetPoint(LinearCells[i][2]));

    if (outPd)
    {
      this->Face->PointIds->SetId(0, this->PointIds->GetId(LinearCells[i][0]));
      this->Face->PointIds->SetId(1, this->PointIds->GetId(LinearCells[i][1]));
      this->Face->PointIds->SetId(2, this->PointIds->GetId(LinearCells[i][2]));
    }

    this->Scalars->SetTuple(0, cellScalars->GetTuple(LinearCells[i][0]));
    this->Scalars->SetTuple(1, cellScalars->GetTuple(LinearCells[i][1]));
    this->Scalars->SetTuple(2, cellScalars->GetTuple(LinearCells[i][2]));

    this->Face->Contour(
      value, this->Scalars, locator, verts, lines, polys, inPd, outPd, inCd, cellId, outCd);
  }
}

//------------------------------------------------------------------------------
// Line-line intersection. Intersection has to occur within [0,1] parametric
// coordinates and with specified tolerance.
int vtkQuadraticTriangle::IntersectWithLine(
  const double* p1, const double* p2, double tol, double& t, double* x, double* pcoords, int& subId)
{
  int subTest;
  subId = 0;

  for (int i = 0; i < 4; i++)
  {
    this->Face->Points->SetPoint(0, this->Points->GetPoint(LinearCells[i][0]));
    this->Face->Points->SetPoint(1, this->Points->GetPoint(LinearCells[i][1]));
    this->Face->Points->SetPoint(2, this->Points->GetPoint(LinearCells[i][2]));

    if (this->Face->IntersectWithLine(p1, p2, tol, t, x, pcoords, subTest))
    {
      return 1;
    }
  }

  return 0;
}

//------------------------------------------------------------------------------
int vtkQuadraticTriangle::TriangulateLocalIds(int vtkNotUsed(index), vtkIdList* ptIds)
{
  // Create four linear triangles
  ptIds->SetNumberOfIds(12);
  constexpr std::array<vtkIdType, 12> localPtIds{ 0, 3, 5, 3, 1, 4, 5, 4, 2, 4, 5, 3 };
  std::copy(localPtIds.begin(), localPtIds.end(), ptIds->begin());
  return 1;
}

//------------------------------------------------------------------------------
void vtkQuadraticTriangle::Derivatives(
  int vtkNotUsed(subId), const double pcoords[3], const double* values, int dim, double* derivs)
{
  double sum[2], p[3];
  double functionDerivs[12];
  double *J[3], J0[3], J1[3], J2[3];
  double *JI[3], JI0[3], JI1[3], JI2[3];

  vtkQuadraticTriangle::InterpolationDerivs(pcoords, functionDerivs);

  // Compute transposed Jacobian and inverse Jacobian
  J[0] = J0;
  J[1] = J1;
  J[2] = J2;
  JI[0] = JI0;
  JI[1] = JI1;
  JI[2] = JI2;
  for (int k = 0; k < 3; k++)
  {
    J0[k] = J1[k] = 0.0;
  }

  for (int i = 0; i < 6; i++)
  {
    this->Points->GetPoint(i, p);
    for (int j = 0; j < 2; j++)
    {
      for (int k = 0; k < 3; k++)
      {
        J[j][k] += p[k] * functionDerivs[j * 6 + i];
      }
    }
  }

  // Compute third row vector in transposed Jacobian and normalize it, so that Jacobian
  // determinant stays the same.
  vtkMath::Cross(J0, J1, J2);
  if (vtkMath::Normalize(J2) == 0.0 || !vtkMath::InvertMatrix(J, JI, 3)) // degenerate
  {
    for (int j = 0; j < dim; j++)
    {
      for (int i = 0; i < 3; i++)
      {
        derivs[j * dim + i] = 0.0;
      }
    }
    return;
  }

  // Loop over "dim" derivative values. For each set of values, compute derivatives
  // in local system and then transform into modelling system.
  // First compute derivatives in local x'-y' coordinate system
  for (int j = 0; j < dim; j++)
  {
    sum[0] = sum[1] = 0.0;
    for (int i = 0; i < 6; i++) // loop over interp. function derivatives
    {
      sum[0] += functionDerivs[i] * values[dim * i + j];
      sum[1] += functionDerivs[6 + i] * values[dim * i + j];
    }

    // Transform into global system (dot product with global axes)
    derivs[3 * j] = sum[0] * JI[0][0] + sum[1] * JI[0][1];
    derivs[3 * j + 1] = sum[0] * JI[1][0] + sum[1] * JI[1][1];
    derivs[3 * j + 2] = sum[0] * JI[2][0] + sum[1] * JI[2][1];
  }
}

//------------------------------------------------------------------------------
// Clip this quadratic triangle using the scalar value provided. Like
// contouring, except that it cuts the triangle to produce other quads
// and triangles.
void vtkQuadraticTriangle::Clip(double value, vtkDataArray* cellScalars,
  vtkIncrementalPointLocator* locator, vtkCellArray* polys, vtkPointData* inPd, vtkPointData* outPd,
  vtkCellData* inCd, vtkIdType cellId, vtkCellData* outCd, int insideOut)
{
  for (int i = 0; i < 4; i++)
  {
    this->Face->Points->SetPoint(0, this->Points->GetPoint(LinearCells[i][0]));
    this->Face->Points->SetPoint(1, this->Points->GetPoint(LinearCells[i][1]));
    this->Face->Points->SetPoint(2, this->Points->GetPoint(LinearCells[i][2]));

    this->Face->PointIds->SetId(0, this->PointIds->GetId(LinearCells[i][0]));
    this->Face->PointIds->SetId(1, this->PointIds->GetId(LinearCells[i][1]));
    this->Face->PointIds->SetId(2, this->PointIds->GetId(LinearCells[i][2]));

    this->Scalars->SetTuple(0, cellScalars->GetTuple(LinearCells[i][0]));
    this->Scalars->SetTuple(1, cellScalars->GetTuple(LinearCells[i][1]));
    this->Scalars->SetTuple(2, cellScalars->GetTuple(LinearCells[i][2]));

    this->Face->Clip(
      value, this->Scalars, locator, polys, inPd, outPd, inCd, cellId, outCd, insideOut);
  }
}

//------------------------------------------------------------------------------
// Compute maximum parametric distance to cell
double vtkQuadraticTriangle::GetParametricDistance(const double pcoords[3])
{
  double pDist, pDistMax = 0.0;
  double pc[3];

  pc[0] = pcoords[0];
  pc[1] = pcoords[1];
  pc[2] = 1.0 - pcoords[0] - pcoords[1];

  for (int i = 0; i < 3; i++)
  {
    if (pc[i] < 0.0)
    {
      pDist = -pc[i];
    }
    else if (pc[i] > 1.0)
    {
      pDist = pc[i] - 1.0;
    }
    else // inside the cell in the parametric direction
    {
      pDist = 0.0;
    }
    pDistMax = std::max(pDist, pDistMax);
  }

  return pDistMax;
}

//------------------------------------------------------------------------------
// Compute interpolation functions. The first three nodes are the triangle
// vertices; the others are mid-edge nodes.
void vtkQuadraticTriangle::InterpolationFunctions(const double pcoords[3], double weights[6])
{
  double r = pcoords[0];
  double s = pcoords[1];
  double t = 1.0 - r - s;

  weights[0] = t * (2.0 * t - 1.0);
  weights[1] = r * (2.0 * r - 1.0);
  weights[2] = s * (2.0 * s - 1.0);
  weights[3] = 4.0 * r * t;
  weights[4] = 4.0 * r * s;
  weights[5] = 4.0 * s * t;
}

//------------------------------------------------------------------------------
// Derivatives in parametric space.
void vtkQuadraticTriangle::InterpolationDerivs(const double pcoords[3], double derivs[12])
{
  double r = pcoords[0];
  double s = pcoords[1];

  // r-derivatives
  derivs[0] = 4.0 * r + 4.0 * s - 3.0;
  derivs[1] = 4.0 * r - 1.0;
  derivs[2] = 0.0;
  derivs[3] = 4.0 - 8.0 * r - 4.0 * s;
  derivs[4] = 4.0 * s;
  derivs[5] = -4.0 * s;

  // s-derivatives
  derivs[6] = 4.0 * r + 4.0 * s - 3.0;
  derivs[7] = 0.0;
  derivs[8] = 4.0 * s - 1.0;
  derivs[9] = -4.0 * r;
  derivs[10] = 4.0 * r;
  derivs[11] = 4.0 - 8.0 * s - 4.0 * r;
}

//------------------------------------------------------------------------------
double* vtkQuadraticTriangle::GetParametricCoords()
{
  return ParametricCoords;
}

//------------------------------------------------------------------------------
void vtkQuadraticTriangle::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Edge:\n";
  this->Edge->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Face:\n";
  this->Face->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Scalars:\n";
  this->Scalars->PrintSelf(os, indent.GetNextIndent());
}
VTK_ABI_NAMESPACE_END
