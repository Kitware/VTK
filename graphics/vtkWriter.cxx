/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWriter.cxx
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
#include "vtkWriter.h"
#include "vtkCommand.h"

// Construct with no start and end write methods or arguments.
vtkWriter::vtkWriter()
{
}

vtkWriter::~vtkWriter()
{
}

vtkDataObject *vtkWriter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  return this->Inputs[0];
}


// Write data to output. Method executes subclasses WriteData() method, as 
// well as StartMethod() and EndMethod() methods.
void vtkWriter::Write()
{
  vtkDataObject *input = this->GetInput();

  // make sure input is available
  if ( !input )
    {
    vtkErrorMacro(<< "No input!");
    return;
    }

  //
  input->Update();
  //
  if (input->GetUpdateTime()<this->WriteTime)
  {
    // we are up to date
    return;
  }
  //
  this->InvokeEvent(vtkCommand::StartEvent,NULL);
  this->WriteData();
  this->InvokeEvent(vtkCommand::EndEvent,NULL);

  if ( input->ShouldIReleaseData() )
    {
    input->ReleaseData();
    }
  this->WriteTime.Modified();
}

// Convenient alias for Write() method.
void vtkWriter::Update()
{
  this->Write();
}

void vtkWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkProcessObject::PrintSelf(os,indent);

}
