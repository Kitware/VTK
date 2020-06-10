/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHigherOrderHexahedron.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHigherOrderHexahedron.h"

#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkHexahedron.h"
#include "vtkHigherOrderCurve.h"
#include "vtkHigherOrderInterpolation.h"
#include "vtkHigherOrderQuadrilateral.h"
#include "vtkIdList.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkTriangle.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

vtkHigherOrderHexahedron::vtkHigherOrderHexahedron()
{
  this->Approx = nullptr;
  this->Order[0] = this->Order[1] = this->Order[2] = 1;
  // Deliberately leave this unset. When GetOrder() is called, it will construct
  // the accompanying data arrays used for other calculations.
  this->Order[3] = 0;
  this->Points->SetNumberOfPoints(8);
  this->PointIds->SetNumberOfIds(8);
  // this->CellScalars->SetNumberOfTuples(this->Order[3]);
  for (vtkIdType i = 0; i < 8; i++)
  {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
    this->PointIds->SetId(i, -1);
  }
}

vtkHigherOrderHexahedron::~vtkHigherOrderHexahedron() = default;

void vtkHigherOrderHexahedron::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Order: " << this->GetOrder(3) << "\n";
  if (this->PointParametricCoordinates)
  {
    os << indent
       << "PointParametricCoordinates: " << this->PointParametricCoordinates->GetNumberOfPoints()
       << " entries\n";
  }
  os << indent << "Approx: " << this->Approx << "\n";
}

void vtkHigherOrderHexahedron::SetEdgeIdsAndPoints(int edgeId,
  const std::function<void(const vtkIdType&)>& set_number_of_ids_and_points,
  const std::function<void(const vtkIdType&, const vtkIdType&)>& set_ids_and_points)
{
  const int* order = this->GetOrder();
  int oi = vtkHigherOrderInterpolation::GetVaryingParameterOfHexEdge(edgeId);
  vtkVector2i eidx = vtkHigherOrderInterpolation::GetPointIndicesBoundingHexEdge(edgeId);
  vtkIdType npts = order[oi] + 1;
  int sn = 0;
  set_number_of_ids_and_points(npts);
  for (int i = 0; i < 2; ++i, ++sn)
  {
    set_ids_and_points(sn, eidx[i]);
  }
  // Now add edge-interior points in axis order:
  int offset = 8;
  if (oi == 2)
  {
    offset += 4 * (order[0] + order[1] - 2);
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
    set_ids_and_points(sn, offset + jj);
  }
}

