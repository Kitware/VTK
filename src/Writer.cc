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
// Specify a function to be called before data is written.
// Function will be called with argument provided.
void vlWriter::SetStartWrite(void (*f)(void *), void *arg)
{
  if ( f != this->StartWrite )
    {
    this->StartWrite = f;
    this->StartWriteArg = arg;
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
    this->EndWrite = f;
    this->EndWriteArg = arg;
    this->Modified();
    }
}

void vlWriter::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlWriter::GetClassName()))
    {
    vlObject::PrintSelf(os,indent);

    if ( this->StartWrite )
      {
      os << indent << "Start Write: (" << this->StartWrite << ")\n";
      }
    else
      {
      os << indent << "Start Write: (none)\n";
      }

    if ( this->EndWrite )
      {
      os << indent << "End Write: (" << this->EndWrite << ")\n";
      }
    else
      {
      os << indent << "End Write: (none)\n";
      }
   }
}
