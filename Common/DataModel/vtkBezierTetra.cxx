/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBezierTetra.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

vtkStandardNewMacro(vtkBezierTetra);
//----------------------------------------------------------------------------
vtkBezierTetra::vtkBezierTetra()
  : vtkHigherOrderTetra()
{
}

//----------------------------------------------------------------------------
vtkBezierTetra::~vtkBezierTetra() = default;

void vtkBezierTetra::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

vtkCell* vtkBezierTetra::GetEdge(int edgeId)
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

vtkCell* vtkBezierTetra::GetFace(int faceId)
{
  vtkBezierTriangle* result = FaceCell;
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
void vtkBezierTetra::EvaluateLocationProjectedNode(
  int& subId, const vtkIdType point_id, double x[3], double* weights)
{
  this->vtkHigherOrderTetra::SetParametricCoords();
  double pcoords[3];
  this->PointParametricCoordinates->GetPoint(this->PointIds->FindIdLocation(point_id), pcoords);
  this->vtkHigherOrderTetra::EvaluateLocation(subId, pcoords, x, weights);
}

/**\brief Set the rational weight of the cell, given a vtkDataSet
 */
void vtkBezierTetra::SetRationalWeightsFromPointData(
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

//----------------------------------------------------------------------------
void vtkBezierTetra::InterpolateFunctions(const double pcoords[3], double* weights)
{
  const int dim = 3;
  const int deg = GetOrder();
  const vtkIdType nPoints = this->GetPoints()->GetNumberOfPoints();
  std::vector<double> coeffs(nPoints, 0.0);

  vtkBezierInterpolation::deCasteljauSimplex(dim, deg, pcoords, &coeffs[0]);
  for (vtkIdType i = 0; i < nPoints; ++i)
  {
    vtkVector3i bv = vtkBezierInterpolation::unflattenSimplex(dim, deg, i);
    vtkIdType lbv[4] = { bv[0], bv[1], bv[2], deg - bv[0] - bv[1] - bv[2] };
    weights[Index(lbv, deg)] = coeffs[i];
  }
  // If the unit cell has rational weigths: weights_i = weights_i * rationalWeights / sum( weights_i
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

//----------------------------------------------------------------------------
void vtkBezierTetra::InterpolateDerivs(const double pcoords[3], double* derivs)
{
  const int dim = 3;
  const int deg = GetOrder();
  const vtkIdType nPoints = this->GetPoints()->GetNumberOfPoints();

  std::vector<double> coeffs(nPoints, 0.0);
  vtkBezierInterpolation::deCasteljauSimplexDeriv(dim, deg, pcoords, &coeffs[0]);
  for (vtkIdType i = 0; i < nPoints; ++i)
  {
    vtkVector3i bv = vtkBezierInterpolation::unflattenSimplex(dim, deg, i);
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
vtkHigherOrderCurve* vtkBezierTetra::getEdgeCell()
{
  return EdgeCell;
}
vtkHigherOrderTriangle* vtkBezierTetra::getFaceCell()
{
  return FaceCell;
}
