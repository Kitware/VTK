// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStreamerBase.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkStreamerBase.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"


//=============================================================================
vtkStreamerBase::vtkStreamerBase()
{
  this->NumberOfPasses = 1;
  this->CurrentIndex = 0;
}

//-----------------------------------------------------------------------------
vtkStreamerBase::~vtkStreamerBase()
{
}

//-----------------------------------------------------------------------------
void vtkStreamerBase::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkStreamerBase::ProcessRequest(vtkInformation* request,
                                    vtkInformationVector** inputVector,
                                    vtkInformationVector* outputVector)
{
  // generate the data
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    return this->RequestData(request, inputVector, outputVector);
  }

  if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
  }

  // execute information
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    return this->RequestInformation(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//-----------------------------------------------------------------------------
int vtkStreamerBase::RequestData(vtkInformation *request,
                                 vtkInformationVector **inputVector,
                                 vtkInformationVector *outputVector)
{
  if (!this->ExecutePass(inputVector, outputVector))
  {
    request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
    return 0;
  }

  this->CurrentIndex++;

  if (  this->CurrentIndex < this->NumberOfPasses )
  {
    // There is still more to do.
    request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
  }
  else
  {
    // We are done.  Finish up.
    request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
    if (!this->PostExecute(inputVector, outputVector))
    {
      return 0;
    }
    this->CurrentIndex = 0;
  }

  return 1;
}
