/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDataStreamer.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "vtkImageDataStreamer.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkCommand.h"


//----------------------------------------------------------------------------
vtkImageDataStreamer* vtkImageDataStreamer::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageDataStreamer");
  if(ret)
    {
    return (vtkImageDataStreamer*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageDataStreamer;
}


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
  vtkImageToImageFilter::PrintSelf(os,indent);

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

  




