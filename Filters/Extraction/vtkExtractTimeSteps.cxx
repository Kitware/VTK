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
#include <vector>


vtkStandardNewMacro(vtkExtractTimeSteps);

vtkExtractTimeSteps::vtkExtractTimeSteps() :
  UseRange(false), TimeStepInterval(1)
{
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
}

//----------------------------------------------------------------------------
void vtkExtractTimeSteps::AddTimeStepIndex(int timeStepIndex)
{
  if (this->TimeStepIndices.insert(timeStepIndex).second)
  {
    this->Modified();
  }
}

void vtkExtractTimeSteps::SetTimeStepIndices(int count, const int *timeStepIndices)
{
  this->TimeStepIndices.clear();
  this->TimeStepIndices.insert(timeStepIndices, timeStepIndices + count);
  this->Modified();
}

void vtkExtractTimeSteps::GetTimeStepIndices(int *timeStepIndices) const
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

//----------------------------------------------------------------------------
int vtkExtractTimeSteps::RequestInformation(vtkInformation*,
                                            vtkInformationVector **inputVector,
                                            vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  if (!this->TimeStepIndices.empty() &&
      inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    double *inTimes =
      inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    int numTimes =
      inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

    std::vector<double> outTimes;
    if (!this->UseRange)
    {
      for (std::set<int>::iterator it = this->TimeStepIndices.begin();
           it != this->TimeStepIndices.end(); ++it)
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
        if (i >= this->Range[0] && i <= this->Range[1])
        {
          if ((i - this->Range[0]) % this->TimeStepInterval == 0)
          {
            outTimes.push_back(inTimes[i]);
          }
        }
      }
    }

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
int vtkExtractTimeSteps::RequestData(vtkInformation *,
                                     vtkInformationVector **inputVector,
                                     vtkInformationVector *outputVector)
{
  vtkDataObject* inData = vtkDataObject::GetData(inputVector[0], 0);
  vtkDataObject* outData = vtkDataObject::GetData(outputVector, 0);

  if (inData && outData)
  {
    outData->ShallowCopy(inData);
  }
  return 1;
}
