/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTrivialProducer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTrivialProducer.h"

#include "vtkImageData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkGarbageCollector.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkTrivialProducer, "1.4");
vtkStandardNewMacro(vtkTrivialProducer);

//----------------------------------------------------------------------------
vtkTrivialProducer::vtkTrivialProducer()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->Output = 0;
}

//----------------------------------------------------------------------------
vtkTrivialProducer::~vtkTrivialProducer()
{
  this->SetOutput(0);
}

//----------------------------------------------------------------------------
void vtkTrivialProducer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkTrivialProducer::SetOutput(vtkDataObject*newOutput)
{
  vtkDataObject* oldOutput = this->Output;
  if(newOutput != oldOutput)
    {
    if(newOutput)
      {
      newOutput->Register(this);
      }
    this->Output = newOutput;
    this->GetExecutive()->SetOutputData(0, newOutput);
    if(oldOutput)
      {
      oldOutput->UnRegister(this);
      }
    this->Modified();
    }
}

//----------------------------------------------------------------------------
unsigned long vtkTrivialProducer::GetMTime()
{
  unsigned long mtime = this->Superclass::GetMTime();
  if(this->Output)
    {
    unsigned long omtime = this->Output->GetMTime();
    if(omtime > mtime)
      {
      mtime = omtime;
      }
    }
  return mtime;
}

//----------------------------------------------------------------------------
vtkExecutive* vtkTrivialProducer::CreateDefaultExecutive()
{
  return vtkStreamingDemandDrivenPipeline::New();
}

//----------------------------------------------------------------------------
int vtkTrivialProducer::FillInputPortInformation(int, vtkInformation*)
{
  return 1;
}

//----------------------------------------------------------------------------
int vtkTrivialProducer::FillOutputPortInformation(int, vtkInformation*)
{
  return 1;
}

//----------------------------------------------------------------------------
int
vtkTrivialProducer::ProcessRequest(vtkInformation* request,
                                   vtkInformationVector**,
                                   vtkInformationVector* outputVector)
{
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()) &&
     this->Output)
    {
    vtkInformation* outputInfo = outputVector->GetInformationObject(0);
    vtkInformation* dataInfo = this->Output->GetInformation();
    if(dataInfo->Get(vtkDataObject::DATA_EXTENT_TYPE()) == VTK_PIECES_EXTENT)
      {
      // There is no real source to  change the output data, so we can
      // produce exactly one piece.
      outputInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(), 1);
      }
    else if(dataInfo->Get(vtkDataObject::DATA_EXTENT_TYPE()) == VTK_3D_EXTENT)
      {
      // The whole extent is just the extent because the output has no
      // real source to change its data.
      int extent[6];
      dataInfo->Get(vtkDataObject::DATA_EXTENT(), extent);
      outputInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
                      extent, 6);
      // for now we only do this for image data, once these ivars are in the
      // info object we can just do a has check
      vtkImageData *id = vtkImageData::SafeDownCast(this->Output);
      if (id)
        {
        outputInfo->Set(vtkDataObject::SCALAR_TYPE(),id->GetScalarType());
        outputInfo->Set(vtkDataObject::SCALAR_NUMBER_OF_COMPONENTS(),
                        id->GetNumberOfScalarComponents());
        }
      }
    }
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()) && this->Output)
    {
    this->Output->DataHasBeenGenerated();
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkTrivialProducer::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  collector->ReportReference(this->Output, "Output");
}

//----------------------------------------------------------------------------
void vtkTrivialProducer::RemoveReferences()
{
  if(this->Output)
    {
    this->Output->UnRegister(this);
    this->Output = 0;
    }
  this->Superclass::RemoveReferences();
}
