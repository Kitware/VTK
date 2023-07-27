// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPoissonDiskSampler.h"

#include "vtkAbstractPointLocator.h"
#include "vtkBitArray.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkKdTreePointLocator.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"

#include <algorithm>
#include <mutex>
#include <numeric>
#include <random>
#include <unordered_set>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPoissonDiskSampler);
vtkCxxSetObjectMacro(vtkPoissonDiskSampler, Locator, vtkAbstractPointLocator);

namespace
{
//------------------------------------------------------------------------------
void DartThrower(
  vtkPointSet* input, vtkAbstractPointLocator* locator, vtkPointSet* output, double radius)
{
  if (!input->GetNumberOfPoints())
  {
    return;
  }

  std::vector<vtkIdType> candidates(input->GetNumberOfPoints());
  std::iota(candidates.begin(), candidates.end(), 0);
  auto rng = std::default_random_engine{};
  std::shuffle(candidates.begin(), candidates.end(), rng);

  vtkNew<vtkIdList> pickedPoints;
  pickedPoints->Allocate(candidates.size());
  vtkNew<vtkBitArray> alreadyProcessed;
  alreadyProcessed->SetNumberOfValues(candidates.size());
  alreadyProcessed->Fill(false);

  vtkNew<vtkIdList> neighbors;
  double p[3];

  vtkPoints* inputPoints = input->GetPoints();

  for (vtkIdType candidate : candidates)
  {
    bool processed = alreadyProcessed->GetValue(candidate);
    if (!processed)
    {
      inputPoints->GetPoint(candidate, p);
      pickedPoints->InsertNextId(candidate);
      locator->FindPointsWithinRadius(radius, p, neighbors);
      for (vtkIdType i = 0; i < neighbors->GetNumberOfIds(); ++i)
      {
        alreadyProcessed->SetValue(neighbors->GetId(i), true);
      }
    }
  }

  // This avoids shuffling the output points ordering inside a multithreaded environment.
  std::sort(pickedPoints->begin(), pickedPoints->end());

  vtkNew<vtkPoints> outputPoints;
  output->SetPoints(outputPoints);
  outputPoints->GetData()->InsertTuplesStartingAt(0, pickedPoints, inputPoints->GetData());

  vtkPointData* inputPD = input->GetPointData();
  vtkPointData* outputPD = output->GetPointData();
  outputPD->CopyAllOn();
  outputPD->CopyAllocate(inputPD);
  outputPD->CopyData(inputPD, pickedPoints);
}
} // anonymous namespace

//------------------------------------------------------------------------------
vtkPoissonDiskSampler::vtkPoissonDiskSampler()
  : Radius(1.0)
  , Locator(nullptr)
{
  this->SetLocator(vtkNew<vtkKdTreePointLocator>());
}

//------------------------------------------------------------------------------
vtkPoissonDiskSampler::~vtkPoissonDiskSampler()
{
  this->SetLocator(nullptr);
}

//------------------------------------------------------------------------------
// Produce the output data
int vtkPoissonDiskSampler::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPointSet* input = vtkPointSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPointSet* output = vtkPointSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Check the input
  if (!input || !output)
  {
    return 1;
  }
  vtkIdType numPts = input->GetNumberOfPoints();
  if (numPts < 1)
  {
    return 1;
  }

  if (!this->Locator)
  {
    vtkWarningMacro(<< "Missing point locator, reinstantiating one");

    this->SetLocator(vtkNew<vtkKdTreePointLocator>());
  }

  if (!input->GetPointLocator())
  {
    this->Locator->SetDataSet(input);
    this->Locator->BuildLocator();
  }

  ::DartThrower(input, this->Locator, output, this->Radius);

  return 1;
}

//------------------------------------------------------------------------------
int vtkPoissonDiskSampler::RequestDataObject(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }
  vtkPointSet* input = vtkPointSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (input)
  {
    vtkInformation* info = outputVector->GetInformationObject(0);
    vtkPointSet* output = vtkPointSet::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT()));

    if (!output)
    {
      output = vtkPointSet::New();
      info->Set(vtkDataObject::DATA_OBJECT(), output);
      output->FastDelete();
    }
    return 1;
  }
  return 0;
}

//------------------------------------------------------------------------------
int vtkPoissonDiskSampler::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  return 1;
}

//------------------------------------------------------------------------------
void vtkPoissonDiskSampler::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
