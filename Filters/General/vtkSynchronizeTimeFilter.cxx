/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSynchronizeTimeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/

#include "vtkSynchronizeTimeFilter.h"

#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <algorithm>
#include <cmath>

vtkStandardNewMacro(vtkSynchronizeTimeFilter);

//----------------------------------------------------------------------------
vtkSynchronizeTimeFilter::vtkSynchronizeTimeFilter()
{
  this->SetNumberOfInputPorts(2);
  this->RelativeTolerance = 0.00001;
}

//----------------------------------------------------------------------------
vtkSynchronizeTimeFilter::~vtkSynchronizeTimeFilter()
{
}

//----------------------------------------------------------------------------
void vtkSynchronizeTimeFilter::SetSourceConnection(
  vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

//----------------------------------------------------------------------------
double vtkSynchronizeTimeFilter::GetInputTimeValue(double outputTimeValue)
{
  double inputTimeValue = outputTimeValue;

  if(outputTimeValue <= this->OutputTimeStepValues.back())
  {
    std::vector<double>::iterator pos = std::find(
      this->OutputTimeStepValues.begin(),
      this->OutputTimeStepValues.end(), outputTimeValue);
    if(pos != this->OutputTimeStepValues.end())
    {
      size_t index = std::distance(this->OutputTimeStepValues.begin(), pos);
      inputTimeValue = this->InputTimeStepValues[index];
    }
  }
  return inputTimeValue;
}

//----------------------------------------------------------------------------
double vtkSynchronizeTimeFilter::GetOutputTimeValue(double inputTimeValue)
{
  double outputTimeValue = inputTimeValue;

  std::vector<double>::iterator pos = std::find(
    this->InputTimeStepValues.begin(),
    this->InputTimeStepValues.end(), inputTimeValue);
  if(pos != this->InputTimeStepValues.end())
  {
    size_t index = std::distance(this->InputTimeStepValues.begin(), pos);
    if(index < this->OutputTimeStepValues.size())
    {
      outputTimeValue = this->OutputTimeStepValues[index];
    }
  }
  return outputTimeValue;
}

//----------------------------------------------------------------------------
int vtkSynchronizeTimeFilter::RequestInformation(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  this->InputTimeStepValues.clear();
  this->OutputTimeStepValues.clear();
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  if(inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    int numberOfTimeSteps =
      inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    double* values =
      inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    this->InputTimeStepValues.resize(numberOfTimeSteps);
    this->OutputTimeStepValues.resize(numberOfTimeSteps);
    for (int i=0;i<numberOfTimeSteps;i++)
    {
      this->InputTimeStepValues[i] = values[i];
      this->OutputTimeStepValues[i] = values[i];
    }

    // time steps for port 1 (sync time steps)
    inInfo = inputVector[1]->GetInformationObject(0);
    if( inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()) )
    {
      int numberOfOutputTimeSteps =
        inInfo->Length( vtkStreamingDemandDrivenPipeline::TIME_STEPS() );
      values =
        inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
      double diffMax = this->RelativeTolerance * std::abs(
        this->InputTimeStepValues[0]-this->InputTimeStepValues[numberOfTimeSteps-1]);
      for(int i=0;i<numberOfOutputTimeSteps;i++)
      {
        for(size_t j=0;j<this->OutputTimeStepValues.size();j++)
        {
          if(std::abs(values[i]-this->OutputTimeStepValues[j]) < diffMax)
          {
            this->OutputTimeStepValues[j] = values[i];
          }
        }
      }
    }
    // check to make sure we don't have any repeated time steps
    for(size_t i=0;i<this->OutputTimeStepValues.size()-1;i++)
    {
      if(this->OutputTimeStepValues[i] == this->OutputTimeStepValues[i+1])
      {
        vtkWarningMacro("The Synchronize Time Filter detected 2 time steps that "
                        << "mapped to the same value. Either the input data has "
                        << "2 time steps with identical time values or the "
                        << "RelativeTolerance parameter (currently set to "
                        << this->RelativeTolerance << ") is too large");
      }
    }

    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    double timeRange[2] = {this->OutputTimeStepValues[0],
                           this->OutputTimeStepValues[numberOfTimeSteps-1]};
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
                 &this->OutputTimeStepValues[0], numberOfTimeSteps);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);

  }
  else
  { // just in case output time steps are set by the second input
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkSynchronizeTimeFilter::RequestUpdateExtent (
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if(outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    double timeValue = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    double requestTimeValue = this->GetInputTimeValue(timeValue);
    inputVector[0]->GetInformationObject(0)->Set
      (vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), requestTimeValue);
  }
  else
  {
    inputVector[0]->GetInformationObject(0)->Remove
      (vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
  }
  // Always remove the request for the update time step from
  // the sync input since we only care about the time step values
  // that it can provide and we already have that.
  inputVector[1]->GetInformationObject(0)->Remove
    (vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

  return 1;
}

//----------------------------------------------------------------------------
int vtkSynchronizeTimeFilter::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());
  output->ShallowCopy(input);

  if(input->GetInformation()->Has(vtkDataObject::DATA_TIME_STEP()) )
  {
    double timeValue = input->GetInformation()->Get(vtkDataObject::DATA_TIME_STEP());
    double outputTimeValue = this->GetOutputTimeValue(timeValue);
    output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), outputTimeValue);
  }
  return 1;
}
