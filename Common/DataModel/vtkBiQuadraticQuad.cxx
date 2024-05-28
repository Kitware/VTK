// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Thanks to Soeren Gebbert  who developed this class and
// integrated it into VTK 5.0.

#include "vtkBiQuadraticQuad.h"

#include "vtkDoubleArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkQuad.h"
#include "vtkQuadraticEdge.h"
#include <algorithm> //std::copy
#include <array>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkBiQuadraticQuad);

//------------------------------------------------------------------------------
// Construct the quad with nine points.
vtkBiQuadraticQuad::vtkBiQuadraticQuad()
{
  this->Edge = vtkQuadraticEdge::New();
  this->Quad = vtkQuad::New();
  this->Points->SetNumberOfPoints(9);
  this->PointIds->SetNumberOfIds(9);
  for (int i = 0; i < 9; i++)
  {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
    this->PointIds->SetId(i, 0);
  }
  this->Scalars = vtkDoubleArray::New();
  this->Scalars->SetNumberOfTuples(4);
}

//------------------------------------------------------------------------------
vtkBiQuadraticQuad::~vtkBiQuadraticQuad()
{
  this->Edge->Delete();
  this->Quad->Delete();

  this->Scalars->Delete();
}

//------------------------------------------------------------------------------
vtkCell* vtkBiQuadraticQuad::GetEdge(int edgeId)
{
  edgeId = (edgeId < 0 ? 0 : (edgeId > 3 ? 3 : edgeId));
  int p = (edgeId + 1) % 4;

  // load point id's
  this->Edge->PointIds->SetId(0, this->PointIds->GetId(edgeId));
  this->Edge->PointIds->SetId(1, this->PointIds->GetId(p));
  this->Edge->PointIds->SetId(2, this->PointIds->GetId(edgeId + 4));

  // load coordinates
  this->Edge->Points->SetPoint(0, this->Points->GetPoint(edgeId));
  this->Edge->Points->SetPoint(1, this->Points->GetPoint(p));
  this->Edge->Points->SetPoint(2, this->Points->GetPoint(edgeId + 4));

  return this->Edge;
}

//------------------------------------------------------------------------------
static int LinearQuads[4][4] = { { 0, 4, 8, 7 }, { 4, 1, 5, 8 }, { 8, 5, 2, 6 }, { 7, 8, 6, 3 } };

//------------------------------------------------------------------------------
int vtkBiQuadraticQuad::EvaluatePosition(const double x[3], double* closestPoint, int& subId,
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

  // four linear quads are used
  for (minDist2 = VTK_DOUBLE_MAX, i = 0; i < 4; i++)
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
      pcoords[1] /= 2.0;
    }
    else if (subId == 1)
    {
      pcoords[0] = 0.5 + (pcoords[0] / 2.0);
      pcoords[1] /= 2.0;
    }
    else if (subId == 2)
    {
      pcoords[0] = 0.5 + (pcoords[0] / 2.0);
      pcoords[1] = 0.5 + (pcoords[1] / 2.0);
    }
    else
    {
      pcoords[0] /= 2.0;
      pcoords[1] = 0.5 + (pcoords[1] / 2.0);
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
      vtkBiQuadraticQuad::InterpolationFunctionsPrivate(pcoords, weights);
    }
  }

  return returnStatus;
}

//------------------------------------------------------------------------------
void vtkBiQuadraticQuad::EvaluateLocation(
  int& vtkNotUsed(subId), const double pcoords[3], double x[3], double* weights)
{
  int i, j;
  const double* pt;

  vtkBiQuadraticQuad::InterpolationFunctionsPrivate(pcoords, weights);

  // Efficient point access
  const auto pointsArray = vtkDoubleArray::FastDownCast(this->Points->GetData());
  if (!pointsArray)
  {
    vtkErrorMacro(<< "Points should be double type");
    return;
  }
  const double* pts = pointsArray->GetPointer(0);

  x[0] = x[1] = x[2] = 0.0;
  for (i = 0; i < 9; i++)
  {
    pt = pts + 3 * i;
    for (j = 0; j < 3; j++)
    {
      x[j] += pt[j] * weights[i];
    }
  }
}

//------------------------------------------------------------------------------
int vtkBiQuadraticQuad::CellBoundary(int subId, const double pcoords[3], vtkIdList* pts)
{
  return this->Quad->CellBoundary(subId, pcoords, pts);
}

