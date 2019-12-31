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
  this->GetEdgeWithoutRationalWeights(result, edgeId);
  return result;
}

vtkCell* vtkLagrangeWedge::GetFace(int faceId)
{
  return this->vtkHigherOrderWedge::GetFaceWithoutRationalWeights(faceId);
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
