/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExporter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


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

  if ( this->StartWrite ) (*this->StartWrite)(this->StartWriteArg);
  this->WriteData();
  if ( this->EndWrite ) (*this->EndWrite)(this->EndWriteArg);
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



