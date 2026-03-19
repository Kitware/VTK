// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// VTK_DEPRECATED_IN_9_7_0()
#define VTK_DEPRECATION_LEVEL 0

#include "vtkIdList.h"
#include "vtkObjectFactory.h"
#include "vtkSMPTools.h" //for parallel sort

#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkIdList);

//------------------------------------------------------------------------------
vtkIdList::vtkIdList()
{
  this->Size = 0; // VTK_DEPRECATED_IN_9_7_0
  this->NumberOfIds = 0;
  this->Buffer = vtkBuffer<vtkIdType>::New();
}

//------------------------------------------------------------------------------
vtkIdList::~vtkIdList()
{
  this->Buffer->Delete();
}

//------------------------------------------------------------------------------
vtkIdType* vtkIdList::Release()
{
  auto retval = this->Buffer->GetBuffer();
  // don't free the buffer when the vtkBuffer is deleted
  this->Buffer->SetFreeFunction(/*noFreeFunction*/ true, nullptr);
  this->Initialize();
  return retval;
}

//------------------------------------------------------------------------------
void vtkIdList::InitializeMemory()
{
  this->Buffer->Allocate(0);
}

//------------------------------------------------------------------------------
void vtkIdList::Initialize()
{
  this->Reset();
  this->Squeeze();
}