//------------------------------------------------------------------------------
void vtkBiQuadraticQuad::Contour(double value, vtkDataArray* cellScalars,
  vtkIncrementalPointLocator* locator, vtkCellArray* verts, vtkCellArray* lines,
  vtkCellArray* polys, vtkPointData* inPd, vtkPointData* outPd, vtkCellData* inCd, vtkIdType cellId,
  vtkCellData* outCd)
{
  // contour each linear quad separately
  for (int i = 0; i < 4; i++)
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
void vtkBiQuadraticQuad::Clip(double value, vtkDataArray* cellScalars,
  vtkIncrementalPointLocator* locator, vtkCellArray* polys, vtkPointData* inPd, vtkPointData* outPd,
  vtkCellData* inCd, vtkIdType cellId, vtkCellData* outCd, int insideOut)
{
  // contour each linear quad separately
  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 4; j++) // for each of the four vertices of the linear quad
    {
      this->Quad->Points->SetPoint(j, this->Points->GetPoint(LinearQuads[i][j]));
      this->Quad->PointIds->SetId(j, this->PointIds->GetId(LinearQuads[i][j]));
      this->Scalars->SetValue(j, cellScalars->GetTuple1(LinearQuads[i][j]));
    }

    this->Quad->Clip(
      value, this->Scalars, locator, polys, inPd, outPd, inCd, cellId, outCd, insideOut);
  }
}

