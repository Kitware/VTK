/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Writer.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "Writer.hh"

// Description:
// Construct with no start and end write methods or arguments.
vtkWriter::vtkWriter()
{
  this->Input = NULL;
  this->StartWrite = NULL;
  this->StartWriteArgDelete = NULL;
  this->StartWriteArg = NULL;
  this->EndWrite = NULL;
  this->EndWriteArgDelete = NULL;
  this->EndWriteArg = NULL;
}

// Description:
// Write data to output. Method executes subclasses WriteData() method, as 
// well as StartWrite() and EndWrite() methods.
void vtkWriter::Write()
{
  // make sure input is available
  if ( !this->Input )
    {
    vtkErrorMacro(<< "No input!");
    return;
    }

  this->Input->Update();

  if ( this->StartWrite ) (*this->StartWrite)(this->StartWriteArg);
  this->WriteData();
  if ( this->EndWrite ) (*this->EndWrite)(this->EndWriteArg);

  if ( this->Input->ShouldIReleaseData() ) this->Input->ReleaseData();
}

// Description:
// Convenient alias for Write() method.
void vtkWriter::Update()
{
  this->Write();
}

// Description:
// Specify a function to be called before data is written.
// Function will be called with argument provided.
void vtkWriter::SetStartWrite(void (*f)(void *), void *arg)
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


// Description:
// Set the arg delete method. This is used to free user memory.
void vtkWriter::SetStartWriteArgDelete(void (*f)(void *))
{
  if ( f != this->StartWriteArgDelete)
    {
    this->StartWriteArgDelete = f;
    this->Modified();
    }
}

// Description:
// Set the arg delete method. This is used to free user memory.
void vtkWriter::SetEndWriteArgDelete(void (*f)(void *))
{
  if ( f != this->EndWriteArgDelete)
    {
    this->EndWriteArgDelete = f;
    this->Modified();
    }
}

// Description:
// Specify a function to be called after data is written.
// Function will be called with argument provided.
void vtkWriter::SetEndWrite(void (*f)(void *), void *arg)
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

void vtkWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  if ( this->Input )
    {
    os << indent << "Input: (" << (void *)this->Input << ")\n";
    }
  else
    {
    os << indent << "Input: (none)\n";
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
