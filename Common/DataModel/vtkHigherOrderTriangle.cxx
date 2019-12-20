/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHigherOrderTriangle.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHigherOrderTriangle.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkHigherOrderCurve.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkTriangle.h"

#define ENABLE_CACHING
#define SEVEN_POINT_TRIANGLE

//----------------------------------------------------------------------------
vtkHigherOrderTriangle::vtkHigherOrderTriangle()
{
  this->Order = 0;

  this->Face = vtkTriangle::New();
  this->Scalars = vtkDoubleArray::New();
  this->Scalars->SetNumberOfTuples(3);

  this->Points->SetNumberOfPoints(3);
  this->PointIds->SetNumberOfIds(3);
  for (vtkIdType i = 0; i < 3; i++)
  {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
    this->PointIds->SetId(i, 0);
  }
}

//----------------------------------------------------------------------------
vtkHigherOrderTriangle::~vtkHigherOrderTriangle()
{
  this->Face->Delete();
  this->Scalars->Delete();
}

//----------------------------------------------------------------------------
void vtkHigherOrderTriangle::GetEdgeWithoutRationalWeights(vtkHigherOrderCurve* result, int edgeId)
{
  result->PointIds->Reset();
  result->Points->Reset();

  vtkIdType order = this->GetOrder();

  vtkIdType bindex[3] = { 0, 0, 0 };
  bindex[(edgeId + 2) % 3] = order;

  for (vtkIdType i = 0; i <= order; i++)
  {
    vtkIdType triangleIndex = this->ToIndex(bindex);

    // The ordering for points in vtkHigherOrderCurve are: first point, then last
    // point, and then the remaining points in sequence. This loop iterates over
    // the edge in sequence starting with the first point. The following value
    // maps from this iteration loop to the edge's ordering.
    vtkIdType edgeIndex = (i == 0 ? 0 : (i == order ? 1 : i + 1));

    result->GetPointIds()->InsertId(edgeIndex, this->PointIds->GetId(triangleIndex));
    result->GetPoints()->InsertPoint(edgeIndex, this->Points->GetPoint(triangleIndex));

    bindex[(edgeId + 2) % 3]--;
    bindex[edgeId]++;
  }
}

//----------------------------------------------------------------------------
void vtkHigherOrderTriangle::Initialize()
{
  vtkIdType order = this->ComputeOrder();

  if (this->Order != order)
  {
    // Reset our caches
    this->Order = order;

    this->NumberOfSubtriangles = this->ComputeNumberOfSubtriangles();

#ifdef ENABLE_CACHING
    this->BarycentricIndexMap.resize(3 * this->GetPointIds()->GetNumberOfIds());
    for (vtkIdType i = 0; i < this->GetPointIds()->GetNumberOfIds(); i++)
    {
      this->BarycentricIndexMap[3 * i] = -1;
    }

    // we sacrifice memory for efficiency here
    vtkIdType nIndexMap = (this->Order + 1) * (this->Order + 1);
    IndexMap.resize(nIndexMap);
    for (vtkIdType i = 0; i < nIndexMap; i++)
    {
      this->IndexMap[i] = -1;
    }

    vtkIdType nSubtriangles = this->GetNumberOfSubtriangles();
    SubtriangleIndexMap.resize(9 * nSubtriangles);
    for (vtkIdType i = 0; i < nSubtriangles; i++)
    {
      this->SubtriangleIndexMap[9 * i] = -1;
    }
#endif
  }
}

//----------------------------------------------------------------------------
vtkIdType vtkHigherOrderTriangle::ComputeNumberOfSubtriangles()
{
#ifdef SEVEN_POINT_TRIANGLE
  if (this->Points->GetNumberOfPoints() == 7)
  {
    return 6;
  }
#endif
  vtkIdType order = this->GetOrder();
  return order * order;
}

