// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkGeneralizedKernel.h"
#include "vtkAbstractPointLocator.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkGeneralizedKernel::vtkGeneralizedKernel()
{
  this->KernelFootprint = vtkGeneralizedKernel::RADIUS;
  this->Radius = 1.0;
  this->NumberOfPoints = 8;
  this->NormalizeWeights = true;
}

//------------------------------------------------------------------------------
vtkGeneralizedKernel::~vtkGeneralizedKernel() = default;

//------------------------------------------------------------------------------
vtkIdType vtkGeneralizedKernel::ComputeBasis(double x[3], vtkIdList* pIds, vtkIdType)
{
  if (this->KernelFootprint == vtkGeneralizedKernel::RADIUS)
  {
    this->Locator->FindPointsWithinRadius(this->Radius, x, pIds);
  }
  else
  {
    this->Locator->FindClosestNPoints(this->NumberOfPoints, x, pIds);
  }

  return pIds->GetNumberOfIds();
}

//------------------------------------------------------------------------------
void vtkGeneralizedKernel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Kernel Footprint: " << this->GetKernelFootprint() << "\n";
  os << indent << "Radius: " << this->GetRadius() << "\n";
  os << indent << "Number of Points: " << this->GetNumberOfPoints() << "\n";
  os << indent << "Normalize Weights: " << (this->GetNormalizeWeights() ? "On\n" : "Off\n");
}
VTK_ABI_NAMESPACE_END
