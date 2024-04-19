// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkShepardKernel.h"
#include "vtkAbstractPointLocator.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkMath.h"
#include "vtkMathUtilities.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkShepardKernel);

//------------------------------------------------------------------------------
vtkShepardKernel::vtkShepardKernel()
{
  this->PowerParameter = 2.0;
}

//------------------------------------------------------------------------------
vtkShepardKernel::~vtkShepardKernel() = default;

//------------------------------------------------------------------------------
vtkIdType vtkShepardKernel::ComputeWeights(
  double x[3], vtkIdList* pIds, vtkDoubleArray* prob, vtkDoubleArray* weights)
{
  vtkIdType numPts = pIds->GetNumberOfIds();
  double d, y[3], sum = 0.0;
  weights->SetNumberOfTuples(numPts);
  double* p = (prob ? prob->GetPointer(0) : nullptr);
  double* w = weights->GetPointer(0);

  for (vtkIdType i = 0; i < numPts; ++i)
  {
    vtkIdType id = pIds->GetId(i);
    this->DataSet->GetPoint(id, y);
    if (this->PowerParameter == 2.0)
    {
      d = vtkMath::Distance2BetweenPoints(x, y);
    }
    else
    {
      d = pow(sqrt(vtkMath::Distance2BetweenPoints(x, y)), this->PowerParameter);
    }
    if (vtkMathUtilities::FuzzyCompare(
          d, 0.0, std::numeric_limits<double>::epsilon() * 256.0)) // precise hit on existing point
    {
      pIds->SetNumberOfIds(1);
      pIds->SetId(0, id);
      weights->SetNumberOfTuples(1);
      weights->SetValue(0, 1.0);
      return 1;
    }
    else
    {
      w[i] = (p ? p[i] / d : 1.0 / d); // take into account probability if provided
      sum += w[i];
    }
  } // over all points

  // Normalize
  if (this->NormalizeWeights && sum != 0.0)
  {
    for (vtkIdType i = 0; i < numPts; ++i)
    {
      w[i] /= sum;
    }
  }

  return numPts;
}

//------------------------------------------------------------------------------
void vtkShepardKernel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Power Parameter: " << this->GetPowerParameter() << "\n";
}
VTK_ABI_NAMESPACE_END
