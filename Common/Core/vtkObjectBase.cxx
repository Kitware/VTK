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
#include "vtkDebug.h"
#include "vtkDebugLeaks.h"
#include "vtkGarbageCollector.h"
#include "vtkWeakPointerBase.h"

#include <cassert>
#include <sstream>

#ifdef VTK_USE_MEMKIND
#include <memkind.h>
struct memkind* MemkindHandle = nullptr;
#endif

#define vtkBaseDebugMacro(x)

class vtkObjectBaseToGarbageCollectorFriendship
{
public:
  static int GiveReference(vtkObjectBase* obj) { return vtkGarbageCollector::GiveReference(obj); }
  static int TakeReference(vtkObjectBase* obj) { return vtkGarbageCollector::TakeReference(obj); }
};

class vtkObjectBaseToWeakPointerBaseFriendship
{
public:
  static void ClearPointer(vtkWeakPointerBase* p) { p->Object = nullptr; }
};

//------------------------------------------------------------------------------
void* vtkCustomMalloc(size_t size)
{
#ifdef VTK_USE_MEMKIND
  if (MemkindHandle == nullptr)
  {
    vtkGenericWarningMacro(<< "memkind_malloc() called before memkind initialized.");
  }
  else
  {
    return memkind_malloc(MemkindHandle, size);
  }
#else
  (void)size;
#endif
  return nullptr;
}

//------------------------------------------------------------------------------
void* vtkCustomRealloc(void* p, size_t size)
{
#ifdef VTK_USE_MEMKIND
  if (MemkindHandle == nullptr)
  {
    vtkGenericWarningMacro(<< "memkind_realloc() called before memkind initialized.");
  }
  else
  {
    return memkind_realloc(MemkindHandle, p, size);
  }
#else
  (void)p;
  (void)size;
#endif
  return nullptr;
}

//------------------------------------------------------------------------------
void vtkCustomFree(void* addr)
{
#ifdef VTK_USE_MEMKIND
  memkind_free(MemkindHandle, addr);
#else
  (void)addr;
#endif
}

#if defined(_WIN32) || defined(VTK_USE_MEMKIND)
//------------------------------------------------------------------------------
// Take control of allocation to avoid dll boundary problems or to use memkind.
void* vtkObjectBase::operator new(size_t nSize)
{
#ifdef VTK_USE_MEMKIND
  return vtkObjectBase::GetCurrentMallocFunction()(nSize);
#else
  return malloc(nSize);
#endif
}

//------------------------------------------------------------------------------
void vtkObjectBase::operator delete(void* p)
{
#ifdef VTK_USE_MEMKIND
  if (static_cast<vtkObjectBase*>(p)->GetIsInMemkind())
  {
    vtkCustomFree(p);
  }
  else
  {
    free(p);
  }
#else
  free(p);
#endif
}
// take control of ... above
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

//------------------------------------------------------------------------------
// Create an object with Debug turned off and modified time initialized
// to zero.
vtkObjectBase::vtkObjectBase()
{
  this->ReferenceCount = 1;
  this->WeakPointers = nullptr;
#ifdef VTK_DEBUG_LEAKS
  vtkDebugLeaks::ConstructingObject(this);
#endif
#ifdef VTK_USE_MEMKIND
  this->SetIsInMemkind(vtkObjectBase::GetUsingMemkind());
#else
  this->IsInMemkind = false;
#endif
}

//------------------------------------------------------------------------------
vtkObjectBase::~vtkObjectBase()
{
#ifdef VTK_DEBUG_LEAKS
  vtkDebugLeaks::DestructingObject(this);
#endif

  // warn user if reference counting is on and the object is being referenced
  // by another object
  if (this->ReferenceCount > 0)
  {
    vtkGenericWarningMacro(<< "Trying to delete object with non-zero reference count.");
  }
}

//------------------------------------------------------------------------------
void vtkObjectBase::InitializeObjectBase()
{
#ifdef VTK_DEBUG_LEAKS
  vtkDebugLeaks::ConstructClass(this);
#endif // VTK_DEBUG_LEAKS
}

//------------------------------------------------------------------------------
#ifdef VTK_WORKAROUND_WINDOWS_MANGLE
#undef GetClassName
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

std::string vtkObjectBase::GetObjectDescription() const
{
  std::stringstream s;
  s << this->GetClassName() << " (" << this << ")";
  return s.str();
}

vtkTypeBool vtkObjectBase::IsTypeOf(const char* name)
{
  if (!strcmp("vtkObjectBase", name))
  {
    return 1;
  }
  return 0;
}

