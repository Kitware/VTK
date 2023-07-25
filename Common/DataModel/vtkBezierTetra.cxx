// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkBezierTetra.h"
#include "vtkBezierInterpolation.h"

#include "vtkBezierCurve.h"
#include "vtkBezierTriangle.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkTetra.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkBezierTetra);
//------------------------------------------------------------------------------
vtkBezierTetra::vtkBezierTetra() = default;

//------------------------------------------------------------------------------
vtkBezierTetra::~vtkBezierTetra() = default;

void vtkBezierTetra::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

vtkCell* vtkBezierTetra::GetEdge(int edgeId)
{
  vtkBezierCurve* result = EdgeCell;
  if (this->GetRationalWeights()->GetNumberOfTuples() > 0)
  {
    const auto set_number_of_ids_and_points = [&](const vtkIdType& npts) -> void {
      result->Points->SetNumberOfPoints(npts);
      result->PointIds->SetNumberOfIds(npts);
      result->GetRationalWeights()->SetNumberOfTuples(npts);
    };
    const auto set_ids_and_points = [&](const vtkIdType& edge_id, const vtkIdType& vol_id) -> void {
      result->Points->SetPoint(edge_id, this->Points->GetPoint(vol_id));
      result->PointIds->SetId(edge_id, this->PointIds->GetId(vol_id));
      result->GetRationalWeights()->SetValue(edge_id, this->GetRationalWeights()->GetValue(vol_id));
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
    const auto set_ids_and_points = [&](const vtkIdType& edge_id, const vtkIdType& vol_id) -> void {
      result->Points->SetPoint(edge_id, this->Points->GetPoint(vol_id));
      result->PointIds->SetId(edge_id, this->PointIds->GetId(vol_id));
    };
    this->SetEdgeIdsAndPoints(edgeId, set_number_of_ids_and_points, set_ids_and_points);
  }

  return result;
}

vtkCell* vtkBezierTetra::GetFace(int faceId)
{
  vtkBezierTriangle* result = FaceCell;
  if (this->GetRationalWeights()->GetNumberOfTuples() > 0)
  {
    const auto set_number_of_ids_and_points = [&](const vtkIdType& npts) -> void {
      result->Points->SetNumberOfPoints(npts);
      result->PointIds->SetNumberOfIds(npts);
      result->GetRationalWeights()->SetNumberOfTuples(npts);
    };
    const auto set_ids_and_points = [&](const vtkIdType& face_id, const vtkIdType& vol_id) -> void {
      result->Points->SetPoint(face_id, this->Points->GetPoint(vol_id));
      result->PointIds->SetId(face_id, this->PointIds->GetId(vol_id));
      result->GetRationalWeights()->SetValue(face_id, this->GetRationalWeights()->GetValue(vol_id));
    };
    this->SetFaceIdsAndPoints(result, faceId, set_number_of_ids_and_points, set_ids_and_points);
  }
  else
  {
    const auto set_number_of_ids_and_points = [&](const vtkIdType& npts) -> void {
      result->Points->SetNumberOfPoints(npts);
      result->PointIds->SetNumberOfIds(npts);
      result->GetRationalWeights()->Reset();
    };
    const auto set_ids_and_points = [&](const vtkIdType& face_id, const vtkIdType& vol_id) -> void {
      result->Points->SetPoint(face_id, this->Points->GetPoint(vol_id));
      result->PointIds->SetId(face_id, this->PointIds->GetId(vol_id));
    };
    this->SetFaceIdsAndPoints(result, faceId, set_number_of_ids_and_points, set_ids_and_points);
  }

  return result;
}

/**\brief Set the rational weight of the cell, given a vtkDataSet
 */
void vtkBezierTetra::SetRationalWeightsFromPointData(vtkPointData* point_data, vtkIdType numPts)
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

//------------------------------------------------------------------------------
void vtkBezierTetra::InterpolateFunctions(const double pcoords[3], double* weights)
{
  const int dim = 3;
  const int deg = GetOrder();
  const vtkIdType nPoints = this->GetPoints()->GetNumberOfPoints();
  std::vector<double> coeffs(nPoints, 0.0);

  vtkBezierInterpolation::DeCasteljauSimplex(dim, deg, pcoords, coeffs.data());
  for (vtkIdType i = 0; i < nPoints; ++i)
  {
    vtkVector3i bv = vtkBezierInterpolation::UnFlattenSimplex(dim, deg, i);
    vtkIdType lbv[4] = { bv[0], bv[1], bv[2], deg - bv[0] - bv[1] - bv[2] };
    weights[Index(lbv, deg)] = coeffs[i];
  }
  // If the unit cell has rational weights: weights_i = weights_i * rationalWeights / sum( weights_i
  // * rationalWeights )
  const bool has_rational_weights = RationalWeights->GetNumberOfTuples() > 0;
  if (has_rational_weights)
  {
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

//------------------------------------------------------------------------------
void vtkBezierTetra::InterpolateDerivs(const double pcoords[3], double* derivs)
{
  const int dim = 3;
  const int deg = GetOrder();
  const vtkIdType nPoints = this->GetPoints()->GetNumberOfPoints();

  std::vector<double> coeffs(nPoints, 0.0);
  vtkBezierInterpolation::DeCasteljauSimplexDeriv(dim, deg, pcoords, coeffs.data());
  for (vtkIdType i = 0; i < nPoints; ++i)
  {
    vtkVector3i bv = vtkBezierInterpolation::UnFlattenSimplex(dim, deg, i);
    vtkIdType lbv[4] = { bv[0], bv[1], bv[2], deg - bv[0] - bv[1] - bv[2] };
    for (int j = 0; j < dim; ++j)
    {
      derivs[j * nPoints + Index(lbv, deg)] = coeffs[j * nPoints + i];
    }
  }
}

vtkDoubleArray* vtkBezierTetra::GetRationalWeights()
{
  return RationalWeights.Get();
}
vtkHigherOrderCurve* vtkBezierTetra::GetEdgeCell()
{
  return EdgeCell;
}
vtkHigherOrderTriangle* vtkBezierTetra::GetFaceCell()
{
  return FaceCell;
}
VTK_ABI_NAMESPACE_END
