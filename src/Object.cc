/*=========================================================================

  Program:   Visualization Library
  Module:    Object.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/

#include "Object.hh"

vlObject::vlObject()
{
  this->RefCount = 0;
  this->Debug = 0;

  this->Modified(); // Insures modified time > than any other time
}

vlObject::~vlObject() 
{
  if (this->RefCount > 0)
    {
    cerr << this->GetClassName()
         <<": Trying to delete object with non-zero refCount\n\n";
    }
  if (this->Debug)
    {
    cerr << this->GetClassName() << " " << this <<", Destructing!\n\n";
    }
}

void vlObject::Register(void *p)
{
  this->RefCount++;
  if ( this->Debug )
    {
    cerr << this->GetClassName() << " " << this
         << ", Registered by " << (void *)p << "\n\n";
    }
}

void vlObject::UnRegister(void *p)
{
  if ( this->Debug )
    {
    cerr << this->GetClassName() << " " << this
         << ", UnRegistered by " << (void *)p << "\n\n";
    }
  if (--this->RefCount <= 0) delete this;
}

void vlObject::Print(ostream& os)
{
  vlIndent indent;

  this->PrintHeader(os,0); 
  this->PrintSelf(os, indent.GetNextIndent());
  this->PrintTrailer(os,0);
}

void vlObject::PrintHeader(ostream& os, vlIndent indent)
{
  os << indent << this->GetClassName() << " (" << this << ")\n";
}

void vlObject::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlObject::GetClassName()))
    {
    os << indent << "Debug state: " << this->Debug << "\n";
    os << indent << "Modified Time: " << this->GetMtime() << "\n";
    os << indent << "Reference Count: " << this->RefCount << "\n";
    }
}

void vlObject::PrintTrailer(ostream& os, vlIndent indent)
{
  os << indent << "\n";
}

void vlObject::DebugOn()
{
  this->Debug = 1;
}

void vlObject::DebugOff()
{
  this->Debug = 0;
}

int vlObject::GetDebug()
{
  return this->Debug;
}
