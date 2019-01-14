/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLagrangeHexahedron.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLagrangeHexahedron.h"

#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkHexahedron.h"
#include "vtkIdList.h"
#include "vtkLagrangeCurve.h"
#include "vtkLagrangeInterpolation.h"
#include "vtkLagrangeQuadrilateral.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkTriangle.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

vtkStandardNewMacro(vtkLagrangeHexahedron);

vtkLagrangeHexahedron::vtkLagrangeHexahedron()
{
  this->Approx = nullptr;
  this->Order[0] = this->Order[1] = this->Order[2] = 1;
  this->Order[3] = 8;
  this->Points->SetNumberOfPoints(8);
  this->PointIds->SetNumberOfIds(8);
  for (int i = 0; i < 8; i++)
    {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
    this->PointIds->SetId(i,-1);
    }
}

vtkLagrangeHexahedron::~vtkLagrangeHexahedron() = default;

void vtkLagrangeHexahedron::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Order: " << this->GetOrder(3) << "\n";
  if (this->PointParametricCoordinates)
    {
    os
      << indent << "PointParametricCoordinates: "
      << this->PointParametricCoordinates->GetNumberOfPoints()
      << " entries\n";
    }
  os << indent << "Approx: " << this->Approx << "\n";
}

vtkCell* vtkLagrangeHexahedron::GetEdge(int edgeId)
{
  vtkLagrangeCurve* result = this->EdgeCell.GetPointer();
  const int* order = this->GetOrder();
  int oi = vtkLagrangeInterpolation::GetVaryingParameterOfHexEdge(edgeId);
  vtkVector2i eidx = vtkLagrangeInterpolation::GetPointIndicesBoundingHexEdge(edgeId);
  vtkIdType npts = order[oi] + 1;
  int sn = 0;
  result->Points->SetNumberOfPoints(npts);
  result->PointIds->SetNumberOfIds(npts);
  for (int i = 0; i < 2; ++i, ++sn)
    {
    result->Points->SetPoint(sn, this->Points->GetPoint(eidx[i]));
    result->PointIds->SetId(sn, this->PointIds->GetId(eidx[i]));
    }
  // Now add edge-interior points in axis order:
  int offset = 8;
  if (oi == 2)
    {
    offset += 4 * (order[0] - 1 + order[1] - 1);
    offset += (edgeId - 8) * (order[2] - 1);
    }
  else
    {
    for (int ee = 0; ee < edgeId; ++ee)
      {
      offset += order[ee % 2 == 0 ? 0 : 1] - 1;
      }
    }
  for (int jj = 0; jj < order[oi] - 1; ++jj, ++sn)
    {
    result->Points->SetPoint(sn, this->Points->GetPoint(offset + jj));
    result->PointIds->SetId(sn, this->PointIds->GetId(offset + jj));
    }
  return result;
}

