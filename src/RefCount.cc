/*=========================================================================

  Program:   Visualization Toolkit
  Module:    RefCount.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "RefCount.hh"

// Description:
// Construct with initial reference count = 0.
vtkRefCount::vtkRefCount()
{
  this->RefCount = 0;
}

vtkRefCount::~vtkRefCount() 
{
  if (this->RefCount > 0)
    {
    vtkErrorMacro(<< "Trying to delete object with non-zero reference count");
    }
}

// Description:
// Increase the reference count (mark as used by another object).
void vtkRefCount::Register(vtkObject* o)
{
  this->RefCount++;
  vtkDebugMacro(<< "Registered by " << o->GetClassName() << " (" << o << ")");
}

// Description:
// Decrease the reference count (release by another object).
void vtkRefCount::UnRegister(vtkObject* o)
{
  vtkDebugMacro(<< "UnRegistered by " << o->GetClassName() << " (" << 0 << ")");

  if (--this->RefCount <= 0) delete this;
}

void vtkRefCount::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  os << indent << "Reference Count: " << this->RefCount << "\n";
}