//----------------------------------------------------------------------------
void vtkHigherOrderTriangle::SubtriangleBarycentricPointIndices(
  vtkIdType cellIndex, vtkIdType (&pointBIndices)[3][3])
{
  assert(cellIndex < this->GetNumberOfSubtriangles());

#ifdef SEVEN_POINT_TRIANGLE
  if (this->Points->GetNumberOfPoints() == 7)
  {
    pointBIndices[0][0] = cellIndex;
    if (cellIndex < 3)
    {
      pointBIndices[1][0] = (cellIndex + 3) % 6;
    }
    else
    {
      pointBIndices[1][0] = (cellIndex + 1) % 3;
    }
    pointBIndices[2][0] = 6;
    return;
  }
#endif

#ifdef ENABLE_CACHING
  vtkIdType cellIndexStart = 9 * cellIndex;
  if (this->SubtriangleIndexMap[cellIndexStart] == -1)
#endif
  {
    vtkIdType order = this->GetOrder();

    if (order == 1)
    {
      pointBIndices[0][0] = 0;
      pointBIndices[0][1] = 0;
      pointBIndices[0][2] = 1;
      pointBIndices[1][0] = 1;
      pointBIndices[1][1] = 0;
      pointBIndices[1][2] = 0;
      pointBIndices[2][0] = 0;
      pointBIndices[2][1] = 1;
      pointBIndices[2][2] = 0;
    }
    else
    {
      vtkIdType nRightSideUp = order * (order + 1) / 2;

      if (cellIndex < nRightSideUp)
      {
        // there are nRightSideUp subtriangles whose orientation is the same as
        // the parent triangle. We traverse them here.
        vtkHigherOrderTriangle::BarycentricIndex(cellIndex, pointBIndices[0], order - 1);
        pointBIndices[0][2] += 1;
        pointBIndices[1][0] = pointBIndices[0][0] + 1;
        pointBIndices[1][1] = pointBIndices[0][1];
        pointBIndices[1][2] = pointBIndices[0][2] - 1;
        pointBIndices[2][0] = pointBIndices[0][0];
        pointBIndices[2][1] = pointBIndices[0][1] + 1;
        pointBIndices[2][2] = pointBIndices[0][2] - 1;
      }
      else
      {
        // the remaining subtriangles are inverted with respect to the parent
        // triangle. We traverse them here.
        if (order == 2)
        {
          pointBIndices[0][0] = 1;
          pointBIndices[0][1] = 1;
          pointBIndices[0][2] = 0;
          pointBIndices[1][0] = 0;
          pointBIndices[1][1] = 1;
          pointBIndices[1][2] = 1;
          pointBIndices[2][0] = 1;
          pointBIndices[2][1] = 0;
          pointBIndices[2][2] = 1;
        }
        else
        {
          vtkHigherOrderTriangle::BarycentricIndex(
            cellIndex - nRightSideUp, pointBIndices[1], order - 2);
          pointBIndices[1][1] += 1;
          pointBIndices[1][2] += 1;

          pointBIndices[2][0] = pointBIndices[1][0] + 1;
          pointBIndices[2][1] = pointBIndices[1][1] - 1;
          pointBIndices[2][2] = pointBIndices[1][2];
          pointBIndices[0][0] = pointBIndices[1][0] + 1;
          pointBIndices[0][1] = pointBIndices[1][1];
          pointBIndices[0][2] = pointBIndices[1][2] - 1;
        }
      }
    }
#ifdef ENABLE_CACHING
    for (vtkIdType i = 0; i < 3; i++)
    {
      for (vtkIdType j = 0; j < 3; j++)
      {
        this->SubtriangleIndexMap[cellIndexStart + 3 * i + j] = pointBIndices[i][j];
      }
    }
#endif
  }
#ifdef ENABLE_CACHING
  else
  {
    for (vtkIdType i = 0; i < 3; i++)
    {
      for (vtkIdType j = 0; j < 3; j++)
      {
        pointBIndices[i][j] = this->SubtriangleIndexMap[cellIndexStart + 3 * i + j];
      }
    }
  }
#endif
}

//----------------------------------------------------------------------------
int vtkHigherOrderTriangle::CellBoundary(
  int vtkNotUsed(subId), const double pcoords[3], vtkIdList* pts)
{
  double t1 = pcoords[0] - pcoords[1];
  double t2 = 0.5 * (1.0 - pcoords[0]) - pcoords[1];
  double t3 = 2.0 * pcoords[0] + pcoords[1] - 1.0;

  pts->SetNumberOfIds(2);

  // compare against three lines in parametric space that divide element
  // into three pieces
  if (t1 >= 0.0 && t2 >= 0.0)
  {
    pts->SetId(0, this->PointIds->GetId(0));
    pts->SetId(1, this->PointIds->GetId(1));
  }

  else if (t2 < 0.0 && t3 >= 0.0)
  {
    pts->SetId(0, this->PointIds->GetId(1));
    pts->SetId(1, this->PointIds->GetId(2));
  }

  else //( t1 < 0.0 && t3 < 0.0 )
  {
    pts->SetId(0, this->PointIds->GetId(2));
    pts->SetId(1, this->PointIds->GetId(0));
  }

  if (pcoords[0] < 0.0 || pcoords[1] < 0.0 || pcoords[0] > 1.0 || pcoords[1] > 1.0 ||
    (1.0 - pcoords[0] - pcoords[1]) < 0.0)
  {
    return 0;
  }
  else
  {
    return 1;
  }
}