vtkCell* vtkLagrangeHexahedron::GetFace(int faceId)
{
  if (faceId < 0 || faceId >= 6)
  {
    return nullptr;
  }

  // Do we need to flip the face to get an outward-pointing normal?
  bool flipFace = (faceId % 2 == ((faceId / 2) % 2) ? true : false);

  vtkLagrangeQuadrilateral* result = this->FaceCell.GetPointer();
  const int* order = this->GetOrder();
  vtkVector2i faceParams = vtkLagrangeInterpolation::GetVaryingParametersOfHexFace(faceId);
  const int* corners = vtkLagrangeInterpolation::GetPointIndicesBoundingHexFace(faceId);
  int npts = (order[faceParams[0]] + 1) * (order[faceParams[1]] + 1);
  result->Points->SetNumberOfPoints(npts);
  result->PointIds->SetNumberOfIds(npts);

  // Add vertex DOFs to result
  int sn = 0;
  if (!flipFace)
  {
    for (int ii = 0; ii < 4; ++ii, ++sn)
    {
      result->Points->SetPoint(sn, this->Points->GetPoint(corners[ii]));
      result->PointIds->SetId(sn, this->PointIds->GetId(corners[ii]));
    }
  }
  else
  {
    for (int ii = 0; ii < 4; ++ii, ++sn)
    {
      result->Points->SetPoint((5 - sn) % 4, this->Points->GetPoint(corners[ii]));
      result->PointIds->SetId((5 - sn) % 4, this->PointIds->GetId(corners[ii]));
    }
  }

  // Add edge DOFs to result
  int offset;
  const int* faceEdges = vtkLagrangeInterpolation::GetEdgeIndicesBoundingHexFace(faceId);
  for (int ii = 0; ii < 4; ++ii)
  {
    offset = 8;
    if (!flipFace)
    {
      int pp = vtkLagrangeInterpolation::GetVaryingParameterOfHexEdge(faceEdges[ii]);
      if (pp == 2)
      {
        offset += 4 * (order[0] - 1 + order[1] - 1);
        offset += (faceEdges[ii] - 8) * (order[2] - 1);
      }
      else
      {
        for (int ee = 0; ee < faceEdges[ii]; ++ee)
        {
          offset += order[ee % 2 == 0 ? 0 : 1] - 1;
        }
      }
      for (int jj = 0; jj < order[pp] - 1; ++jj, ++sn)
      {
        result->Points->SetPoint(sn, this->Points->GetPoint(offset + jj));
        result->PointIds->SetId(sn, this->PointIds->GetId(offset + jj));
      }
    }
    else
    {
      // Flip both the edge position among edges (ii => (4 - ii) % 4)
      // and the edge's node order (jj => order[pp] - jj - 1).
      int pp = vtkLagrangeInterpolation::GetVaryingParameterOfHexEdge(faceEdges[(4 - ii) % 4]);
      if (pp == 2)
      {
        offset += 4 * (order[0] - 1 + order[1] - 1);
        offset += (faceEdges[(4 - ii) % 4] - 8) * (order[2] - 1);
      }
      else
      {
        for (int ee = 0; ee < faceEdges[(4 - ii) % 4]; ++ee)
        {
          offset += order[ee % 2 == 0 ? 0 : 1] - 1;
        }
      }
      if (ii % 2 == 0)
      {
        for (int jj = 0; jj < order[pp] - 1; ++jj, ++sn)
        {
          result->Points->SetPoint(sn, this->Points->GetPoint(offset + order[pp] - jj - 2));
          result->PointIds->SetId(sn, this->PointIds->GetId(offset + order[pp] - jj - 2));
        }
      }
      else
      {
        for (int jj = 0; jj < order[pp] - 1; ++jj, ++sn)
        {
          result->Points->SetPoint(sn, this->Points->GetPoint(offset + jj));
          result->PointIds->SetId(sn, this->PointIds->GetId(offset + jj));
        }
      }
    }
  }

  // Now add face DOF
  offset = 8 + 4 * (order[0] - 1 + order[1] - 1 + order[2] - 1);
  // skip DOF for other faces of hex before this one
  for (int ff = 0; ff < faceId; ++ff)
  {
    vtkVector2i tmp = vtkLagrangeInterpolation::GetVaryingParametersOfHexFace(ff);
    offset += (order[tmp[0]] - 1) * (order[tmp[1]] - 1);
  }
  if (!flipFace)
  {
    int nfdof = (order[faceParams[0]] - 1) * (order[faceParams[1]] - 1);
    for (int ii = 0; ii < nfdof; ++ii, ++sn)
    {
      result->Points->SetPoint(sn, this->Points->GetPoint(offset + ii));
      result->PointIds->SetId(sn, this->PointIds->GetId(offset + ii));
    }
  }
  else
  {
    int delta = order[faceParams[0]] - 1;
    for (int jj = 0; jj < (order[faceParams[1]] - 1); ++jj)
    {
      for (int ii = delta - 1; ii >= 0; --ii, ++sn)
      {
        result->Points->SetPoint(sn, this->Points->GetPoint(offset + ii + jj * delta));
        result->PointIds->SetId(sn, this->PointIds->GetId(offset + ii + jj * delta));
      }
    }
  }
  /*
  std::cout << "Hex Face " << faceId << "\n";
  for (int yy = 0; yy < npts; ++yy)
  {
  vtkVector3d xx;
  result->Points->GetPoint(yy, xx.GetData());
  std::cout << "  " << yy << "  " << result->PointIds->GetId(yy) << " " << xx << "\n";
  }
  */
  return result;
}