void vtkHigherOrderHexahedron::SetFaceIdsAndPoints(vtkHigherOrderQuadrilateral* result, int faceId,
  const std::function<void(const vtkIdType&)>& set_number_of_ids_and_points,
  const std::function<void(const vtkIdType&, const vtkIdType&)>& set_ids_and_points)
{
  if (faceId < 0 || faceId >= 6)
  {
    return;
  }

  // Do we need to flip the face to get an outward-pointing normal?
  bool flipFace = (faceId % 2 == ((faceId / 2) % 2) ? true : false);

  const int* order = this->GetOrder();
  vtkVector2i faceParams = vtkHigherOrderInterpolation::GetVaryingParametersOfHexFace(faceId);
  const int* corners = vtkHigherOrderInterpolation::GetPointIndicesBoundingHexFace(faceId);
  int npts = (order[faceParams[0]] + 1) * (order[faceParams[1]] + 1);
  set_number_of_ids_and_points(npts);
  result->SetOrder(order[faceParams[0]], order[faceParams[1]]);

  // Add vertex DOFs to result
  int sn = 0;
  if (!flipFace)
  {
    for (int ii = 0; ii < 4; ++ii, ++sn)
    {
      set_ids_and_points(sn, corners[ii]);
    }
  }
  else
  {
    for (int ii = 0; ii < 4; ++ii, ++sn)
    {
      set_ids_and_points((5 - sn) % 4, corners[ii]);
    }
  }

  // Add edge DOFs to result
  int offset;
  const int* faceEdges = vtkHigherOrderInterpolation::GetEdgeIndicesBoundingHexFace(faceId);
  for (int ii = 0; ii < 4; ++ii)
  {
    offset = 8;
    if (!flipFace)
    {
      const int edgeId = faceEdges[ii];
      const int pp = vtkHigherOrderInterpolation::GetVaryingParameterOfHexEdge(edgeId);
      if (pp == 2)
      {
        offset += 4 * (order[0] + order[1] - 2);
        offset += (edgeId - 8) * (order[2] - 1);
      }
      else
      {
        for (int ee = 0; ee < edgeId; ++ee)
        {
          offset += order[ee % 2 == 0 ? 0 : 1] - 1;
        }
      }
      for (int jj = 0; jj < order[pp] - 1; ++jj, ++sn)
      {
        set_ids_and_points(sn, offset + jj);
      }
    }
    else
    {
      // Flip both the edge position among edges (ii => (4 - ii) % 4)
      // and the edge's node order (jj => order[pp] - jj - 1).
      const int edgeId = faceEdges[(4 - ii) % 4];
      const int pp = vtkHigherOrderInterpolation::GetVaryingParameterOfHexEdge(edgeId);
      if (pp == 2)
      {
        offset += 4 * (order[0] + order[1] - 2);
        offset += (edgeId - 8) * (order[2] - 1);
      }
      else
      {
        for (int ee = 0; ee < edgeId; ++ee)
        {
          offset += order[ee % 2 == 0 ? 0 : 1] - 1;
        }
      }
      if (ii % 2 == 0)
      {
        for (int jj = 0; jj < order[pp] - 1; ++jj, ++sn)
        {
          set_ids_and_points(sn, offset + order[pp] - jj - 2);
        }
      }
      else
      {
        for (int jj = 0; jj < order[pp] - 1; ++jj, ++sn)
        {
          set_ids_and_points(sn, offset + jj);
        }
      }
    }
  }

  // Now add face DOF
  offset = 8 + 4 * (order[0] + order[1] + order[2] - 3);
  // skip DOF for other faces of hex before this one
  for (int ff = 0; ff < faceId; ++ff)
  {
    vtkVector2i tmp = vtkHigherOrderInterpolation::GetVaryingParametersOfHexFace(ff);
    offset += (order[tmp[0]] - 1) * (order[tmp[1]] - 1);
  }
  if (!flipFace)
  {
    int nfdof = (order[faceParams[0]] - 1) * (order[faceParams[1]] - 1);
    for (int ii = 0; ii < nfdof; ++ii, ++sn)
    {
      set_ids_and_points(sn, offset + ii);
    }
  }
  else
  {
    int delta = order[faceParams[0]] - 1;
    for (int jj = 0; jj < (order[faceParams[1]] - 1); ++jj)
    {
      for (int ii = delta - 1; ii >= 0; --ii, ++sn)
      {
        set_ids_and_points(sn, offset + ii + jj * delta);
      }
    }
  }
}

void vtkHigherOrderHexahedron::Initialize() {}

