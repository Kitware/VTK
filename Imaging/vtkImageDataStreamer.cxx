/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDataStreamer.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageDataStreamer.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkCommand.h"

vtkCxxRevisionMacro(vtkImageDataStreamer, "1.26");
vtkStandardNewMacro(vtkImageDataStreamer);

//----------------------------------------------------------------------------
vtkImageDataStreamer::vtkImageDataStreamer()
{
  // default to 10 divisions
  this->NumberOfStreamDivisions = 10;

  // create default translator
  this->ExtentTranslator = vtkExtentTranslator::New();
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
  output->AllocateScalars();
  
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
      this->UpdateProgress((float)piece/(this->NumberOfStreamDivisions - 1.0));
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

  