//----------------------------------------------------------------------------
int vtkHigherOrderTriangle::EvaluatePosition(const double x[3], double closestPoint[3], int& subId,
  double pcoords[3], double& minDist2, double weights[])
{
  double pc[3], dist2, tempWeights[3], closest[3];
  double pcoordsMin[3] = { 0., 0., 0. };
  int returnStatus = 0, status, ignoreId;
  vtkIdType minBIndices[3][3], bindices[3][3], pointIndices[3];

  vtkIdType order = this->GetOrder();
  vtkIdType numberOfSubtriangles = this->GetNumberOfSubtriangles();

  minDist2 = VTK_DOUBLE_MAX;
  for (vtkIdType subCellId = 0; subCellId < numberOfSubtriangles; subCellId++)
  {
    this->SubtriangleBarycentricPointIndices(subCellId, bindices);

    for (vtkIdType i = 0; i < 3; i++)
    {
      pointIndices[i] = this->ToIndex(bindices[i]);
      this->Face->Points->SetPoint(i, this->Points->GetPoint(pointIndices[i]));
    }

    status = this->Face->EvaluatePosition(x, closest, ignoreId, pc, dist2, tempWeights);

    if (status != -1 && dist2 < minDist2)
    {
      returnStatus = status;
      minDist2 = dist2;
      subId = subCellId;
      pcoordsMin[0] = pc[0];
      pcoordsMin[1] = pc[1];
      for (vtkIdType i = 0; i < 3; i++)
      {
        for (vtkIdType j = 0; j < 3; j++)
        {
          minBIndices[i][j] = bindices[i][j];
        }
      }
    }
  }

  // adjust parametric coordinates
  if (returnStatus != -1)
  {
    for (vtkIdType i = 0; i < 3; i++)
    {
      pcoords[i] =
        (i < 2 ? (minBIndices[0][i] + pcoordsMin[0] * (minBIndices[1][i] - minBIndices[0][i]) +
                   pcoordsMin[1] * (minBIndices[2][i] - minBIndices[0][i])) /
              order
               : 0.);
    }

    if (closestPoint != nullptr)
    {
      // Compute both closestPoint and weights
      this->EvaluateLocation(subId, pcoords, closestPoint, weights);
    }
    else
    {
      // Compute weights only
      this->InterpolateFunctions(pcoords, weights);
    }
  }

  return returnStatus;
}

//----------------------------------------------------------------------------
void vtkHigherOrderTriangle::EvaluateLocation(
  int& vtkNotUsed(subId), const double pcoords[3], double x[3], double* weights)
{
  x[0] = x[1] = x[2] = 0.;

  this->InterpolateFunctions(pcoords, weights);

  double p[3];
  vtkIdType nPoints = this->GetPoints()->GetNumberOfPoints();
  for (vtkIdType idx = 0; idx < nPoints; idx++)
  {
    this->Points->GetPoint(idx, p);
    for (vtkIdType jdx = 0; jdx < 3; jdx++)
    {
      x[jdx] += p[jdx] * weights[idx];
    }
  }
}

