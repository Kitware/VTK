/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkForceTime.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkForceTime.h"

#include "vtkDataObject.h"
#include "vtkDataObjectTypes.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkForceTime);

//----------------------------------------------------------------------------
vtkForceTime::vtkForceTime()
{
  this->ForcedTime = 0.0;
  this->IgnorePipelineTime = true;
  this->Cache = NULL;
  this->PipelineTime = -1;
  this->PipelineTimeFlag = false;
}

//----------------------------------------------------------------------------
vtkForceTime::~vtkForceTime()
{
  if (this->Cache != NULL)
  {
    this->Cache->Delete();
    this->Cache = NULL;
  }
}

//----------------------------------------------------------------------------
void vtkForceTime::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ForcedTime: " << this->ForcedTime << endl;
  os << indent << "IgnorePipelineTime: " << this->IgnorePipelineTime << endl;
}

//----------------------------------------------------------------------------
int vtkForceTime::RequestInformation(vtkInformation * vtkNotUsed(request),
                                     vtkInformationVector **inputVector,
                                     vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  if (inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_RANGE()))
  {
    double range[2];
    inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), range);
    if (this->IgnorePipelineTime)
    {
      range[0] = this->ForcedTime;
      range[1] = this->ForcedTime;
    }
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(),
                 range, 2);
  }

  if (inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    double *inTimes =
      inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    int numTimes =
      inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    double *outTimes;
    if (this->IgnorePipelineTime)
    {
      outTimes = new double [numTimes];
      for (int i = 0; i < numTimes; i++)
      {
        outTimes[i] = this->ForcedTime;
      }
    }
    else
    {
      outTimes = inTimes;
    }
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
                 outTimes, numTimes);

    if (this->IgnorePipelineTime)
    {
      delete [] outTimes;
    }
  }

  // Upstream filters changed, invalidate cache
  if (this->IgnorePipelineTime && this->Cache)
  {
    this->Cache->Delete();
    this->Cache = NULL;
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkForceTime::RequestData(vtkInformation *request,
                              vtkInformationVector **inputVector,
                              vtkInformationVector *outputVector)
{
  vtkDataObject* inData = vtkDataObject::GetData(inputVector[0], 0);
  vtkDataObject* outData = vtkDataObject::GetData(outputVector, 0);

  if (!inData)
  {
    return 1;
  }

  // Filter is "disabled", just pass input data
  if (!this->IgnorePipelineTime)
  {
    outData->ShallowCopy(inData);
    return 1;
  }

  // Create Cache
  if (!this->Cache)
  {
    request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
    this->Cache = vtkDataObjectTypes::NewDataObject(inData->GetClassName());
    this->Cache->DeepCopy(inData);
    this->PipelineTimeFlag = true;
  }
  else if (this->PipelineTimeFlag)
  {
    // Stop the pipeline loop
    request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
    this->PipelineTimeFlag = false;
  }
  outData->ShallowCopy(this->Cache);
  return 1;
}

//----------------------------------------------------------------------------
int vtkForceTime::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
                                      vtkInformationVector** inputVector,
                                      vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  if (this->IgnorePipelineTime && !this->Cache)
  {
    double *inTimes = inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    if (inTimes)
    {
      // Save current pipeline time step
      this->PipelineTime =
        inInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
      inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(),
                  this->ForcedTime);
    }
  }
  else if (this->PipelineTimeFlag)
  {
    // Restore pipeline time
    double *inTimes = inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    if (inTimes)
    {
      inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(),
                  this->PipelineTime);
    }
  }

  return 1;
}
