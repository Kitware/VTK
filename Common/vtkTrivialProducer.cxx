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

#include "vtkDataObject.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkGarbageCollector.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStructuredGrid.h"
#include "vtkRectilinearGrid.h"
#include "vtkImageData.h"

vtkCxxRevisionMacro(vtkTrivialProducer, "1.2");
vtkStandardNewMacro(vtkTrivialProducer);

//----------------------------------------------------------------------------
vtkTrivialProducer::vtkTrivialProducer()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->Output = 0;
  this->DebugOn();
}

//----------------------------------------------------------------------------
vtkTrivialProducer::~vtkTrivialProducer()
{
  this->SetOutput(0);
}

//----------------------------------------------------------------------------
void vtkTrivialProducer::SetOutput(vtkDataObject*
#ifdef VTK_USE_EXECUTIVES
                                   newOutput
#endif
  )
{
#ifdef VTK_USE_EXECUTIVES
  vtkDataObject* oldOutput = this->Output;
  if(newOutput != oldOutput)
    {
    this->Output = newOutput;
    if(newOutput)
      {
      newOutput->Register(this);
      newOutput->SetProducerPort(this->GetOutputPort(0));
      }
    if(oldOutput)
      {
      oldOutput->SetProducerPort(0);
      oldOutput->UnRegister(this);
      }
    this->Modified();
    }
#endif
}

//----------------------------------------------------------------------------
void vtkTrivialProducer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
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
int vtkTrivialProducer::ProcessUpstreamRequest(vtkInformation*,
                                               vtkInformationVector*,
                                               vtkInformationVector*)
{
  return 1;
}

//----------------------------------------------------------------------------
int
vtkTrivialProducer::ProcessDownstreamRequest(vtkInformation* request,
                                             vtkInformationVector*,
                                             vtkInformationVector* outputVector)
{
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()) ||
     request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
    {
    vtkInformation* outputInfo = outputVector->GetInformationObject(0);
    outputInfo->Set(vtkInformation::DATA_OBJECT(), this->Output);
    }
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()) &&
     this->Output)
    {
    vtkInformation* outputInfo = outputVector->GetInformationObject(0);

    // The whole extent is just the extent because the output has no
    // real source to change its data.
    if(vtkStructuredGrid* sg = vtkStructuredGrid::SafeDownCast(this->Output))
      {
      int extent[6];
      sg->GetExtent(extent);
      outputInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
                      extent, 6);
      }
    else if(vtkImageData* id = vtkImageData::SafeDownCast(this->Output))
      {
      int extent[6];
      id->GetExtent(extent);
      outputInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
                      extent, 6);
      }
    else if(vtkRectilinearGrid* rg = vtkRectilinearGrid::SafeDownCast(this->Output))
      {
      int extent[6];
      rg->GetExtent(extent);
      outputInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
                      extent, 6);
      }
    else if(this->Output->GetExtentType() == VTK_PIECES_EXTENT)
      {
      outputInfo->Set(
        vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(), 1);
      }
    }
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()) && this->Output)
    {
    this->Output->DataHasBeenGenerated();
    this->Output->Print(cout);
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
  this->SetOutput(0);
  this->Superclass::RemoveReferences();
}