vtkTypeBool vtkObjectBase::IsA(const char* name)
{
  return this->vtkObjectBase::IsTypeOf(name);
}

vtkIdType vtkObjectBase::GetNumberOfGenerationsFromBaseType(const char* name)
{
  if (!strcmp("vtkObjectBase", name))
  {
    return 0;
  }
  // Return the lowest value for vtkIdType. Because of recursion, the returned
  // value for derived classes will be this value added to the type distance to
  // vtkObjectBase. This sum will still be a negative (and, therefore, invalid)
  // value.
  return VTK_ID_MIN;
}

vtkIdType vtkObjectBase::GetNumberOfGenerationsFromBase(const char* name)
{
  return this->vtkObjectBase::GetNumberOfGenerationsFromBaseType(name);
}

// Delete a vtk object. This method should always be used to delete an object
// when the new operator was used to create it. Using the C++ delete method
// will not work with reference counting.
void vtkObjectBase::Delete()
{
  this->UnRegister(static_cast<vtkObjectBase*>(nullptr));
}

void vtkObjectBase::FastDelete()
{
  // Remove the reference without doing a collection check even if
  // this object normally participates in garbage collection.
  this->UnRegisterInternal(nullptr, 0);
}

void vtkObjectBase::Print(ostream& os)
{
  vtkIndent indent;

  this->PrintHeader(os, vtkIndent(0));
  this->PrintSelf(os, indent.GetNextIndent());
  this->PrintTrailer(os, vtkIndent(0));
}

