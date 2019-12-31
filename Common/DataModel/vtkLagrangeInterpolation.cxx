/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLagrangeInterpolation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
#include "vtkLagrangeInterpolation.h"

#include "vtkDoubleArray.h"
#include "vtkLagrangeTriangle.h"
#include "vtkLagrangeWedge.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

#include <array>
#include <vector>

vtkStandardNewMacro(vtkLagrangeInterpolation);

vtkLagrangeInterpolation::vtkLagrangeInterpolation()
  : vtkHigherOrderInterpolation()
{
}

vtkLagrangeInterpolation::~vtkLagrangeInterpolation() = default;

void vtkLagrangeInterpolation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

/// Evaluate 1-D shape functions for the given \a order at the given \a pcoord (in [0,1]).
void vtkLagrangeInterpolation::EvaluateShapeFunctions(
  const int order, const double pcoord, double* shape)
{
  int j, k;
  double v = order * pcoord;
  for (j = 0; j <= order; ++j)
  {
    shape[j] = 1.;
    for (k = 0; k <= order; ++k)
    {
      if (j != k)
      { // FIXME: (j - k) could be a register decremented inside the k loop:
        // and even better: normalization 1./(j - k) could be pre-computed and stored
        // somehow for each order that is actually used, removing division operations.
        shape[j] *= (v - k) / (j - k);
      }
    }
  }
}

/// Evaluate 1-D shape functions and their derivatives for the given \a order at the given \a pcoord
/// (in [0,1]).
void vtkLagrangeInterpolation::EvaluateShapeAndGradient(
  int order, double pcoord, double* shape, double* derivs)
{
  int j, k, q;
  double dtmp;
  double v = order * pcoord;
  for (j = 0; j <= order; ++j)
  {
    // std::cout << "ShapeDeriv j = " << j << "  v = " << v << "\n";
    shape[j] = 1.;
    derivs[j] = 0.;
    for (k = 0; k <= order; ++k)
    {
      if (j != k)
      { // FIXME: (j - k) could be a register decremented inside the k loop:
        // and even better: normalization could be pre-computed and stored
        // somehow for each order that is actually used.
        shape[j] *= (v - k) / (j - k);

        // Now compute the derivative of shape[j]; we use the differentiation
        // rule d/dx(a * b) = a * d/dx(b) + b * d/dx(a) instead of faster
        // methods because it keeps the truncation error low(er):
        dtmp = 1.;
        // std::cout << "           k = " << k;
        for (q = 0; q <= order; ++q)
        {
          if (q == j)
          {
            continue;
          }
          dtmp *= (q == k ? 1. : (v - q)) / (j - q);
          // std::cout << "  q " << q << " dtmp *= " << ((q == k ? 1. : (v - q)) / (j - q));
        }
        derivs[j] += order * dtmp;
      }
    }
  }
}

int vtkLagrangeInterpolation::Tensor1ShapeFunctions(
  const int order[1], const double* pcoords, double* shape)
{
  return vtkHigherOrderInterpolation::Tensor1ShapeFunctions(
    order, pcoords, shape, vtkLagrangeInterpolation::EvaluateShapeFunctions);
}

int vtkLagrangeInterpolation::Tensor1ShapeDerivatives(
  const int order[1], const double* pcoords, double* derivs)
{
  return vtkHigherOrderInterpolation::Tensor1ShapeDerivatives(
    order, pcoords, derivs, vtkLagrangeInterpolation::EvaluateShapeAndGradient);
}

/// Quadrilateral shape function computation
int vtkLagrangeInterpolation::Tensor2ShapeFunctions(
  const int order[2], const double pcoords[3], double* shape)
{
  return vtkHigherOrderInterpolation::Tensor2ShapeFunctions(
    order, pcoords, shape, vtkLagrangeInterpolation::EvaluateShapeFunctions);
}

// Quadrilateral shape-function derivatives
int vtkLagrangeInterpolation::Tensor2ShapeDerivatives(
  const int order[2], const double pcoords[2], double* derivs)
{
  return vtkHigherOrderInterpolation::Tensor2ShapeDerivatives(
    order, pcoords, derivs, vtkLagrangeInterpolation::EvaluateShapeAndGradient);
}

/// Hexahedral shape function computation
int vtkLagrangeInterpolation::Tensor3ShapeFunctions(
  const int order[3], const double pcoords[3], double* shape)
{
  return vtkHigherOrderInterpolation::Tensor3ShapeFunctions(
    order, pcoords, shape, vtkLagrangeInterpolation::EvaluateShapeFunctions);
}

int vtkLagrangeInterpolation::Tensor3ShapeDerivatives(
  const int order[3], const double pcoords[3], double* derivs)
{
  return vtkHigherOrderInterpolation::Tensor3ShapeDerivatives(
    order, pcoords, derivs, vtkLagrangeInterpolation::EvaluateShapeAndGradient);
}

void vtkLagrangeInterpolation::Tensor3EvaluateDerivative(const int order[3], const double* pcoords,
  vtkPoints* points, const double* fieldVals, int fieldDim, double* fieldDerivs)
{
  this->vtkHigherOrderInterpolation::Tensor3EvaluateDerivative(order, pcoords, points, fieldVals,
    fieldDim, fieldDerivs, vtkLagrangeInterpolation::EvaluateShapeAndGradient);
}

/// Wedge shape function computation
void vtkLagrangeInterpolation::WedgeShapeFunctions(
  const int order[3], const vtkIdType numberOfPoints, const double pcoords[3], double* shape)
{
  static vtkNew<vtkLagrangeTriangle> tri;
  vtkHigherOrderInterpolation::WedgeShapeFunctions(
    order, numberOfPoints, pcoords, shape, *tri, vtkLagrangeInterpolation::EvaluateShapeFunctions);
}

/// Wedge shape-function derivative evaluation
void vtkLagrangeInterpolation::WedgeShapeDerivatives(
  const int order[3], const vtkIdType numberOfPoints, const double pcoords[3], double* derivs)
{
  static vtkNew<vtkLagrangeTriangle> tri;
  vtkHigherOrderInterpolation::WedgeShapeDerivatives(order, numberOfPoints, pcoords, derivs, *tri,
    vtkLagrangeInterpolation::EvaluateShapeAndGradient);
}

void vtkLagrangeInterpolation::WedgeEvaluate(const int order[3], const vtkIdType numberOfPoints,
  const double* pcoords, double* fieldVals, int fieldDim, double* fieldAtPCoords)
{
  static vtkNew<vtkLagrangeTriangle> tri;
  this->vtkHigherOrderInterpolation::WedgeEvaluate(order, numberOfPoints, pcoords, fieldVals,
    fieldDim, fieldAtPCoords, *tri, vtkLagrangeInterpolation::EvaluateShapeFunctions);
}

void vtkLagrangeInterpolation::WedgeEvaluateDerivative(const int order[3], const double* pcoords,
  vtkPoints* points, const double* fieldVals, int fieldDim, double* fieldDerivs)
{
  static vtkNew<vtkLagrangeTriangle> tri;
  this->vtkHigherOrderInterpolation::WedgeEvaluateDerivative(order, pcoords, points, fieldVals,
    fieldDim, fieldDerivs, *tri, vtkLagrangeInterpolation::EvaluateShapeAndGradient);
}
