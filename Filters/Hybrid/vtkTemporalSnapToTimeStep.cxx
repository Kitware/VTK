/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTemporalSnapToTimeStep.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTemporalSnapToTimeStep.h"

#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <cmath>

vtkStandardNewMacro(vtkTemporalSnapToTimeStep);

//----------------------------------------------------------------------------
vtkTemporalSnapToTimeStep::vtkTemporalSnapToTimeStep()
{
  this->HasDiscrete = 0;
  this->SnapMode = 0;
}

//----------------------------------------------------------------------------
vtkTemporalSnapToTimeStep::~vtkTemporalSnapToTimeStep()
{
}

//----------------------------------------------------------------------------
int vtkTemporalSnapToTimeStep::ProcessRequest(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  //modify the time in either of these passes
  if(  request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_TIME())
     ||request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
      return this->RequestUpdateExtent(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
// Change the information
int vtkTemporalSnapToTimeStep::RequestInformation (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  this->HasDiscrete = 0;

  // unset the time steps if they are set
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  }

  if (inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    int numTimes =
      inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    this->InputTimeValues.resize(numTimes);
    inInfo->Get( vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
      &this->InputTimeValues[0] );
    this->HasDiscrete = 1;
  }

  if (inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_RANGE()))
  {
    double *inRange =
      inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
    double outRange[2];
    outRange[0] = inRange[0];
    outRange[1] = inRange[1];
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(),
                 outRange,2);
  }
  return 1;
}

//----------------------------------------------------------------------------
// This method simply copies by reference the input data to the output.
int vtkTemporalSnapToTimeStep::RequestData(
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

    // fill in the time steps
    double inTime = inData->GetInformation()->Get(vtkDataObject::DATA_TIME_STEP());

    if(inData->GetInformation()->Has(vtkDataObject::DATA_TIME_STEP()))
    {
      double outTime = inTime;
      outData->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), outTime);
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkTemporalSnapToTimeStep::RequestUpdateExtent (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  // find the nearest timestep in the input
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    double upTime =
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

    double *inTimes = new double [1];

    if (!this->HasDiscrete || this->InputTimeValues.size()==0)
    {
      inTimes[0] = upTime;
    }
    else
    {
      double dist = VTK_DOUBLE_MAX;
      int index = -1;
      for (unsigned int t=0; t<this->InputTimeValues.size(); t++)
      {
        double thisdist = fabs(upTime-this->InputTimeValues[t]);
        if (this->SnapMode==VTK_SNAP_NEAREST && thisdist<dist)
        {
          index = t;
          dist = thisdist;
        }
        else if (this->SnapMode==VTK_SNAP_NEXTBELOW_OR_EQUAL)
        {
          if (this->InputTimeValues[t]==upTime)
          {
            index = t;
            break;
          }
          else if (this->InputTimeValues[t]<upTime)
          {
            index = t;
          }
          else if (this->InputTimeValues[t]>upTime)
          {
            break;
          }
        }
        else if (this->SnapMode==VTK_SNAP_NEXTABOVE_OR_EQUAL)
        {
          if (this->InputTimeValues[t]==upTime)
          {
            index = t;
            break;
          }
          else if (this->InputTimeValues[t]>upTime)
          {
            index = t;
            break;
          }
        }
      }
      upTime = this->InputTimeValues[index==-1 ? 0 : index];
    }
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(),  upTime);
    delete [] inTimes;
  }

  return 1;
}
//----------------------------------------------------------------------------
void vtkTemporalSnapToTimeStep::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "SnapMode: " << this->SnapMode << endl;
}
//----------------------------------------------------------------------------