void vtkLagrangeHexahedron::Initialize()
{
}

int vtkLagrangeHexahedron::CellBoundary(
  int vtkNotUsed(subId), const double pcoords[3], vtkIdList* pts)
{
  double t1=pcoords[0]-pcoords[1];
  double t2=1.0-pcoords[0]-pcoords[1];
  double t3=pcoords[1]-pcoords[2];
  double t4=1.0-pcoords[1]-pcoords[2];
  double t5=pcoords[2]-pcoords[0];
  double t6=1.0-pcoords[2]-pcoords[0];

  pts->SetNumberOfIds(4);

  // compare against six planes in parametric space that divide element
  // into six pieces.
  if ( t3 >= 0.0 && t4 >= 0.0 && t5 < 0.0 && t6 >= 0.0 )
  {
    pts->SetId(0,this->PointIds->GetId(0));
    pts->SetId(1,this->PointIds->GetId(1));
    pts->SetId(2,this->PointIds->GetId(2));
    pts->SetId(3,this->PointIds->GetId(3));
  }

  else if ( t1 >= 0.0 && t2 < 0.0 && t5 < 0.0 && t6 < 0.0 )
  {
    pts->SetId(0,this->PointIds->GetId(1));
    pts->SetId(1,this->PointIds->GetId(2));
    pts->SetId(2,this->PointIds->GetId(6));
    pts->SetId(3,this->PointIds->GetId(5));
  }

  else if ( t1 >= 0.0 && t2 >= 0.0 && t3 < 0.0 && t4 >= 0.0 )
  {
    pts->SetId(0,this->PointIds->GetId(0));
    pts->SetId(1,this->PointIds->GetId(1));
    pts->SetId(2,this->PointIds->GetId(5));
    pts->SetId(3,this->PointIds->GetId(4));
  }

  else if ( t3 < 0.0 && t4 < 0.0 && t5 >= 0.0 && t6 < 0.0 )
  {
    pts->SetId(0,this->PointIds->GetId(4));
    pts->SetId(1,this->PointIds->GetId(5));
    pts->SetId(2,this->PointIds->GetId(6));
    pts->SetId(3,this->PointIds->GetId(7));
  }

  else if ( t1 < 0.0 && t2 >= 0.0 && t5 >= 0.0 && t6 >= 0.0 )
  {
    pts->SetId(0,this->PointIds->GetId(0));
    pts->SetId(1,this->PointIds->GetId(4));
    pts->SetId(2,this->PointIds->GetId(7));
    pts->SetId(3,this->PointIds->GetId(3));
  }

  else // if ( t1 < 0.0 && t2 < 0.0 && t3 >= 0.0 && t6 < 0.0 )
  {
    pts->SetId(0,this->PointIds->GetId(2));
    pts->SetId(1,this->PointIds->GetId(3));
    pts->SetId(2,this->PointIds->GetId(7));
    pts->SetId(3,this->PointIds->GetId(6));
  }


  if ( pcoords[0] < 0.0 || pcoords[0] > 1.0 ||
       pcoords[1] < 0.0 || pcoords[1] > 1.0 ||
       pcoords[2] < 0.0 || pcoords[2] > 1.0 )
  {
    return 0;
  }
  else
  {
    return 1;
  }
}

