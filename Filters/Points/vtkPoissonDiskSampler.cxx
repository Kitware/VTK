/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPoissonDiskSampler.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

vtkStandardNewMacro(vtkPoissonDiskSampler);
vtkCxxSetObjectMacro(vtkPoissonDiskSampler, Locator, vtkAbstractPointLocator);

namespace
{
//==============================================================================
struct DartThrowerWorker
{
  DartThrowerWorker(
    vtkPointSet* input, vtkAbstractPointLocator* locator, vtkPointSet* output, double radius)
    : Input(input)
    , Locator(locator)
    , Output(output)
    , Radius(radius)
  {
    this->AlreadyProcessed->SetNumberOfComponents(1);
    this->AlreadyProcessed->SetNumberOfValues(input->GetNumberOfPoints());
    this->AlreadyProcessed->Fill(false);
  }

  void operator()(vtkIdType startId, vtkIdType endId)
  {
    if (!this->Input->GetNumberOfPoints())
    {
      return;
    }

    std::vector<vtkIdType> candidates(endId - startId);
    std::iota(candidates.begin(), candidates.end(), startId);
    auto rng = std::default_random_engine{};
    std::shuffle(candidates.begin(), candidates.end(), rng);

    vtkNew<vtkIdList> neighbors;
    double p[3], lockedP[3];
    double squaredRadius = this->Radius * this->Radius;

    vtkPointData* inputPointData = this->Input->GetPointData();
    vtkPointData* outputPointData = this->Output->GetPointData();
    outputPointData->CopyAllOn();

    vtkPoints* inputPoints = this->Input->GetPoints();
    vtkPoints* outputPoints = this->Output->GetPoints();

    for (vtkIdType candidate : candidates)
    {
      this->Mutex.lock();
      bool alreadyProcessed = this->AlreadyProcessed->GetValue(candidate);
      if (!alreadyProcessed)
      {
        inputPoints->GetPoint(candidate, p);
        bool locked = false;
        for (vtkIdType lockedId : this->LockedIds)
        {
          inputPoints->GetPoint(lockedId, lockedP);
          if (vtkMath::Distance2BetweenPoints(p, lockedP) < squaredRadius)
          {
            locked = true;
            break;
          }
        }
        if (locked)
        {
          this->Mutex.unlock();
          continue;
        }
        this->LockedIds.insert(candidate);
        this->Mutex.unlock();
        this->OutputMutex.lock();
        outputPoints->InsertNextPoint(p);
        outputPointData->InsertNextTuple(candidate, inputPointData);
        this->OutputMutex.unlock();
        this->Locator->FindPointsWithinRadius(this->Radius, p, neighbors);
        for (vtkIdType i = 0; i < neighbors->GetNumberOfIds(); ++i)
        {
          if (!this->AlreadyProcessed->GetValue(neighbors->GetId(i)))
          {
            this->AlreadyProcessed->SetValue(neighbors->GetId(i), true);
          }
        }
        this->Mutex.lock();
        this->LockedIds.erase(candidate);
        this->Mutex.unlock();
      }
      else
      {
        this->Mutex.unlock();
      }
    }
  }

  vtkPointSet* Input;
  vtkAbstractPointLocator* Locator;
  vtkPointSet* Output;
  double Radius;

  /**
   * This bit array keeps track of which points are already been processed
   */
  vtkNew<vtkBitArray> AlreadyProcessed;

  /**
   * Lock on points being treated by on thread.
   * Any point within Radius of being treated points are not processed and skipped.
   */
  std::unordered_set<vtkIdType> LockedIds;

  //@{
  /**
   * Mutices used to ensure atomic read and write.
   */
  std::mutex Mutex;
  std::mutex OutputMutex;
  //@}
};
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

  output->SetPoints(vtkNew<vtkPoints>());
  output->GetPointData()->CopyStructure(input->GetPointData());

  DartThrowerWorker worker(input, this->Locator, output, this->Radius);
  vtkSMPTools::For(0, input->GetNumberOfPoints(), worker);

  output->GetPointData()->SetNormals(
    output->GetPointData()->GetArray(input->GetPointData()->GetNormals()->GetName()));

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
