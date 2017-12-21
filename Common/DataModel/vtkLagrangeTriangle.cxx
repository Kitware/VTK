/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLagrangeTriangle.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLagrangeTriangle.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkLagrangeCurve.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkTriangle.h"

#define ENABLE_CACHING
#define SEVEN_POINT_TRIANGLE

vtkStandardNewMacro(vtkLagrangeTriangle);
//----------------------------------------------------------------------------
vtkLagrangeTriangle::vtkLagrangeTriangle() : ParametricCoordinates(nullptr)
{
  this->Order = 0;

  this->Edge = vtkLagrangeCurve::New();
  this->Face = vtkTriangle::New();
  this->Scalars = vtkDoubleArray::New();
  this->Scalars->SetNumberOfTuples(3);

  this->Points->SetNumberOfPoints(3);
  this->PointIds->SetNumberOfIds(3);
  for (int i = 0; i < 3; i++)
    {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
    this->PointIds->SetId(i,0);
    }
}

//----------------------------------------------------------------------------
vtkLagrangeTriangle::~vtkLagrangeTriangle()
{
  delete [] this->ParametricCoordinates;
  this->Edge->Delete();
  this->Face->Delete();
  this->Scalars->Delete();
}

//----------------------------------------------------------------------------
vtkCell *vtkLagrangeTriangle::GetEdge(int edgeId)
{
  vtkIdType order = this->GetOrder();

  vtkIdType bindex[3] = {0,0,0};
  bindex[(edgeId+2)%3] = order;
  for (vtkIdType i=0;i<=order;i++)
    {
    this->EdgeIds[i] = this->PointIds->GetId(this->ToIndex(bindex));
    bindex[(edgeId+2)%3]--;
    bindex[edgeId]++;
    }
  this->Edge->vtkCell::Initialize(order + 1, this->EdgeIds, this->Points);
  return this->Edge;
}

//----------------------------------------------------------------------------
void vtkLagrangeTriangle::Initialize()
{
  vtkIdType order = this->ComputeOrder();

  if (this->Order != order)
    {
    // Reset our caches
    this->Order = order;

    this->NumberOfSubtriangles = this->ComputeNumberOfSubtriangles();

#ifdef ENABLE_CACHING
    for (vtkIdType i = 0; i < this->GetPointIds()->GetNumberOfIds(); i++)
      {
      this->BarycentricIndexMap[3*i] = -1;
      }

    // we sacrifice memory for efficiency here
    vtkIdType nIndexMap = (this->Order+1)*(this->Order+1);
    for (vtkIdType i = 0; i < nIndexMap; i++)
      {
      this->IndexMap[i] = -1;
      }

    vtkIdType nSubtriangles = this->GetNumberOfSubtriangles();
    for (vtkIdType i = 0; i < nSubtriangles; i++)
      {
      this->SubtriangleIndexMap[9*i] = -1;
      }
#endif
    }
}

//----------------------------------------------------------------------------
vtkIdType vtkLagrangeTriangle::ComputeNumberOfSubtriangles()
{
#ifdef SEVEN_POINT_TRIANGLE
  if (this->Points->GetNumberOfPoints() == 7)
    {
    return 6;
    }
#endif
  vtkIdType order = this->GetOrder();
  return order*order;
}

