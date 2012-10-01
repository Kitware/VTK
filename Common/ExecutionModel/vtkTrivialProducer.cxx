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
#include "vtkDataObject.h"

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
int vtkTrivialProducer::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
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
      outputInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(), -1);
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

    if (this->Output->IsA("vtkImageData"))
      {
      vtkImageData* img = static_cast<vtkImageData*>(this->Output);

      double spacing[3];
      img->GetSpacing(spacing);
      outputInfo->Set(vtkDataObject::SPACING(), spacing[0], spacing[1], spacing[2]);

      double origin[3];
      img->GetOrigin(origin);
      outputInfo->Set(vtkDataObject::ORIGIN(), origin[0], origin[1], origin[2]);

      vtkDataObject::SetPointDataActiveScalarInfo(outputInfo,
                                                  img->GetScalarType(),
                                                  img->GetNumberOfScalarComponents());

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
    vtkInformation* outputInfo = outputVector->GetInformationObject(0);

    // If downstream wants exact structured extent that is less than
    // whole, we need make a copy of the original dataset and crop it
    // - if EXACT_EXTENT() is specified.
    vtkInformation* dataInfo = this->Output->GetInformation();
    if(dataInfo->Get(vtkDataObject::DATA_EXTENT_TYPE()) == VTK_3D_EXTENT)
      {
      int wholeExt[6];
      outputInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
                      wholeExt);
      int updateExt[6];
      outputInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
                      updateExt);

      if(outputInfo->Has(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT()) &&
         outputInfo->Get(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT()))
        {

        if(updateExt[0] != wholeExt[0] ||
           updateExt[1] != wholeExt[1] ||
           updateExt[2] != wholeExt[2] ||
           updateExt[3] != wholeExt[3] ||
           updateExt[4] != wholeExt[4] ||
           updateExt[5] != wholeExt[5])
          {
          vtkDataObject* newOutput = this->Output->NewInstance();
          newOutput->ShallowCopy(this->Output);
          newOutput->Crop(
            outputInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()));
          outputInfo->Set(vtkDataObject::DATA_OBJECT(), newOutput);
          newOutput->Delete();
          }
        else
          {
          // If we didn't replace the output, it should be same as original
          // dataset. If not, fix it.
          vtkDataObject* output = outputInfo->Get(vtkDataObject::DATA_OBJECT());
          if (output != this->Output)
            {
            outputInfo->Set(vtkDataObject::DATA_OBJECT(), this->Output);
            }
          }
        }
      else
        {
        // If EXACT_EXTENT() is not there,
        // make sure that we provide requested extent or more
        int dataExt[6];
        vtkDataObject* output = outputInfo->Get(vtkDataObject::DATA_OBJECT());
        output->GetInformation()->Get(vtkDataObject::DATA_EXTENT(), dataExt);
        if (updateExt[0] < dataExt[0] ||
            updateExt[1] > dataExt[1] ||
            updateExt[2] < dataExt[2] ||
            updateExt[3] > dataExt[3] ||
            updateExt[4] < dataExt[4] ||
            updateExt[5] > dataExt[5])
          {
          if (output != this->Output)
            {
            outputInfo->Set(vtkDataObject::DATA_OBJECT(), this->Output);
            }
          else
            {
            vtkErrorMacro("This data object does not contain the requested extent.");
            }
          }
        }
      }

    // Pretend we generated the output.
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
