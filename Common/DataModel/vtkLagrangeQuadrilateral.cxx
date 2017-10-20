/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLagrangeQuadrilateral.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLagrangeQuadrilateral.h"

#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkLagrangeCurve.h"
#include "vtkLagrangeInterpolation.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkQuad.h"
#include "vtkTriangle.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

vtkStandardNewMacro(vtkLagrangeQuadrilateral);

vtkLagrangeQuadrilateral::vtkLagrangeQuadrilateral()
{
  this->Approx = nullptr;
  this->Order[0] = this->Order[1] = this->Order[2] = 1;
  this->Points->SetNumberOfPoints(4);
  this->PointIds->SetNumberOfIds(4);
  for (int i = 0; i < 4; i++)
    {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
    this->PointIds->SetId(i,-1);
    }
}

vtkLagrangeQuadrilateral::~vtkLagrangeQuadrilateral()
{
}

void vtkLagrangeQuadrilateral::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Order: " << this->GetOrder(0) << "\n";
  if (this->PointParametricCoordinates)
    {
    os
      << indent << "PointParametricCoordinates: "
      << this->PointParametricCoordinates->GetNumberOfPoints()
      << " entries\n";
    }
  os << indent << "Approx: " << this->Approx << "\n";
}

vtkCell* vtkLagrangeQuadrilateral::GetEdge(int edgeId)
{
  vtkLagrangeCurve* result = this->EdgeCell.GetPointer();
  const int* order = this->GetOrder();
  // Note in calls below: quad has same edges as first 4 of hex
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
  int offset = 4;
  for (int ee = 0; ee < edgeId; ++ee)
    {
    offset += order[ee % 2 == 0 ? 0 : 1] - 1;
    }
  for (int jj = 0; jj < order[oi] - 1; ++jj, ++sn)
    {
    result->Points->SetPoint(sn, this->Points->GetPoint(offset + jj));
    result->PointIds->SetId(sn, this->PointIds->GetId(offset + jj));
    }
  return result;
}

void vtkLagrangeQuadrilateral::Initialize()
{
}

int vtkLagrangeQuadrilateral::CellBoundary(
  int vtkNotUsed(subId), double pcoords[3], vtkIdList* pts)
{
  double t1=pcoords[0]-pcoords[1];
  double t2=1.0-pcoords[0]-pcoords[1];

  pts->SetNumberOfIds(2);

  // compare against two lines in parametric space that divide element
  // into four pieces.
  if ( t1 >= 0.0 && t2 >= 0.0 )
  {
    pts->SetId(0,this->PointIds->GetId(0));
    pts->SetId(1,this->PointIds->GetId(1));
  }

  else if ( t1 >= 0.0 && t2 < 0.0 )
  {
    pts->SetId(0,this->PointIds->GetId(1));
    pts->SetId(1,this->PointIds->GetId(2));
  }

  else if ( t1 < 0.0 && t2 < 0.0 )
  {
    pts->SetId(0,this->PointIds->GetId(2));
    pts->SetId(1,this->PointIds->GetId(3));
  }

  else //( t1 < 0.0 && t2 >= 0.0 )
  {
    pts->SetId(0,this->PointIds->GetId(3));
    pts->SetId(1,this->PointIds->GetId(0));
  }

  if ( pcoords[0] < 0.0 || pcoords[0] > 1.0 ||
       pcoords[1] < 0.0 || pcoords[1] > 1.0 )
  {
    return 0;
  }
  else
  {
    return 1;
  }
}

