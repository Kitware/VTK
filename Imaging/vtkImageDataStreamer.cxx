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

vtkCxxRevisionMacro(vtkImageDataStreamer, "1.31.2.1");
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
#ifndef VTK_USE_EXECUTIVES
void vtkImageDataStreamer::UpdateData(vtkDataObject *vtkNotUsed(out))
{
  int idx;
  vtkImageData *input = this->GetInput();
  vtkImageData *output = this->GetOutput();
  int piece;
  
  // prevent chasing our tail
  if (this->Updating)
    {
    return;
    }
  
  // Propagate the update call - make sure everything we
  // might rely on is up-to-date
  // Must call PropagateUpdateExtent before UpdateData if multiple 
  // inputs since they may lead back to the same data object.
  this->Updating = 1;

  if (!input || !output)
    {
    vtkWarningMacro("ImageDataStreamer Requires an input to execute!");
    return;
    }
  
  // Initialize all the outputs
  vtkExtentTranslator *translator = this->GetExtentTranslator();
  output->PrepareForNewData(); 

  // If there is a start method, call it
  this->AbortExecute = 0;
  this->Progress = 0.0;
  this->InvokeEvent(vtkCommand::StartEvent,NULL);
  output->SetExtent(output->GetUpdateExtent());
  //output->AllocateScalars();
  AllocateOutputData(output);
  
  // now start the loop over the number of pieces
  translator->SetWholeExtent(output->GetUpdateExtent());
  translator->SetNumberOfPieces(this->NumberOfStreamDivisions);
  for (piece = 0; 
       piece < this->NumberOfStreamDivisions && !this->AbortExecute; 
       piece++)
    {
    translator->SetPiece(piece);
    if (translator->PieceToExtentByPoints())
      {
      input->SetUpdateExtent(translator->GetExtent());
      input->PropagateUpdateExtent();
      input->UpdateData();
      // copy the resulting data into the output buffer
      output->CopyAndCastFrom(input, translator->GetExtent());    
      this->UpdateProgress((float)(piece+1.0)/(float)this->NumberOfStreamDivisions);
      }
    }
  
  this->Updating = 0;  
  
  // If we ended due to aborting, push the progress up to 1.0 (since
  // it probably didn't end there)
  if ( !this->AbortExecute )
    {
    this->UpdateProgress(1.0);
    }

  // Call the end method, if there is one
  this->InvokeEvent(vtkCommand::EndEvent,NULL);
    
  // Now we have to mark the data as up to data.
  for (idx = 0; idx < this->NumberOfOutputs; ++idx)
    {
    if (this->Outputs[idx])
      {
      this->Outputs[idx]->DataHasBeenGenerated();
      }
    }
  
  // Release any inputs if marked for release
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    if (this->Inputs[idx] != NULL)
      {
      if ( this->Inputs[idx]->ShouldIReleaseData() )
        {
        this->Inputs[idx]->ReleaseData();
        }
      }  
    }
  
  // Information gets invalidated as soon as Update is called,
  // so validate it again here.
  this->InformationTime.Modified();
}
#endif

  
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

int vtkImageDataStreamer::ProcessUpstreamRequest(
  vtkInformation *request, 
  vtkInformationVector *inputVector, 
  vtkInformationVector *outputVector)
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
    
    inputVector->GetInformationObject(0)
      ->Get(vtkAlgorithm::INPUT_CONNECTION_INFORMATION())
      ->GetInformationObject(0)
      ->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inExt, 6);
    
    return 1;
    }
  return this->Superclass::ProcessUpstreamRequest(request, inputVector,
                                                  outputVector);
}

int vtkImageDataStreamer::ProcessDownstreamRequest(
  vtkInformation *request, 
  vtkInformationVector *inputVector, 
  vtkInformationVector *outputVector)
{
#ifdef VTK_USE_EXECUTIVES

  // this is basically execute information
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
    vtkDebugMacro("ProcessDownstreamRequest(REQUEST_INFORMATION) "
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
  return 0;
#else
  return this->Superclass::ProcessDownstreamRequest(request, inputVector,
                                                    outputVector);
#endif
}


