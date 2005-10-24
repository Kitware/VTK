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

vtkCxxRevisionMacro(vtkTrivialProducer, "1.10");
vtkStandardNewMacro(vtkTrivialProducer);

// This compile-time switch determines whether the update extent is
// checked.  If so this algorithm will produce an error message when
// the update extent is smaller than the whole extent which will
// result in lost data.  There are real cases in which this is a valid
// thing so an error message should normally not be produced.  However
// there are hard-to-find bugs that can be revealed quickly if this
// option is enabled.  This should be enabled only for debugging
// purposes in a working checkout of VTK.  Do not commit a change that
// turns on this switch!
#define VTK_TRIVIAL_PRODUCER_CHECK_UPDATE_EXTENT 0

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
                                   vtkInformationVector** inputVector,
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
      }
    }
#if VTK_TRIVIAL_PRODUCER_CHECK_UPDATE_EXTENT
  if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
    {
    // If an exact extent smaller than the whole extent has been
    // requested then warn.
    vtkInformation* outputInfo = outputVector->GetInformationObject(0);
    if(outputInfo->Get(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT()))
      {
      vtkInformation* dataInfo = this->Output->GetInformation();
      if(dataInfo->Get(vtkDataObject::DATA_EXTENT_TYPE()) == VTK_3D_EXTENT)
        {
        // Compare the update extent to the whole extent.
        int updateExtent[6] = {0,-1,0,-1,0,-1};
        int wholeExtent[6] = {0,-1,0,-1,0,-1};
        outputInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
                        wholeExtent);
        outputInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
                        updateExtent);
        if(updateExtent[0] != wholeExtent[0] ||
           updateExtent[1] != wholeExtent[1] ||
           updateExtent[2] != wholeExtent[2] ||
           updateExtent[3] != wholeExtent[3] ||
           updateExtent[4] != wholeExtent[4] ||
           updateExtent[5] != wholeExtent[5])
          {
          vtkErrorMacro("Request for exact extent "
                        << updateExtent[0] << " " << updateExtent[1] << " "
                        << updateExtent[2] << " " << updateExtent[3] << " "
                        << updateExtent[4] << " " << updateExtent[5]
                        << " will lose data because it is not the whole extent "
                        << wholeExtent[0] << " " << wholeExtent[1] << " "
                        << wholeExtent[2] << " " << wholeExtent[3] << " "
                        << wholeExtent[4] << " " << wholeExtent[5] << ".");
          }
        }
      }
    }
#endif
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_NOT_GENERATED()))
    {
    // We do not really generate the output.  Do not let the executive
    // initialize it.
    vtkInformation* outputInfo = outputVector->GetInformationObject(0);
    outputInfo->Set(vtkDemandDrivenPipeline::DATA_NOT_GENERATED(), 1);
    }
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()) && this->Output)
    {
    // Pretend we generated the output.
    vtkInformation* outputInfo = outputVector->GetInformationObject(0);
    outputInfo->Remove(vtkDemandDrivenPipeline::DATA_NOT_GENERATED());
    }

  return this->Superclass::ProcessRequest(request,inputVector,outputVector);
}

//----------------------------------------------------------------------------
void vtkTrivialProducer::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  vtkGarbageCollectorReport(collector, this->Output, "Output");
}