int vtkLagrangeQuadrilateral::EvaluatePosition(
  double* x,
  double* closestPoint,
  int& subId,
  double pcoords[3],
  double& minDist2,
  double* weights)
{
  int result = 0;

  int dummySubId;
  double linearWeights[4];
  double tmpDist2;
  vtkVector3d params;
  vtkVector3d tmpClosestPt;

  minDist2 = VTK_DOUBLE_MAX;
  vtkIdType nquad = vtkLagrangeInterpolation::NumberOfIntervals<2>(this->GetOrder());
  for (int subCell = 0; subCell < nquad; ++subCell)
    {
    vtkQuad* approx = this->GetApproximateQuad(subCell, nullptr, nullptr);
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

void vtkLagrangeQuadrilateral::EvaluateLocation(
  int& subId,
  double pcoords[3],
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

void vtkLagrangeQuadrilateral::Contour(
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
  vtkIdType nquad = vtkLagrangeInterpolation::NumberOfIntervals<2>(this->GetOrder());
  for (int i = 0; i < nquad; ++i)
    {
    vtkQuad* approx = this->GetApproximateQuad(i, this->CellScalars.GetPointer(), this->Scalars.GetPointer());
    approx->Contour(
      value, this->Scalars.GetPointer(), locator,
      verts, lines, polys, this->ApproxPD, outPd, this->ApproxCD, cellId, outCd);
    }
}

void vtkLagrangeQuadrilateral::Clip(
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
  vtkIdType nquad = vtkLagrangeInterpolation::NumberOfIntervals<2>(this->GetOrder());
  for (int i = 0; i < nquad; ++i)
    {
    vtkQuad* approx = this->GetApproximateQuad(i, this->CellScalars.GetPointer(), this->Scalars.GetPointer());
    approx->Clip(
      value, this->Scalars.GetPointer(), locator,
      polys, this->ApproxPD, outPd, this->ApproxCD, cellId,
      outCd, insideOut);
    }
}

int vtkLagrangeQuadrilateral::IntersectWithLine(
  double* p1,
  double* p2,
  double tol,
  double& t,
  double* x,
  double* pcoords,
  int& subId)
{
  vtkIdType nquad = vtkLagrangeInterpolation::NumberOfIntervals<2>(this->GetOrder());
  double tFirst = VTK_DOUBLE_MAX;
  bool intersection = false;
  vtkVector3d tmpX;
  vtkVector3d tmpP;
  int tmpId;
  for (int i = 0; i < nquad; ++i)
  {
    vtkQuad* approx = this->GetApproximateQuad(i);
    if (approx->IntersectWithLine(p1, p2, tol, t, tmpX.GetData(), tmpP.GetData(), tmpId))
    {
      // Record the point closest to p1 in the direction of p2 unless there is no other intersection,
      // in which case we will report a point "before" p1 (further from p2 than p1).
      if (!intersection || (t >= 0 && (t < tFirst || tFirst < 0)))
      {
        tFirst = t;
        subId = i;
        for (int ii = 0; ii < 3; ++ii)
        {
          x[ii] = tmpX[ii];
          pcoords[ii] = tmpP[ii]; // Translate this after we're sure it's the closest hit.
        }
      }
      intersection = true;
    }
  }
  if (intersection)
  {
    intersection &= this->TransformApproxToCellParams(subId, pcoords);
    t = tFirst;
  }
  return intersection ? 1 : 0;
}

int vtkLagrangeQuadrilateral::Triangulate(
  int vtkNotUsed(index),
  vtkIdList* ptIds,
  vtkPoints* pts)
{
  ptIds->Reset();
  pts->Reset();

  vtkIdType nquad = vtkLagrangeInterpolation::NumberOfIntervals<2>(this->GetOrder());
  vtkVector3i ijk;
  for (int i = 0; i < nquad; ++i)
    {
    vtkQuad* approx = this->GetApproximateQuad(i);
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

void vtkLagrangeQuadrilateral::Derivatives(
  int vtkNotUsed(subId),
  double vtkNotUsed(pcoords)[3],
  double* vtkNotUsed(values),
  int vtkNotUsed(dim),
  double* vtkNotUsed(derivs))
{
  // TODO: Fill me in?
  return;
}

double* vtkLagrangeQuadrilateral::GetParametricCoords()
{
  if (!this->PointParametricCoordinates)
    {
    this->PointParametricCoordinates = vtkSmartPointer<vtkPoints>::New();
    this->PointParametricCoordinates->SetDataTypeToDouble();
    }

  // Ensure Order is up-to-date and check that current point size matches:
  if (static_cast<int>(this->PointParametricCoordinates->GetNumberOfPoints()) != this->GetOrder(2))
    {
    this->PointParametricCoordinates->Initialize();
    vtkLagrangeInterpolation::AppendQuadrilateralCollocationPoints(
      this->PointParametricCoordinates, this->Order);
    }

  return
    vtkDoubleArray::SafeDownCast(
      this->PointParametricCoordinates->GetData())->GetPointer(0);
}

double vtkLagrangeQuadrilateral::GetParametricDistance(double pcoords[3])
{
  double pDist, pDistMax = 0.0;

  for (int ii = 0; ii < 2; ++ii)
    {
    pDist =
      (pcoords[ii] < 0. ? -pcoords[ii] :
       (pcoords[ii] > 1. ? pcoords[ii] - 1. :
        0.));
    if ( pDist > pDistMax )
      {
      pDistMax = pDist;
      }
    }

  // The quadrilateral's 3rd parametric coordinate should always be 0:
  if (pcoords[2] != 0.0 && (pDist = std::abs(pcoords[2])) > pDistMax)
    {
    pDistMax = pDist;
    }

  return pDistMax;
}

const int* vtkLagrangeQuadrilateral::GetOrder()
{
  // FIXME: The interpolation routines can handle different order along each axis
  //   but we cannot infer the order from the number of points in that case.
  //   This method currrently assumes quads are of the same order on each axis.
  //   We populate the Order array for use with the interpolation class.
  vtkIdType npts = this->Points->GetNumberOfPoints();
  if (this->Order[2] != npts)
    {
    int pointsPerAxis = static_cast<int>(ceil(pow(npts, 1./2.))); // number of points along each axis
    for (int i = 0; i < 2; ++i)
      {
      this->Order[i] = pointsPerAxis - 1; // order 1 is linear, 2 is quadratic, ...
      }
    this->Order[2] = static_cast<int>(npts);
    this->CellScalars->SetNumberOfTuples(npts);
    }
  return this->Order;
}

void vtkLagrangeQuadrilateral::InterpolateFunctions(
  double pcoords[3], double* weights)
{
  vtkLagrangeInterpolation::Tensor2ShapeFunctions(this->GetOrder(), pcoords, weights);
}

void vtkLagrangeQuadrilateral::InterpolateDerivs(
  double pcoords[3], double* derivs)
{
  vtkLagrangeInterpolation::Tensor2ShapeDerivatives(this->GetOrder(), pcoords, derivs);
}

/// Return a linear quadrilateral used to approximate a region of the nonlinear quadrilateral.
vtkQuad* vtkLagrangeQuadrilateral::GetApprox()
{
  if (!this->Approx)
    {
    this->Approx = vtkSmartPointer<vtkQuad>::New();
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
void vtkLagrangeQuadrilateral::PrepareApproxData(vtkPointData* pd, vtkCellData* cd, vtkIdType cellId, vtkDataArray* cellScalars)
{
  this->GetApprox(); // Ensure this->Approx{PD,CD} are non-NULL.
  this->GetOrder(); // Ensure the order has been updated to match this element.
  vtkIdType npts = this->Order[2];
  vtkIdType nele = this->Order[0] * this->Order[1];
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

/**\brief Populate the linear quadrilateral returned by GetApprox() with point-data from one voxel-like interval of this cell.
  *
  * Ensure that you have called GetOrder() before calling this method
  * so that this->Order is up to date. This method does no checking
  * before using it to map connectivity-array offsets.
  */
vtkQuad* vtkLagrangeQuadrilateral::GetApproximateQuad(
  int subId, vtkDataArray* scalarsIn, vtkDataArray* scalarsOut)
{
  vtkQuad* approx = this->GetApprox();
  bool doScalars = (scalarsIn && scalarsOut);
  if (doScalars)
    {
    scalarsOut->SetNumberOfTuples(4);
    }
  int i, j, k;
  if (!this->SubCellCoordinatesFromId(i, j, k, subId))
  {
    vtkErrorMacro("Invalid subId " << subId);
    return nullptr;
  }
  // Get the point ids (and optionally scalars) for each of the 4 corners
  // in the approximating quadrilateral spanned by (i, i+1) x (j, j+1):
  for (int ic = 0; ic < 4; ++ic)
    {
    int corner = this->PointIndexFromIJK(
      i + ((((ic + 1) / 2) % 2) ? 1 : 0),
      j + (((ic / 2) % 2) ? 1 : 0),
      0);
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
bool vtkLagrangeQuadrilateral::SubCellCoordinatesFromId(vtkVector3i& ijk, int subId)
{
  return this->SubCellCoordinatesFromId(ijk[0], ijk[1], ijk[2], subId);
}

/**\brief Given an integer specifying an approximating linear quad, compute its IJK coordinate-position in this cell.
 *
 * The \a subId specifies the lower-, left-, front-most vertex of the approximating quad.
 * This sets the ijk coordinates of that point.
 *
 * You must have called this->GetOrder() **before** invoking this method so that the order will be up to date.
 */
bool vtkLagrangeQuadrilateral::SubCellCoordinatesFromId(int& i, int& j, int& k, int subId)
{
  if (subId < 0)
    {
    return false;
    }

  i = subId % this->Order[0];
  j = (subId / this->Order[0]) % this->Order[1];
  k = 0;
  return i + this->Order[0] * j == subId ? true : false;
}

/**\brief A convenience function to get a connectivity offset from a control-point tuple.
  *
  * Ensure that you have called GetOrder() before calling this method
  * so that this->Order is up to date. This method does no checking
  * before using it to map connectivity-array offsets.
  */
int vtkLagrangeQuadrilateral::PointIndexFromIJK(int i, int j, int vtkNotUsed(k))
{
  return vtkLagrangeQuadrilateral::PointIndexFromIJK(i, j, this->Order);
}

/**\brief Given (i,j,k) coordinates within the Lagrange quad, return an offset into the local connectivity (PointIds) array.
  *
  * The \a order parameter must point to the start of an array of 2 integers.
  */
int vtkLagrangeQuadrilateral::PointIndexFromIJK(int i, int j, const int* order)
{
  bool ibdy = (i == 0 || i == order[0]);
  bool jbdy = (j == 0 || j == order[1]);
  // How many boundaries do we lie on at once?
  int nbdy = (ibdy ? 1 : 0) + (jbdy ? 1 : 0);

  if (nbdy == 2) // Vertex DOF
    { // ijk is a corner node. Return the proper index (somewhere in [0,7]):
    return (i ? (j ? 2 : 1) : (j ? 3 : 0));
    }

  int offset = 4;
  if (nbdy == 1) // Edge DOF
    {
    if (!ibdy)
      { // On i axis
      return (i - 1) +
        (j ? order[0] - 1 + order[1] - 1 : 0) +
        offset;
      }
    if (!jbdy)
      { // On j axis
      return (j - 1) +
        (i ? order[0] - 1 : 2 * (order[0] - 1) + order[1] - 1) +
        offset;
      }
    }

  offset += 2 * (order[0] - 1 + order[1] - 1);
  // nbdy == 0: Face DOF
  return offset +
    (i - 1) + (order[0] - 1) * (
      (j - 1));
}

/**\brief Given the index, \a subCell, of a linear approximating-quad, translate pcoords from that quad into this nonlinear quad.
  *
  * You must call this->GetOrder() **before** invoking this method as it assumes
  * the order is up to date.
  */
bool vtkLagrangeQuadrilateral::TransformApproxToCellParams(int subCell, double* pcoords)
{
  vtkVector3i ijk;
  if (!this->SubCellCoordinatesFromId(ijk, subCell))
    {
    return false;
    }
  for (int pp = 0; pp < 2; ++pp)
    {
    pcoords[pp] = (pcoords[pp] + ijk[pp]) / this->Order[pp];
    }
  pcoords[2] = 0.;
  return true;
}
