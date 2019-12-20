/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLagrangeQuadrilateral.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLagrangeQuadrilateral.h"

#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkLagrangeCurve.h"
#include "vtkLagrangeInterpolation.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkQuad.h"
#include "vtkTriangle.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

vtkStandardNewMacro(vtkLagrangeQuadrilateral);

vtkLagrangeQuadrilateral::vtkLagrangeQuadrilateral()
  : vtkHigherOrderQuadrilateral()
{
}

vtkLagrangeQuadrilateral::~vtkLagrangeQuadrilateral() = default;

void vtkLagrangeQuadrilateral::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

vtkCell* vtkLagrangeQuadrilateral::GetEdge(int edgeId)
{
  vtkLagrangeCurve* result = EdgeCell;
  this->GetEdgeWithoutRationalWeights(result, edgeId);
  return result;
}

/**\brief Populate the linear quadrilateral returned by GetApprox() with point-data from one
 * voxel-like interval of this cell.
 *
 * Ensure that you have called GetOrder() before calling this method
 * so that this->Order is up to date. This method does no checking
 * before using it to map connectivity-array offsets.
 */
vtkQuad* vtkLagrangeQuadrilateral::GetApproximateQuad(
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
    this->Points->GetPoint(corner, cp.GetData());
    approx->Points->SetPoint(ic, cp.GetData());
    approx->PointIds->SetId(ic, doScalars ? corner : this->PointIds->GetId(corner));
    if (doScalars)
    {
      scalarsOut->SetTuple(ic, scalarsIn->GetTuple(corner));
    }
  }
  return approx;
}

void vtkLagrangeQuadrilateral::InterpolateFunctions(const double pcoords[3], double* weights)
{
  vtkLagrangeInterpolation::Tensor2ShapeFunctions(this->GetOrder(), pcoords, weights);
}

void vtkLagrangeQuadrilateral::InterpolateDerivs(const double pcoords[3], double* derivs)
{
  vtkLagrangeInterpolation::Tensor2ShapeDerivatives(this->GetOrder(), pcoords, derivs);
}

vtkHigherOrderCurve* vtkLagrangeQuadrilateral::getEdgeCell()
{
  return EdgeCell;
}