int vtkLagrangeHexahedron::EvaluatePosition(
  const double x[3],
  double closestPoint[3],
  int& subId,
  double pcoords[3],
  double& minDist2,
  double weights[])
{
  int result = 0;

  int dummySubId;
  double linearWeights[8];
  double tmpDist2;
  vtkVector3d params;
  vtkVector3d tmpClosestPt;

  minDist2 = VTK_DOUBLE_MAX;
  vtkIdType nhex = vtkLagrangeInterpolation::NumberOfIntervals<3>(this->GetOrder());
  for (int subCell = 0; subCell < nhex; ++subCell)
    {
    vtkHexahedron* approx = this->GetApproximateHex(subCell, nullptr, nullptr);
    int stat = approx->EvaluatePosition(x, tmpClosestPt.GetData(), dummySubId, params.GetData(), tmpDist2, linearWeights);
    if (stat != -1 && tmpDist2 < minDist2)
      {
      result = stat;
      subId = subCell;
      minDist2 = tmpDist2;
      for (int ii = 0; ii < 3; ++ii)
        {
        pcoords[ii] = params[ii]; // We will translate the winning parameter values later.
        if (closestPoint)
          {
          closestPoint[ii] = tmpClosestPt[ii];
          }
        }
      }
    }

  if (result != -1)
    {
    this->TransformApproxToCellParams(subId, pcoords);
    if (closestPoint)
      {
      this->EvaluateLocation(dummySubId, pcoords, closestPoint, weights);
      }
    else
      {
      this->InterpolateFunctions(pcoords, weights);
      }
    }

  return result;
}

void vtkLagrangeHexahedron::EvaluateLocation(
  int& subId,
  const double pcoords[3],
  double x[3], double* weights)
{
  subId = 0; // TODO: Should this be -1?
  this->InterpolateFunctions(pcoords, weights);

  double p[3];
  x[0] = x[1] = x[2] = 0.;
  vtkIdType nPoints = this->GetPoints()->GetNumberOfPoints();
  for (vtkIdType idx = 0; idx < nPoints; ++idx)
    {
    this->Points->GetPoint(idx, p);
    for (vtkIdType jdx = 0; jdx < 3; ++jdx)
      {
      x[jdx] += p[jdx] * weights[idx];
      }
    }
}

void vtkLagrangeHexahedron::Contour(
  double value,
  vtkDataArray* cellScalars,
  vtkIncrementalPointLocator* locator,
  vtkCellArray* verts,
  vtkCellArray* lines,
  vtkCellArray* polys,
  vtkPointData* inPd,
  vtkPointData* outPd,
  vtkCellData* inCd,
  vtkIdType cellId,
  vtkCellData* outCd)
{
  this->PrepareApproxData(inPd, inCd, cellId, cellScalars); // writes to this->{CellScalars, ApproxPD, ApproxCD}
  vtkIdType nhex = vtkLagrangeInterpolation::NumberOfIntervals<3>(this->GetOrder());
  for (int i = 0; i < nhex; ++i)
    {
    vtkHexahedron* approx = this->GetApproximateHex(i, this->CellScalars.GetPointer(), this->Scalars.GetPointer());
    approx->Contour(
      value, this->Scalars.GetPointer(), locator,
      verts, lines, polys, this->ApproxPD, outPd, this->ApproxCD, cellId, outCd);
    }
}

void vtkLagrangeHexahedron::Clip(
  double value,
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
  this->PrepareApproxData(inPd, inCd, cellId, cellScalars); // writes to this->{CellScalars, ApproxPD, ApproxCD}
  vtkIdType nhex = vtkLagrangeInterpolation::NumberOfIntervals<3>(this->GetOrder());
  for (int i = 0; i < nhex; ++i)
    {
    vtkHexahedron* approx = this->GetApproximateHex(i, this->CellScalars.GetPointer(), this->Scalars.GetPointer());
    approx->Clip(
      value, this->Scalars.GetPointer(), locator,
      polys, this->ApproxPD, outPd, this->ApproxCD, cellId,
      outCd, insideOut);
    }
}