//----------------------------------------------------------------------------
void vtkHigherOrderTriangle::Contour(double value, vtkDataArray* cellScalars,
  vtkIncrementalPointLocator* locator, vtkCellArray* verts, vtkCellArray* lines,
  vtkCellArray* polys, vtkPointData* inPd, vtkPointData* outPd, vtkCellData* inCd, vtkIdType cellId,
  vtkCellData* outCd)
{
  vtkIdType bindices[3][3];
  vtkIdType numberOfSubtriangles = this->GetNumberOfSubtriangles();

  for (vtkIdType subCellId = 0; subCellId < numberOfSubtriangles; subCellId++)
  {
    this->SubtriangleBarycentricPointIndices(subCellId, bindices);

    for (vtkIdType i = 0; i < 3; i++)
    {
      vtkIdType pointIndex = this->ToIndex(bindices[i]);
      this->Face->Points->SetPoint(i, this->Points->GetPoint(pointIndex));
      if (outPd)
      {
        this->Face->PointIds->SetId(i, this->PointIds->GetId(pointIndex));
      }
      this->Scalars->SetTuple(i, cellScalars->GetTuple(pointIndex));
    }

    this->Face->Contour(
      value, this->Scalars, locator, verts, lines, polys, inPd, outPd, inCd, cellId, outCd);
  }
}

//----------------------------------------------------------------------------
void vtkHigherOrderTriangle::Clip(double value, vtkDataArray* cellScalars,
  vtkIncrementalPointLocator* locator, vtkCellArray* polys, vtkPointData* inPd, vtkPointData* outPd,
  vtkCellData* inCd, vtkIdType cellId, vtkCellData* outCd, int insideOut)
{
  vtkIdType bindices[3][3];
  vtkIdType numberOfSubtriangles = this->GetNumberOfSubtriangles();

  for (vtkIdType subCellId = 0; subCellId < numberOfSubtriangles; subCellId++)
  {
    this->SubtriangleBarycentricPointIndices(subCellId, bindices);

    for (vtkIdType i = 0; i < 3; i++)
    {
      vtkIdType pointIndex = this->ToIndex(bindices[i]);
      this->Face->Points->SetPoint(i, this->Points->GetPoint(pointIndex));
      if (outPd)
      {
        this->Face->PointIds->SetId(i, this->PointIds->GetId(pointIndex));
      }
      this->Scalars->SetTuple(i, cellScalars->GetTuple(pointIndex));
    }

    this->Face->Clip(
      value, this->Scalars, locator, polys, inPd, outPd, inCd, cellId, outCd, insideOut);
  }
}

//----------------------------------------------------------------------------
int vtkHigherOrderTriangle::IntersectWithLine(
  const double* p1, const double* p2, double tol, double& t, double* x, double* pcoords, int& subId)
{
  vtkIdType bindices[3][3];
  vtkIdType order = this->GetOrder();
  vtkIdType numberOfSubtriangles = this->GetNumberOfSubtriangles();
  int subTest;

  t = VTK_DOUBLE_MAX;
  double tTmp;
  double xMin[3], pcoordsMin[3];

  for (vtkIdType subCellId = 0; subCellId < numberOfSubtriangles; subCellId++)
  {
    this->SubtriangleBarycentricPointIndices(subCellId, bindices);

    for (vtkIdType i = 0; i < 3; i++)
    {
      vtkIdType pointIndex = this->ToIndex(bindices[i]);
      this->Face->Points->SetPoint(i, this->Points->GetPoint(pointIndex));
    }

    if (this->Face->IntersectWithLine(p1, p2, tol, tTmp, xMin, pcoordsMin, subTest) && tTmp < t)
    {
      for (vtkIdType i = 0; i < 3; i++)
      {
        x[i] = xMin[i];
        pcoords[i] = (i < 2 ? (bindices[0][i] + pcoordsMin[0] * (bindices[1][i] - bindices[0][i]) +
                                pcoordsMin[1] * (bindices[2][i] - bindices[0][i])) /
              order
                            : 0.);
      }
      t = tTmp;
    }
  }

  subId = 0;
  return (t == VTK_DOUBLE_MAX ? 0 : 1);
}

