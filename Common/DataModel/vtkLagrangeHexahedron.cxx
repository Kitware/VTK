/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLagrangeHexahedron.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLagrangeHexahedron.h"

#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkHexahedron.h"
#include "vtkIdList.h"
#include "vtkLagrangeCurve.h"
#include "vtkLagrangeInterpolation.h"
#include "vtkLagrangeQuadrilateral.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkTriangle.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

vtkStandardNewMacro(vtkLagrangeHexahedron);

vtkLagrangeHexahedron::vtkLagrangeHexahedron()
  : vtkHigherOrderHexahedron()
{
}

vtkLagrangeHexahedron::~vtkLagrangeHexahedron() = default;

void vtkLagrangeHexahedron::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

vtkCell* vtkLagrangeHexahedron::GetEdge(int edgeId)
{
  vtkLagrangeCurve* result = EdgeCell;
  this->GetEdgeWithoutRationalWeights(result, edgeId);
  return result;
}

vtkCell* vtkLagrangeHexahedron::GetFace(int faceId)
{
  if (faceId < 0 || faceId >= 6)
  {
    return nullptr;
  }
  vtkLagrangeQuadrilateral* result = FaceCell;
  this->GetFaceWithoutRationalWeights(result, faceId);
  return result;
}

/**\brief Populate the linear hex returned by GetApprox() with point-data from one voxel-like
 * intervals of this cell.
 *
 * Ensure that you have called GetOrder() before calling this method
 * so that this->Order is up to date. This method does no checking
 * before using it to map connectivity-array offsets.
 */
vtkHexahedron* vtkLagrangeHexahedron::GetApproximateHex(
  int subId, vtkDataArray* scalarsIn, vtkDataArray* scalarsOut)
{
  vtkHexahedron* approx = this->GetApprox();
  bool doScalars = (scalarsIn && scalarsOut);
  if (doScalars)
  {
    scalarsOut->SetNumberOfTuples(8);
  }
  int i, j, k;
  if (!this->SubCellCoordinatesFromId(i, j, k, subId))
  {
    vtkErrorMacro("Invalid subId " << subId);
    return nullptr;
  }
  // Get the point coordinates (and optionally scalars) for each of the 8 corners
  // in the approximating hexahedron spanned by (i, i+1) x (j, j+1) x (k, k+1):
  for (vtkIdType ic = 0; ic < 8; ++ic)
  {
    const vtkIdType corner = this->PointIndexFromIJK(
      i + ((((ic + 1) / 2) % 2) ? 1 : 0), j + (((ic / 2) % 2) ? 1 : 0), k + ((ic / 4) ? 1 : 0));
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

void vtkLagrangeHexahedron::InterpolateFunctions(const double pcoords[3], double* weights)
{
  vtkLagrangeInterpolation::Tensor3ShapeFunctions(this->GetOrder(), pcoords, weights);
}

void vtkLagrangeHexahedron::InterpolateDerivs(const double pcoords[3], double* derivs)
{
  vtkLagrangeInterpolation::Tensor3ShapeDerivatives(this->GetOrder(), pcoords, derivs);
}
vtkHigherOrderCurve* vtkLagrangeHexahedron::getEdgeCell()
{
  return EdgeCell;
}
vtkHigherOrderQuadrilateral* vtkLagrangeHexahedron::getFaceCell()
{
  return FaceCell;
}
vtkHigherOrderInterpolation* vtkLagrangeHexahedron::getInterp()
{
  return Interp;
};