int vtkLagrangeHexahedron::IntersectWithLine(
  const double* p1,
  const double* p2,
  double tol,
  double& t,
  double* x,
  double* pcoords,
  int& subId)
{
  double tFirst = VTK_DOUBLE_MAX;
  bool intersection = false;
  vtkVector3d tmpX;
  vtkVector3d tmpP;
  int tmpId;
  this->GetOrder(); // Ensure Order is up to date.
  for (int ff = 0; ff < this->GetNumberOfFaces(); ++ff)
  {
    vtkCell* bdy = this->GetFace(ff);
    if (bdy->IntersectWithLine(p1, p2, tol, t, tmpX.GetData(), tmpP.GetData(), tmpId))
    {
      intersection = true;
      if (t < tFirst)
      {
        tFirst = t;
        subId = ff;
        for (int ii = 0; ii < 3; ++ii)
        {
          x[ii] = tmpX[ii];
          pcoords[ii] = tmpP[ii]; // Translate this after we're sure it's the closest hit.
        }
      }
    }
  }
  if (intersection)
  {
    intersection &= this->TransformFaceToCellParams(subId, pcoords);
    t = tFirst;
  }
  return intersection ? 1 : 0;
}

int vtkLagrangeHexahedron::Triangulate(
  int vtkNotUsed(index),
  vtkIdList* ptIds,
  vtkPoints* pts)
{
  ptIds->Reset();
  pts->Reset();

  vtkIdType nhex = vtkLagrangeInterpolation::NumberOfIntervals<3>(this->GetOrder());
  vtkVector3i ijk;
  for (int i = 0; i < nhex; ++i)
    {
    vtkHexahedron* approx = this->GetApproximateHex(i);
    if (!this->SubCellCoordinatesFromId(ijk, i))
    {
      continue;
    }
    if (approx->Triangulate(
        (ijk[0] + ijk[1] + ijk[2]) % 2,
        this->TmpIds.GetPointer(),
        this->TmpPts.GetPointer()))
      {
      // Sigh. Triangulate methods all reset their points/ids
      // so we must copy them to our output.
      vtkIdType np = this->TmpPts->GetNumberOfPoints();
      vtkIdType ni = this->TmpIds->GetNumberOfIds();
      vtkIdType offset = pts->GetNumberOfPoints();
      for (vtkIdType ii = 0; ii < np; ++ii)
        {
        pts->InsertNextPoint(this->TmpPts->GetPoint(ii));
        }
      for (vtkIdType ii = 0; ii < ni; ++ii)
        {
        ptIds->InsertNextId(this->TmpIds->GetId(ii) + offset);
        }
      }
    }
  return 1;
}

void vtkLagrangeHexahedron::Derivatives(
  int vtkNotUsed(subId),
  const double pcoords[3],
  const double* values,
  int dim,
  double* derivs)
{
  this->Interp->Tensor3EvaluateDerivative(this->Order, pcoords, this->GetPoints(), values, dim, derivs);
}

double* vtkLagrangeHexahedron::GetParametricCoords()
{
  if (!this->PointParametricCoordinates)
    {
    this->PointParametricCoordinates = vtkSmartPointer<vtkPoints>::New();
    this->PointParametricCoordinates->SetDataTypeToDouble();
    }

  // Ensure Order is up-to-date and check that current point size matches:
  if (static_cast<int>(this->PointParametricCoordinates->GetNumberOfPoints()) != this->GetOrder(3))
    {
    this->PointParametricCoordinates->Initialize();
    vtkLagrangeInterpolation::AppendHexahedronCollocationPoints(
      this->PointParametricCoordinates, this->Order);
    }

  return
    vtkDoubleArray::SafeDownCast(
      this->PointParametricCoordinates->GetData())->GetPointer(0);
}

