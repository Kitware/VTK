// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkInterpolateCalculator.h"

#include "vtkBoundingBox.h"
#include "vtkCellAttribute.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkVectorOperators.h"

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

void vtkInterpolateCalculator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkInterpolateCalculator::EvaluateDerivative(
  vtkIdType cellId, const vtkVector3d& rst, std::vector<double>& jacobian, double neighborhood)
{
  std::vector<double> p0;
  std::vector<double> p1;
  std::vector<double> p2;
  std::vector<double> p3;
  this->Evaluate(cellId, rst, p0);
  std::size_t numValues = p0.size();
  jacobian.resize(numValues * 3);
  this->Evaluate(cellId, rst + vtkVector3d(neighborhood, 0., 0.), p1);
  this->Evaluate(cellId, rst + vtkVector3d(0., neighborhood, 0.), p2);
  this->Evaluate(cellId, rst + vtkVector3d(0., 0., neighborhood), p3);
  for (std::size_t ii = 0; ii < p0.size(); ++ii)
  {
    jacobian[ii] = (p1[ii] - p0[ii]) / neighborhood;
    jacobian[numValues + ii] = (p2[ii] - p0[ii]) / neighborhood;
    jacobian[2 * numValues + ii] = (p3[ii] - p0[ii]) / neighborhood;
  }
}

VTK_ABI_NAMESPACE_END
