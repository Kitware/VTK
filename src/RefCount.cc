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
// Construct with initial reference count = 1 and reference counting on.
vtkRefCount::vtkRefCount()
{
  this->RefCount = 1;
  this->ReferenceCounting = 1;
}

// Description:
// Overload vtkObject's Delete() method. For reference counted objects the
// Delete() method simply unregisters the use of the object. This may or
// may not result in the destruction of the object depending upon whether 
// another object is referencing it.
void vtkRefCount::Delete()
{
  this->UnRegister((vtkObject *)NULL);
}

// Description:
// Destructor for reference counted objects. Reference counted objects should 
// almost always use the combination of new/Delete() to create and delete 
// objects. Automatic reference counted objects (i.e., creating them on the 
// stack) are not encouraged. However, if you desire to this, you will have to
// use the ReferenceCountingOff() method to avoid warning messages when the
// objects are automatically deleted upon scope termination.
vtkRefCount::~vtkRefCount() 
{
  // warn user if reference counting is on and the object is being referenced
  // by another object
  if ( this->RefCount > 0 && this->ReferenceCounting )
    {
    vtkErrorMacro(<< "Trying to delete object with non-zero reference count");
    }
}

// Description:
// Increase the reference count (mark as used by another object).
void vtkRefCount::Register(vtkObject* o)
{
  if ( this->ReferenceCounting == 0 )
    {
    vtkErrorMacro(<<"Attempting to Register an object which has reference counting turned off.");
    }
  this->RefCount++;
  vtkDebugMacro(<< "Registered by " << o->GetClassName() << " (" << o << ")");
}

// Description:
// Decrease the reference count (release by another object).
void vtkRefCount::UnRegister(vtkObject* o)
{
  if ( this->ReferenceCounting == 0 )
    {
    vtkErrorMacro(<<"Attempting to UnRegister an object which has reference counting turned off.");
    }

  vtkDebugMacro(<< "UnRegistered by " <<o->GetClassName() << " (" << 0 << ")");

  if (--this->RefCount <= 0) delete this;
}

void vtkRefCount::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  os << indent << "Reference Count: " << this->RefCount << "\n";
  os << indent << "Reference Counting: "<< (this->ReferenceCounting ? "On\n" : "Off\n");
}
