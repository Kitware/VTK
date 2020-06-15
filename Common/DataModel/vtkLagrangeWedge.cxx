/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLagrangeWedge.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
#include "vtkVectorOperators.h"
#include "vtkWedge.h"

vtkStandardNewMacro(vtkLagrangeWedge);

vtkLagrangeWedge::vtkLagrangeWedge()
  : vtkHigherOrderWedge()
{
}

vtkLagrangeWedge::~vtkLagrangeWedge() = default;

void vtkLagrangeWedge::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

vtkCell* vtkLagrangeWedge::GetEdge(int edgeId)
{
  vtkLagrangeCurve* result = EdgeCell;
  const auto set_number_of_ids_and_points = [&](const vtkIdType& npts) -> void {
    result->Points->SetNumberOfPoints(npts);
    result->PointIds->SetNumberOfIds(npts);
  };
  const auto set_ids_and_points = [&](const vtkIdType& edge_id, const vtkIdType& vol_id) -> void {
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
    const auto set_number_of_ids_and_points = [&](const vtkIdType& npts) -> void {
      result->Points->SetNumberOfPoints(npts);
      result->PointIds->SetNumberOfIds(npts);
    };
    const auto set_ids_and_points = [&](const vtkIdType& face_id, const vtkIdType& vol_id) -> void {
      result->Points->SetPoint(face_id, this->Points->GetPoint(vol_id));
      result->PointIds->SetId(face_id, this->PointIds->GetId(vol_id));
    };
    this->GetTriangularFace(result, faceId, set_number_of_ids_and_points, set_ids_and_points);
    return result;
  }
  else
  {
    vtkLagrangeQuadrilateral* result = BdyQuad;
    const auto set_number_of_ids_and_points = [&](const vtkIdType& npts) -> void {
      result->Points->SetNumberOfPoints(npts);
      result->PointIds->SetNumberOfIds(npts);
    };
    const auto set_ids_and_points = [&](const vtkIdType& face_id, const vtkIdType& vol_id) -> void {
      result->Points->SetPoint(face_id, this->Points->GetPoint(vol_id));
      result->PointIds->SetId(face_id, this->PointIds->GetId(vol_id));
    };
    this->GetQuadrilateralFace(result, faceId, set_number_of_ids_and_points, set_ids_and_points);
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

vtkHigherOrderQuadrilateral* vtkLagrangeWedge::getBdyQuad()
{
  return BdyQuad;
};
vtkHigherOrderTriangle* vtkLagrangeWedge::getBdyTri()
{
  return BdyTri;
};
vtkHigherOrderCurve* vtkLagrangeWedge::getEdgeCell()
{
  return EdgeCell;
}
vtkHigherOrderInterpolation* vtkLagrangeWedge::getInterp()
{
  return Interp;
};
