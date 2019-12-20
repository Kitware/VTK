/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLagrangeCurve.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLagrangeCurve.h"

#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkLagrangeInterpolation.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkTriangle.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

vtkStandardNewMacro(vtkLagrangeCurve);
vtkLagrangeCurve::vtkLagrangeCurve()
  : vtkHigherOrderCurve()
{
}

vtkLagrangeCurve::~vtkLagrangeCurve() = default;

void vtkLagrangeCurve::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

/**\brief Populate the linear segment returned by GetApprox() with point-data from one voxel-like
 * intervals of this cell.
 *
 * Ensure that you have called GetOrder() before calling this method
 * so that this->Order is up to date. This method does no checking
 * before using it to map connectivity-array offsets.
 */
vtkLine* vtkLagrangeCurve::GetApproximateLine(
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

void vtkLagrangeCurve::InterpolateFunctions(const double pcoords[3], double* weights)
{
  vtkLagrangeInterpolation::Tensor1ShapeFunctions(this->GetOrder(), pcoords, weights);
}

void vtkLagrangeCurve::InterpolateDerivs(const double pcoords[3], double* derivs)
{
  vtkLagrangeInterpolation::Tensor1ShapeDerivatives(this->GetOrder(), pcoords, derivs);
}
