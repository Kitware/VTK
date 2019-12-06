/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractTimeSteps.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractTimeSteps.h"

#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <algorithm>
#include <cmath>
#include <vector>

vtkStandardNewMacro(vtkExtractTimeSteps);

vtkExtractTimeSteps::vtkExtractTimeSteps()
  : UseRange(false)
  , TimeStepInterval(1)
  , TimeEstimationMode(PREVIOUS_TIMESTEP)
{
  this->Range[0] = 0;
  this->Range[1] = 0;
}

//----------------------------------------------------------------------------
void vtkExtractTimeSteps::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  int count = static_cast<int>(this->TimeStepIndices.size());
  os << indent << "Number of Time Steps: " << count << std::endl;
  if (count > 0)
  {
    std::set<int>::iterator it = this->TimeStepIndices.begin();
    os << indent << "Time Step Indices: " << *it++;
    for (int i = 1; i < std::min(count, 4); ++i)
    {
      os << ", " << *it++;
    }
    if (count > 9)
    {
      std::advance(it, count - 8);
      os << ", ... ";
    }
    while (it != this->TimeStepIndices.end())
    {
      os << ", " << *it++;
    }
    os << std::endl;
  }

  os << indent << "UseRange: " << (this->UseRange ? "true" : "false") << std::endl;
  os << indent << "Range: " << this->Range[0] << ", " << this->Range[1] << std::endl;
  os << indent << "TimeStepInterval: " << this->TimeStepInterval << std::endl;
  os << indent << "TimeEstimationMode: ";
  switch (this->TimeEstimationMode)
  {
    case PREVIOUS_TIMESTEP:
      os << "Previous Timestep" << std::endl;
      break;
    case NEXT_TIMESTEP:
      os << "Next Timestep" << std::endl;
      break;
    case NEAREST_TIMESTEP:
      os << "Nearest Timestep" << std::endl;
      break;
  }
}

//----------------------------------------------------------------------------
void vtkExtractTimeSteps::AddTimeStepIndex(int timeStepIndex)
{
  if (this->TimeStepIndices.insert(timeStepIndex).second)
  {
    this->Modified();
  }
}

void vtkExtractTimeSteps::SetTimeStepIndices(int count, const int* timeStepIndices)
{
  this->TimeStepIndices.clear();
  this->TimeStepIndices.insert(timeStepIndices, timeStepIndices + count);
  this->Modified();
}

void vtkExtractTimeSteps::GetTimeStepIndices(int* timeStepIndices) const
{
  std::copy(this->TimeStepIndices.begin(), this->TimeStepIndices.end(), timeStepIndices);
}

void vtkExtractTimeSteps::GenerateTimeStepIndices(int begin, int end, int step)
{
  if (step != 0)
  {
    this->TimeStepIndices.clear();
    for (int i = begin; i < end; i += step)
    {
      this->TimeStepIndices.insert(i);
    }
    this->Modified();
  }
}

namespace
{

void getTimeSteps(vtkInformation* inInfo, const std::set<int>& timeStepIndices, bool useRange,
  int* range, int timeStepInterval, std::vector<double>& outTimes)
{
  double* inTimes = inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  int numTimes = inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

  if (!useRange)
  {
    for (std::set<int>::iterator it = timeStepIndices.begin(); it != timeStepIndices.end(); ++it)
    {
      if (*it >= 0 && *it < numTimes)
      {
        outTimes.push_back(inTimes[*it]);
      }
    }
  }
  else
  {
    for (int i = 0; i < numTimes; ++i)
    {
      if (i >= range[0] && i <= range[1])
      {
        if ((i - range[0]) % timeStepInterval == 0)
        {
          outTimes.push_back(inTimes[i]);
        }
      }
    }
  }
}

}

//----------------------------------------------------------------------------
int vtkExtractTimeSteps::RequestInformation(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  if (!this->TimeStepIndices.empty() && inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {

    std::vector<double> outTimes;
    getTimeSteps(
      inInfo, this->TimeStepIndices, this->UseRange, this->Range, this->TimeStepInterval, outTimes);

    if (!outTimes.empty())
    {
      outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &outTimes[0],
        static_cast<int>(outTimes.size()));

      double range[2] = { outTimes.front(), outTimes.back() };
      outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), range, 2);
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkExtractTimeSteps::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    double updateTime = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

    std::vector<double> outTimes;
    getTimeSteps(
      inInfo, this->TimeStepIndices, this->UseRange, this->Range, this->TimeStepInterval, outTimes);

    if (outTimes.size() == 0)
    {
      vtkErrorMacro("Input has no time steps.");
      return 0;
    }

    double inputTime;
    if (updateTime >= outTimes.back())
    {
      inputTime = outTimes.back();
    }
    else if (updateTime <= outTimes.front())
    {
      inputTime = outTimes.front();
    }
    else
    {
      auto gtindex = std::upper_bound(outTimes.begin(), outTimes.end(), updateTime);
      auto leindex = gtindex - 1;
      if (updateTime == *leindex)
      {
        inputTime = updateTime;
      }
      else
      {
        switch (this->TimeEstimationMode)
        {
          default:
          case PREVIOUS_TIMESTEP:
            inputTime = *leindex;
            break;
          case NEXT_TIMESTEP:
            inputTime = *gtindex;
            break;
          case NEAREST_TIMESTEP:
            if (std::abs(updateTime - *leindex) <= std::abs(*gtindex - updateTime))
            {
              inputTime = *leindex;
            }
            else
            {
              inputTime = *gtindex;
            }
            break;
        }
      }
    }
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), inputTime);
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkExtractTimeSteps::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* inData = vtkDataObject::GetData(inputVector[0], 0);
  vtkDataObject* outData = vtkDataObject::GetData(outputVector, 0);

  if (inData && outData)
  {
    outData->ShallowCopy(inData);
  }
  return 1;
}
