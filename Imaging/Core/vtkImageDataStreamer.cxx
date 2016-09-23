/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDataStreamer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageDataStreamer.h"

#include "vtkCommand.h"
#include "vtkExtentTranslator.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkImageDataStreamer);
vtkCxxSetObjectMacro(vtkImageDataStreamer,ExtentTranslator,vtkExtentTranslator);

//----------------------------------------------------------------------------
vtkImageDataStreamer::vtkImageDataStreamer()
{
  // default to 10 divisions
  this->NumberOfStreamDivisions = 10;
  this->CurrentDivision = 0;

  // create default translator
  this->ExtentTranslator = vtkExtentTranslator::New();

  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

vtkImageDataStreamer::~vtkImageDataStreamer()
{
  if (this->ExtentTranslator)
  {
    this->ExtentTranslator->Delete();
  }
}

//----------------------------------------------------------------------------
void vtkImageDataStreamer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "NumberOfStreamDivisions: " << this->NumberOfStreamDivisions << endl;
  if ( this->ExtentTranslator )
  {
    os << indent << "ExtentTranslator:\n";
    this->ExtentTranslator->PrintSelf(os,indent.GetNextIndent());
  }
  else
  {
    os << indent << "ExtentTranslator: (none)\n";
  }
}

//----------------------------------------------------------------------------
int vtkImageDataStreamer::ProcessRequest(vtkInformation* request,
                                         vtkInformationVector** inputVector,
                                         vtkInformationVector* outputVector)
{
  if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    // we must set the extent on the input
    vtkInformation* outInfo = outputVector->GetInformationObject(0);

    // get the requested update extent
    int outExt[6];
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), outExt);

    // setup the inputs update extent
    int inExt[6] = {0, -1, 0, -1, 0, -1};
    vtkExtentTranslator *translator = this->GetExtentTranslator();

    translator->SetWholeExtent(outExt);
    translator->SetNumberOfPieces(this->NumberOfStreamDivisions);
    translator->SetPiece(this->CurrentDivision);
    if (translator->PieceToExtentByPoints())
    {
      translator->GetExtent(inExt);
    }

    inputVector[0]->GetInformationObject(0)
      ->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inExt, 6);

    return 1;
  }

  // generate the data
  else if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    // get the output data object
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    vtkImageData *output =
      vtkImageData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));


    // is this the first request
    if (!this->CurrentDivision)
    {
      // Tell the pipeline to start looping.
      request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
      this->AllocateOutputData(output, outInfo);
    }

    // actually copy the data
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    vtkImageData *input =
      vtkImageData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

    int inExt[6];
    inInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inExt);

    output->CopyAndCastFrom(input, inExt);

    // update the progress
    this->UpdateProgress(
      static_cast<float>(this->CurrentDivision+1.0)
      /static_cast<float>(this->NumberOfStreamDivisions));

    this->CurrentDivision++;
    if (this->CurrentDivision == this->NumberOfStreamDivisions)
    {
      // Tell the pipeline to stop looping.
      request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
      this->CurrentDivision = 0;
    }

    return 1;
  }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}