//------------------------------------------------------------------------------
// Line-line intersection. Intersection has to occur within [0,1] parametric
// coordinates and with specified tolerance.
int vtkBiQuadraticQuad::IntersectWithLine(
  const double* p1, const double* p2, double tol, double& t, double* x, double* pcoords, int& subId)
{
  int subTest, i;
  subId = 0;

  // intersect the four linear quads
  for (i = 0; i < 4; i++)
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
int vtkBiQuadraticQuad::TriangulateLocalIds(int vtkNotUsed(index), vtkIdList* ptIds)
{
  constexpr std::array<vtkIdType, 24> localPtIds{ 0, 4, 7, 4, 1, 5, 5, 2, 6, 6, 3, 7, 4, 8, 7, 4, 5,
    8, 5, 6, 8, 6, 7, 8 };
  ptIds->SetNumberOfIds(24);
  std::copy(localPtIds.begin(), localPtIds.end(), ptIds->begin());
  return 1;
}

//------------------------------------------------------------------------------
void vtkBiQuadraticQuad::Derivatives(
  int vtkNotUsed(subId), const double pcoords[3], const double* values, int dim, double* derivs)
{
  double sum[2], p[3], weights[9];
  double functionDerivs[18];
  double *J[3], J0[3], J1[3], J2[3];
  double *JI[3], JI0[3], JI1[3], JI2[3];

  vtkBiQuadraticQuad::InterpolationFunctionsPrivate(pcoords, weights);
  vtkBiQuadraticQuad::InterpolationDerivsPrivate(pcoords, functionDerivs);

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

  for (int i = 0; i < 9; i++)
  {
    this->Points->GetPoint(i, p);
    for (int j = 0; j < 2; j++)
    {
      for (int k = 0; k < 3; k++)
      {
        J[j][k] += p[k] * functionDerivs[j * 9 + i];
      }
    }
  }

  // Compute third row vector in transposed Jacobian and normalize it, so that Jacobian determinant
  // stays the same.
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

  // Loop over "dim" derivative values. For each set of values,
  // compute derivatives
  // in local system and then transform into modelling system.
  // First compute derivatives in local x'-y' coordinate system
  for (int j = 0; j < dim; j++)
  {
    sum[0] = sum[1] = 0.0;
    for (int i = 0; i < 9; i++) // loop over interp. function derivatives
    {
      sum[0] += functionDerivs[i] * values[dim * i + j];
      sum[1] += functionDerivs[9 + i] * values[dim * i + j];
    }
    //    dBydx = sum[0]*JI[0][0] + sum[1]*JI[0][1];
    //    dBydy = sum[0]*JI[1][0] + sum[1]*JI[1][1];

    // Transform into global system (dot product with global axes)
    derivs[3 * j] = sum[0] * JI[0][0] + sum[1] * JI[0][1];
    derivs[3 * j + 1] = sum[0] * JI[1][0] + sum[1] * JI[1][1];
    derivs[3 * j + 2] = sum[0] * JI[2][0] + sum[1] * JI[2][1];
  }
}

//------------------------------------------------------------------------------
// Compute interpolation functions. The first four nodes are the corner
// vertices; the others are mid-edge nodes, the last one is the mid-center
// node.
void vtkBiQuadraticQuad::InterpolationFunctionsPrivate(const double pcoords[3], double weights[9])
{
  // Normally these coordinates are named r and s, but I chose x and y,
  // because you can easily mark and paste these functions to the
  // gnuplot splot function. :D
  double x = pcoords[0];
  double y = pcoords[1];

  // midedge weights
  weights[0] = 4.0 * (1.0 - x) * (x - 0.5) * (1.0 - y) * (y - 0.5);
  weights[1] = -4.0 * (x) * (x - 0.5) * (1.0 - y) * (y - 0.5);
  weights[2] = 4.0 * (x) * (x - 0.5) * (y) * (y - 0.5);
  weights[3] = -4.0 * (1.0 - x) * (x - 0.5) * (y) * (y - 0.5);
  // corner weights
  weights[4] = 8.0 * (x) * (1.0 - x) * (1.0 - y) * (0.5 - y);
  weights[5] = -8.0 * (x) * (0.5 - x) * (1.0 - y) * (y);
  weights[6] = -8.0 * (x) * (1.0 - x) * (y) * (0.5 - y);
  weights[7] = 8.0 * (1.0 - x) * (0.5 - x) * (1.0 - y) * (y);
  // surface center weight
  weights[8] = 16.0 * (x) * (1.0 - x) * (1.0 - y) * (y);
}

//------------------------------------------------------------------------------
// Derivatives in parametric space.
void vtkBiQuadraticQuad::InterpolationDerivsPrivate(const double pcoords[3], double derivs[18])
{
  // Coordinate conversion
  double x = pcoords[0];
  double y = pcoords[1];

  // Derivatives in the x-direction
  // edge
  derivs[0] = 4.0 * (1.5 - 2.0 * x) * (1.0 - y) * (y - 0.5);
  derivs[1] = -4.0 * (2.0 * x - 0.5) * (1.0 - y) * (y - 0.5);
  derivs[2] = 4.0 * (2.0 * x - 0.5) * (y) * (y - 0.5);
  derivs[3] = -4.0 * (1.5 - 2.0 * x) * (y) * (y - 0.5);
  // midedge
  derivs[4] = 8.0 * (1.0 - 2.0 * x) * (1.0 - y) * (0.5 - y);
  derivs[5] = -8.0 * (0.5 - 2.0 * x) * (1.0 - y) * (y);
  derivs[6] = -8.0 * (1.0 - 2.0 * x) * (y) * (0.5 - y);
  derivs[7] = 8.0 * (2.0 * x - 1.5) * (1.0 - y) * (y);
  // center
  derivs[8] = 16.0 * (1.0 - 2.0 * x) * (1.0 - y) * (y);

  // Derivatives in the y-direction
  // edge
  derivs[9] = 4.0 * (1.0 - x) * (x - 0.5) * (1.5 - 2.0 * y);
  derivs[10] = -4.0 * (x) * (x - 0.5) * (1.5 - 2.0 * y);
  derivs[11] = 4.0 * (x) * (x - 0.5) * (2.0 * y - 0.5);
  derivs[12] = -4.0 * (1.0 - x) * (x - 0.5) * (2.0 * y - 0.5);
  // midedge
  derivs[13] = 8.0 * (x) * (1.0 - x) * (2.0 * y - 1.5);
  derivs[14] = -8.0 * (x) * (0.5 - x) * (1.0 - 2.0 * y);
  derivs[15] = -8.0 * (x) * (1.0 - x) * (0.5 - 2.0 * y);
  derivs[16] = 8.0 * (1.0 - x) * (0.5 - x) * (1.0 - 2.0 * y);
  // center
  derivs[17] = 16.0 * (x) * (1.0 - x) * (1.0 - 2.0 * y);
}

//------------------------------------------------------------------------------
static double vtkQQuadCellPCoords[27] = { 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0,
  0.0, 0.5, 0.0, 0.0, 1.0, 0.5, 0.0, 0.5, 1.0, 0.0, 0.0, 0.5, 0.0, 0.5, 0.5, 0.0 };

double* vtkBiQuadraticQuad::GetParametricCoords()
{
  return vtkQQuadCellPCoords;
}

//------------------------------------------------------------------------------
void vtkBiQuadraticQuad::PrintSelf(ostream& os, vtkIndent indent)
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
