/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageIterateFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageIterateFilter.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"


//----------------------------------------------------------------------------
vtkImageIterateFilter::vtkImageIterateFilter()
{
  // for filters that execute multiple times
  this->Iteration = 0;
  this->NumberOfIterations = 0;
  this->IterationData = NULL;
  this->SetNumberOfIterations(1);
  this->InputVector = vtkInformationVector::New();
  this->OutputVector = vtkInformationVector::New();
}

//----------------------------------------------------------------------------
vtkImageIterateFilter::~vtkImageIterateFilter()
{
  this->SetNumberOfIterations(0);
  this->InputVector->Delete();
  this->OutputVector->Delete();
}

//----------------------------------------------------------------------------
void vtkImageIterateFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "NumberOfIterations: " << this->NumberOfIterations << "\n";

  // This variable is included here to pass the PrintSelf test.
  // The variable is public to get around a compiler issue.
  // this->Iteration
}


//----------------------------------------------------------------------------
vtkImageData *vtkImageIterateFilter::GetIterationInput()
{
  if (this->IterationData == NULL || this->Iteration == 0)
    {
    // error, or return input ???
    return vtkImageData::SafeDownCast(this->GetInput());
    }
  return this->IterationData[this->Iteration];
}


//----------------------------------------------------------------------------
vtkImageData *vtkImageIterateFilter::GetIterationOutput()
{
  if (this->IterationData == NULL || 
      this->Iteration == this->NumberOfIterations-1)
    {
    // error, or return output ???
    return this->GetOutput();
    }
  return this->IterationData[this->Iteration+1];
}

//----------------------------------------------------------------------------
int
vtkImageIterateFilter
::RequestInformation(vtkInformation* request,
                     vtkInformationVector** inputVector,
                     vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkImageData* input =
    vtkImageData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkImageData* output =
    vtkImageData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  this->IterationData[0] = input;
  this->IterationData[this->NumberOfIterations] = output;

  vtkInformation* in = inInfo;
  for(int i=0; i < this->NumberOfIterations; ++i)
    {
    this->Iteration = i;

    vtkInformation* out = this->IterationData[i+1]->GetPipelineInformation();
    vtkDataObject* outData = out->Get(vtkDataObject::DATA_OBJECT());
    outData->CopyInformationToPipeline(request, in);
    out->CopyEntry(in, vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());

    if (!this->IterativeRequestInformation(in, out))
      {
      return 0;
      }

    in = out;
    }

  return 1;
}

//----------------------------------------------------------------------------
int
vtkImageIterateFilter
::RequestUpdateExtent(vtkInformation*,
                      vtkInformationVector**,
                      vtkInformationVector* outputVector)
{
  vtkInformation* out = outputVector->GetInformationObject(0);
  for(int i=this->NumberOfIterations-1; i >= 0; --i)
    {
    this->Iteration = i;

    vtkInformation* in = this->IterationData[i]->GetPipelineInformation();
    in->CopyEntry(out, vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());

    if (!this->IterativeRequestUpdateExtent(in, out))
      {
      return 0;
      }

    out = in;
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkImageIterateFilter::RequestData(vtkInformation* request,
                                        vtkInformationVector** inputVector,
                                        vtkInformationVector*)
{
  vtkInformation* in = inputVector[0]->GetInformationObject(0);
  for(int i=0; i < this->NumberOfIterations; ++i)
    {
    this->Iteration = i;

    vtkInformation* out = this->IterationData[i+1]->GetPipelineInformation();
    vtkDataObject* outData = out->Get(vtkDataObject::DATA_OBJECT());
    outData->CopyInformationFromPipeline(request);

    this->InputVector->SetInformationObject(0, in);
    this->OutputVector->SetInformationObject(0, out);
    if (!this->IterativeRequestData(request, &this->InputVector,
                                    this->OutputVector))
      {
      return 0;
      }

    if(in->Get(vtkDemandDrivenPipeline::RELEASE_DATA()))
      {
      vtkDataObject* inData = in->Get(vtkDataObject::DATA_OBJECT());
      inData->ReleaseData();
      }

    in = out;
    }
  this->InputVector->SetNumberOfInformationObjects(0);
  this->OutputVector->SetNumberOfInformationObjects(0);

  return 1;
}

//----------------------------------------------------------------------------
// Called by the above for each decomposition.  Subclass can modify
// the defaults by implementing this method.
int vtkImageIterateFilter::IterativeRequestInformation(vtkInformation*,
                                                       vtkInformation*)
{
  return 1;
}

//----------------------------------------------------------------------------
// Called by the above for each decomposition.  Subclass can modify
// the defaults by implementing this method.
int vtkImageIterateFilter::IterativeRequestUpdateExtent(vtkInformation*,
                                                        vtkInformation*)
{
  return 1;
}

//----------------------------------------------------------------------------
// Called by the above for each decomposition.  Subclass can modify
// the defaults by implementing this method.
int
vtkImageIterateFilter::IterativeRequestData(vtkInformation* request,
                                            vtkInformationVector** inputVector,
                                            vtkInformationVector* outputVector)
{
  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
// Filters that execute multiple times per update use this internal method.
void vtkImageIterateFilter::SetNumberOfIterations(int num)
{
  int idx;
  
  if (num == this->NumberOfIterations)
    {
    return;
    }
  
  // delete previous temporary caches 
  // (first and last are global input and output)
  if (this->IterationData)
    {
    for (idx = 1; idx < this->NumberOfIterations; ++idx)
      {
      this->IterationData[idx]->Delete();
      this->IterationData[idx] = NULL;
      }
    delete [] this->IterationData;
    this->IterationData = NULL;
    }

  // special case for destructor
  if (num == 0)
    {
    return;
    }
  
  // create new ones (first and last set later to input and output)
  this->IterationData =
    reinterpret_cast<vtkImageData **>( new void *[num + 1]);
  this->IterationData[0] = this->IterationData[num] = NULL;
  for (idx = 1; idx < num; ++idx)
    {
    this->IterationData[idx] = vtkImageData::New();
    this->IterationData[idx]->ReleaseDataFlagOn();
    this->IterationData[idx]->GetProducerPort();
    }

  this->NumberOfIterations = num;
  this->Modified();
}
