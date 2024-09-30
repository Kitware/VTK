// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkBezierWedge.h"

#include "vtkBezierCurve.h"
#include "vtkBezierInterpolation.h"
#include "vtkBezierQuadrilateral.h"
#include "vtkBezierTriangle.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkTriangle.h"
#include "vtkVector.h"
#include "vtkWedge.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkBezierWedge);

vtkBezierWedge::vtkBezierWedge() = default;

vtkBezierWedge::~vtkBezierWedge() = default;

void vtkBezierWedge::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

vtkCell* vtkBezierWedge::GetEdge(int edgeId)
{
  vtkBezierCurve* result = EdgeCell;
  if (this->GetRationalWeights()->GetNumberOfTuples() > 0)
  {
    const auto set_number_of_ids_and_points = [&](const vtkIdType& npts) -> void
    {
      result->Points->SetNumberOfPoints(npts);
      result->PointIds->SetNumberOfIds(npts);
      result->GetRationalWeights()->SetNumberOfTuples(npts);
    };
    const auto set_ids_and_points = [&](const vtkIdType& edge_id, const vtkIdType& vol_id) -> void
    {
      result->Points->SetPoint(edge_id, this->Points->GetPoint(vol_id));
      result->PointIds->SetId(edge_id, this->PointIds->GetId(vol_id));
      result->GetRationalWeights()->SetValue(edge_id, this->GetRationalWeights()->GetValue(vol_id));
    };
    this->SetEdgeIdsAndPoints(edgeId, set_number_of_ids_and_points, set_ids_and_points);
  }
  else
  {
    const auto set_number_of_ids_and_points = [&](const vtkIdType& npts) -> void
    {
      result->Points->SetNumberOfPoints(npts);
      result->PointIds->SetNumberOfIds(npts);
      result->GetRationalWeights()->Reset();
    };
    const auto set_ids_and_points = [&](const vtkIdType& edge_id, const vtkIdType& vol_id) -> void
    {
      result->Points->SetPoint(edge_id, this->Points->GetPoint(vol_id));
      result->PointIds->SetId(edge_id, this->PointIds->GetId(vol_id));
    };
    this->SetEdgeIdsAndPoints(edgeId, set_number_of_ids_and_points, set_ids_and_points);
  }

  return result;
}

vtkCell* vtkBezierWedge::GetFace(int faceId)
{

  std::function<void(const vtkIdType&)> set_number_of_ids_and_points;
  std::function<void(const vtkIdType&, const vtkIdType&)> set_ids_and_points;
  if (faceId < 2)
  {
    vtkBezierTriangle* result = BdyTri;
    if (this->GetRationalWeights()->GetNumberOfTuples() > 0)
    {
      set_number_of_ids_and_points = [&](const vtkIdType& npts) -> void
      {
        result->Points->SetNumberOfPoints(npts);
        result->PointIds->SetNumberOfIds(npts);
        result->GetRationalWeights()->SetNumberOfTuples(npts);
      };
      set_ids_and_points = [&](const vtkIdType& face_id, const vtkIdType& vol_id) -> void
      {
        result->Points->SetPoint(face_id, this->Points->GetPoint(vol_id));
        result->PointIds->SetId(face_id, this->PointIds->GetId(vol_id));
        result->GetRationalWeights()->SetValue(
          faceId, this->GetRationalWeights()->GetValue(vol_id));
      };
    }
    else
    {
      set_number_of_ids_and_points = [&](const vtkIdType& npts) -> void
      {
        result->Points->SetNumberOfPoints(npts);
        result->PointIds->SetNumberOfIds(npts);
        result->GetRationalWeights()->Reset();
      };
      set_ids_and_points = [&](const vtkIdType& face_id, const vtkIdType& vol_id) -> void
      {
        result->Points->SetPoint(face_id, this->Points->GetPoint(vol_id));
        result->PointIds->SetId(face_id, this->PointIds->GetId(vol_id));
      };
    }
    vtkHigherOrderWedge::GetTriangularFace(
      faceId, this->Order, set_number_of_ids_and_points, set_ids_and_points);
    result->Initialize();
    return result;
  }
  else
  {
    vtkBezierQuadrilateral* result = BdyQuad;
    if (this->GetRationalWeights()->GetNumberOfTuples() > 0)
    {
      set_number_of_ids_and_points = [&](const vtkIdType& npts) -> void
      {
        result->Points->SetNumberOfPoints(npts);
        result->PointIds->SetNumberOfIds(npts);
        result->GetRationalWeights()->SetNumberOfTuples(npts);
      };
      set_ids_and_points = [&](const vtkIdType& face_id, const vtkIdType& vol_id) -> void
      {
        result->Points->SetPoint(face_id, this->Points->GetPoint(vol_id));
        result->PointIds->SetId(face_id, this->PointIds->GetId(vol_id));
        result->GetRationalWeights()->SetValue(
          faceId, this->GetRationalWeights()->GetValue(vol_id));
      };
    }
    else
    {
      set_number_of_ids_and_points = [&](const vtkIdType& npts) -> void
      {
        result->Points->SetNumberOfPoints(npts);
        result->PointIds->SetNumberOfIds(npts);
        result->GetRationalWeights()->Reset();
      };
      set_ids_and_points = [&](const vtkIdType& face_id, const vtkIdType& vol_id) -> void
      {
        result->Points->SetPoint(face_id, this->Points->GetPoint(vol_id));
        result->PointIds->SetId(face_id, this->PointIds->GetId(vol_id));
      };
    }
    int faceOrder[2];
    vtkHigherOrderWedge::GetQuadrilateralFace(
      faceId, this->Order, set_number_of_ids_and_points, set_ids_and_points, faceOrder);
    result->SetOrder(faceOrder[0], faceOrder[1]);
    return result;
  }
}

void vtkBezierWedge::InterpolateFunctions(const double pcoords[3], double* weights)
{
  vtkBezierInterpolation::WedgeShapeFunctions(
    this->GetOrder(), this->GetOrder()[3], pcoords, weights);

  // If the unit cell has rational weights: weights_i = weights_i * rationalWeights / sum( weights_i
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

void vtkBezierWedge::InterpolateDerivs(const double pcoords[3], double* derivs)
{
  vtkBezierInterpolation::WedgeShapeDerivatives(
    this->GetOrder(), this->GetOrder()[3], pcoords, derivs);
}

/**\brief Set the rational weight of the cell, given a vtkDataSet
 */
void vtkBezierWedge::SetRationalWeightsFromPointData(vtkPointData* point_data, vtkIdType numPts)
{
  vtkDataArray* v = point_data->GetRationalWeights();
  if (v)
  {
    this->GetRationalWeights()->SetNumberOfTuples(numPts);
    for (vtkIdType i = 0; i < numPts; i++)
    {
      this->GetRationalWeights()->SetValue(i, v->GetTuple1(this->PointIds->GetId(i)));
    }
  }
  else
    this->GetRationalWeights()->Reset();
}

vtkDoubleArray* vtkBezierWedge::GetRationalWeights()
{
  return RationalWeights.Get();
}
vtkHigherOrderQuadrilateral* vtkBezierWedge::GetBoundaryQuad()
{
  return BdyQuad;
}
vtkHigherOrderTriangle* vtkBezierWedge::GetBoundaryTri()
{
  return BdyTri;
}
vtkHigherOrderCurve* vtkBezierWedge::GetEdgeCell()
{
  return EdgeCell;
}
vtkHigherOrderInterpolation* vtkBezierWedge::GetInterpolation()
{
  return Interp;
}
VTK_ABI_NAMESPACE_END