//----------------------------------------------------------------------------
int vtkHigherOrderTriangle::Triangulate(int vtkNotUsed(index), vtkIdList* ptIds, vtkPoints* pts)
{
  pts->Reset();
  ptIds->Reset();

#ifdef SEVEN_POINT_TRIANGLE
  if (this->Points->GetNumberOfPoints() == 7)
  {
    static constexpr vtkIdType edgeOrder[7] = { 0, 3, 1, 4, 2, 5, 0 };
    pts->SetNumberOfPoints(18);
    ptIds->SetNumberOfIds(18);
    vtkIdType pointId = 0;
    for (vtkIdType i = 0; i < 6; i++)
    {
      ptIds->SetId(pointId, this->PointIds->GetId(edgeOrder[i]));
      pts->SetPoint(pointId, this->Points->GetPoint(edgeOrder[i]));
      pointId++;
      ptIds->SetId(pointId, this->PointIds->GetId(edgeOrder[i + 1]));
      pts->SetPoint(pointId, this->Points->GetPoint(edgeOrder[i + 1]));
      pointId++;
      ptIds->SetId(pointId, this->PointIds->GetId(6));
      pts->SetPoint(pointId, this->Points->GetPoint(6));
      pointId++;
    }
    return 1;
  }
#endif

  vtkIdType bindices[3][3];
  vtkIdType numberOfSubtriangles = this->GetNumberOfSubtriangles();

  pts->SetNumberOfPoints(3 * numberOfSubtriangles);
  ptIds->SetNumberOfIds(3 * numberOfSubtriangles);
  for (vtkIdType subCellId = 0; subCellId < numberOfSubtriangles; subCellId++)
  {
    this->SubtriangleBarycentricPointIndices(subCellId, bindices);

    for (vtkIdType i = 0; i < 3; i++)
    {
      vtkIdType pointIndex = this->ToIndex(bindices[i]);
      ptIds->SetId(3 * subCellId + i, this->PointIds->GetId(pointIndex));
      pts->SetPoint(3 * subCellId + i, this->Points->GetPoint(pointIndex));
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkHigherOrderTriangle::JacobianInverse(
  const double pcoords[3], double** inverse, double* derivs)
{
  // Given parametric coordinates compute inverse Jacobian transformation
  // matrix. Returns 9 elements of 3x3 inverse Jacobian plus interpolation
  // function derivatives.

  int i, j, k;
  double *m[3], m0[3], m1[3], m2[3];
  double x[3];

  vtkIdType numberOfPoints = this->Points->GetNumberOfPoints();

  // compute interpolation function derivatives
  this->InterpolateDerivs(pcoords, derivs);

  // create Jacobian matrix
  m[0] = m0;
  m[1] = m1;
  m[2] = m2;
  for (i = 0; i < 3; i++) // initialize matrix
  {
    m0[i] = m1[i] = m2[i] = 0.0;
  }

  for (j = 0; j < numberOfPoints; j++)
  {
    this->Points->GetPoint(j, x);
    for (i = 0; i < 3; i++)
    {
      for (k = 0; k < this->GetCellDimension(); k++)
      {
        m[k][i] += x[i] * derivs[numberOfPoints * k + j];
      }
    }
  }

  // Compute third row vector in transposed Jacobian and normalize it, so that
  // Jacobian determinant stays the same.
  if (this->GetCellDimension() == 2)
  {
    vtkMath::Cross(m0, m1, m2);
  }

  if (vtkMath::Normalize(m2) == 0.0 || !vtkMath::InvertMatrix(m, inverse, 3))
  {
    vtkErrorMacro(<< "Jacobian inverse not found");
    return;
  }
}

//----------------------------------------------------------------------------
void vtkHigherOrderTriangle::Derivatives(
  int vtkNotUsed(subId), const double pcoords[3], const double* values, int dim, double* derivs)
{
  double *jI[3], j0[3], j1[3], j2[3];
  std::vector<double> fDs(2 * this->Points->GetNumberOfPoints());
  double sum[3];
  int i, j, k;
  vtkIdType numberOfPoints = this->Points->GetNumberOfPoints();

  // compute inverse Jacobian and interpolation function derivatives
  jI[0] = j0;
  jI[1] = j1;
  jI[2] = j2;
  this->JacobianInverse(pcoords, jI, &fDs[0]);

  // now compute derivates of values provided
  for (k = 0; k < dim; k++) // loop over values per vertex
  {
    sum[0] = sum[1] = sum[2] = 0.0;
    for (i = 0; i < numberOfPoints; i++) // loop over interp. function derivatives
    {
      sum[0] += fDs[i] * values[dim * i + k];
      sum[1] += fDs[numberOfPoints + i] * values[dim * i + k];
    }
    for (j = 0; j < 3; j++) // loop over derivative directions
    {
      derivs[3 * k + j] = 0.;
      for (i = 0; i < this->GetCellDimension(); i++)
      {
        derivs[3 * k + j] += sum[i] * jI[j][i];
      }
    }
  }
}

//----------------------------------------------------------------------------
#ifdef SEVEN_POINT_TRIANGLE
namespace
{
double SevenPointTriangleCoords[7 * 3] = { 0., 0., 0., 1., 0., 0., 0., 1., 0., .5, 0., 0., .5, .5,
  0., 0., .5, 0., 1. / 3., 1. / 3., 0. };
}
#endif

void vtkHigherOrderTriangle::SetParametricCoords()
{
#ifdef SEVEN_POINT_TRIANGLE
  if (this->Points->GetNumberOfPoints() == 7)
  {
    return;
  }
#endif

  if (!this->PointParametricCoordinates)
  {
    this->PointParametricCoordinates = vtkSmartPointer<vtkPoints>::New();
    this->PointParametricCoordinates->SetDataTypeToDouble();
  }

  // Ensure Order is up-to-date and check that current point size matches:
  vtkIdType order = this->GetOrder();
  vtkIdType nPoints = (order + 1) * (order + 2) / 2;
  if (this->PointParametricCoordinates->GetNumberOfPoints() != nPoints)
  {
    this->PointParametricCoordinates->Initialize();

    double order_d = static_cast<vtkIdType>(order);

    this->PointParametricCoordinates->SetNumberOfPoints(nPoints);

    double max = static_cast<double>(order);
    double min = 0.;
    vtkIdType pIdx = 0;
    double p[3];
    vtkIdType ord;
    for (ord = order; ord > 0; ord -= 3)
    {
      const double min_over_order = min / order_d;
      const double max_over_order = max / order_d;
      // add the vertex points
      this->PointParametricCoordinates->SetPoint(pIdx++, min_over_order, min_over_order, 0);
      this->PointParametricCoordinates->SetPoint(pIdx++, max_over_order, min_over_order, 0);
      this->PointParametricCoordinates->SetPoint(pIdx++, min_over_order, max_over_order, 0);

      // add the edge points
      if (ord > 1)
      {
        for (vtkIdType dim = 0; dim < 3; dim++)
        {
          p[dim] = p[(dim + 1) % 3] = min_over_order;
          p[(dim + 2) % 3] = max_over_order;
          for (vtkIdType i = 0; i < ord - 1; i++)
          {
            p[dim] += 1. / order_d;
            p[(dim + 2) % 3] -= 1. / order_d;
            this->PointParametricCoordinates->SetPoint(pIdx++, p[0], p[1], 0);
          }
        }
      }
      max -= 2.;
      min += 1.;
    }

    if (ord == 0)
    {
      const double min_over_order = min / order_d;
      this->PointParametricCoordinates->SetPoint(pIdx++, min_over_order, min_over_order, 0);
    }
  }
}

double* vtkHigherOrderTriangle::GetParametricCoords()
{
#ifdef SEVEN_POINT_TRIANGLE
  if (this->Points->GetNumberOfPoints() == 7)
  {
    return SevenPointTriangleCoords;
  }
#endif
  this->SetParametricCoords();

  return vtkDoubleArray::SafeDownCast(this->PointParametricCoordinates->GetData())->GetPointer(0);
}

//----------------------------------------------------------------------------
int vtkHigherOrderTriangle::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = pcoords[1] = 1. / 3.;
  pcoords[2] = 0.;
  return 0;
}

//----------------------------------------------------------------------------
double vtkHigherOrderTriangle::GetParametricDistance(const double pcoords[3])
{
  int i;
  double pDist, pDistMax = 0.0;
  double pc[3];

  pc[0] = pcoords[0];
  pc[1] = pcoords[1];
  pc[2] = 1.0 - pcoords[0] - pcoords[1];

  for (i = 0; i < 3; i++)
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
    if (pDist > pDistMax)
    {
      pDistMax = pDist;
    }
  }

  return pDistMax;
}

//----------------------------------------------------------------------------
double vtkHigherOrderTriangle::eta(vtkIdType n, vtkIdType chi, double sigma)
{
  double result = 1.;
  for (vtkIdType i = 1; i <= chi; i++)
  {
    result *= (n * sigma - i + 1.) / i;
  }
  return result;
}

//----------------------------------------------------------------------------
double vtkHigherOrderTriangle::d_eta(vtkIdType n, vtkIdType chi, double sigma)
{
  if (chi == 0)
  {
    return 0.;
  }
  else
  {
    double chi_d = static_cast<double>(chi);
    return (n / chi_d * eta(n, chi - 1, sigma) +
      (n * sigma - chi_d + 1.) / chi_d * d_eta(n, chi - 1, sigma));
  }
}

//----------------------------------------------------------------------------
vtkIdType vtkHigherOrderTriangle::ComputeOrder()
{
  // when order = n, # points = (n+1)*(n+2)/2
  return (sqrt(8 * this->GetPoints()->GetNumberOfPoints() + 1) - 3) / 2;
}

//----------------------------------------------------------------------------
void vtkHigherOrderTriangle::ToBarycentricIndex(vtkIdType index, vtkIdType* bindex)
{
#ifdef ENABLE_CACHING
  if (this->BarycentricIndexMap[3 * index] == -1)
  {
    vtkHigherOrderTriangle::BarycentricIndex(
      index, &this->BarycentricIndexMap[3 * index], this->GetOrder());
  }
  for (vtkIdType i = 0; i < 3; i++)
  {
    bindex[i] = this->BarycentricIndexMap[3 * index + i];
  }
#else
  return vtkHigherOrderTriangle::BarycentricIndex(index, bindex, this->GetOrder());
#endif
}

//----------------------------------------------------------------------------
vtkIdType vtkHigherOrderTriangle::ToIndex(const vtkIdType* bindex)
{
#ifdef SEVEN_POINT_TRIANGLE
  if (this->Points->GetNumberOfPoints() == 7)
  {
    return bindex[0];
  }
#endif

#ifdef ENABLE_CACHING
  vtkIdType cacheIdx = ((this->Order + 1) * bindex[0] + bindex[1]);
  if (this->IndexMap[cacheIdx] == -1)
  {
    this->IndexMap[cacheIdx] = vtkHigherOrderTriangle::Index(bindex, this->GetOrder());
  }
  return this->IndexMap[cacheIdx];
#else
  return vtkHigherOrderTriangle::Index(bindex, this->GetOrder());
#endif
}

//----------------------------------------------------------------------------
void vtkHigherOrderTriangle::BarycentricIndex(vtkIdType index, vtkIdType* bindex, vtkIdType order)
{
  // "Barycentric index" is a triplet of integers, each running from 0 to
  // <Order>. It is the index of a point on the triangle in barycentric
  // coordinates.

  assert(order >= 1);

  vtkIdType max = order;
  vtkIdType min = 0;

  // scope into the correct triangle
  while (index != 0 && index >= 3 * order)
  {
    index -= 3 * order;
    max -= 2;
    min++;
    order -= 3;
  }

  if (index < 3)
  {
    // we are on a vertex
    bindex[index] = bindex[(index + 1) % 3] = min;
    bindex[(index + 2) % 3] = max;
  }
  else
  {
    // we are on an edge
    index -= 3;
    vtkIdType dim = index / (order - 1);
    vtkIdType offset = (index - dim * (order - 1));
    bindex[(dim + 1) % 3] = min;
    bindex[(dim + 2) % 3] = (max - 1) - offset;
    bindex[dim] = (min + 1) + offset;
  }
}

//----------------------------------------------------------------------------
vtkIdType vtkHigherOrderTriangle::Index(const vtkIdType* bindex, vtkIdType order)
{
  vtkIdType index = 0;

  assert(order >= 1);
  assert(bindex[0] + bindex[1] + bindex[2] == order);

  vtkIdType max = order;
  vtkIdType min = 0;

  vtkIdType bmin = std::min(std::min(bindex[0], bindex[1]), bindex[2]);

  // scope into the correct triangle
  while (bmin > min)
  {
    index += 3 * order;
    max -= 2;
    min++;
    order -= 3;
  }

  for (vtkIdType dim = 0; dim < 3; dim++)
  {
    if (bindex[(dim + 2) % 3] == max)
    {
      // we are on a vertex
      return index;
    }
    index++;
  }

  for (vtkIdType dim = 0; dim < 3; dim++)
  {
    if (bindex[(dim + 1) % 3] == min)
    {
      // we are on an edge
      return index + bindex[dim] - (min + 1);
    }
    index += max - (min + 1);
  }

  return index;
}

//----------------------------------------------------------------------------
void vtkHigherOrderTriangle::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
