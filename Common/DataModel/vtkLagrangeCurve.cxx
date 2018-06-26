/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLagrangeCurve.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLagrangeCurve.h"

#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkLine.h"
#include "vtkIdList.h"
#include "vtkLagrangeInterpolation.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkTriangle.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

vtkStandardNewMacro(vtkLagrangeCurve);

vtkLagrangeCurve::vtkLagrangeCurve()
{
  this->Approx = nullptr;
  this->Order[0] = 1;
  this->Order[1] = 2;
  this->Points->SetNumberOfPoints(2);
  this->PointIds->SetNumberOfIds(2);
  for (int i = 0; i < 2; i++)
    {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
    this->PointIds->SetId(i, i);
    }
}

vtkLagrangeCurve::~vtkLagrangeCurve() = default;

void vtkLagrangeCurve::PrintSelf(ostream& os, vtkIndent indent)
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

void vtkLagrangeCurve::Initialize()
{
}

int vtkLagrangeCurve::CellBoundary(
  int vtkNotUsed(subId), const double pcoords[3], vtkIdList* pts)
{
  pts->SetNumberOfIds(1);
  if (pcoords[0] <= 0.5)
  {
    pts->SetId(0, this->TmpIds->GetId(0));
  }
  else
  {
    pts->SetId(0, this->TmpIds->GetId(this->TmpIds->GetNumberOfIds()));
  }
  return pcoords[0] >= 0.0 && pcoords[0] <= 1.0 ? 1 : 0;
}

