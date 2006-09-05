/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTemporalShiftScale.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTemporalShiftScale.h"

#include "vtkTemporalDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkCxxRevisionMacro(vtkTemporalShiftScale, "1.3");
vtkStandardNewMacro(vtkTemporalShiftScale);

//----------------------------------------------------------------------------
vtkTemporalShiftScale::vtkTemporalShiftScale()
{
  this->Shift = 0;
  this->Scale = 1;
}

//----------------------------------------------------------------------------
vtkTemporalShiftScale::~vtkTemporalShiftScale()
{
}

//----------------------------------------------------------------------------
void vtkTemporalShiftScale::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Scale: " << this->Scale << endl;
  os << indent << "Shift: " << this->Shift << endl;
}

//----------------------------------------------------------------------------
// Change the information
int vtkTemporalShiftScale::RequestInformation (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  if (inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
    {
    double *inTimes =
      inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    int numTimes =
      inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    double *outTimes = new double [numTimes];
    int i;
    for (i = 0; i < numTimes; ++i)
      {
      outTimes[i] = inTimes[i]*this->Scale+this->Shift;
      }
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
                 outTimes,numTimes);
    delete [] outTimes;
    }

  if (inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_RANGE()))
    {
    double *inRange =
      inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
    double outRange[2];
    outRange[0] = inRange[0]*this->Scale+this->Shift;
    outRange[1] = inRange[1]*this->Scale+this->Shift;
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(),
                 outRange,2);
    }
  return 1;
}


//----------------------------------------------------------------------------
// This method simply copies by reference the input data to the output.
int vtkTemporalShiftScale::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  
  vtkTemporalDataSet *inData = vtkTemporalDataSet::SafeDownCast
    (inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkTemporalDataSet *outData = vtkTemporalDataSet::SafeDownCast
    (outInfo->Get(vtkDataObject::DATA_OBJECT()));
  
  // shallow copy the data
  if (inData && outData)
    {
    outData->ShallowCopy(inData);
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkTemporalShiftScale::RequestUpdateExtent (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  // reverse trnslate the times
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS()))
    {
    double *upTimes =
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS());
    int numTimes = 
      outInfo->Length(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS());
    double *inTimes = new double [numTimes];
    int i;
    for (i = 0; i < numTimes; ++i)
      {
      inTimes[i] = (upTimes[i] - this->Shift)/this->Scale;
      }
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS(),
                inTimes,numTimes);
    delete [] inTimes;
    }

  return 1;
}
