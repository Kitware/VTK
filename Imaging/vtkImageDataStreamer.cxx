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

vtkCxxRevisionMacro(vtkImageDataStreamer, "1.32");
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

  this->Information->Set(
    vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(),0);
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
int vtkImageDataStreamer::FillOutputPortInformation(
  int port, vtkInformation* info)
{
  // invoke super first
  int retVal = this->Superclass::FillOutputPortInformation(port, info);
  
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
  
  return retVal;
}

int vtkImageDataStreamer::FillInputPortInformation(
  int port, vtkInformation* info)
{
  // invoke super first
  int retVal = this->Superclass::FillInputPortInformation(port, info);
  
  // now add our info
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  
  return retVal;
}

//----------------------------------------------------------------------------
int vtkImageDataStreamer::ProcessRequest(vtkInformation* request,
                                         vtkInformationVector* inputVector,
                                         vtkInformationVector* outputVector)
{
  // this is basically execute information
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
    vtkDebugMacro("ProcessRequest(REQUEST_INFORMATION) "
                  "calling ExecuteInformation.");

    // Ask the subclass to fill in the information for the outputs.
    this->InvokeEvent(vtkCommand::ExecuteInformationEvent, NULL);

    // information, we just need to change any that should be different from
    // the input
    vtkInformation* outInfo = outputVector->GetInformationObject(0);

    // make sure the output is there
    vtkDataObject *output = 
      vtkImageData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
    
    if (!output)
      {
      output = vtkImageData::New();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
      output->Delete();
      }
    return 1;
    }

  else if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
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
    
    inputVector->GetInformationObject(0)
      ->Get(vtkAlgorithm::INPUT_CONNECTION_INFORMATION())
      ->GetInformationObject(0)
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
      output->PrepareForNewData();
      this->AbortExecute = 0;
      this->Progress = 0.0;
      this->InvokeEvent(vtkCommand::StartEvent,NULL);
      // tell the pipeline to loop
      this->Information->Set(
        vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(),1);
      int outUpExt[6];
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
                   outUpExt);
      output->SetUpdateExtent(outUpExt);
      this->AllocateOutputData(output);
      }

    // actually copy the data
    vtkInformation* inInfo = inputVector->GetInformationObject(0)
      ->Get(vtkAlgorithm::INPUT_CONNECTION_INFORMATION())
      ->GetInformationObject(0);
    vtkImageData *input = 
      vtkImageData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

    int inExt[6];
    inInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inExt);
    output->CopyAndCastFrom(input, inExt);    

    // update the progress
    this->UpdateProgress(
      (float)(this->CurrentDivision+1.0)/(float)this->NumberOfStreamDivisions);
    
    this->CurrentDivision++;
    if (this->CurrentDivision == this->NumberOfStreamDivisions)
      {
      if(!this->AbortExecute)
        {
        this->UpdateProgress(1.0);
        }
      this->InvokeEvent(vtkCommand::EndEvent,NULL);

      // Mark the data as up-to-date.
      output->DataHasBeenGenerated();
      this->Information->Set(
        vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(),0);
      this->CurrentDivision = 0;
      }
    
    return 1;
    }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}
