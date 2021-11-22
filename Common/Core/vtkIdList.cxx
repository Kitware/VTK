/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIdList.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkIdList.h"
#include "vtkObjectFactory.h"
#include "vtkSMPTools.h" //for parallel sort

vtkStandardNewMacro(vtkIdList);

//------------------------------------------------------------------------------
vtkIdList::vtkIdList()
{
  this->NumberOfIds = 0;
  this->Size = 0;
  this->Ids = nullptr;
  this->ManageMemory = true;
}

//------------------------------------------------------------------------------
vtkIdList::~vtkIdList()
{
  if (this->ManageMemory)
  {
    delete[] this->Ids;
  }
}

//------------------------------------------------------------------------------
vtkIdType* vtkIdList::Release()
{
  auto retval = this->Ids;
  this->Ids = nullptr;
  this->Initialize();
  return retval;
}

//------------------------------------------------------------------------------
void vtkIdList::Initialize()
{
  if (this->ManageMemory)
  {
    delete[] this->Ids;
  }
  this->ManageMemory = true;
  this->Ids = nullptr;
  this->NumberOfIds = 0;
  this->Size = 0;
}

//------------------------------------------------------------------------------
int vtkIdList::Allocate(const vtkIdType sz, const int vtkNotUsed(strategy))
{
  if (sz > this->Size)
  {
    this->Initialize();
    this->Size = (sz > 0 ? sz : 1);
    if ((this->Ids = new vtkIdType[this->Size]) == nullptr)
    {
      return 0;
    }
  }
  this->NumberOfIds = 0;
  return 1;
}

//------------------------------------------------------------------------------
void vtkIdList::SetNumberOfIds(const vtkIdType number)
{
  this->Allocate(number, 0);
  this->NumberOfIds = number;
}

//------------------------------------------------------------------------------
vtkIdType vtkIdList::InsertUniqueId(const vtkIdType vtkid)
{
  for (vtkIdType i = 0; i < this->NumberOfIds; i++)
  {
    if (vtkid == this->Ids[i])
    {
      return i;
    }
  }

  return this->InsertNextId(vtkid);
}

//------------------------------------------------------------------------------
vtkIdType* vtkIdList::WritePointer(const vtkIdType i, const vtkIdType number)
{
  vtkIdType newSize = i + number;
  if (newSize > this->Size)
  {
    this->Resize(newSize);
  }
  if (newSize > this->NumberOfIds)
  {
    this->NumberOfIds = newSize;
  }
  return this->Ids + i;
}

//------------------------------------------------------------------------------
void vtkIdList::SetArray(vtkIdType* array, vtkIdType size, bool save)
{
  if (this->ManageMemory)
  {
    delete[] this->Ids;
  }
  if (!array)
  {
    if (size)
    {
      vtkWarningMacro(<< "Passed a nullptr with a non-zero size... Setting size to 0.");
      size = 0;
    }
    if (!save)
    {
      vtkWarningMacro(<< "Passed a nullptr while setting save to false... Setting save to true.");
      save = true;
    }
  }
  this->ManageMemory = save;
  this->Ids = array;
  this->NumberOfIds = size;
  this->Size = size;
}

//------------------------------------------------------------------------------
void vtkIdList::DeleteId(vtkIdType vtkid)
{
  vtkIdType i = 0;

  // while loop is necessary to delete all occurrences of vtkid
  while (i < this->NumberOfIds)
  {
    for (; i < this->NumberOfIds; i++)
    {
      if (this->Ids[i] == vtkid)
      {
        break;
      }
    }

    // if found; replace current id with last
    if (i < this->NumberOfIds)
    {
      this->SetId(i, this->Ids[this->NumberOfIds - 1]);
      this->NumberOfIds--;
    }
  }
}

//------------------------------------------------------------------------------
void vtkIdList::DeepCopy(vtkIdList* ids)
{
  this->SetNumberOfIds(ids->NumberOfIds);
  if (ids->NumberOfIds > 0)
  {
    std::copy(ids->Ids, ids->Ids + ids->NumberOfIds, this->Ids);
  }
  this->Squeeze();
}

//------------------------------------------------------------------------------
vtkIdType* vtkIdList::Resize(const vtkIdType sz)
{
  vtkIdType* newIds;
  vtkIdType newSize;

  if (sz > this->Size)
  {
    newSize = this->Size + sz;
  }
  else if (sz == this->Size)
  {
    return this->Ids;
  }
  else
  {
    newSize = sz;
  }

  if (newSize <= 0)
  {
    this->Initialize();
    return nullptr;
  }

  if ((newIds = new vtkIdType[newSize]) == nullptr)
  {
    vtkErrorMacro(<< "Cannot allocate memory\n");
    return nullptr;
  }

  if (this->NumberOfIds > newSize)
  {
    this->NumberOfIds = newSize;
  }

  if (this->Ids)
  {
    memcpy(newIds, this->Ids,
      static_cast<size_t>(sz < this->Size ? sz : this->Size) * sizeof(vtkIdType));
    if (this->ManageMemory)
    {
      delete[] this->Ids;
    }
  }
  this->ManageMemory = true;

  this->Size = newSize;
  this->Ids = newIds;
  return this->Ids;
}

//------------------------------------------------------------------------------
#define VTK_TMP_ARRAY_SIZE 500
// Intersect this list with another vtkIdList. Updates current list according
// to result of intersection operation.
void vtkIdList::IntersectWith(vtkIdList* otherIds)
{
  // Fast method due to Dr. Andreas Mueller of ISE Integrated Systems
  // Engineering (CH).
  vtkIdType thisNumIds = this->GetNumberOfIds();

  if (thisNumIds <= VTK_TMP_ARRAY_SIZE)
  { // Use fast method if we can fit in temporary storage
    vtkIdType thisIds[VTK_TMP_ARRAY_SIZE];
    vtkIdType i, vtkid;

    for (i = 0; i < thisNumIds; i++)
    {
      thisIds[i] = this->GetId(i);
    }
    for (this->Reset(), i = 0; i < thisNumIds; i++)
    {
      vtkid = thisIds[i];
      if (otherIds->IsId(vtkid) != (-1))
      {
        this->InsertNextId(vtkid);
      }
    }
  }
  else
  { // use slower method for extreme cases
    vtkIdType* thisIds = new vtkIdType[thisNumIds];
    vtkIdType i, vtkid;

    for (i = 0; i < thisNumIds; i++)
    {
      *(thisIds + i) = this->GetId(i);
    }
    for (this->Reset(), i = 0; i < thisNumIds; i++)
    {
      vtkid = *(thisIds + i);
      if (otherIds->IsId(vtkid) != (-1))
      {
        this->InsertNextId(vtkid);
      }
    }
    delete[] thisIds;
  }
}
#undef VTK_TMP_ARRAY_SIZE

//------------------------------------------------------------------------------
void vtkIdList::Sort()
{
  if (this->Ids == nullptr || this->NumberOfIds < 2)
  {
    return;
  }
  vtkSMPTools::Sort(this->Ids, this->Ids + this->NumberOfIds);
}

//------------------------------------------------------------------------------
void vtkIdList::Fill(vtkIdType value)
{
  if (this->Ids == nullptr || this->NumberOfIds < 1)
  {
    return;
  }
  vtkSMPTools::Fill(this->Ids, this->Ids + this->NumberOfIds, value);
}

//------------------------------------------------------------------------------
void vtkIdList::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Number of Ids: " << this->NumberOfIds << "\n";
}
