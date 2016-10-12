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
#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkCompositeDataPipeline.h"

#include <cassert>

vtkStandardNewMacro(vtkTemporalShiftScale);

//----------------------------------------------------------------------------
vtkTemporalShiftScale::vtkTemporalShiftScale()
{
  this->PreShift               = 0;
  this->PostShift              = 0;
  this->Scale                  = 1;
  this->Periodic               = 0;
  this->PeriodicEndCorrection  = 1;
  this->MaximumNumberOfPeriods = 1;

  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);

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
  os << indent << "PreShift: " << this->PreShift << endl;
  os << indent << "PostShift: " << this->PostShift << endl;
  os << indent << "Periodic: " << this->Periodic << endl;
  os << indent << "PeriodicEndCorrection: " << this->PeriodicEndCorrection << endl;
  os << indent << "MaximumNumberOfPeriods: " << this->MaximumNumberOfPeriods << endl;
}

//----------------------------------------------------------------------------
int vtkTemporalShiftScale::ProcessRequest(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // create the output
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
  {
    return this->RequestDataObject(request, inputVector, outputVector);
  }

  // generate the data
  if(request->Has(vtkCompositeDataPipeline::REQUEST_DATA()))
  {
    int retVal = this->RequestData(request, inputVector, outputVector);
    return retVal;
  }

  // execute information
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    return this->RequestInformation(request, inputVector, outputVector);
  }

  // set update extent
  if(  request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_TIME())
     ||request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}


//----------------------------------------------------------------------------
int vtkTemporalShiftScale::FillInputPortInformation(
  int port,
  vtkInformation* info)
{
  if (port == 0) {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  }
  return 1;
}

int vtkTemporalShiftScale::FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}

int vtkTemporalShiftScale::RequestDataObject( vtkInformation*,
                                             vtkInformationVector** inputVector ,
                                             vtkInformationVector* outputVector)
{
  if (this->GetNumberOfInputPorts() == 0 || this->GetNumberOfOutputPorts() == 0)
  {
    return 1;
  }

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }
  vtkDataObject *input = inInfo->Get(vtkDataObject::DATA_OBJECT());

  if (input)
  {
    // for each output
    for(int i=0; i < this->GetNumberOfOutputPorts(); ++i)
    {
      vtkInformation* info = outputVector->GetInformationObject(i);
      vtkDataObject *output = info->Get(vtkDataObject::DATA_OBJECT());

      if (!output || !output->IsA(input->GetClassName()))
      {
        vtkDataObject* newOutput = input->NewInstance();
        info->Set(vtkDataObject::DATA_OBJECT(), newOutput);
        newOutput->Delete();
      }
    }
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
inline double vtkTemporalShiftScale::ForwardConvert(double T0)
{
  return (T0 + this->PreShift)*this->Scale + this->PostShift;
}
//----------------------------------------------------------------------------
inline double vtkTemporalShiftScale::BackwardConvert(double T1)
{
  return (T1 - this->PostShift)/this->Scale - this->PreShift;
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

  this->InRange[0] = 0.0;
  this->InRange[1] = 0.0;
  //
  if (inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_RANGE()))
  {
    inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_RANGE(),this->InRange);
    this->OutRange[0] = this->ForwardConvert(this->InRange[0]);
    this->OutRange[1] = this->ForwardConvert(this->InRange[1]);
    this->PeriodicRange[0] = this->OutRange[0];
    this->PeriodicRange[1] = this->OutRange[1];
    if (this->Periodic)
    {
        this->OutRange[1] = this->OutRange[0] +
          (this->OutRange[1]-this->OutRange[0])*this->MaximumNumberOfPeriods;
    }
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(),
      this->OutRange,2);
  }

  if (inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    double *inTimes =
      inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    int numTimes =
      inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    double range = this->PeriodicRange[1] - this->PeriodicRange[0];
    double *outTimes;
    int numOutTimes = numTimes;
    this->PeriodicN = numTimes;
    if (this->Periodic && this->PeriodicEndCorrection)
    {
        PeriodicN = numTimes-1;
    }
    if (this->Periodic)
    {
        numOutTimes = static_cast<int>(this->PeriodicN*this->MaximumNumberOfPeriods);
    }
    outTimes = new double [numOutTimes];
    int i;
    for (i=0; i<numOutTimes; ++i)
    {
      int m = i/PeriodicN;
      int o = i%PeriodicN;
      if (m==0)
      {
        outTimes[i] = this->ForwardConvert(inTimes[o]);
      }
      else if (this->PeriodicEndCorrection)
      {
        outTimes[i] = outTimes[o] + m*range;
      }
      else if (!this->PeriodicEndCorrection)
      {
        outTimes[i] = outTimes[o] + m*range;
      }
    }
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
                 outTimes,numOutTimes);
    delete [] outTimes;
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

  vtkDataObject *inData = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkDataObject *outData = outInfo->Get(vtkDataObject::DATA_OBJECT());

  // shallow copy the data
  if (inData && outData)
  {
    outData->ShallowCopy(inData);
  }

  // @TODO The time value set here is not correct if periodic is true

  double inTime = inData->GetInformation()->Get(vtkDataObject::DATA_TIME_STEP());

  double range = this->PeriodicRange[1] - this->PeriodicRange[0];

  double outTime = this->ForwardConvert(inTime);
  if (this->Periodic)
  {
    outTime += this->TempMultiplier*range;
  }
  outData->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), outTime);

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

  // reverse translate the times
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    double upTime =
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());


    this->TempMultiplier = 0.0;

    double range = this->PeriodicRange[1] - this->PeriodicRange[0];

    double ttime = upTime;
    if (this->Periodic)
    {
      if (ttime>this->PeriodicRange[1])
      {
        double m = floor((ttime - this->PeriodicRange[0])/range);
        this->TempMultiplier = m;
        ttime = ttime - range*m;
      }
    }
    double inTime = this->BackwardConvert(ttime);

    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(),inTime);
  }

  return 1;
}