double vtkLagrangeHexahedron::GetParametricDistance(const double pcoords[3])
{
  double pDist, pDistMax = 0.0;

  for (int ii = 0; ii < 3; ++ii)
    {
    pDist =
      (pcoords[ii] < 0. ? -pcoords[ii] :
       (pcoords[ii] > 1. ? pcoords[ii] - 1. :
        0.));
    if (pDist > pDistMax)
      {
      pDistMax = pDist;
      }
    }

  return pDistMax;
}

const int* vtkLagrangeHexahedron::GetOrder()
{
  // FIXME: The interpolation routines can handle different order along each axis
  //   but we cannot infer the order from the number of points in that case.
  //   This method currently assumes hexahedra are of the same order on each axis.
  //   We populate the Order array for use with the interpolation class.
  vtkIdType npts = this->Points->GetNumberOfPoints();
  if (this->Order[3] != npts)
    {
    int pointsPerAxis = static_cast<int>(ceil(pow(npts, 1./3.))); // number of points along each axis
    for (int i = 0; i < 3; ++i)
      {
      this->Order[i] = pointsPerAxis - 1; // order 1 is linear, 2 is quadratic, ...
      }
    this->Order[3] = static_cast<int>(npts);
    this->CellScalars->SetNumberOfTuples(npts);
    }
  return this->Order;
}

void vtkLagrangeHexahedron::InterpolateFunctions(
  const double pcoords[3], double* weights)
{
  vtkLagrangeInterpolation::Tensor3ShapeFunctions(this->GetOrder(), pcoords, weights);
}

void vtkLagrangeHexahedron::InterpolateDerivs(
  const double pcoords[3], double* derivs)
{
  vtkLagrangeInterpolation::Tensor3ShapeDerivatives(this->GetOrder(), pcoords, derivs);
}

/// Return a linear hexahedron used to approximate a region of the nonlinear hex.
vtkHexahedron* vtkLagrangeHexahedron::GetApprox()
{
  if (!this->Approx)
    {
    this->Approx = vtkSmartPointer<vtkHexahedron>::New();
    this->ApproxPD = vtkSmartPointer<vtkPointData>::New();
    this->ApproxCD = vtkSmartPointer<vtkCellData>::New();
    }
  return this->Approx.GetPointer();
}

/**\brief Prepare point data for use by linear approximating-elements.
  *
  * This copies the point data for the current cell into a new point-data
  * object so that the point ids and scalar ids can match.
  */
void vtkLagrangeHexahedron::PrepareApproxData(vtkPointData* pd, vtkCellData* cd, vtkIdType cellId, vtkDataArray* cellScalars)
{
  this->GetApprox(); // Ensure this->Approx{PD,CD} are non-NULL.
  this->GetOrder(); // Ensure the order has been updated to match this element.
  vtkIdType npts = this->Order[3];
  vtkIdType nele = this->Order[0] * this->Order[1] * this->Order[2];
  this->ApproxPD->Initialize();
  this->ApproxCD->Initialize();
  this->ApproxPD->CopyAllOn();
  this->ApproxCD->CopyAllOn();
  this->ApproxPD->CopyAllocate(pd, npts);
  this->ApproxCD->CopyAllocate(cd, nele);
  for (int pp = 0; pp < npts; ++pp)
    {
    this->ApproxPD->CopyData(pd, this->PointIds->GetId(pp), pp);
    this->CellScalars->SetValue(pp, cellScalars->GetTuple1(pp));
    }
  for (int ee = 0; ee < nele; ++ee)
    {
    this->ApproxCD->CopyData(cd, cellId, ee);
    }
}

/**\brief Populate the linear hex returned by GetApprox() with point-data from one voxel-like intervals of this cell.
  *
  * Ensure that you have called GetOrder() before calling this method
  * so that this->Order is up to date. This method does no checking
  * before using it to map connectivity-array offsets.
  */