//----------------------------------------------------------------------------
void vtkLagrangeTriangle::SubtriangleBarycentricPointIndices(
  vtkIdType cellIndex, vtkIdType (&pointBIndices)[3][3])
{
  assert(cellIndex < this->GetNumberOfSubtriangles());

#ifdef SEVEN_POINT_TRIANGLE
  if (this->Points->GetNumberOfPoints() == 7)
    {
      pointBIndices[0][0] = cellIndex;
    if (cellIndex < 3)
      {
      pointBIndices[1][0] = (cellIndex+3)%6;
      }
    else
      {
      pointBIndices[1][0] = (cellIndex+1)%3;
      }
    pointBIndices[2][0] = 6;
    return;
    }
#endif

#ifdef ENABLE_CACHING
  vtkIdType cellIndexStart = 9*cellIndex;
  if (this->SubtriangleIndexMap[cellIndexStart] == -1)
#endif
    {
    vtkIdType order = this->GetOrder();

    if (order == 1)
      {
      pointBIndices[0][0] = 0; pointBIndices[0][1] = 0; pointBIndices[0][2] = 1;
      pointBIndices[1][0] = 1; pointBIndices[1][1] = 0; pointBIndices[1][2] = 0;
      pointBIndices[2][0] = 0; pointBIndices[2][1] = 1; pointBIndices[2][2] = 0;
      }
    else
      {
      vtkIdType nRightSideUp = order*(order+1)/2;

      if (cellIndex < nRightSideUp)
        {
        // there are nRightSideUp subtriangles whose orientation is the same as
        // the parent triangle. We traverse them here.
        vtkLagrangeTriangle::BarycentricIndex(cellIndex, pointBIndices[0],
                                              order - 1);
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
          pointBIndices[0][0]=1; pointBIndices[0][1]=1; pointBIndices[0][2]=0;
          pointBIndices[1][0]=0; pointBIndices[1][1]=1; pointBIndices[1][2]=1;
          pointBIndices[2][0]=1; pointBIndices[2][1]=0; pointBIndices[2][2]=1;
          }
        else
          {
          vtkLagrangeTriangle::BarycentricIndex(cellIndex - nRightSideUp,
                                                pointBIndices[1], order - 2);
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
    for (vtkIdType i=0;i<3;i++)
      {
      for (vtkIdType j=0;j<3;j++)
        {
        this->SubtriangleIndexMap[cellIndexStart + 3*i + j]=pointBIndices[i][j];
        }
      }
#endif
    }
#ifdef ENABLE_CACHING
  else
    {
    for (vtkIdType i=0;i<3;i++)
      {
      for (vtkIdType j=0;j<3;j++)
        {
        pointBIndices[i][j]=this->SubtriangleIndexMap[cellIndexStart + 3*i + j];
        }
      }
    }
#endif
}

//----------------------------------------------------------------------------
int vtkLagrangeTriangle::CellBoundary(int vtkNotUsed(subId), double pcoords[3],
                                      vtkIdList *pts)
{
  double t1=pcoords[0]-pcoords[1];
  double t2=0.5*(1.0-pcoords[0])-pcoords[1];
  double t3=2.0*pcoords[0]+pcoords[1]-1.0;

  pts->SetNumberOfIds(2);

  // compare against three lines in parametric space that divide element
  // into three pieces
  if ( t1 >= 0.0 && t2 >= 0.0 )
  {
    pts->SetId(0,this->PointIds->GetId(0));
    pts->SetId(1,this->PointIds->GetId(1));
  }

  else if ( t2 < 0.0 && t3 >= 0.0 )
  {
    pts->SetId(0,this->PointIds->GetId(1));
    pts->SetId(1,this->PointIds->GetId(2));
  }

  else //( t1 < 0.0 && t3 < 0.0 )
  {
    pts->SetId(0,this->PointIds->GetId(2));
    pts->SetId(1,this->PointIds->GetId(0));
  }

  if ( pcoords[0] < 0.0 || pcoords[1] < 0.0 ||
       pcoords[0] > 1.0 || pcoords[1] > 1.0 ||
       (1.0 - pcoords[0] - pcoords[1]) < 0.0 )
  {
    return 0;
  }
  else
  {
    return 1;
  }

}

//----------------------------------------------------------------------------
int vtkLagrangeTriangle::EvaluatePosition(double* x, double* closestPoint,
                                          int& subId, double pcoords[3],
                                          double& minDist2, double *weights)
{
  double pc[3], dist2, tempWeights[3], closest[3];
  double pcoordsMin[3] = {0., 0., 0.};
  int returnStatus=0, status, ignoreId;
  vtkIdType minBIndices[3][3], bindices[3][3], pointIndices[3];

  vtkIdType order = this->GetOrder();
  vtkIdType numberOfSubtriangles = this->GetNumberOfSubtriangles();

  minDist2 = VTK_DOUBLE_MAX;
  for (vtkIdType subCellId = 0; subCellId < numberOfSubtriangles; subCellId++)
    {
    this->SubtriangleBarycentricPointIndices(subCellId, bindices);

    for (vtkIdType i=0;i<3;i++)
      {
      pointIndices[i] = this->ToIndex(bindices[i]);
      this->Face->Points->SetPoint(i, this->Points->GetPoint(pointIndices[i]));
      }

    status = this->Face->EvaluatePosition(x, closest, ignoreId, pc, dist2,
                                          tempWeights);

    if ( status != -1 && dist2 < minDist2 )
      {
      returnStatus = status;
      minDist2 = dist2;
      subId = subCellId;
      pcoordsMin[0] = pc[0];
      pcoordsMin[1] = pc[1];
      for (vtkIdType i=0;i<3;i++)
        {
        for (vtkIdType j=0;j<3;j++)
          {
          minBIndices[i][j] = bindices[i][j];
          }
        }
      }
    }

  // adjust parametric coordinates
  if ( returnStatus != -1 )
    {
    for (vtkIdType i=0;i<3;i++)
      {
      pcoords[i] =
        (i < 2 ? (minBIndices[0][i] +
                  pcoordsMin[0]*(minBIndices[1][i] - minBIndices[0][i]) +
                  pcoordsMin[1]*(minBIndices[2][i] - minBIndices[0][i]))/order
         : 0.);
      }

    if(closestPoint!=nullptr)
      {
      // Compute both closestPoint and weights
      this->EvaluateLocation(subId,pcoords,closestPoint,weights);
      }
    else
      {
      // Compute weights only
      this->InterpolateFunctions(pcoords,weights);
      }
    }

  return returnStatus;
}

//----------------------------------------------------------------------------
void vtkLagrangeTriangle::EvaluateLocation(int& vtkNotUsed(subId),
                                           double pcoords[3], double x[3],
                                           double *weights)
{
  x[0] = x[1] = x[2] = 0.;

  this->InterpolateFunctions(pcoords,weights);

  double p[3];
  vtkIdType nPoints = this->GetPoints()->GetNumberOfPoints();
  for (vtkIdType idx = 0; idx < nPoints; idx++)
    {
    this->Points->GetPoint(idx,p);
    for (vtkIdType jdx = 0; jdx < 3; jdx++)
      {
      x[jdx] += p[jdx]*weights[idx];
      }
    }
}

//----------------------------------------------------------------------------
void vtkLagrangeTriangle::Contour(double value,
                                  vtkDataArray* cellScalars,
                                  vtkIncrementalPointLocator* locator,
                                  vtkCellArray *verts,
                                  vtkCellArray* lines,
                                  vtkCellArray* polys,
                                  vtkPointData* inPd,
                                  vtkPointData* outPd,
                                  vtkCellData* inCd,
                                  vtkIdType cellId,
                                  vtkCellData* outCd)
{
  vtkIdType bindices[3][3];
  vtkIdType numberOfSubtriangles = this->GetNumberOfSubtriangles();

  for (vtkIdType subCellId = 0; subCellId < numberOfSubtriangles; subCellId++)
    {
    this->SubtriangleBarycentricPointIndices(subCellId, bindices);

    for (vtkIdType i=0;i<3;i++)
      {
      vtkIdType pointIndex = this->ToIndex(bindices[i]);
      this->Face->Points->SetPoint(i, this->Points->GetPoint(pointIndex));
      if ( outPd )
        {
        this->Face->PointIds->SetId(i, this->PointIds->GetId(pointIndex));
        }
      this->Scalars->SetTuple(i, cellScalars->GetTuple(pointIndex));
      }

    this->Face->Contour(value, this->Scalars, locator, verts,
                        lines, polys, inPd, outPd, inCd, cellId, outCd);

    }
}

//----------------------------------------------------------------------------
void vtkLagrangeTriangle::Clip(double value,
                               vtkDataArray* cellScalars,
                               vtkIncrementalPointLocator* locator,
                               vtkCellArray* polys,
                               vtkPointData* inPd,
                               vtkPointData* outPd,
                               vtkCellData* inCd,
                               vtkIdType cellId,
                               vtkCellData* outCd,
                               int insideOut)
{
  vtkIdType bindices[3][3];
  vtkIdType numberOfSubtriangles = this->GetNumberOfSubtriangles();

  for (vtkIdType subCellId = 0; subCellId < numberOfSubtriangles; subCellId++)
    {
    this->SubtriangleBarycentricPointIndices(subCellId, bindices);

    for (vtkIdType i=0;i<3;i++)
      {
      vtkIdType pointIndex = this->ToIndex(bindices[i]);
      this->Face->Points->SetPoint(i, this->Points->GetPoint(pointIndex));
      if ( outPd )
        {
        this->Face->PointIds->SetId(i, this->PointIds->GetId(pointIndex));
        }
      this->Scalars->SetTuple(i, cellScalars->GetTuple(pointIndex));
      }

    this->Face->Clip(value, this->Scalars, locator, polys, inPd, outPd,
                     inCd, cellId, outCd, insideOut);
    }
}

//----------------------------------------------------------------------------
int vtkLagrangeTriangle::IntersectWithLine(double* p1,
                                           double* p2,
                                           double tol,
                                           double& t,
                                           double* x,
                                           double* pcoords,
                                           int& subId)
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

    for (vtkIdType i=0;i<3;i++)
      {
      vtkIdType pointIndex = this->ToIndex(bindices[i]);
      this->Face->Points->SetPoint(i, this->Points->GetPoint(pointIndex));
      }

    if (this->Face->IntersectWithLine(p1, p2, tol, tTmp, xMin, pcoordsMin,
                                      subTest) && tTmp < t)
      {
      for (vtkIdType i=0;i<3;i++)
        {
        x[i] = xMin[i];
        pcoords[i] = (i < 2 ?
                      (bindices[0][i] +
                       pcoordsMin[0]*(bindices[1][i] - bindices[0][i]) +
                       pcoordsMin[1]*(bindices[2][i] - bindices[0][i]))/order
                      : 0.);
        }
      t = tTmp;
      }
    }

  subId = 0;
  return (t == VTK_DOUBLE_MAX ? 0 : 1);
}

