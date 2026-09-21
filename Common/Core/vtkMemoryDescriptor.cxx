// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkMemoryDescriptor.h"

#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkMemoryDescriptor);

//------------------------------------------------------------------------------
vtkMemoryDescriptor::vtkMemoryDescriptor() = default;

//------------------------------------------------------------------------------
vtkMemoryDescriptor::~vtkMemoryDescriptor()
{
  this->ClearOwnership();
  this->SetMemorySpace(nullptr);
  this->SetRole(nullptr);
}

//------------------------------------------------------------------------------
void vtkMemoryDescriptor::ClearOwnership()
{
  if (this->Owner)
  {
    this->Owner->UnRegister(this);
    this->Owner = nullptr;
  }
  if (this->Release)
  {
    // Take a local copy and clear the members first: the callback may do
    // anything at all, including re-entering this object.
    ReleaseFunction release = this->Release;
    void* context = this->ReleaseContext;
    this->Release = nullptr;
    this->ReleaseContext = nullptr;
    release(context);
  }
}

//------------------------------------------------------------------------------
void vtkMemoryDescriptor::Set(
  vtkTypeInt64 pointer, vtkTypeInt64 sizeInBytes, const char* memorySpace, const char* role)
{
  this->Pointer = pointer;
  this->SizeInBytes = sizeInBytes;
  this->SetMemorySpace(memorySpace);
  this->SetRole(role);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkMemoryDescriptor::SetOwner(vtkObject* owner)
{
  if (this->Owner == owner)
  {
    return;
  }
  // Register the new owner before dropping the old hold: they may be the same
  // object reached by different paths, and releasing first could destroy it.
  if (owner)
  {
    owner->Register(this);
  }
  this->ClearOwnership();
  this->Owner = owner;
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkMemoryDescriptor::SetRelease(ReleaseFunction release, void* context)
{
  if (this->Release == release && this->ReleaseContext == context)
  {
    return;
  }
  this->ClearOwnership();
  this->Release = release;
  this->ReleaseContext = context;
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkMemoryDescriptor::SetReleaseAddress(vtkTypeInt64 release, vtkTypeInt64 context)
{
  this->SetRelease(reinterpret_cast<ReleaseFunction>(release), reinterpret_cast<void*>(context));
}

//------------------------------------------------------------------------------
bool vtkMemoryDescriptor::TakeRelease(ReleaseFunction& release, void*& context)
{
  if (!this->Release)
  {
    release = nullptr;
    context = nullptr;
    return false;
  }
  release = this->Release;
  context = this->ReleaseContext;
  this->Release = nullptr;
  this->ReleaseContext = nullptr;
  this->Modified();
  return true;
}

//------------------------------------------------------------------------------
void vtkMemoryDescriptor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Pointer: 0x" << std::hex << this->Pointer << std::dec << "\n";
  os << indent << "SizeInBytes: " << this->SizeInBytes << "\n";
  os << indent << "MemorySpace: " << (this->MemorySpace ? this->MemorySpace : "(none)") << "\n";
  os << indent << "Role: " << (this->Role ? this->Role : "(none)") << "\n";
  os << indent << "Ownership: ";
  if (this->Owner)
  {
    os << "holds a reference to " << this->Owner->GetClassName() << "\n";
  }
  else if (this->Release)
  {
    os << "holds a release callback\n";
  }
  else
  {
    os << "none -- the caller manages this memory's lifetime\n";
  }
}

VTK_ABI_NAMESPACE_END