vtkHexahedron* vtkLagrangeHexahedron::GetApproximateHex(
  int subId, vtkDataArray* scalarsIn, vtkDataArray* scalarsOut)
{
  vtkHexahedron* approx = this->GetApprox();
  bool doScalars = (scalarsIn && scalarsOut);
  if (doScalars)
    {
    scalarsOut->SetNumberOfTuples(8);
    }
  int i, j, k;
  if (!this->SubCellCoordinatesFromId(i, j, k, subId))
  {
    vtkErrorMacro("Invalid subId " << subId);
    return nullptr;
  }
  // Get the point coordinates (and optionally scalars) for each of the 8 corners
  // in the approximating hexahedron spanned by (i, i+1) x (j, j+1) x (k, k+1):
  for (int ic = 0; ic < 8; ++ic)
    {
    int corner = this->PointIndexFromIJK(
      i + ((((ic + 1) / 2) % 2) ? 1 : 0),
      j + (((ic / 2) % 2) ? 1 : 0),
      k + ((ic / 4) ? 1 : 0));
    vtkVector3d cp;
    this->Points->GetPoint(corner, cp.GetData());
    approx->Points->SetPoint(ic, cp.GetData());
    approx->PointIds->SetId(ic, doScalars ? corner : this->PointIds->GetId(corner));
    if (doScalars)
      {
      scalarsOut->SetTuple(ic,
        scalarsIn->GetTuple(
          corner));
      }
    }
  return approx;
}

/// A convenience method; see the overloaded variant for more information.
bool vtkLagrangeHexahedron::SubCellCoordinatesFromId(vtkVector3i& ijk, int subId)
{
  return this->SubCellCoordinatesFromId(ijk[0], ijk[1], ijk[2], subId);
}

/**\brief Given an integer specifying an approximating linear hex, compute its IJK coordinate-position in this cell.
 *
 * The \a subId specifies the lower-, left-, front-most vertex of the approximating hex.
 * This sets the ijk coordinates of that point.
 *
 * You must have called this->GetOrder() **before** invoking this method so that the order will be up to date.
 */
bool vtkLagrangeHexahedron::SubCellCoordinatesFromId(int& i, int& j, int& k, int subId)
{
  if (subId < 0)
    {
    return false;
    }

  int layerSize = this->Order[0] * this->Order[1];
  i = subId % this->Order[0];
  j = (subId / this->Order[0]) % this->Order[1];
  k = subId / layerSize;
  return true; // TODO: detect more invalid subId values
}

/**\brief Given (i,j,k) coordinates within the Lagrange hex, return an offset into the local connectivity (PointIds) array.
  *
  * Ensure that you have called GetOrder() before calling this method
  * so that this->Order is up to date. This method does no checking
  * before using it to map connectivity-array offsets.
  */
int vtkLagrangeHexahedron::PointIndexFromIJK(int i, int j, int k)
{
  return vtkLagrangeHexahedron::PointIndexFromIJK(i, j, k, this->Order);
}

/**\brief Given (i,j,k) coordinates within the Lagrange hex, return an offset into the local connectivity (PointIds) array.
  *
  * The \a order parameter must point to an array of 3 integers specifying the order
  * along each axis of the hexahedron.
  */
