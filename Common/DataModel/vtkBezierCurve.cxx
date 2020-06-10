/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBezierCurve.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBezierCurve.h"

#include "vtkBezierInterpolation.h"
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

vtkStandardNewMacro(vtkBezierCurve);

vtkBezierCurve::vtkBezierCurve()
  : vtkHigherOrderCurve()
{
}

vtkBezierCurve::~vtkBezierCurve() = default;

void vtkBezierCurve::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

/**\brief EvaluateLocation Given a point_id. This is required by Bezier because the interior points
 * are non-interpolatory .
 */
void vtkBezierCurve::EvaluateLocationProjectedNode(
  int& subId, const vtkIdType point_id, double x[3], double* weights)
{
  this->vtkHigherOrderCurve::SetParametricCoords();
  double pcoords[3];
  this->PointParametricCoordinates->GetPoint(this->PointIds->FindIdLocation(point_id), pcoords);
  this->vtkHigherOrderCurve::EvaluateLocation(subId, pcoords, x, weights);
}

/**\brief Set the rational weight of the cell, given a vtkDataSet
 */
void vtkBezierCurve::SetRationalWeightsFromPointData(
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

/**\brief Populate the linear segment returned by GetApprox() with point-data from one voxel-like
 * intervals of this cell.
 *
 * Ensure that you have called GetOrder() before calling this method
 * so that this->Order is up to date. This method does no checking
 * before using it to map connectivity-array offsets.
 */
vtkLine* vtkBezierCurve::GetApproximateLine(
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
  for (vtkIdType ic = 0; ic < 2; ++ic)
  {
    const vtkIdType corner = this->PointIndexFromIJK(i + ic, 0, 0);
    vtkVector3d cp;
    // Only the first four corners are interpolatory, we need to project the value of the other
    // nodes
    if (corner < 2)
    {
      this->Points->GetPoint(corner, cp.GetData());
    }
    else
    {
      this->SetParametricCoords();
      double pcoords[3];
      this->PointParametricCoordinates->GetPoint(corner, pcoords);
      int subIdtps;
      const int numtripts = (this->Order[0] + 1);
      std::vector<double> weights(numtripts);
      this->vtkHigherOrderCurve::EvaluateLocation(subIdtps, pcoords, cp.GetData(), weights.data());
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

void vtkBezierCurve::InterpolateFunctions(const double pcoords[3], double* weights)
{
  vtkBezierInterpolation::Tensor1ShapeFunctions(this->GetOrder(), pcoords, weights);

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

void vtkBezierCurve::InterpolateDerivs(const double pcoords[3], double* derivs)
{
  vtkBezierInterpolation::Tensor1ShapeDerivatives(this->GetOrder(), pcoords, derivs);
}
vtkDoubleArray* vtkBezierCurve::GetRationalWeights()
{
  return RationalWeights.Get();
}
