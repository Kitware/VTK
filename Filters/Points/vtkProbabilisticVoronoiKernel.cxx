// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkProbabilisticVoronoiKernel.h"
#include "vtkAbstractPointLocator.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkMath.h"
#include "vtkMathUtilities.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkProbabilisticVoronoiKernel);

//------------------------------------------------------------------------------
vtkProbabilisticVoronoiKernel::vtkProbabilisticVoronoiKernel() = default;

//------------------------------------------------------------------------------
vtkProbabilisticVoronoiKernel::~vtkProbabilisticVoronoiKernel() = default;

//------------------------------------------------------------------------------
vtkIdType vtkProbabilisticVoronoiKernel::ComputeWeights(
  double x[3], vtkIdList* pIds, vtkDoubleArray* prob, vtkDoubleArray* weights)
{
  vtkIdType numPts = pIds->GetNumberOfIds();
  double* p = (prob ? prob->GetPointer(0) : nullptr);
  double highestProbability = VTK_FLOAT_MIN;
  vtkIdType id, mostProbableId = 0;

  if (p) // return the point in the neighborhood with the highest probability
  {
    for (vtkIdType i = 0; i < numPts; ++i)
    {
      if (p[i] > highestProbability)
      {
        mostProbableId = pIds->GetId(i);
        highestProbability = p[i];
      }
    }
  }

  else // return the closest point in the footprint provided
  {
    double y[3], d, minD = VTK_FLOAT_MAX;
    for (vtkIdType i = 0; i < numPts; ++i)
    {
      id = pIds->GetId(i);
      this->DataSet->GetPoint(id, y);
      d = vtkMath::Distance2BetweenPoints(x, y);
      if (vtkMathUtilities::FuzzyCompare(d, 0.0,
            std::numeric_limits<double>::epsilon() * 256.0)) // precise hit on existing point
      {
        mostProbableId = id;
        break;
      }
      else if (d <= minD)
      {
        mostProbableId = id;
        minD = d;
      }
    } // over all points
  }

  // Okay let's get out
  pIds->SetNumberOfIds(1);
  pIds->SetId(0, mostProbableId);
  weights->SetNumberOfTuples(1);
  weights->SetValue(0, 1.0);

  return 1;
}

//------------------------------------------------------------------------------
void vtkProbabilisticVoronoiKernel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