void vtkObjectBase::PrintHeader(ostream& os, vtkIndent indent)
{
  os << indent << this->GetObjectDescription() << "\n";
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

//------------------------------------------------------------------------------
// Only meant to be called by specific subclasses for their own reasons.
void vtkObjectBase::ClearReferenceCounts()
{
  this->ReferenceCount = 0;
  vtkBaseDebugMacro(<< "Reference Count cleared");
}

//------------------------------------------------------------------------------
void vtkObjectBase::Register(vtkObjectBase* o)
{
  this->RegisterInternal(o, this->UsesGarbageCollector());
}

//------------------------------------------------------------------------------
void vtkObjectBase::UnRegister(vtkObjectBase* o)
{
  this->UnRegisterInternal(o, this->UsesGarbageCollector());
}

//------------------------------------------------------------------------------
void vtkObjectBase::RegisterInternal(vtkObjectBase*, vtkTypeBool check)
{
  // If a reference is available from the garbage collector, use it.
  // Otherwise create a new reference by incrementing the reference
  // count.
  if (!(check && vtkObjectBaseToGarbageCollectorFriendship::TakeReference(this)))
  {
    this->ReferenceCount++;
  }
}

//------------------------------------------------------------------------------
void vtkObjectBase::UnRegisterInternal(vtkObjectBase*, vtkTypeBool check)
{
  // If the garbage collector accepts a reference, do not decrement
  // the count.
  if (check && this->ReferenceCount > 1 &&
    vtkObjectBaseToGarbageCollectorFriendship::GiveReference(this))
  {
    return;
  }

  // Decrement the reference count, delete object if count goes to zero.
  if (--this->ReferenceCount <= 0)
  {
    // Let subclasses know the object is on its way out.
    this->ObjectFinalize();

    // Clear all weak pointers to the object before deleting it.
    if (this->WeakPointers)
    {
      vtkWeakPointerBase** p = this->WeakPointers;
      while (*p)
      {
        vtkObjectBaseToWeakPointerBaseFriendship::ClearPointer(*p++);
      }
      delete[] this->WeakPointers;
    }
#ifdef VTK_DEBUG_LEAKS
    vtkDebugLeaks::DestructClass(this);
#endif
    delete this;
  }
  else if (check)
  {
    // The garbage collector did not accept the reference, but the
    // object still exists and is participating in garbage collection.
    // This means either that delayed garbage collection is disabled
    // or the collector has decided it is time to do a check.
    vtkGarbageCollector::Collect(this);
  }
}

//------------------------------------------------------------------------------
void vtkObjectBase::ObjectFinalize() {}

//------------------------------------------------------------------------------
const char* vtkObjectBase::GetDebugClassName() const
{
  return this->GetClassName();
}

//------------------------------------------------------------------------------
void vtkObjectBase::ReportReferences(vtkGarbageCollector*)
{
  // vtkObjectBase has no references to report.
}

namespace
{
#ifdef VTK_USE_MEMKIND
VTK_THREAD_LOCAL char* MemkindDirectory = nullptr;
#endif
VTK_THREAD_LOCAL bool UsingMemkind = false;
VTK_THREAD_LOCAL vtkMallocingFunction CurrentMallocFunction = malloc;
VTK_THREAD_LOCAL vtkReallocingFunction CurrentReallocFunction = realloc;
VTK_THREAD_LOCAL vtkFreeingFunction CurrentFreeFunction = free;
VTK_THREAD_LOCAL vtkFreeingFunction AlternateFreeFunction = vtkCustomFree;
}

//------------------------------------------------------------------------------
void vtkObjectBase::SetMemkindDirectory(const char* directoryname)
{
#ifdef VTK_USE_MEMKIND
  if (MemkindDirectory == nullptr && MemkindHandle == nullptr)
  {
    MemkindDirectory = strdup(directoryname);
    int err = 0;
    if (!strncmp(directoryname, "ALLOCATOR_ONLY", 14))
    {
      // This gives us memkind's managed allocator but without extended memory.
      // It is useful for comparison and has performance benefits from page fault avoidance.
      MemkindHandle = MEMKIND_DEFAULT;
    }
    else
    {
      if (!strncmp(directoryname, "DAX_KMEM", 8))
      {
#if VTK_MEMKIND_HAS_DAX_KMEM
        MemkindHandle = MEMKIND_DAX_KMEM;
#else
        vtkGenericWarningMacro(<< "Warning, DAX_KMEM requires memkind >= 1.10");
        MemkindHandle = MEMKIND_DEFAULT;
#endif
      }
      else
      {
        err = memkind_create_pmem(MemkindDirectory, 0, &MemkindHandle);
      }
    }
    if (err)
    {
      perror("memkind_create_pmem()");
      free(MemkindDirectory);
      MemkindDirectory = nullptr;
    }
  }
  else
  {
    vtkGenericWarningMacro(<< "Warning, can only initialize memkind once.");
  }
#else
  (void)directoryname;
#endif
}

//------------------------------------------------------------------------------
bool vtkObjectBase::GetUsingMemkind()
{
  return UsingMemkind;
}

//------------------------------------------------------------------------------
void vtkObjectBase::SetUsingMemkind(bool b)
{
#ifdef VTK_USE_MEMKIND
  UsingMemkind = b;
  if (b)
  {
    CurrentMallocFunction = vtkCustomMalloc;
    CurrentReallocFunction = vtkCustomRealloc;
    CurrentFreeFunction = vtkCustomFree;
  }
  else
  {
    CurrentMallocFunction = malloc;
    CurrentReallocFunction = realloc;
    CurrentFreeFunction = free;
  }
#else
  // no harm in the above but avoid the cycles if we can
  (void)b;
  assert(!b);
#endif
}

//------------------------------------------------------------------------------
vtkMallocingFunction vtkObjectBase::GetCurrentMallocFunction()
{
  return CurrentMallocFunction;
}
//------------------------------------------------------------------------------
vtkReallocingFunction vtkObjectBase::GetCurrentReallocFunction()
{
  return CurrentReallocFunction;
}
//------------------------------------------------------------------------------
vtkFreeingFunction vtkObjectBase::GetCurrentFreeFunction()
{
  return CurrentFreeFunction;
}
//------------------------------------------------------------------------------
vtkFreeingFunction vtkObjectBase::GetAlternateFreeFunction()
{
  return AlternateFreeFunction;
}

//------------------------------------------------------------------------------
bool vtkObjectBase::GetIsInMemkind() const
{
  return this->IsInMemkind;
}

//------------------------------------------------------------------------------
void vtkObjectBase::SetIsInMemkind(bool v)
{
#ifdef VTK_USE_MEMKIND
  this->IsInMemkind = v;
#else
  (void)v;
  assert(!v);
#endif
}

//------------------------------------------------------------------------------
vtkObjectBase::vtkMemkindRAII::vtkMemkindRAII(bool newValue)
{
  this->Save(newValue);
}

//------------------------------------------------------------------------------
vtkObjectBase::vtkMemkindRAII::~vtkMemkindRAII()
{
  this->Restore();
}

//------------------------------------------------------------------------------
void vtkObjectBase::vtkMemkindRAII::Save(bool newValue)
{
#ifdef VTK_USE_MEMKIND
  this->OriginalValue = vtkObjectBase::GetUsingMemkind();
  vtkObjectBase::SetUsingMemkind(newValue);
#else
  (void)newValue;
#endif
}

//------------------------------------------------------------------------------
void vtkObjectBase::vtkMemkindRAII::Restore()
{
#ifdef VTK_USE_MEMKIND
  vtkObjectBase::SetUsingMemkind(this->OriginalValue);
#endif
}
