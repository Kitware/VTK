/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBezierWedge.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
#include "vtkVectorOperators.h"
#include "vtkWedge.h"

vtkStandardNewMacro(vtkBezierWedge);

vtkBezierWedge::vtkBezierWedge()
  : vtkHigherOrderWedge()
{
}

vtkBezierWedge::~vtkBezierWedge() = default;

void vtkBezierWedge::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

vtkCell* vtkBezierWedge::GetEdge(int edgeId)
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

vtkCell* vtkBezierWedge::GetFace(int faceId)
{
  if (this->GetRationalWeights()->GetNumberOfTuples() > 0)
  {
    vtkCell* resultHigherOrder = this->vtkHigherOrderWedge::GetFaceWithoutRationalWeights(faceId);
    vtkIdType npts = resultHigherOrder->Points->GetNumberOfPoints();
    if (resultHigherOrder->GetCellType() == VTK_BEZIER_QUADRILATERAL)
    {
      vtkBezierQuadrilateral* result = dynamic_cast<vtkBezierQuadrilateral*>(resultHigherOrder);
      result->GetRationalWeights()->SetNumberOfTuples(npts);
      for (vtkIdType i = 0; i < npts; i++)
        result->GetRationalWeights()->SetValue(
          i, this->GetRationalWeights()->GetValue(result->PointIds->GetId(i)));
      return result;
    }
    else
    {
      vtkBezierTriangle* result = dynamic_cast<vtkBezierTriangle*>(resultHigherOrder);
      result->GetRationalWeights()->SetNumberOfTuples(npts);
      for (vtkIdType i = 0; i < npts; i++)
        result->GetRationalWeights()->SetValue(
          i, this->GetRationalWeights()->GetValue(result->PointIds->GetId(i)));
      return result;
    }
  }
  else
  {
    return this->vtkHigherOrderWedge::GetFaceWithoutRationalWeights(faceId);
  }
}

/**\brief EvaluateLocation Given a point_id. This is required by Bezier because the interior points
 * are non-interpolatory .
 */
void vtkBezierWedge::EvaluateLocationProjectedNode(
  int& subId, const vtkIdType point_id, double x[3], double* weights)
{
  this->vtkHigherOrderWedge::SetParametricCoords();
  double pcoords[3];
  this->PointParametricCoordinates->GetPoint(this->PointIds->FindIdLocation(point_id), pcoords);
  this->vtkHigherOrderWedge::EvaluateLocation(subId, pcoords, x, weights);
}

void vtkBezierWedge::InterpolateFunctions(const double pcoords[3], double* weights)
{
  vtkBezierInterpolation::WedgeShapeFunctions(
    this->GetOrder(), this->GetOrder()[3], pcoords, weights);

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

void vtkBezierWedge::InterpolateDerivs(const double pcoords[3], double* derivs)
{
  vtkBezierInterpolation::WedgeShapeDerivatives(
    this->GetOrder(), this->GetOrder()[3], pcoords, derivs);
}

/**\brief Set the rational weight of the cell, given a vtkDataSet
 */
void vtkBezierWedge::SetRationalWeightsFromPointData(
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

vtkDoubleArray* vtkBezierWedge::GetRationalWeights()
{
  return RationalWeights.Get();
}
vtkHigherOrderQuadrilateral* vtkBezierWedge::getBdyQuad()
{
  return BdyQuad;
};
vtkHigherOrderTriangle* vtkBezierWedge::getBdyTri()
{
  return BdyTri;
};
vtkHigherOrderCurve* vtkBezierWedge::getEdgeCell()
{
  return EdgeCell;
}
vtkHigherOrderInterpolation* vtkBezierWedge::getInterp()
{
  return Interp;
};