int vtkHigherOrderHexahedron::CellBoundary(
  int vtkNotUsed(subId), const double pcoords[3], vtkIdList* pts)
{
  double t1 = pcoords[0] - pcoords[1];
  double t2 = 1.0 - pcoords[0] - pcoords[1];
  double t3 = pcoords[1] - pcoords[2];
  double t4 = 1.0 - pcoords[1] - pcoords[2];
  double t5 = pcoords[2] - pcoords[0];
  double t6 = 1.0 - pcoords[2] - pcoords[0];

  pts->SetNumberOfIds(4);

  // compare against six planes in parametric space that divide element
  // into six pieces.
  if (t3 >= 0.0 && t4 >= 0.0 && t5 < 0.0 && t6 >= 0.0)
  {
    pts->SetId(0, this->PointIds->GetId(0));
    pts->SetId(1, this->PointIds->GetId(1));
    pts->SetId(2, this->PointIds->GetId(2));
    pts->SetId(3, this->PointIds->GetId(3));
  }

  else if (t1 >= 0.0 && t2 < 0.0 && t5 < 0.0 && t6 < 0.0)
  {
    pts->SetId(0, this->PointIds->GetId(1));
    pts->SetId(1, this->PointIds->GetId(2));
    pts->SetId(2, this->PointIds->GetId(6));
    pts->SetId(3, this->PointIds->GetId(5));
  }

  else if (t1 >= 0.0 && t2 >= 0.0 && t3 < 0.0 && t4 >= 0.0)
  {
    pts->SetId(0, this->PointIds->GetId(0));
    pts->SetId(1, this->PointIds->GetId(1));
    pts->SetId(2, this->PointIds->GetId(5));
    pts->SetId(3, this->PointIds->GetId(4));
  }

  else if (t3 < 0.0 && t4 < 0.0 && t5 >= 0.0 && t6 < 0.0)
  {
    pts->SetId(0, this->PointIds->GetId(4));
    pts->SetId(1, this->PointIds->GetId(5));
    pts->SetId(2, this->PointIds->GetId(6));
    pts->SetId(3, this->PointIds->GetId(7));
  }

  else if (t1 < 0.0 && t2 >= 0.0 && t5 >= 0.0 && t6 >= 0.0)
  {
    pts->SetId(0, this->PointIds->GetId(0));
    pts->SetId(1, this->PointIds->GetId(4));
    pts->SetId(2, this->PointIds->GetId(7));
    pts->SetId(3, this->PointIds->GetId(3));
  }

  else // if ( t1 < 0.0 && t2 < 0.0 && t3 >= 0.0 && t6 < 0.0 )
  {
    pts->SetId(0, this->PointIds->GetId(2));
    pts->SetId(1, this->PointIds->GetId(3));
    pts->SetId(2, this->PointIds->GetId(7));
    pts->SetId(3, this->PointIds->GetId(6));
  }

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

int vtkHigherOrderHexahedron::EvaluatePosition(const double x[3], double closestPoint[3],
  int& subId, double pcoords[3], double& minDist2, double weights[])
{
  int result = 0;

  int dummySubId;
  double linearWeights[8];
  double tmpDist2;
  vtkVector3d params;
  vtkVector3d tmpClosestPt;

  minDist2 = VTK_DOUBLE_MAX;
  vtkIdType nhex = vtkHigherOrderInterpolation::NumberOfIntervals<3>(this->GetOrder());
  for (int subCell = 0; subCell < nhex; ++subCell)
  {
    vtkHexahedron* approx = this->GetApproximateHex(subCell, nullptr, nullptr);
    int stat = approx->EvaluatePosition(
      x, tmpClosestPt.GetData(), dummySubId, params.GetData(), tmpDist2, linearWeights);
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

void vtkHigherOrderHexahedron::EvaluateLocation(
  int& subId, const double pcoords[3], double x[3], double* weights)
{
  subId = 0; // LagrangeHexahedron tests that this is set to 0
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

void vtkHigherOrderHexahedron::Contour(double value, vtkDataArray* cellScalars,
  vtkIncrementalPointLocator* locator, vtkCellArray* verts, vtkCellArray* lines,
  vtkCellArray* polys, vtkPointData* inPd, vtkPointData* outPd, vtkCellData* inCd, vtkIdType cellId,
  vtkCellData* outCd)
{
  this->PrepareApproxData(
    inPd, inCd, cellId, cellScalars); // writes to this->{CellScalars, ApproxPD, ApproxCD}
  vtkIdType nhex = vtkHigherOrderInterpolation::NumberOfIntervals<3>(this->GetOrder());
  for (int i = 0; i < nhex; ++i)
  {
    vtkHexahedron* approx =
      this->GetApproximateHex(i, this->CellScalars.GetPointer(), this->Scalars.GetPointer());
    approx->Contour(value, this->Scalars.GetPointer(), locator, verts, lines, polys, this->ApproxPD,
      outPd, this->ApproxCD, cellId, outCd);
  }
}

void vtkHigherOrderHexahedron::Clip(double value, vtkDataArray* cellScalars,
  vtkIncrementalPointLocator* locator, vtkCellArray* polys, vtkPointData* inPd, vtkPointData* outPd,
  vtkCellData* inCd, vtkIdType cellId, vtkCellData* outCd, int insideOut)
{
  this->PrepareApproxData(
    inPd, inCd, cellId, cellScalars); // writes to this->{CellScalars, ApproxPD, ApproxCD}
  vtkIdType nhex = vtkHigherOrderInterpolation::NumberOfIntervals<3>(this->GetOrder());
  for (int i = 0; i < nhex; ++i)
  {
    vtkHexahedron* approx =
      this->GetApproximateHex(i, this->CellScalars.GetPointer(), this->Scalars.GetPointer());
    approx->Clip(value, this->Scalars.GetPointer(), locator, polys, this->ApproxPD, outPd,
      this->ApproxCD, cellId, outCd, insideOut);
  }
}

int vtkHigherOrderHexahedron::IntersectWithLine(
  const double* p1, const double* p2, double tol, double& t, double* x, double* pcoords, int& subId)
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

int vtkHigherOrderHexahedron::Triangulate(int vtkNotUsed(index), vtkIdList* ptIds, vtkPoints* pts)
{
  ptIds->Reset();
  pts->Reset();

  vtkIdType nhex = vtkHigherOrderInterpolation::NumberOfIntervals<3>(this->GetOrder());
  for (int i = 0; i < nhex; ++i)
  {
    vtkHexahedron* approx = this->GetApproximateHex(i);
    if (approx->Triangulate(1, this->TmpIds.GetPointer(), this->TmpPts.GetPointer()))
    {
      // Sigh. Triangulate methods all reset their points/ids
      // so we must copy them to our output.
      vtkIdType np = this->TmpPts->GetNumberOfPoints();
      vtkIdType ni = this->TmpIds->GetNumberOfIds();
      for (vtkIdType ii = 0; ii < np; ++ii)
      {
        pts->InsertNextPoint(this->TmpPts->GetPoint(ii));
      }
      for (vtkIdType ii = 0; ii < ni; ++ii)
      {
        ptIds->InsertNextId(this->TmpIds->GetId(ii));
      }
    }
  }
  return 1;
}

void vtkHigherOrderHexahedron::Derivatives(
  int vtkNotUsed(subId), const double pcoords[3], const double* values, int dim, double* derivs)
{
  this->getInterp()->Tensor3EvaluateDerivative(
    this->Order, pcoords, this->GetPoints(), values, dim, derivs);
}

void vtkHigherOrderHexahedron::SetParametricCoords()
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
    vtkHigherOrderInterpolation::AppendHexahedronCollocationPoints(
      this->PointParametricCoordinates, this->Order);
  }
}

double* vtkHigherOrderHexahedron::GetParametricCoords()
{
  this->SetParametricCoords();

  return vtkDoubleArray::SafeDownCast(this->PointParametricCoordinates->GetData())->GetPointer(0);
}

double vtkHigherOrderHexahedron::GetParametricDistance(const double pcoords[3])
{
  double pDist, pDistMax = 0.0;

  for (int ii = 0; ii < 3; ++ii)
  {
    pDist = (pcoords[ii] < 0. ? -pcoords[ii] : (pcoords[ii] > 1. ? pcoords[ii] - 1. : 0.));
    if (pDist > pDistMax)
    {
      pDistMax = pDist;
    }
  }

  return pDistMax;
}

/// Return a linear hexahedron used to approximate a region of the nonlinear hex.
vtkHexahedron* vtkHigherOrderHexahedron::GetApprox()
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
void vtkHigherOrderHexahedron::PrepareApproxData(
  vtkPointData* pd, vtkCellData* cd, vtkIdType cellId, vtkDataArray* cellScalars)
{
  this->GetApprox(); // Ensure this->Approx{PD,CD} are non-NULL.
  // this->GetOrder(); // Ensure the order has been updated to match this element.
  this->SetOrderFromCellData(cd, this->Points->GetNumberOfPoints(), cellId);
  vtkIdType npts = this->Order[3];
  vtkIdType nele = this->Order[0] * this->Order[1] * this->Order[2];
  this->ApproxPD->Initialize();
  this->ApproxCD->Initialize();
  this->ApproxPD->CopyAllOn();
  this->ApproxCD->CopyAllOn();
  this->ApproxPD->CopyAllocate(pd, npts);
  this->ApproxCD->CopyAllocate(cd, nele);
  this->CellScalars->SetNumberOfTuples(npts);
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

/// A convenience method; see the overloaded variant for more information.
bool vtkHigherOrderHexahedron::SubCellCoordinatesFromId(vtkVector3i& ijk, int subId)
{
  return this->SubCellCoordinatesFromId(ijk[0], ijk[1], ijk[2], subId);
}

/**\brief Given an integer specifying an approximating linear hex, compute its IJK
 * coordinate-position in this cell.
 *
 * The \a subId specifies the lower-, left-, front-most vertex of the approximating hex.
 * This sets the ijk coordinates of that point.
 *
 * You must have called this->GetOrder() **before** invoking this method so that the order will be
 * up to date.
 */
bool vtkHigherOrderHexahedron::SubCellCoordinatesFromId(int& i, int& j, int& k, int subId)
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

/**\brief Given (i,j,k) coordinates within the HigherOrder hex, return an offset into the local
 * connectivity (PointIds) array.
 *
 * Ensure that you have called GetOrder() before calling this method
 * so that this->Order is up to date. This method does no checking
 * before using it to map connectivity-array offsets.
 */
int vtkHigherOrderHexahedron::PointIndexFromIJK(int i, int j, int k)
{
  return vtkHigherOrderHexahedron::PointIndexFromIJK(i, j, k, this->Order);
}

/**\brief Given (i,j,k) coordinates within the HigherOrder hex, return an offset into the local
 * connectivity (PointIds) array.
 *
 * The \a order parameter must point to an array of 3 integers specifying the order
 * along each axis of the hexahedron.
 */
int vtkHigherOrderHexahedron::PointIndexFromIJK(int i, int j, int k, const int* order)
{
  bool ibdy = (i == 0 || i == order[0]);
  bool jbdy = (j == 0 || j == order[1]);
  bool kbdy = (k == 0 || k == order[2]);
  // How many boundaries do we lie on at once?
  int nbdy = (ibdy ? 1 : 0) + (jbdy ? 1 : 0) + (kbdy ? 1 : 0);

  if (nbdy == 3) // Vertex DOF
  {              // ijk is a corner node. Return the proper index (somewhere in [0,7]):
    return (i ? (j ? 2 : 1) : (j ? 3 : 0)) + (k ? 4 : 0);
  }

  int offset = 8;
  if (nbdy == 2) // Edge DOF
  {
    if (!ibdy)
    { // On i axis
      return (i - 1) + (j ? order[0] + order[1] - 2 : 0) + (k ? 2 * (order[0] + order[1] - 2) : 0) +
        offset;
    }
    if (!jbdy)
    { // On j axis
      return (j - 1) + (i ? order[0] - 1 : 2 * (order[0] - 1) + order[1] - 1) +
        (k ? 2 * (order[0] + order[1] - 2) : 0) + offset;
    }
    // !kbdy, On k axis
    offset += 4 * (order[0] - 1) + 4 * (order[1] - 1);
    return (k - 1) + (order[2] - 1) * (i ? (j ? 2 : 1) : (j ? 3 : 0)) + offset;
  }

  offset += 4 * (order[0] + order[1] + order[2] - 3);
  if (nbdy == 1) // Face DOF
  {
    if (ibdy) // On i-normal face
    {
      return (j - 1) + ((order[1] - 1) * (k - 1)) + (i ? (order[1] - 1) * (order[2] - 1) : 0) +
        offset;
    }
    offset += 2 * (order[1] - 1) * (order[2] - 1);
    if (jbdy) // On j-normal face
    {
      return (i - 1) + ((order[0] - 1) * (k - 1)) + (j ? (order[2] - 1) * (order[0] - 1) : 0) +
        offset;
    }
    offset += 2 * (order[2] - 1) * (order[0] - 1);
    // kbdy, On k-normal face
    return (i - 1) + ((order[0] - 1) * (j - 1)) + (k ? (order[0] - 1) * (order[1] - 1) : 0) +
      offset;
  }

  // nbdy == 0: Body DOF
  offset += 2 *
    ((order[1] - 1) * (order[2] - 1) + (order[2] - 1) * (order[0] - 1) +
      (order[0] - 1) * (order[1] - 1));
  return offset + (i - 1) + (order[0] - 1) * ((j - 1) + (order[1] - 1) * ((k - 1)));
}

vtkIdType vtkHigherOrderHexahedron::NodeNumberingMappingFromVTK8To9(
  const int order[3], const vtkIdType node_id_vtk8)
{
  int numPtsPerEdgeWithoutCorners[3];
  numPtsPerEdgeWithoutCorners[0] = order[0] - 1;
  numPtsPerEdgeWithoutCorners[1] = order[1] - 1;
  numPtsPerEdgeWithoutCorners[2] = order[2] - 1;

  int offset = 8 + 4 * (numPtsPerEdgeWithoutCorners[0] + numPtsPerEdgeWithoutCorners[1]) +
    2 * numPtsPerEdgeWithoutCorners[2];
  if ((node_id_vtk8 < offset) || (node_id_vtk8 >= offset + 2 * numPtsPerEdgeWithoutCorners[2]))
    return node_id_vtk8;
  else if (node_id_vtk8 < offset + numPtsPerEdgeWithoutCorners[2])
    return node_id_vtk8 + numPtsPerEdgeWithoutCorners[2];
  else
    return node_id_vtk8 - numPtsPerEdgeWithoutCorners[2];
}

/**\brief Given the index, \a subCell, of a linear approximating-hex, translate pcoords from that
 * hex into this nonlinear hex.
 *
 * You must call this->GetOrder() **before** invoking this method as it assumes
 * the order is up to date.
 */
bool vtkHigherOrderHexahedron::TransformApproxToCellParams(int subCell, double* pcoords)
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

/**\brief Given the index, \a subCell, of a linear approximating-hex, translate pcoords from that
 * hex into this nonlinear hex.
 *
 * You must call this->GetOrder() **before** invoking this method as it assumes
 * the order is up to date.
 */
bool vtkHigherOrderHexahedron::TransformFaceToCellParams(int bdyFace, double* pcoords)
{
  if (bdyFace < 0 || bdyFace >= 6)
  {
    return false;
  }

  vtkVector2i faceParams = vtkHigherOrderInterpolation::GetVaryingParametersOfHexFace(bdyFace);
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
  pp = vtkHigherOrderInterpolation::GetFixedParameterOfHexFace(bdyFace);
  pcoords[pp] = (bdyFace % 2 == 0 ? 0.0 : 1.0);
  return true;
}

/**\brief Set the degree  of the cell, given a vtkDataSet and cellId
 */
void vtkHigherOrderHexahedron::SetOrderFromCellData(
  vtkCellData* cell_data, const vtkIdType numPts, const vtkIdType cell_id)
{
  if (cell_data->SetActiveAttribute(
        "HigherOrderDegrees", vtkDataSetAttributes::AttributeTypes::HIGHERORDERDEGREES) != -1)
  {
    double degs[3];
    vtkDataArray* v = cell_data->GetHigherOrderDegrees();
    v->GetTuple(cell_id, degs);
    this->SetOrder(degs[0], degs[1], degs[2]);
    if (this->Order[3] != numPts)
      vtkErrorMacro("The degrees are not correctly set in the input file.");
  }
  else
  {
    this->SetUniformOrderFromNumPoints(numPts);
  }
}

void vtkHigherOrderHexahedron::SetUniformOrderFromNumPoints(vtkIdType numPts)
{
  const int deg = static_cast<int>(round(std::cbrt(static_cast<int>(numPts)))) - 1;
  this->SetOrder(deg, deg, deg);
  if (static_cast<int>(numPts) != this->Order[3])
    vtkErrorMacro("The degrees are direction dependents, and should be set in the input file.");
}

void vtkHigherOrderHexahedron::SetOrder(int s, int t, int u)
{
  if (this->PointParametricCoordinates && (Order[0] != s || Order[1] != t || Order[2] != u))
    this->PointParametricCoordinates->Reset();
  Order[0] = s;
  Order[1] = t;
  Order[2] = u;
  Order[3] = (s + 1) * (t + 1) * (u + 1);
}

const int* vtkHigherOrderHexahedron::GetOrder()
{
  //   The interpolation routines can handle different order along each axis
  //   The connectivity array contains three additional entries at the end which specify the Order
  //   in s, t, and u The unstructure grid calls SetOrder with those three additional entries
  vtkIdType numPts = this->Points->GetNumberOfPoints();
  if (this->Order[3] != numPts)
  {
    if (numPts == 8)
      this->SetUniformOrderFromNumPoints(numPts);
    else
      vtkErrorMacro("The degrees might be direction dependents, and should be set before GetOrder "
                    "is called. numPts is "
        << numPts << " and Order[3] " << Order[3]);
  }
  return this->Order;
}
