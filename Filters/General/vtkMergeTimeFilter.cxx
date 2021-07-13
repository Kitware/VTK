/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMergeTimeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/

#include "vtkMergeTimeFilter.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMathUtilities.h"
#include "vtkMultiBlockDataGroupFilter.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSMPTools.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <algorithm>

vtkStandardNewMacro(vtkMergeTimeFilter);

//------------------------------------------------------------------------------
void vtkMergeTimeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Tolerance: " << this->Tolerance << endl;
  os << indent << "UseRelativeTolerance: " << this->UseRelativeTolerance << endl;
  os << indent << "UseIntersection: " << this->UseIntersection << endl;
}

//------------------------------------------------------------------------------
int vtkMergeTimeFilter::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//------------------------------------------------------------------------------
bool vtkMergeTimeFilter::AreTimesWithinTolerance(double t1, double t2)
{
  if (t1 == t2)
  {
    return true;
  }

  if (this->UseRelativeTolerance && t1 != 0)
  {
    return vtkMathUtilities::NearlyEqual(t1, t2, this->Tolerance);
  }

  return vtkMathUtilities::FuzzyCompare(t1, t2, this->Tolerance);
}

//------------------------------------------------------------------------------
double vtkMergeTimeFilter::MapToInputTime(int input, double outputTime)
{
  double inputTime = outputTime;
  for (double time : this->InputsTimeSteps[input])
  {
    if (this->AreTimesWithinTolerance(time, outputTime))
    {
      inputTime = time;
      break;
    }
    // times are sorted, no need to go further.
    if (time > outputTime)
    {
      break;
    }

    inputTime = time;
  }

  return inputTime;
}

//------------------------------------------------------------------------------
void vtkMergeTimeFilter::MergeTimeSteps(const std::vector<double>& timeSteps)
{
  // Clamp input values to existing one, when possible.
  std::vector<double> newTimeSteps;
  for (double newTime : timeSteps)
  {
    // lambda to find TimeStep in the list, depending on Tolerance.
    auto insideTolerance = [=](double outputTime) {
      return this->AreTimesWithinTolerance(outputTime, newTime);
    };

    auto it =
      std::find_if(this->OutputTimeSteps.begin(), this->OutputTimeSteps.end(), insideTolerance);

    if (it != this->OutputTimeSteps.end())
    {
      newTimeSteps.push_back(*it);
    }
    else
    {
      newTimeSteps.push_back(newTime);
    }
  }

  vtkSMPTools::Sort(newTimeSteps.begin(), newTimeSteps.end());

  // Merge new list inside current list, following the strategy (union or intersection).
  std::vector<double> mergedTimeSteps;
  if (this->UseIntersection)
  {
    if (this->OutputTimeSteps.empty())
    {
      mergedTimeSteps = newTimeSteps;
    }
    else
    {
      std::set_intersection(this->OutputTimeSteps.begin(), this->OutputTimeSteps.end(),
        newTimeSteps.begin(), newTimeSteps.end(), std::back_inserter(mergedTimeSteps));
    }
  }
  else
  {
    std::set_union(this->OutputTimeSteps.begin(), this->OutputTimeSteps.end(), newTimeSteps.begin(),
      newTimeSteps.end(), std::back_inserter(mergedTimeSteps));
  }

  std::swap(this->OutputTimeSteps, mergedTimeSteps);
  auto last = std::unique(this->OutputTimeSteps.begin(), this->OutputTimeSteps.end());
  this->OutputTimeSteps.erase(last, this->OutputTimeSteps.end());
}

//------------------------------------------------------------------------------
int vtkMergeTimeFilter::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Remove(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());

  this->OutputTimeSteps.clear();
  int numInputs = inputVector[0]->GetNumberOfInformationObjects();
  for (int i = 0; i < numInputs; i++)
  {
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(i);
    if (inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
    {
      double* inputTimes = inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
      int nbOfTimes = inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
      std::vector<double> originalTimeSteps;
      for (int idx = 0; idx < nbOfTimes; idx++)
      {
        double toInsert = inputTimes[idx];
        originalTimeSteps.push_back(toInsert);
      }
      this->InputsTimeSteps.push_back(originalTimeSteps);
      this->MergeTimeSteps(originalTimeSteps);
    }
  }

  if (!this->OutputTimeSteps.empty())
  {
    double outRange[2] = { this->OutputTimeSteps.front(), this->OutputTimeSteps.back() };
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), outRange, 2);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &this->OutputTimeSteps[0],
      static_cast<int>(this->OutputTimeSteps.size()));
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkMergeTimeFilter::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  int numInputs = inputVector[0]->GetNumberOfInformationObjects();
  for (int i = 0; i < numInputs; i++)
  {
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(i);
    if (inInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
    {
      inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
        inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);
    }
  }

  // manage update TimeSteps : forward requested TimeStep to all inputs.
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    double timeValue = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    this->RequestedTimeValue = timeValue;
    for (int i = 0; i < numInputs; i++)
    {
      double requestedTimeForInput = this->MapToInputTime(i, timeValue);
      vtkInformation* inInfo = inputVector[0]->GetInformationObject(i);
      inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), requestedTimeForInput);
    }
  }
  else
  {
    for (int i = 0; i < numInputs; i++)
    {
      vtkInformation* inInfo = inputVector[0]->GetInformationObject(i);
      inInfo->Remove(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    }
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkMergeTimeFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkMultiBlockDataSet* output =
    vtkMultiBlockDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (!output)
  {
    return 0;
  }

  vtkNew<vtkMultiBlockDataGroupFilter> groupInputs;
  int numInputs = inputVector[0]->GetNumberOfInformationObjects();
  for (int idx = 0; idx < numInputs; idx++)
  {
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(idx);
    vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
    groupInputs->AddInputData(input);
  }
  groupInputs->Update();
  output->ShallowCopy(groupInputs->GetOutput());

  output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), this->RequestedTimeValue);

  return 1;
}
