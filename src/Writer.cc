/*=========================================================================

  Program:   Visualization Library
  Module:    Writer.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "Writer.hh"

void vlWriter::SetStartWrite(void (*f)())
{
  if ( f != this->StartWrite )
    {
    this->StartWrite = f;
    this->Modified();
    }
}

void vlWriter::SetEndWrite(void (*f)())
{
  if ( f != this->EndWrite )
    {
    this->EndWrite = f;
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
