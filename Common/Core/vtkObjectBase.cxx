/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkObjectBase.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkObjectBase.h"
#include "vtkDebugLeaks.h"
#include "vtkGarbageCollector.h"
#include "vtkWeakPointerBase.h"

#include <sstream>

#define vtkBaseDebugMacro(x)

class vtkObjectBaseToGarbageCollectorFriendship
{
public:
  static int GiveReference(vtkObjectBase* obj)
  {
    return vtkGarbageCollector::GiveReference(obj);
  }
  static int TakeReference(vtkObjectBase* obj)
  {
    return vtkGarbageCollector::TakeReference(obj);
  }
};

class vtkObjectBaseToWeakPointerBaseFriendship
{
public:
  static void ClearPointer(vtkWeakPointerBase *p)
  {
    p->Object = NULL;
  }
};

// avoid dll boundary problems
#ifdef _WIN32
void* vtkObjectBase::operator new(size_t nSize)
{
  void* p=malloc(nSize);
  return p;
}

void vtkObjectBase::operator delete( void *p )
{
  free(p);
}
#endif

// ------------------------------------vtkObjectBase----------------------
// This operator allows all subclasses of vtkObjectBase to be printed via <<.
// It in turn invokes the Print method, which in turn will invoke the
// PrintSelf method that all objects should define, if they have anything
// interesting to print out.
ostream& operator<<(ostream& os, vtkObjectBase& o)
{
  o.Print(os);
  return os;
}

// Create an object with Debug turned off and modified time initialized
// to zero.
vtkObjectBase::vtkObjectBase()
{
  this->ReferenceCount = 1;
  this->WeakPointers = 0;
#ifdef VTK_DEBUG_LEAKS
  vtkDebugLeaks::ConstructingObject(this);
#endif
}

vtkObjectBase::~vtkObjectBase()
{
#ifdef VTK_DEBUG_LEAKS
  vtkDebugLeaks::DestructingObject(this);
#endif

  // warn user if reference counting is on and the object is being referenced
  // by another object
  if ( this->ReferenceCount > 0)
  {
    vtkGenericWarningMacro(<< "Trying to delete object with non-zero reference count.");
  }
}

//----------------------------------------------------------------------------
void vtkObjectBase::InitializeObjectBase()
{
#ifdef VTK_DEBUG_LEAKS
  vtkDebugLeaks::ConstructClass(this->GetClassName());
#endif // VTK_DEBUG_LEAKS
}

//----------------------------------------------------------------------------
#ifdef VTK_WORKAROUND_WINDOWS_MANGLE
# undef GetClassName
// Define possible mangled names.
const char* vtkObjectBase::GetClassNameA() const
{
  return this->GetClassNameInternal();
}
const char* vtkObjectBase::GetClassNameW() const
{
  return this->GetClassNameInternal();
}
#endif
const char* vtkObjectBase::GetClassName() const
{
  return this->GetClassNameInternal();
}

vtkTypeBool vtkObjectBase::IsTypeOf(const char *name)
{
  if ( !strcmp("vtkObjectBase",name) )
  {
    return 1;
  }
  return 0;
}

vtkTypeBool vtkObjectBase::IsA(const char *type)
{
  return this->vtkObjectBase::IsTypeOf(type);
}

// Delete a vtk object. This method should always be used to delete an object
// when the new operator was used to create it. Using the C++ delete method
// will not work with reference counting.
void vtkObjectBase::Delete()
{
  this->UnRegister(static_cast<vtkObjectBase *>(NULL));
}

void vtkObjectBase::FastDelete()
{
  // Remove the reference without doing a collection check even if
  // this object normally participates in garbage collection.
  this->UnRegisterInternal(0, 0);
}

void vtkObjectBase::Print(ostream& os)
{
  vtkIndent indent;

  this->PrintHeader(os,vtkIndent(0));
  this->PrintSelf(os, indent.GetNextIndent());
  this->PrintTrailer(os,vtkIndent(0));
}

void vtkObjectBase::PrintHeader(ostream& os, vtkIndent indent)
{
  os << indent << this->GetClassName() << " (" << this << ")\n";
}

// Chaining method to print an object's instance variables, as well as
// its superclasses.
void vtkObjectBase::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "Reference Count: " << this->ReferenceCount << "\n";
}

void vtkObjectBase::PrintTrailer(ostream& os, vtkIndent indent)
{
  os << indent << "\n";
}

// Description:
// Sets the reference count (use with care)
void vtkObjectBase::SetReferenceCount(int ref)
{
  this->ReferenceCount = ref;
  vtkBaseDebugMacro(<< "Reference Count set to " << this->ReferenceCount);
}

//----------------------------------------------------------------------------
void vtkObjectBase::Register(vtkObjectBase* o)
{
  // Do not participate in garbage collection by default.
  this->RegisterInternal(o, 0);
}

//----------------------------------------------------------------------------
void vtkObjectBase::UnRegister(vtkObjectBase* o)
{
  // Do not participate in garbage collection by default.
  this->UnRegisterInternal(o, 0);
}

//----------------------------------------------------------------------------
void vtkObjectBase::RegisterInternal(vtkObjectBase*, vtkTypeBool check)
{
  // If a reference is available from the garbage collector, use it.
  // Otherwise create a new reference by incrementing the reference
  // count.
  if(!(check &&
       vtkObjectBaseToGarbageCollectorFriendship::TakeReference(this)))
  {
    this->ReferenceCount++;
  }
}

//----------------------------------------------------------------------------
void vtkObjectBase::UnRegisterInternal(vtkObjectBase*, vtkTypeBool check)
{
  // If the garbage collector accepts a reference, do not decrement
  // the count.
  if(check && this->ReferenceCount > 1 &&
     vtkObjectBaseToGarbageCollectorFriendship::GiveReference(this))
  {
    return;
  }

  // Decrement the reference count, delete object if count goes to zero.
  if(--this->ReferenceCount <= 0)
  {
    // Clear all weak pointers to the object before deleting it.
    if (this->WeakPointers)
    {
      vtkWeakPointerBase **p = this->WeakPointers;
      while (*p)
      {
        vtkObjectBaseToWeakPointerBaseFriendship::ClearPointer(*p++);
      }
      delete [] this->WeakPointers;
    }
#ifdef VTK_DEBUG_LEAKS
    vtkDebugLeaks::DestructClass(this->GetClassName());
#endif
    delete this;
  }
  else if(check)
  {
    // The garbage collector did not accept the reference, but the
    // object still exists and is participating in garbage collection.
    // This means either that delayed garbage collection is disabled
    // or the collector has decided it is time to do a check.
    vtkGarbageCollector::Collect(this);
  }
}

//----------------------------------------------------------------------------
void vtkObjectBase::ReportReferences(vtkGarbageCollector*)
{
  // vtkObjectBase has no references to report.
}
