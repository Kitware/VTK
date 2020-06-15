/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBezierQuadrilateral.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBezierQuadrilateral.h"

#include "vtkBezierCurve.h"
#include "vtkBezierInterpolation.h"
#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkQuad.h"
#include "vtkTriangle.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

vtkStandardNewMacro(vtkBezierQuadrilateral);

vtkBezierQuadrilateral::vtkBezierQuadrilateral()
  : vtkHigherOrderQuadrilateral()
{
}

vtkBezierQuadrilateral::~vtkBezierQuadrilateral() = default;

void vtkBezierQuadrilateral::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

vtkCell* vtkBezierQuadrilateral::GetEdge(int edgeId)
{
  vtkBezierCurve* result = EdgeCell;

  if (this->GetRationalWeights()->GetNumberOfTuples() > 0)
  {
    const auto set_number_of_ids_and_points = [&](const vtkIdType& npts) -> void {
      result->Points->SetNumberOfPoints(npts);
      result->PointIds->SetNumberOfIds(npts);
      result->GetRationalWeights()->SetNumberOfTuples(npts);
    };
    const auto set_ids_and_points = [&](
                                      const vtkIdType& edge_id, const vtkIdType& face_id) -> void {
      result->Points->SetPoint(edge_id, this->Points->GetPoint(face_id));
      result->PointIds->SetId(edge_id, this->PointIds->GetId(face_id));
      result->GetRationalWeights()->SetValue(
        edge_id, this->GetRationalWeights()->GetValue(face_id));
    };
    this->SetEdgeIdsAndPoints(edgeId, set_number_of_ids_and_points, set_ids_and_points);
  }
  else
  {
    const auto set_number_of_ids_and_points = [&](const vtkIdType& npts) -> void {
      result->Points->SetNumberOfPoints(npts);
      result->PointIds->SetNumberOfIds(npts);
      result->GetRationalWeights()->Reset();
    };
    const auto set_ids_and_points = [&](
                                      const vtkIdType& edge_id, const vtkIdType& face_id) -> void {
      result->Points->SetPoint(edge_id, this->Points->GetPoint(face_id));
      result->PointIds->SetId(edge_id, this->PointIds->GetId(face_id));
    };
    this->SetEdgeIdsAndPoints(edgeId, set_number_of_ids_and_points, set_ids_and_points);
  }

  return result;
}

/**\brief EvaluateLocation Given a point_id. This is required by Bezier because the interior points
 * are non-interpolatory .
 */
void vtkBezierQuadrilateral::EvaluateLocationProjectedNode(
  int& subId, const vtkIdType point_id, double x[3], double* weights)
{
  this->vtkHigherOrderQuadrilateral::SetParametricCoords();
  double pcoords[3];
  this->PointParametricCoordinates->GetPoint(this->PointIds->FindIdLocation(point_id), pcoords);
  this->vtkHigherOrderQuadrilateral::EvaluateLocation(subId, pcoords, x, weights);
}

/**\brief Populate the linear quadrilateral returned by GetApprox() with point-data from one
 * voxel-like interval of this cell.
 *
 * Ensure that you have called GetOrder() before calling this method
 * so that this->Order is up to date. This method does no checking
 * before using it to map connectivity-array offsets.
 */
vtkQuad* vtkBezierQuadrilateral::GetApproximateQuad(
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
  for (vtkIdType ic = 0; ic < 4; ++ic)
  {
    const vtkIdType corner =
      this->PointIndexFromIJK(i + ((((ic + 1) / 2) % 2) ? 1 : 0), j + (((ic / 2) % 2) ? 1 : 0), 0);
    vtkVector3d cp;

    // Only the first four corners are interpolatory, we need to project the value of the other
    // nodes
    if (corner < 4)
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
      this->vtkHigherOrderQuadrilateral::EvaluateLocation(
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

void vtkBezierQuadrilateral::InterpolateFunctions(const double pcoords[3], double* weights)
{
  vtkBezierInterpolation::Tensor2ShapeFunctions(this->GetOrder(), pcoords, weights);

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

void vtkBezierQuadrilateral::InterpolateDerivs(const double pcoords[3], double* derivs)
{
  vtkBezierInterpolation::Tensor2ShapeDerivatives(this->GetOrder(), pcoords, derivs);
}

/**\brief Set the rational weight of the cell, given a vtkDataSet
 */
void vtkBezierQuadrilateral::SetRationalWeightsFromPointData(
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
  else
    this->GetRationalWeights()->Reset();
}

vtkDoubleArray* vtkBezierQuadrilateral::GetRationalWeights()
{
  return RationalWeights.Get();
}
vtkHigherOrderCurve* vtkBezierQuadrilateral::getEdgeCell()
{
  return EdgeCell;
}
