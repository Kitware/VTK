/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExporter.cxx
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
#include "vtkExporter.h"

// Construct with no start and end write methods or arguments.
vtkExporter::vtkExporter()
{
  this->RenderWindow = NULL;
  this->StartWrite = NULL;
  this->StartWriteArgDelete = NULL;
  this->StartWriteArg = NULL;
  this->EndWrite = NULL;
  this->EndWriteArgDelete = NULL;
  this->EndWriteArg = NULL;
}

vtkExporter::~vtkExporter()
{
  this->SetRenderWindow(NULL);
  
  if ((this->StartWriteArg)&&(this->StartWriteArgDelete))
    {
    (*this->StartWriteArgDelete)(this->StartWriteArg);
    }
  if ((this->EndWriteArg)&&(this->EndWriteArgDelete))
    {
    (*this->EndWriteArgDelete)(this->EndWriteArg);
    }
}


// Write data to output. Method executes subclasses WriteData() method, as 
// well as StartWrite() and EndWrite() methods.
void vtkExporter::Write()
{
  // make sure input is available
  if ( !this->RenderWindow )
    {
    vtkErrorMacro(<< "No render window provided!");
    return;
    }

  if ( this->StartWrite )
    {
    (*this->StartWrite)(this->StartWriteArg);
    }
  this->WriteData();
  if ( this->EndWrite )
    {
    (*this->EndWrite)(this->EndWriteArg);
    }
}

// Convenient alias for Write() method.
void vtkExporter::Update()
{
  this->Write();
}

// Specify a function to be called before data is written.
// Function will be called with argument provided.
void vtkExporter::SetStartWrite(void (*f)(void *), void *arg)
{
  if ( f != this->StartWrite )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->StartWriteArg)&&(this->StartWriteArgDelete))
      {
      (*this->StartWriteArgDelete)(this->StartWriteArg);
      }
    this->StartWrite = f;
    this->StartWriteArg = arg;
    this->Modified();
    }
}


// Set the arg delete method. This is used to free user memory.
void vtkExporter::SetStartWriteArgDelete(void (*f)(void *))
{
  if ( f != this->StartWriteArgDelete)
    {
    this->StartWriteArgDelete = f;
    this->Modified();
    }
}

// Set the arg delete method. This is used to free user memory.
void vtkExporter::SetEndWriteArgDelete(void (*f)(void *))
{
  if ( f != this->EndWriteArgDelete)
    {
    this->EndWriteArgDelete = f;
    this->Modified();
    }
}

// Specify a function to be called after data is written.
// Function will be called with argument provided.
void vtkExporter::SetEndWrite(void (*f)(void *), void *arg)
{
  if ( f != this->EndWrite )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->EndWriteArg)&&(this->EndWriteArgDelete))
      {
      (*this->EndWriteArgDelete)(this->EndWriteArg);
      }
    this->EndWrite = f;
    this->EndWriteArg = arg;
    this->Modified();
    }
}

void vtkExporter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  if ( this->RenderWindow )
    {
    os << indent << "Render Window: (" << (void *)this->RenderWindow << ")\n";
    }
  else
    {
    os << indent << "Render Window: (none)\n";
    }

  if ( this->StartWrite )
    {
    os << indent << "Start Write: (" << (void *)this->StartWrite << ")\n";
    }
  else
    {
    os << indent << "Start Write: (none)\n";
    }

  if ( this->EndWrite )
    {
    os << indent << "End Write: (" << (void *)this->EndWrite << ")\n";
    }
  else
    {
    os << indent << "End Write: (none)\n";
    }
}

unsigned long int vtkExporter::GetMTime()
{
  unsigned long mTime=this-> vtkObject::GetMTime();
  unsigned long time;

  if ( this->RenderWindow != NULL )
    {
    time = this->RenderWindow->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }
  return mTime;
}