int vtkLagrangeHexahedron::PointIndexFromIJK(int i, int j, int k, const int* order)
{
  bool ibdy = (i == 0 || i == order[0]);
  bool jbdy = (j == 0 || j == order[1]);
  bool kbdy = (k == 0 || k == order[2]);
  // How many boundaries do we lie on at once?
  int nbdy = (ibdy ? 1 : 0) + (jbdy ? 1 : 0) + (kbdy ? 1 : 0);

  if (nbdy == 3) // Vertex DOF
    { // ijk is a corner node. Return the proper index (somewhere in [0,7]):
    return (i ? (j ? 2 : 1) : (j ? 3 : 0)) + (k ? 4 : 0);
    }

  int offset = 8;
  if (nbdy == 2) // Edge DOF
    {
    if (!ibdy)
      { // On i axis
      return (i - 1) +
        (j ? order[0] - 1 + order[1] - 1 : 0) +
        (k ? 2 * (order[0] - 1 + order[1] - 1) : 0) +
        offset;
      }
    if (!jbdy)
      { // On j axis
      return (j - 1) +
        (i ? order[0] - 1 : 2 * (order[0] - 1) + order[1] - 1) +
        (k ? 2 * (order[0] - 1 + order[1] - 1) : 0) +
        offset;
      }
    // !kbdy, On k axis
    offset += 4 * (order[0] - 1) + 4 * (order[1] - 1);
    return (k - 1) + (order[2] - 1) * (i ? (j ? 3 : 1) : (j ? 2 : 0)) + offset;
    }

  offset += 4 * (order[0] - 1 + order[1] - 1 + order[2] - 1);
  if (nbdy == 1) // Face DOF
    {
    if (ibdy) // On i-normal face
      {
      return (j - 1) + ((order[1] - 1) * (k - 1)) + (i ? (order[1] - 1) * (order[2] - 1) : 0) + offset;
      }
    offset += 2 * (order[1] - 1) * (order[2] - 1);
    if (jbdy) // On j-normal face
      {
      return (i - 1) + ((order[0] - 1) * (k - 1)) + (j ? (order[2] - 1) * (order[0] - 1) : 0) + offset;
      }
    offset += 2 * (order[2] - 1) * (order[0] - 1);
    // kbdy, On k-normal face
    return (i - 1) + ((order[0] - 1) * (j - 1)) + (k ? (order[0] - 1) * (order[1] - 1) : 0) + offset;
    }

  // nbdy == 0: Body DOF
  offset += 2 * (
    (order[1] - 1) * (order[2] - 1) +
    (order[2] - 1) * (order[0] - 1) +
    (order[0] - 1) * (order[1] - 1));
  return offset +
    (i - 1) + (order[0] - 1) * (
      (j - 1) + (order[1] - 1) * (
        (k - 1)));
}

/**\brief Given the index, \a subCell, of a linear approximating-hex, translate pcoords from that hex into this nonlinear hex.
  *
  * You must call this->GetOrder() **before** invoking this method as it assumes
  * the order is up to date.
  */
bool vtkLagrangeHexahedron::TransformApproxToCellParams(int subCell, double* pcoords)
{
  vtkVector3i ijk;
  if (!this->SubCellCoordinatesFromId(ijk, subCell))
    {
    return false;
    }
  for (int pp = 0; pp < 3; ++pp)
    {
    pcoords[pp] = (pcoords[pp] + ijk[pp]) / this->Order[pp];
    }
  return true;
}

/**\brief Given the index, \a subCell, of a linear approximating-hex, translate pcoords from that hex into this nonlinear hex.
  *
  * You must call this->GetOrder() **before** invoking this method as it assumes
  * the order is up to date.
  */
bool vtkLagrangeHexahedron::TransformFaceToCellParams(int bdyFace, double* pcoords)
{
  if (bdyFace < 0 || bdyFace >= 6)
    {
    return false;
    }

  vtkVector2i faceParams = vtkLagrangeInterpolation::GetVaryingParametersOfHexFace(bdyFace);
  vtkVector3d tmp(pcoords);
  int pp;
  for (pp = 0; pp < 2; ++pp)
    {
    pcoords[faceParams[pp]] = tmp[pp];
    }
  if (bdyFace % 2 == ((bdyFace / 2) % 2))
    {
    // Flip first parametric axis of "positive" faces to compensate for GetFace,
    // which flips odd faces to obtain inward-pointing normals for each boundary.
    pcoords[faceParams[0]] = 1. - pcoords[faceParams[0]];
    }
  pp = vtkLagrangeInterpolation::GetFixedParameterOfHexFace(bdyFace);
  pcoords[pp] = (bdyFace % 2 == 0 ? 0.0 : 1.0);
  return true;
}
