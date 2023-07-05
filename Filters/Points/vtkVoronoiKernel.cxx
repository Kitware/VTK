// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkVoronoiKernel.h"
#include "vtkAbstractPointLocator.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkVoronoiKernel);

//------------------------------------------------------------------------------
vtkVoronoiKernel::vtkVoronoiKernel() = default;

//------------------------------------------------------------------------------
vtkVoronoiKernel::~vtkVoronoiKernel() = default;

//------------------------------------------------------------------------------
vtkIdType vtkVoronoiKernel::ComputeBasis(double x[3], vtkIdList* pIds, vtkIdType)
{
  pIds->SetNumberOfIds(1);
  vtkIdType pId = this->Locator->FindClosestPoint(x);
  pIds->SetId(0, pId);

  return 1;
}

//------------------------------------------------------------------------------
vtkIdType vtkVoronoiKernel::ComputeWeights(double*, vtkIdList*, vtkDoubleArray* weights)
{
  weights->SetNumberOfTuples(1);
  weights->SetValue(0, 1.0);

  return 1;
}

//------------------------------------------------------------------------------
void vtkVoronoiKernel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
