/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExporter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExporter.h"

#include "vtkRenderWindow.h"


vtkCxxSetObjectMacro(vtkExporter,RenderWindow,vtkRenderWindow);


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
  this->Superclass::PrintSelf(os,indent);

  if ( this->RenderWindow )
    {
    os << indent << "Render Window: (" <<
      static_cast<void *>(this->RenderWindow) << ")\n";
    }
  else
    {
    os << indent << "Render Window: (none)\n";
    }

  if ( this->StartWrite )
    {
    os << indent << "Start Write: (" <<
      static_cast<void (*)(void *)>(this->StartWrite) << ")\n";
    }
  else
    {
    os << indent << "Start Write: (none)\n";
    }

  if ( this->EndWrite )
    {
    os << indent << "End Write: (" <<
      static_cast<void (*)(void *)>(this->EndWrite) << ")\n";
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



