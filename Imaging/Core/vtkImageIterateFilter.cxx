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

#include "vtkDataSetAttributes.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTrivialProducer.h"


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
int
vtkImageIterateFilter
::RequestInformation(vtkInformation* vtkNotUsed(request),
                     vtkInformationVector** inputVector,
                     vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkInformation* in = inInfo;
  for(int i=0; i < this->NumberOfIterations; ++i)
  {
    this->Iteration = i;

    int next = i + 1;
    vtkInformation* out;
    if (next == this->NumberOfIterations)
    {
      out = outInfo;
    }
    else
    {
      out = this->IterationData[next]->GetOutputInformation(0);
    }
    out->CopyEntry(in, vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());

    out->CopyEntry(in, vtkDataObject::ORIGIN());
    out->CopyEntry(in, vtkDataObject::SPACING());

    vtkInformation* scalarInfo =
      vtkDataObject::GetActiveFieldInformation(
        in,
        vtkDataObject::FIELD_ASSOCIATION_POINTS,
        vtkDataSetAttributes::SCALARS);
    if (scalarInfo)
    {
      int scalarType = VTK_DOUBLE;
      if (scalarInfo->Has(vtkDataObject::FIELD_ARRAY_TYPE()))
      {
        scalarType = scalarInfo->Get(vtkDataObject::FIELD_ARRAY_TYPE());
      }
      int numComp = 1;
      if (scalarInfo->Has(vtkDataObject::FIELD_NUMBER_OF_COMPONENTS()))
      {
        numComp = scalarInfo->Get(vtkDataObject::FIELD_NUMBER_OF_COMPONENTS());
      }
      vtkDataObject::SetPointDataActiveScalarInfo(
        out, scalarType, numComp);
    }

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
                      vtkInformationVector** inputVector,
                      vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* out = outputVector->GetInformationObject(0);
  for(int i=this->NumberOfIterations-1; i >= 0; --i)
  {
    this->Iteration = i;

    vtkInformation* in;
    if (i == 0)
    {
      in = inInfo;
    }
    else
    {
      in = this->IterationData[i]->GetOutputInformation(0);
    }
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
                                        vtkInformationVector* outputVector)
{
  vtkInformation* in = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  for(int i=0; i < this->NumberOfIterations; ++i)
  {
    this->Iteration = i;

    int next = i + 1;
    vtkInformation* out;
    if (next == this->NumberOfIterations)
    {
      out = outInfo;
    }
    else
    {
      out = this->IterationData[next]->GetOutputInformation(0);
    }

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
    reinterpret_cast<vtkAlgorithm **>( new void *[num + 1]);
  this->IterationData[0] = this->IterationData[num] = NULL;
  for (idx = 1; idx < num; ++idx)
  {
    vtkImageData* cache = vtkImageData::New();
    vtkTrivialProducer* tp = vtkTrivialProducer::New();
    tp->ReleaseDataFlagOn();
    tp->SetOutput(cache);
    this->IterationData[idx] = tp;
    cache->Delete();
  }

  this->NumberOfIterations = num;
  this->Modified();
}
