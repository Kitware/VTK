/*=========================================================================

  Program:   Visualization Library
  Module:    RefCount.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "RefCount.hh"

// Description:
// Construct with initial reference count = 0.
vlRefCount::vlRefCount()
{
  this->RefCount = 0;
}

vlRefCount::~vlRefCount() 
{
  if (this->RefCount > 0)
    {
    vlErrorMacro(<< "Trying to delete object with non-zero reference count");
    }
}

// Description:
// Increase the reference count (mark as used by another object).
void vlRefCount::Register(vlObject* o)
{
  this->RefCount++;
  vlDebugMacro(<< "Registered by " << o->GetClassName() << " (" << o << ")");
}

// Description:
// Decrease the reference count (release by another object).
void vlRefCount::UnRegister(vlObject* o)
{
  vlDebugMacro(<< "UnRegistered by " << o->GetClassName() << " (" << 0 << ")");

  if (--this->RefCount <= 0) delete this;
}

void vlRefCount::PrintSelf(ostream& os, vlIndent indent)
{
  vlObject::PrintSelf(os,indent);

  os << indent << "Reference Count: " << this->RefCount << "\n";
}
