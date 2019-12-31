/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBezierHexahedron.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBezierHexahedron.h"

#include "vtkBezierCurve.h"
#include "vtkBezierInterpolation.h"
#include "vtkBezierQuadrilateral.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkHexahedron.h"
#include "vtkIdList.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkTriangle.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

vtkStandardNewMacro(vtkBezierHexahedron);

vtkBezierHexahedron::vtkBezierHexahedron()
  : vtkHigherOrderHexahedron()
{
}

vtkBezierHexahedron::~vtkBezierHexahedron() = default;

void vtkBezierHexahedron::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

vtkCell* vtkBezierHexahedron::GetEdge(int edgeId)
{
  vtkBezierCurve* result = EdgeCell;
  this->GetEdgeWithoutRationalWeights(result, edgeId);

  if (this->GetRationalWeights()->GetNumberOfTuples() > 0)
  {
    vtkIdType npts = result->Points->GetNumberOfPoints();
    result->GetRationalWeights()->SetNumberOfTuples(npts);
    for (vtkIdType i = 0; i < npts; i++)
      result->GetRationalWeights()->SetValue(
        i, this->GetRationalWeights()->GetValue(result->PointIds->GetId(i)));
  }
  return result;
}

vtkCell* vtkBezierHexahedron::GetFace(int faceId)
{
  if (faceId < 0 || faceId >= 6)
  {
    return nullptr;
  }
  vtkBezierQuadrilateral* result = FaceCell;
  this->GetFaceWithoutRationalWeights(result, faceId);
  if (this->GetRationalWeights()->GetNumberOfTuples() > 0)
  {
    vtkIdType npts = result->Points->GetNumberOfPoints();
    result->GetRationalWeights()->SetNumberOfTuples(npts);
    for (vtkIdType i = 0; i < npts; i++)
      result->GetRationalWeights()->SetValue(
        i, this->GetRationalWeights()->GetValue(result->PointIds->GetId(i)));
  }
  return result;
}

/**\brief EvaluateLocation Given a point_id. This is required by Bezier because the interior points
 * are non-interpolatory .
 */
void vtkBezierHexahedron::EvaluateLocationProjectedNode(
  int& subId, const vtkIdType point_id, double x[3], double* weights)
{
  this->vtkHigherOrderHexahedron::SetParametricCoords();
  double pcoords[3];
  this->PointParametricCoordinates->GetPoint(this->PointIds->FindIdLocation(point_id), pcoords);
  this->vtkHigherOrderHexahedron::EvaluateLocation(subId, pcoords, x, weights);
}

/**\brief Populate the linear hex returned by GetApprox() with point-data from one voxel-like
 * intervals of this cell.
 *
 * Ensure that you have called GetOrder() before calling this method
 * so that this->Order is up to date. This method does no checking
 * before using it to map connectivity-array offsets.
 */
vtkHexahedron* vtkBezierHexahedron::GetApproximateHex(
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
  for (vtkIdType ic = 0; ic < 8; ++ic)
  {
    const vtkIdType corner = this->PointIndexFromIJK(
      i + ((((ic + 1) / 2) % 2) ? 1 : 0), j + (((ic / 2) % 2) ? 1 : 0), k + ((ic / 4) ? 1 : 0));
    vtkVector3d cp;
    // Only the first four corners are interpolatory, we need to project the value of the other
    // nodes
    if (corner < 8)
    {
      this->Points->GetPoint(corner, cp.GetData());
    }
    else
    {
      this->SetParametricCoords();
      double pcoords[3];
      this->PointParametricCoordinates->GetPoint(corner, pcoords);
      int subIdtps;
      std::vector<double> weights(this->Points->GetNumberOfPoints());
      this->vtkHigherOrderHexahedron::EvaluateLocation(
        subIdtps, pcoords, cp.GetData(), weights.data());
    }
    approx->Points->SetPoint(ic, cp.GetData());
    approx->PointIds->SetId(ic, doScalars ? corner : this->PointIds->GetId(corner));
    if (doScalars)
    {
      scalarsOut->SetTuple(ic, scalarsIn->GetTuple(corner));
    }
  }
  return approx;
}

void vtkBezierHexahedron::InterpolateFunctions(const double pcoords[3], double* weights)
{
  vtkBezierInterpolation::Tensor3ShapeFunctions(this->GetOrder(), pcoords, weights);

  // If the unit cell has rational weigths: weights_i = weights_i * rationalWeights / sum( weights_i
  // * rationalWeights )
  const bool has_rational_weights = RationalWeights->GetNumberOfTuples() > 0;
  if (has_rational_weights)
  {
    vtkIdType nPoints = this->GetPoints()->GetNumberOfPoints();
    double w = 0;
    for (vtkIdType idx = 0; idx < nPoints; ++idx)
    {
      weights[idx] *= RationalWeights->GetTuple1(idx);
      w += weights[idx];
    }
    const double one_over_rational_weight = 1. / w;
    for (vtkIdType idx = 0; idx < nPoints; ++idx)
      weights[idx] *= one_over_rational_weight;
  }
}

void vtkBezierHexahedron::InterpolateDerivs(const double pcoords[3], double* derivs)
{
  vtkBezierInterpolation::Tensor3ShapeDerivatives(this->GetOrder(), pcoords, derivs);
}

/**\brief Set the rational weight of the cell, given a vtkDataSet
 */
void vtkBezierHexahedron::SetRationalWeightsFromPointData(
  vtkPointData* point_data, const vtkIdType numPts)
{
  if (point_data->SetActiveAttribute(
        "RationalWeights", vtkDataSetAttributes::AttributeTypes::RATIONALWEIGHTS) != -1)
  {
    vtkDataArray* v = point_data->GetRationalWeights();
    this->GetRationalWeights()->SetNumberOfTuples(numPts);
    for (vtkIdType i = 0; i < numPts; i++)
    {
      this->GetRationalWeights()->SetValue(i, v->GetTuple1(this->PointIds->GetId(i)));
    }
  }
}

vtkDoubleArray* vtkBezierHexahedron::GetRationalWeights()
{
  return RationalWeights.Get();
}
vtkHigherOrderCurve* vtkBezierHexahedron::getEdgeCell()
{
  return EdgeCell;
}
vtkHigherOrderQuadrilateral* vtkBezierHexahedron::getFaceCell()
{
  return FaceCell;
}
vtkHigherOrderInterpolation* vtkBezierHexahedron::getInterp()
{
  return Interp;
};