int vtkLagrangeCurve::EvaluatePosition(
  const double x[3],
  double closestPoint[3],
  int& subId,
  double pcoords[3],
  double& minDist2,
  double weights[])
{
  int result = 0;

  int dummySubId;
  double linearWeights[2];
  double tmpDist2;
  vtkVector3d params;
  vtkVector3d tmpClosestPt;

  minDist2 = VTK_DOUBLE_MAX;
  vtkIdType nseg = vtkLagrangeInterpolation::NumberOfIntervals<1>(this->GetOrder());
  for (int subCell = 0; subCell < nseg; ++subCell)
    {
    vtkLine* approx = this->GetApproximateLine(subCell, nullptr, nullptr);
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

void vtkLagrangeCurve::EvaluateLocation(
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

void vtkLagrangeCurve::Contour(
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
  vtkIdType nseg = vtkLagrangeInterpolation::NumberOfIntervals<1>(this->GetOrder());
  for (int i = 0; i < nseg; ++i)
    {
    vtkLine* approx = this->GetApproximateLine(i, this->CellScalars.GetPointer(), this->Scalars.GetPointer());
    approx->Contour(
      value, this->Scalars.GetPointer(), locator,
      verts, lines, polys, this->ApproxPD, outPd, this->ApproxCD, cellId, outCd);
    }
}

void vtkLagrangeCurve::Clip(
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
  vtkIdType nseg = vtkLagrangeInterpolation::NumberOfIntervals<1>(this->GetOrder());
  for (int i = 0; i < nseg; ++i)
    {
    vtkLine* approx = this->GetApproximateLine(i, this->CellScalars.GetPointer(), this->Scalars.GetPointer());
    approx->Clip(
      value, this->Scalars.GetPointer(), locator,
      polys, this->ApproxPD, outPd, this->ApproxCD, cellId,
      outCd, insideOut);
    }
}

int vtkLagrangeCurve::IntersectWithLine(
  const double* p1,
  const double* p2,
  double tol,
  double& t,
  double* x,
  double* pcoords,
  int& subId)
{
  vtkIdType nseg = vtkLagrangeInterpolation::NumberOfIntervals<1>(this->GetOrder());
  double tFirst = VTK_DOUBLE_MAX;
  bool intersection = false;
  vtkVector3d tmpX;
  vtkVector3d tmpP;
  int tmpId;
  for (int i = 0; i < nseg; ++i)
    {
    vtkLine* approx = this->GetApproximateLine(i);
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

int vtkLagrangeCurve::Triangulate(
  int vtkNotUsed(index),
  vtkIdList* ptIds,
  vtkPoints* pts)
{
  ptIds->Reset();
  pts->Reset();

  vtkIdType nseg = vtkLagrangeInterpolation::NumberOfIntervals<1>(this->GetOrder());
  vtkVector3i ijk;
  for (int i = 0; i < nseg; ++i)
    {
    vtkLine* approx = this->GetApproximateLine(i);
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

void vtkLagrangeCurve::Derivatives(
  int vtkNotUsed(subId),
  const double vtkNotUsed(pcoords)[3],
  const double* vtkNotUsed(values),
  int vtkNotUsed(dim),
  double* vtkNotUsed(derivs))
{
  // TODO: Fill me in?
}

double* vtkLagrangeCurve::GetParametricCoords()
{
  if (!this->PointParametricCoordinates)
    {
    this->PointParametricCoordinates = vtkSmartPointer<vtkPoints>::New();
    this->PointParametricCoordinates->SetDataTypeToDouble();
    }

  // Ensure Order is up-to-date and check that current point size matches:
  if (static_cast<int>(this->PointParametricCoordinates->GetNumberOfPoints()) != this->GetOrder(1))
    {
    this->PointParametricCoordinates->Initialize();
    vtkLagrangeInterpolation::AppendCurveCollocationPoints(
      this->PointParametricCoordinates, this->Order);
    }

  return
    vtkDoubleArray::SafeDownCast(
      this->PointParametricCoordinates->GetData())->GetPointer(0);
}

double vtkLagrangeCurve::GetParametricDistance(const double pcoords[3])
{
  double pDist, pDistMax;

  pDistMax =
    (pcoords[0] < 0. ? -pcoords[0] :
     (pcoords[0] > 1. ? pcoords[0] - 1. :
      0.));

  // The quadrilateral's 2nd and 3rd parametric coordinate should always be 0:
  for (int ii = 1; ii < 3; ++ii)
    {
    if (pcoords[ii] != 0.0 && (pDist = std::abs(pcoords[ii])) > pDistMax)
      {
      pDistMax = pDist;
      }
    }

  return pDistMax;
}

const int* vtkLagrangeCurve::GetOrder()
{
  vtkIdType npts = this->Points->GetNumberOfPoints();
  if (this->Order[1] != npts)
    {
    int pointsPerAxis = static_cast<int>(npts); // number of points along each axis
    this->Order[0] = pointsPerAxis - 1; // order 1 is linear, 2 is quadratic, ...
    this->Order[1] = pointsPerAxis;
    this->CellScalars->SetNumberOfTuples(pointsPerAxis);
    }
  return this->Order;
}

void vtkLagrangeCurve::InterpolateFunctions(
  const double pcoords[3], double* weights)
{
  vtkLagrangeInterpolation::Tensor1ShapeFunctions(this->GetOrder(), pcoords, weights);
}

void vtkLagrangeCurve::InterpolateDerivs(
  const double pcoords[3], double* derivs)
{
  vtkLagrangeInterpolation::Tensor1ShapeDerivatives(this->GetOrder(), pcoords, derivs);
}

/// Return a linear segment used to approximate a region of the nonlinear curve.
vtkLine* vtkLagrangeCurve::GetApprox()
{
  if (!this->Approx)
    {
    this->Approx = vtkSmartPointer<vtkLine>::New();
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
void vtkLagrangeCurve::PrepareApproxData(vtkPointData* pd, vtkCellData* cd, vtkIdType cellId, vtkDataArray* cellScalars)
{
  this->GetApprox(); // Ensure this->Approx{PD,CD} are non-NULL.
  this->GetOrder(); // Ensure the order has been updated to match this element.
  vtkIdType npts = this->Order[1];
  vtkIdType nele = this->Order[0];
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

/**\brief Populate the linear segment returned by GetApprox() with point-data from one voxel-like intervals of this cell.
  *
  * Ensure that you have called GetOrder() before calling this method
  * so that this->Order is up to date. This method does no checking
  * before using it to map connectivity-array offsets.
  */
vtkLine* vtkLagrangeCurve::GetApproximateLine(
  int subId, vtkDataArray* scalarsIn, vtkDataArray* scalarsOut)
{
  vtkLine* approx = this->GetApprox();
  bool doScalars = (scalarsIn && scalarsOut);
  if (doScalars)
    {
    scalarsOut->SetNumberOfTuples(2);
    }
  int i;
  if (!this->SubCellCoordinatesFromId(i, subId))
  {
    vtkErrorMacro("Invalid subId " << subId);
    return nullptr;
  }
  // Get the point ids (and optionally scalars) for each of the 2 corners
  // in the approximating line spanned by (i, i+1):
  for (int ic = 0; ic < 2; ++ic)
    {
    int corner = this->PointIndexFromIJK(
      i + ic,
      0,
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
bool vtkLagrangeCurve::SubCellCoordinatesFromId(vtkVector3i& ijk, int subId)
{
  ijk[1] = ijk[2] = 0;
  return this->SubCellCoordinatesFromId(ijk[0], subId);
}

/**\brief Given an integer specifying an approximating linear segment, compute its IJK coordinate-position in this cell.
 *
 * The \a subId specifies the lower-, left-, front-most vertex of the approximating segment.
 * This sets the ijk coordinates of that point.
 *
 * You must have called this->GetOrder() **before** invoking this method so that the order will be up to date.
 */
bool vtkLagrangeCurve::SubCellCoordinatesFromId(int& i, int subId)
{
  if (subId < 0)
    {
    return false;
    }

  i = subId % this->Order[0];
  return true; // TODO: detect more invalid subId values
}

/**\brief Given (i,j,k) coordinates within the Lagrange curve, return an offset into the local connectivity (PointIds) array.
  *
  * Ensure that you have called GetOrder() before calling this method
  * so that this->Order is up to date. This method does no checking
  * before using it to map connectivity-array offsets.
  */
int vtkLagrangeCurve::PointIndexFromIJK(int i, int vtkNotUsed(j), int vtkNotUsed(k))
{
  bool ibdy = (i == 0 || i == this->Order[0]);
  // How many boundaries do we lie on at once?
  int nbdy = (ibdy ? 1 : 0);

  if (nbdy == 1) // Vertex DOF
    { // ijk is a corner node. Return the proper index (somewhere in [0,7]):
    return i ? 1 : 0;
    }

  int offset = 2;
  return (i - 1) + offset;
}

/**\brief Given the index, \a subCell, of a linear approximating-segment, translate pcoords from that segment into this nonlinear curve.
  *
  * You must call this->GetOrder() **before** invoking this method as it assumes
  * the order is up to date.
  */
bool vtkLagrangeCurve::TransformApproxToCellParams(int subCell, double* pcoords)
{
  vtkVector3i ijk;
  if (!this->SubCellCoordinatesFromId(ijk, subCell))
    {
    return false;
    }

  pcoords[0] = (pcoords[0] + ijk[0]) / this->Order[0];
  for (int pp = 1; pp < 3; ++pp)
    {
    pcoords[pp] = 0.;
    }
  return true;
}
