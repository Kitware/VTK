// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkLinearKernel.h"
#include "vtkAbstractPointLocator.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkLinearKernel);

//------------------------------------------------------------------------------
vtkLinearKernel::vtkLinearKernel() = default;

//------------------------------------------------------------------------------
vtkLinearKernel::~vtkLinearKernel() = default;

//------------------------------------------------------------------------------
vtkIdType vtkLinearKernel::ComputeWeights(
  double*, vtkIdList* pIds, vtkDoubleArray* prob, vtkDoubleArray* weights)
{
  vtkIdType numPts = pIds->GetNumberOfIds();
  double* p = (prob ? prob->GetPointer(0) : nullptr);
  weights->SetNumberOfTuples(numPts);
  double* w = weights->GetPointer(0);
  double weight = 1.0 / static_cast<double>(numPts);

  if (!prob) // standard linear interpolation
  {
    for (vtkIdType i = 0; i < numPts; ++i)
    {
      w[i] = weight;
    }
  }

  else // weight by probability
  {
    double sum = 0.0;
    for (vtkIdType i = 0; i < numPts; ++i)
    {
      w[i] = weight * p[i];
      sum += w[i];
    }
    // Now normalize
    if (this->NormalizeWeights && sum != 0.0)
    {
      for (vtkIdType i = 0; i < numPts; ++i)
      {
        w[i] /= sum;
      }
    }
  }

  return numPts;
}

//------------------------------------------------------------------------------
void vtkLinearKernel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
