/*=========================================================================

  Program:   Visualization Library
  Module:    Writer.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "Writer.hh"

// Description:
// Construct with no start and end write methods or arguments.
vlWriter::vlWriter()
{
  this->StartWrite = NULL;
  this->StartWriteArgDelete = NULL;
  this->StartWriteArg = NULL;
  this->EndWrite = NULL;
  this->EndWriteArgDelete = NULL;
  this->EndWriteArg = NULL;
}

// Description:
// Specify a function to be called before data is written.
// Function will be called with argument provided.
void vlWriter::SetStartWrite(void (*f)(void *), void *arg)
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
void vlWriter::SetStartWriteArgDelete(void (*f)(void *))
{
  if ( f != this->StartWriteArgDelete)
    {
    this->StartWriteArgDelete = f;
    this->Modified();
    }
}

// Description:
// Set the arg delete method. This is used to free user memory.
void vlWriter::SetEndWriteArgDelete(void (*f)(void *))
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
void vlWriter::SetEndWrite(void (*f)(void *), void *arg)
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

void vlWriter::PrintSelf(ostream& os, vlIndent indent)
{
  vlObject::PrintSelf(os,indent);

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
