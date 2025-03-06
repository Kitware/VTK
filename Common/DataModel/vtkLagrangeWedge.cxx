// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkLagrangeWedge.h"

#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkLagrangeCurve.h"
#include "vtkLagrangeInterpolation.h"
#include "vtkLagrangeQuadrilateral.h"
#include "vtkLagrangeTriangle.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkTriangle.h"
#include "vtkVector.h"
#include "vtkWedge.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkLagrangeWedge);

vtkLagrangeWedge::vtkLagrangeWedge() = default;

vtkLagrangeWedge::~vtkLagrangeWedge() = default;

void vtkLagrangeWedge::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

vtkCell* vtkLagrangeWedge::GetEdge(int edgeId)
{
  vtkLagrangeCurve* result = EdgeCell;
  const auto set_number_of_ids_and_points = [&](const vtkIdType& npts) -> void
  {
    result->Points->SetNumberOfPoints(npts);
    result->PointIds->SetNumberOfIds(npts);
  };
  const auto set_ids_and_points = [&](const vtkIdType& edge_id, const vtkIdType& vol_id) -> void
  {
    result->Points->SetPoint(edge_id, this->Points->GetPoint(vol_id));
    result->PointIds->SetId(edge_id, this->PointIds->GetId(vol_id));
  };
  this->SetEdgeIdsAndPoints(edgeId, set_number_of_ids_and_points, set_ids_and_points);
  return result;
}

vtkCell* vtkLagrangeWedge::GetFace(int faceId)
{
  // If faceId = 0 or 1, triangular face, else if 2, 3, or 4, quad face.
  if (faceId < 2)
  {
    vtkLagrangeTriangle* result = BdyTri;
    const auto set_number_of_ids_and_points = [&](const vtkIdType& npts) -> void
    {
      result->Points->SetNumberOfPoints(npts);
      result->PointIds->SetNumberOfIds(npts);
    };
    const auto set_ids_and_points = [&](const vtkIdType& face_id, const vtkIdType& vol_id) -> void
    {
      result->Points->SetPoint(face_id, this->Points->GetPoint(vol_id));
      result->PointIds->SetId(face_id, this->PointIds->GetId(vol_id));
    };
    vtkHigherOrderWedge::GetTriangularFace(
      faceId, this->Order, set_number_of_ids_and_points, set_ids_and_points);
    result->Initialize();
    return result;
  }
  else
  {
    vtkLagrangeQuadrilateral* result = BdyQuad;
    const auto set_number_of_ids_and_points = [&](const vtkIdType& npts) -> void
    {
      result->Points->SetNumberOfPoints(npts);
      result->PointIds->SetNumberOfIds(npts);
    };
    const auto set_ids_and_points = [&](const vtkIdType& face_id, const vtkIdType& vol_id) -> void
    {
      result->Points->SetPoint(face_id, this->Points->GetPoint(vol_id));
      result->PointIds->SetId(face_id, this->PointIds->GetId(vol_id));
    };
    int faceOrder[2];
    vtkHigherOrderWedge::GetQuadrilateralFace(
      faceId, this->Order, set_number_of_ids_and_points, set_ids_and_points, faceOrder);
    result->SetOrder(faceOrder[0], faceOrder[1]);
    return result;
  }
}

void vtkLagrangeWedge::InterpolateFunctions(const double pcoords[3], double* weights)
{
  vtkLagrangeInterpolation::WedgeShapeFunctions(
    this->GetOrder(), this->GetOrder()[3], pcoords, weights);
}

void vtkLagrangeWedge::InterpolateDerivs(const double pcoords[3], double* derivs)
{
  vtkLagrangeInterpolation::WedgeShapeDerivatives(
    this->GetOrder(), this->GetOrder()[3], pcoords, derivs);
}

vtkHigherOrderQuadrilateral* vtkLagrangeWedge::GetBoundaryQuad()
{
  return BdyQuad;
}
vtkHigherOrderTriangle* vtkLagrangeWedge::GetBoundaryTri()
{
  return BdyTri;
}
vtkHigherOrderCurve* vtkLagrangeWedge::GetEdgeCell()
{
  return EdgeCell;
}
vtkHigherOrderInterpolation* vtkLagrangeWedge::GetInterpolation()
{
  return Interp;
}
VTK_ABI_NAMESPACE_END