//----------------------------------------------------------------------------
int vtkLagrangeTriangle::Triangulate(int vtkNotUsed(index), vtkIdList *ptIds,
                                     vtkPoints *pts)
{
  pts->Reset();
  ptIds->Reset();

#ifdef SEVEN_POINT_TRIANGLE
  if (this->Points->GetNumberOfPoints() == 7)
  {
    static const int edgeOrder[7] = { 0, 3, 1, 4, 2, 5, 0 };
    pts->SetNumberOfPoints(18);
    ptIds->SetNumberOfIds(18);
    vtkIdType pointId = 0;
    for (vtkIdType i=0;i<6;i++)
    {
      ptIds->SetId(pointId,this->PointIds->GetId(edgeOrder[i]));
      pts->SetPoint(pointId,this->Points->GetPoint(edgeOrder[i]));
      pointId++;
      ptIds->SetId(pointId,this->PointIds->GetId(edgeOrder[i+1]));
      pts->SetPoint(pointId,this->Points->GetPoint(edgeOrder[i+1]));
      pointId++;
      ptIds->SetId(pointId,this->PointIds->GetId(6));
      pts->SetPoint(pointId,this->Points->GetPoint(6));
      pointId++;
    }
    return 1;
  }
#endif

  vtkIdType bindices[3][3];
  vtkIdType numberOfSubtriangles = this->GetNumberOfSubtriangles();

  pts->SetNumberOfPoints(3*numberOfSubtriangles);
  ptIds->SetNumberOfIds(3*numberOfSubtriangles);
  for (vtkIdType subCellId = 0; subCellId < numberOfSubtriangles; subCellId++)
    {
    this->SubtriangleBarycentricPointIndices(subCellId, bindices);

    for (vtkIdType i=0;i<3;i++)
      {
      vtkIdType pointIndex = this->ToIndex(bindices[i]);
      ptIds->SetId(3*subCellId+i,this->PointIds->GetId(pointIndex));
      pts->SetPoint(3*subCellId+i,this->Points->GetPoint(pointIndex));
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkLagrangeTriangle::JacobianInverse(double pcoords[3], double**inverse,
                                          double* derivs)
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
  m[0] = m0; m[1] = m1; m[2] = m2;
  for (i=0; i < 3; i++) //initialize matrix
    {
    m0[i] = m1[i] = m2[i] = 0.0;
    }

  for (j=0; j < numberOfPoints; j++)
    {
    this->Points->GetPoint(j, x);
    for (i=0; i < 3; i++)
      {
      for (k=0; k < this->GetCellDimension(); k++)
        {
        m[k][i] += x[i] * derivs[numberOfPoints*k + j];
        }
      }
    }

  // Compute third row vector in transposed Jacobian and normalize it, so that
  // Jacobian determinant stays the same.
  if (this->GetCellDimension() == 2)
    {
    vtkMath::Cross(m0,m1,m2);
    }

  if ( vtkMath::Normalize(m2) == 0.0 || !vtkMath::InvertMatrix(m,inverse,3))
    {
    vtkErrorMacro(<<"Jacobian inverse not found");
    return;
    }
}

//----------------------------------------------------------------------------
void vtkLagrangeTriangle::Derivatives(int vtkNotUsed(subId),
                                      double pcoords[3],
                                      double* values,
                                      int dim,
                                      double *derivs)
{
  double *jI[3], j0[3], j1[3], j2[3];
  double fDs[(VTK_LAGRANGE_TRIANGLE_MAX_ORDER + 1) *
             (VTK_LAGRANGE_TRIANGLE_MAX_ORDER + 2)];
  double sum[3];
  int i, j, k;
  vtkIdType numberOfPoints = this->Points->GetNumberOfPoints();

  // compute inverse Jacobian and interpolation function derivatives
  jI[0] = j0; jI[1] = j1; jI[2] = j2;
  this->JacobianInverse(pcoords, jI, fDs);

  // now compute derivates of values provided
  for (k=0; k < dim; k++) //loop over values per vertex
    {
    sum[0] = sum[1] = sum[2] = 0.0;
    for ( i=0; i < numberOfPoints; i++) //loop over interp. function derivatives
      {
      sum[0] += fDs[i] * values[dim*i + k];
      sum[1] += fDs[numberOfPoints + i] * values[dim*i + k];
      }
    for (j=0; j < 3; j++) //loop over derivative directions
      {
      derivs[3*k + j] = 0.;
      for (i=0; i < this->GetCellDimension(); i++)
        {
        derivs[3*k + j] += sum[i]*jI[j][i];
        }
      }
    }
}

//----------------------------------------------------------------------------
#ifdef SEVEN_POINT_TRIANGLE
namespace
{
double SevenPointTriangleCoords[7*3] = {0.,0.,0.,
                                        1.,0.,0.,
                                        0.,1.,0.,
                                        .5,0.,0.,
                                        .5,.5,0.,
                                        0.,.5,0.,
                                        1./3.,1./3.,0.};
}
#endif

double* vtkLagrangeTriangle::GetParametricCoords()
{
#ifdef SEVEN_POINT_TRIANGLE
  if (this->Points->GetNumberOfPoints() == 7)
    {
    return SevenPointTriangleCoords;
    }
#endif

  if (!this->ParametricCoordinates)
    {
    vtkIdType order = this->GetOrder();

    vtkIdType nPoints = (order + 1)*(order + 2)/2;
    this->ParametricCoordinates = new double[3*nPoints];
    ComputeParametricCoords(this->ParametricCoordinates,order);
    }
  return this->ParametricCoordinates;
}

//----------------------------------------------------------------------------
void vtkLagrangeTriangle::ComputeParametricCoords(double* coords,
                                                  vtkIdType order)
{
  double max = static_cast<double>(order);
  double min = 0.;
  vtkIdType pIdx = 0;
  double p[3];
  vtkIdType ord;
  for (ord = order; ord > 0; ord-=3)
    {
    // add the vertex points
    for (vtkIdType dim=0;dim<3;dim++)
      {
      coords[pIdx + dim]       = min/order;
      coords[pIdx + (dim+1)%3] = min/order;
      coords[pIdx + (dim+2)%3] = max/order;
      pIdx += 3;
      }

    // add the edge points
    if (ord > 1)
      {
      for (vtkIdType dim=0;dim<3;dim++)
        {
        p[dim] = p[(dim+1)%3] = min/order;
        p[(dim+2)%3] = max/order;
        for (vtkIdType i=0;i<ord-1;i++)
          {
          p[dim] += 1./order;
          p[(dim+2)%3] -= 1./order;
          for (vtkIdType j=0;j<3;j++)
            {
            coords[pIdx + j] = p[j];
            }
          pIdx += 3;
          }
        }
      }
    max -= 2.;
    min += 1.;
    }

  if (ord == 0)
    {
    coords[pIdx]     = min/order;
    coords[pIdx + 1] = min/order;
    coords[pIdx + 2] = min/order;
    pIdx += 3;
    }

  // For parametric coordinates, we project the barycentric coordinates
  // onto the z=0 plane
  for (vtkIdType i=2;i<pIdx;i+=3)
    {
    coords[i] = 0.;
    }
}

//----------------------------------------------------------------------------
int vtkLagrangeTriangle::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = pcoords[1] = 1./3.;
  pcoords[2] = 0.;
  return 0;
}

//----------------------------------------------------------------------------
double vtkLagrangeTriangle::GetParametricDistance(double pcoords[3])
{
  int i;
  double pDist, pDistMax=0.0;
  double pc[3];

  pc[0] = pcoords[0];
  pc[1] = pcoords[1];
  pc[2] = 1.0 - pcoords[0] - pcoords[1];

  for (i=0; i<3; i++)
    {
    if ( pc[i] < 0.0 )
      {
      pDist = -pc[i];
      }
    else if ( pc[i] > 1.0 )
      {
      pDist = pc[i] - 1.0;
      }
    else //inside the cell in the parametric direction
      {
      pDist = 0.0;
      }
    if ( pDist > pDistMax )
      {
      pDistMax = pDist;
      }
    }

  return pDistMax;
}

//----------------------------------------------------------------------------
double vtkLagrangeTriangle::eta(vtkIdType n, vtkIdType chi, double sigma)
{
  double result = 1.;
  for (vtkIdType i=1;i<=chi;i++)
    {
    result *= (n*sigma - i + 1.)/i;
    }
  return result;
}

//----------------------------------------------------------------------------
double vtkLagrangeTriangle::d_eta(vtkIdType n, vtkIdType chi, double sigma)
 {
   if (chi == 0)
     {
     return 0.;
     }
   else
     {
     double chi_d = static_cast<double>(chi);
     return (n/chi_d*eta(n,chi-1,sigma) +
             (n*sigma-chi_d+1.)/chi_d*d_eta(n,chi-1,sigma));

     }
 }

//----------------------------------------------------------------------------
void vtkLagrangeTriangle::InterpolateFunctions(double pcoords[3],
                                               double* weights)
{
  // Adapted from P. Silvester, "High-Order Polynomial Triangular Finite
  // Elements for Potential Problems". Int. J. Engng Sci. Vol. 7, pp. 849-861.
  // Pergamon Press, 1969. The generic method is valid for all orders, but we
  // unroll the first two orders to reduce computational cost.

  double tau[3] = {pcoords[0], pcoords[1], 1. - pcoords[0] - pcoords[1]};

  vtkIdType n = this->GetOrder();

  if (n == 1)
    {
    // for the linear case, we simply return the parametric coordinates, rotated
    // into the parametric frame (e.g. barycentric tau_2 = parametric x).
    weights[0] = tau[2];
    weights[1] = tau[0];
    weights[2] = tau[1];
    }
  else if (n == 2)
    {
#ifdef SEVEN_POINT_TRIANGLE
    if (this->GetPoints()->GetNumberOfPoints() == 7)
      {
      double rs = tau[0]*tau[1];
      double rt = tau[0]*tau[2];
      double st = tau[1]*tau[2];
      double rst = rs*tau[2];
      weights[0] = tau[2] + 3.0*rst - 2.0*rt - 2.0*st;
      weights[1] = tau[0] + 3.0*rst - 2.0*rt - 2.0*rs;
      weights[2] = tau[1] + 3.0*rst - 2.0*rs - 2.0*st;
      weights[3] = 4.0*rt - 12.0*rst;
      weights[4] = 4.0*rs - 12.0*rst;
      weights[5] = 4.0*st - 12.0*rst;
      weights[6] = 27.0*rst;
      return;
      }
#endif
    weights[0] = tau[2]*(2.0*tau[2] - 1.0);
    weights[1] = tau[0]*(2.0*tau[0] - 1.0);
    weights[2] = tau[1]*(2.0*tau[1] - 1.0);
    weights[3] = 4.0 * tau[0] * tau[2];
    weights[4] = 4.0 * tau[0] * tau[1];
    weights[5] = 4.0 * tau[1] * tau[2];
    }
  else
    {
    vtkIdType nPoints = this->GetPoints()->GetNumberOfPoints();

    for (vtkIdType idx=0; idx < nPoints; idx++)
      {
      weights[idx] = 1.;
      vtkIdType lambda[3];
      this->ToBarycentricIndex(idx,lambda);

      for (vtkIdType dim=0;dim<3;dim++)
        {
        weights[idx] *= eta(n, lambda[dim], tau[dim]);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkLagrangeTriangle::InterpolateDerivs(double pcoords[3],
                                            double* derivs)
{
  // Analytic differentiation of the triangle shape functions, as defined in
  // P. Silvester, "High-Order Polynomial Triangular Finite Elements for
  // Potential Problems". Int. J. Engng Sci. Vol. 7, pp. 849-861. Pergamon
  // Press, 1969. The generic method is valid for all orders, but we unroll the
  // first two orders to reduce computational cost.

  double tau[3] = {pcoords[0], pcoords[1], 1. - pcoords[0] - pcoords[1]};

  vtkIdType n = this->GetOrder();

  if (n == 1)
    {
    derivs[0] = -1;
    derivs[1] = 1;
    derivs[2] = 0;
    derivs[3] = -1;
    derivs[4] = 0;
    derivs[5] = 1;
    }
  else if (n == 2)
    {
#ifdef SEVEN_POINT_TRIANGLE
    if (this->GetPoints()->GetNumberOfPoints() == 7)
      {
      double tmr = tau[2] - tau[0];
      double tms = tau[2] - tau[1];
      derivs[0] = -1.0 + 3.0*tau[1]*tmr - 2.0*tmr + 2.0*tau[1];
      derivs[1] = 1.0 + 3.0*tau[1]*tmr - 2.0*tmr - 2.0*tau[1];
      derivs[2] = 3.0*tau[1]*tmr;
      derivs[3] = 4.0*tmr - 12.0*tau[1]*tmr;
      derivs[4] = 4.0*tau[1] - 12.0*tau[1]*tmr;
      derivs[5] = -4.0*tau[1] - 12.0*tau[1]*tmr;
      derivs[6] = 27.0*tau[1]*tmr;
      derivs[7] = -1.0 + 3.0*tau[0]*tms - 2.0*tms + 2.0*tau[0];
      derivs[8] = 3.0*tau[0]*tms;
      derivs[9] = 1.0 + 3.0*tau[0]*tms - 2.0*tms - 2.0*tau[0];
      derivs[10] = -4.0*tau[0] - 12.0*tau[0]*tms;
      derivs[11] = 4.0*tau[0] - 12.0*tau[0]*tms;
      derivs[12] = 4.0*tms - 12.0*tau[0]*tms;
      derivs[13] = 27.0*tau[0]*tms;
      return;
      }
#endif
    derivs[0] = 1.0 - 4.0*tau[2];
    derivs[1] = 4.0*tau[0] - 1.0;
    derivs[2] = 0.0;
    derivs[3] = 4.0*(tau[2] - tau[0]);
    derivs[4] = 4.0*tau[1];
    derivs[5] = -4.0*tau[1];
    derivs[6] = 1.0 - 4.0*tau[2];
    derivs[7] = 0.0;
    derivs[8] = 4.0*tau[1] - 1.0;
    derivs[9] = -4.0*tau[0];
    derivs[10] = 4.0*tau[0];
    derivs[11] = 4.0*(tau[2] - tau[1]);
    }
  else
    {
    vtkIdType nPoints = this->GetPoints()->GetNumberOfPoints();

    for (vtkIdType idx = 0; idx < nPoints; idx++)
      {
      vtkIdType lambda[3];
      this->ToBarycentricIndex(idx,lambda);

      double eta_alpha = eta(n,lambda[0],tau[0]);
      double eta_beta = eta(n,lambda[1],tau[1]);
      double eta_gamma = eta(n,lambda[2],tau[2]);

      double d_eta_alpha = d_eta(n,lambda[0],tau[0]);
      double d_eta_beta = d_eta(n,lambda[1],tau[1]);
      double d_eta_gamma = d_eta(n,lambda[2],tau[2]);

      double d_f_d_tau1 = (d_eta_alpha*eta_beta*eta_gamma -
                           eta_alpha*eta_beta*d_eta_gamma);
      double d_f_d_tau2 = (eta_alpha*d_eta_beta*eta_gamma -
                           eta_alpha*eta_beta*d_eta_gamma);

      derivs[idx] = d_f_d_tau1;
      derivs[nPoints + idx] = d_f_d_tau2;
      }
    }
}

//----------------------------------------------------------------------------
vtkIdType vtkLagrangeTriangle::ComputeOrder()
{
  // when order = n, # points = (n+1)*(n+2)/2
  return (sqrt(8*this->GetPoints()->GetNumberOfPoints() + 1) - 3)/2;
}

//----------------------------------------------------------------------------
void vtkLagrangeTriangle::ToBarycentricIndex(vtkIdType index,
                                             vtkIdType* bindex)
{
#ifdef ENABLE_CACHING
  if (this->BarycentricIndexMap[3*index] == -1)
    {
    vtkLagrangeTriangle::BarycentricIndex(index,
                                          &this->BarycentricIndexMap[3*index],
                                          this->GetOrder());
    }
  for (vtkIdType i=0;i<3;i++)
    {
    bindex[i] = this->BarycentricIndexMap[3*index + i];
    }
#else
  return vtkLagrangeTriangle::BarycentricIndex(index,bindex,this->GetOrder());
#endif
}

//----------------------------------------------------------------------------
vtkIdType vtkLagrangeTriangle::ToIndex(const vtkIdType* bindex)
{
#ifdef SEVEN_POINT_TRIANGLE
  if (this->Points->GetNumberOfPoints() == 7)
    {
    return bindex[0];
    }
#endif

#ifdef ENABLE_CACHING
  vtkIdType cacheIdx = ((this->Order+1)*bindex[0] + bindex[1]);
  if (this->IndexMap[cacheIdx] == -1)
    {
    this->IndexMap[cacheIdx] =
      vtkLagrangeTriangle::Index(bindex, this->GetOrder());
    }
  return this->IndexMap[cacheIdx];
#else
  return vtkLagrangeTriangle::Index(bindex, this->GetOrder());
#endif
}

//----------------------------------------------------------------------------
void vtkLagrangeTriangle::BarycentricIndex(vtkIdType index, vtkIdType* bindex,
                                           vtkIdType order)
{
  // "Barycentric index" is a triplet of integers, each running from 0 to
  // <Order>. It is the index of a point on the triangle in barycentric
  // coordinates.

  assert(order >= 1);

  vtkIdType max = order;
  vtkIdType min = 0;

  // scope into the correct triangle
  while (index != 0 && index >= 3*order)
    {
    index -= 3*order;
    max-=2;
    min++;
    order -= 3;
    }

  if (index < 3)
    {
    // we are on a vertex
    bindex[index] = bindex[(index+1)%3] = min;
    bindex[(index+2)%3] = max;
    }
  else
    {
    // we are on an edge
    index -= 3;
    vtkIdType dim = index/(order - 1);
    vtkIdType offset = (index - dim*(order - 1));
    bindex[(dim+1)%3] = min;
    bindex[(dim+2)%3] = (max - 1) - offset;
    bindex[dim] = (min + 1) + offset;
    }
}

//----------------------------------------------------------------------------
vtkIdType vtkLagrangeTriangle::Index(const vtkIdType* bindex, vtkIdType order)
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
    index += 3*order;
    max-=2;
    min++;
    order -= 3;
    }

  for (vtkIdType dim = 0; dim < 3; dim++)
    {
    if (bindex[(dim+2)%3] == max)
      {
      // we are on a vertex
      return index;
      }
    index++;
    }

  for (vtkIdType dim = 0; dim < 3; dim++)
    {
    if (bindex[(dim+1)%3] == min)
      {
      // we are on an edge
      return index + bindex[dim] - (min + 1);
      }
    index += max - (min + 1);
    }

  return index;
}

//----------------------------------------------------------------------------
void vtkLagrangeTriangle::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