//------------------------------------------------------------------------------
bool vtkIdList::AllocateInternal(vtkIdType sz, vtkIdType numberOfIds)
{
  this->Initialize();
  if (this->Reserve(sz))
  {
    this->NumberOfIds = numberOfIds;
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
vtkTypeBool vtkIdList::Allocate(const vtkIdType size, const int vtkNotUsed(strategy))
{
  this->NumberOfIds = 0;
  if (size > this->GetCapacity() || size == 0)
  {
    this->Size = 0; // VTK_DEPRECATED_IN_9_7_0

    if (this->Buffer->Allocate(size))
    {
      this->Size = size; // VTK_DEPRECATED_IN_9_7_0
    }
    else
    {
      vtkErrorMacro(
        "Unable to allocate " << size << " elements of size " << sizeof(vtkIdType) << " bytes. ");
#if !defined VTK_DONT_THROW_BAD_ALLOC
      // We can throw something that has universal meaning
      throw std::bad_alloc();
#else
      // We indicate that alloc failed by return
      return 0;
#endif
    }
  }
  return 1;
}

//------------------------------------------------------------------------------
vtkTypeBool vtkIdList::Reserve(vtkIdType size)
{
  if (size <= this->GetCapacity())
  {
    return 1;
  }
  // Requested size is bigger than current size.  Allocate enough
  // memory to fit the requested size and be more than double the
  // currently allocated memory.
  vtkIdType numIds = this->GetCapacity() + size;
  if (this->Buffer->Reallocate(numIds))
  {
    this->Size = numIds; // VTK_DEPRECATED_IN_9_7_0
  }
  else
  {
    vtkErrorMacro(
      "Unable to allocate " << numIds << " elements of size " << sizeof(vtkIdType) << " bytes. ");
#if !defined NDEBUG
    // We're debugging, crash here preserving the stack
    abort();
#elif !defined VTK_DONT_THROW_BAD_ALLOC
    // We can throw something that has universal meaning
    throw std::bad_alloc();
#else
    // We indicate that malloc failed by return
    return 0;
#endif
  }
  return 1;
}

//------------------------------------------------------------------------------
void vtkIdList::Squeeze()
{
  if (this->GetCapacity() > this->NumberOfIds)
  {
    if (this->Buffer->Reallocate(this->NumberOfIds))
    {
      this->Size = this->NumberOfIds; // VTK_DEPRECATED_IN_9_7_0
    }
    else
    {
      vtkErrorMacro("Unable to allocate " << this->NumberOfIds << " elements of size "
                                          << sizeof(vtkIdType) << " bytes. ");
#if !defined NDEBUG
      // We're debugging, crash here preserving the stack
      abort();
#elif !defined VTK_DONT_THROW_BAD_ALLOC
      // We can throw something that has universal meaning
      throw std::bad_alloc();
#else
      // We indicate that malloc failed by return
      return;
#endif
    }
  }
}

//------------------------------------------------------------------------------
void vtkIdList::SetNumberOfIds(const vtkIdType size)
{
  if (this->Reserve(size))
  {
    this->NumberOfIds = size;
  }
}

//------------------------------------------------------------------------------
vtkIdType vtkIdList::InsertUniqueId(const vtkIdType id)
{
  for (vtkIdType i = 0; i < this->NumberOfIds; i++)
  {
    if (id == this->Buffer->GetBuffer()[i])
    {
      return i;
    }
  }

  return this->InsertNextId(id);
}

//------------------------------------------------------------------------------
vtkIdType* vtkIdList::WritePointer(const vtkIdType i, const vtkIdType number)
{
  vtkIdType newSize = i + number;
  if (newSize > this->GetCapacity())
  {
    this->Reserve(newSize);
  }
  this->NumberOfIds = std::max(newSize, this->NumberOfIds);
  return this->Buffer->GetBuffer() + i;
}

//------------------------------------------------------------------------------
void vtkIdList::SetList(vtkIdType* array, vtkIdType size, bool save, int deleteMethod)
{
  this->Buffer->SetBuffer(array, size);

  if (deleteMethod == VTK_DATA_ARRAY_DELETE)
  {
    this->Buffer->SetFreeFunction(save, ::operator delete[]);
  }
  else if (deleteMethod == VTK_DATA_ARRAY_ALIGNED_FREE)
  {
#ifdef _WIN32
    this->Buffer->SetFreeFunction(save, _aligned_free);
#else
    this->Buffer->SetFreeFunction(save, free);
#endif
  }
  else if (deleteMethod == VTK_DATA_ARRAY_USER_DEFINED || deleteMethod == VTK_DATA_ARRAY_FREE)
  {
    this->Buffer->SetFreeFunction(save, free);
  }

  this->Size = size; // VTK_DEPRECATED_IN_9_7_0
  this->NumberOfIds = size;
}

//------------------------------------------------------------------------------
void vtkIdList::SetArray(vtkIdType* array, vtkIdType size, bool manageMemory)
{
  this->SetList(array, size, !manageMemory, VTK_DATA_ARRAY_DELETE);
}

//------------------------------------------------------------------------------
void vtkIdList::DeleteId(vtkIdType id)
{
  vtkIdType i = 0;

  // while loop is necessary to delete all occurrences of id
  while (i < this->NumberOfIds)
  {
    for (; i < this->NumberOfIds; i++)
    {
      if (this->Buffer->GetBuffer()[i] == id)
      {
        break;
      }
    }

    // if found; replace current id with last
    if (i < this->NumberOfIds)
    {
      this->SetId(i, this->Buffer->GetBuffer()[this->NumberOfIds - 1]);
      this->NumberOfIds--;
    }
  }
}

//------------------------------------------------------------------------------
void vtkIdList::ShallowCopy(vtkIdList* list)
{
  this->NumberOfIds = list->NumberOfIds;
  this->Size = list->Size; // VTK_DEPRECATED_IN_9_7_0
  if (list->Buffer && this->Buffer != list->Buffer)
  {
    this->Buffer->Delete();
    this->Buffer = list->Buffer;
    this->Buffer->Register(this);
  }
}

//------------------------------------------------------------------------------
void vtkIdList::DeepCopy(vtkIdList* ids)
{
  this->SetNumberOfIds(ids->NumberOfIds);
  if (ids->NumberOfIds > 0)
  {
    std::copy_n(ids->Buffer->GetBuffer(), ids->NumberOfIds, this->Buffer->GetBuffer());
  }
  this->Squeeze();
}

//------------------------------------------------------------------------------
vtkIdType* vtkIdList::Resize(const vtkIdType size)
{
  if (size <= 0)
  {
    this->Initialize();
    return nullptr;
  }
  if (this->GetCapacity() >= size)
  {
    this->NumberOfIds = size;
    this->Squeeze();
    return this->Buffer->GetBuffer();
  }
  this->Reserve(size);
  return this->Buffer->GetBuffer();
}

//------------------------------------------------------------------------------
// Intersect this list with another vtkIdList. Updates current list according
// to result of intersection operation.
void vtkIdList::IntersectWith(vtkIdList* otherIds)
{
  static constexpr vtkIdType VTK_TMP_ARRAY_SIZE = 500;
  // Fast method due to Dr. Andreas Mueller of ISE Integrated Systems
  // Engineering (CH).
  vtkIdType thisNumIds = this->GetNumberOfIds();

  if (thisNumIds <= VTK_TMP_ARRAY_SIZE)
  { // Use fast method if we can fit in temporary storage
    vtkIdType thisIds[VTK_TMP_ARRAY_SIZE];
    for (vtkIdType i = 0; i < thisNumIds; i++)
    {
      thisIds[i] = this->GetId(i);
    }
    this->Reset();
    for (vtkIdType i = 0; i < thisNumIds; i++)
    {
      vtkIdType& id = thisIds[i];
      if (otherIds->IsId(id) != -1)
      {
        this->InsertNextId(id);
      }
    }
  }
  else // use slower method for extreme cases
  {
    std::vector<vtkIdType> thisIds(this->begin(), this->end());
    this->Reset();
    for (vtkIdType i = 0; i < thisNumIds; i++)
    {
      vtkIdType& id = thisIds[i];
      if (otherIds->IsId(id) != -1)
      {
        this->InsertNextId(id);
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkIdList::Sort()
{
  if (this->Buffer->GetBuffer() == nullptr || this->NumberOfIds < 2)
  {
    return;
  }
  vtkSMPTools::Sort(this->Buffer->GetBuffer(), this->Buffer->GetBuffer() + this->NumberOfIds);
}

//------------------------------------------------------------------------------
void vtkIdList::Fill(vtkIdType value)
{
  if (this->Buffer->GetBuffer() == nullptr || this->NumberOfIds < 1)
  {
    return;
  }
  vtkSMPTools::Fill(
    this->Buffer->GetBuffer(), this->Buffer->GetBuffer() + this->NumberOfIds, value);
}

//------------------------------------------------------------------------------
void vtkIdList::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Number of Ids: " << this->NumberOfIds << "\n";
}
VTK_ABI_NAMESPACE_END
