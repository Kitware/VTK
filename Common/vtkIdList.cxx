/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIdList.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkIdList.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkIdList, "1.42");
vtkStandardNewMacro(vtkIdList);

vtkIdList::vtkIdList()
{
  this->NumberOfIds = 0;
  this->Size = 0;
  this->Ids = NULL;
}

vtkIdList::~vtkIdList()
{
  if ( this->Ids != NULL )
    {
    delete [] this->Ids;
    }
}

void vtkIdList::Initialize()
{
  if ( this->Ids != NULL )
    {
    delete [] this->Ids;
    this->Ids = NULL;
    }
  this->NumberOfIds = 0;
  this->Size = 0;
}

int vtkIdList::Allocate(const int sz, const int vtkNotUsed(strategy))
{
  if ( sz > this->Size)
    {
    this->Initialize();
    this->Size = ( sz > 0 ? sz : 1);
    if ( (this->Ids = new vtkIdType[this->Size]) == NULL )
      {
      return 0;
      }
    }
  this->NumberOfIds = 0;
  return 1;
}

void vtkIdList::SetNumberOfIds(const vtkIdType number)
{
  this->Allocate(number,0);
  this->NumberOfIds = number;
}

void vtkIdList::InsertId(const vtkIdType i, const vtkIdType id)
{
  if ( i >= this->Size )
    {
    this->Resize(i+1);
    }
  this->Ids[i] = id;
  if ( i >= this->NumberOfIds )
    {
    this->NumberOfIds = i + 1;
    }
}

vtkIdType vtkIdList::InsertUniqueId(const vtkIdType id)
{
  for (vtkIdType i=0; i < this->NumberOfIds; i++)
    {
    if ( id == this->Ids[i] )
      {
      return i;
      }
    }
  
  return this->InsertNextId(id);
}

vtkIdType *vtkIdList::WritePointer(const vtkIdType i, const vtkIdType number)
{
  vtkIdType newSize=i+number;
  if ( newSize > this->Size )
    {
    this->Resize(newSize);
    }
  if ( newSize > this->NumberOfIds )
    {
    this->NumberOfIds = newSize;
    }
  return this->Ids + i;
}

void vtkIdList::DeleteId(vtkIdType id)
{
  vtkIdType i=0;

  // while loop is necessary to delete all occurences of id
  while ( i < this->NumberOfIds )
    {
    for ( ; i < this->NumberOfIds; i++)
      {
      if ( this->Ids[i] == id )
        {
        break;
        }
      }

    // if found; replace current id with last
    if ( i < this->NumberOfIds )
      {
      this->SetId(i,this->Ids[this->NumberOfIds-1]);
      this->NumberOfIds--;
      }
    }
}

void vtkIdList::DeepCopy(vtkIdList *ids)
{
  this->Initialize();
  this->NumberOfIds = ids->NumberOfIds;
  this->Size = ids->Size;
  this->Ids = new vtkIdType [ids->Size];
  for (vtkIdType i=0; i < ids->NumberOfIds; i++)
    {
    this->Ids[i] = ids->Ids[i];
    }
}

vtkIdType *vtkIdList::Resize(const vtkIdType sz)
{
  vtkIdType *newIds;
  vtkIdType newSize;

  if ( sz > this->Size ) 
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
    return 0;
    }

  if ( (newIds = new vtkIdType[newSize]) == NULL )
    { 
    vtkErrorMacro(<< "Cannot allocate memory\n");
    return 0;
    }

  if (this->Ids)
    {
    memcpy(newIds, this->Ids,
           (sz < this->Size ? sz : this->Size) * sizeof(vtkIdType));
    delete [] this->Ids;
    }

  this->Size = newSize;
  this->Ids = newIds;
  return this->Ids;
}

#define VTK_TMP_ARRAY_SIZE 500
// Intersect this list with another vtkIdList. Updates current list according
// to result of intersection operation.
void vtkIdList::IntersectWith(vtkIdList& otherIds)
{
  // Fast method due to Dr. Andreas Mueller of ISE Integrated Systems 
  // Engineering (CH).
  vtkIdType thisNumIds = this->GetNumberOfIds();

  if (thisNumIds <= VTK_TMP_ARRAY_SIZE) 
    {//Use fast method if we can fit in temporary storage
    int  thisIds[VTK_TMP_ARRAY_SIZE];
    vtkIdType i, id;
    
    for (i=0; i < thisNumIds; i++)
      {
      thisIds[i] = this->GetId(i);
      }
    for (this->Reset(), i=0; i < thisNumIds; i++) 
      {
      id = thisIds[i];
      if ( otherIds.IsId(id) != (-1) )
        {
        this->InsertNextId(id);
        }
      }
    } 
  else 
    {//use slower method for extreme cases
    vtkIdType *thisIds = new vtkIdType [thisNumIds];
    vtkIdType  i, id;
    
    for (i=0; i < thisNumIds; i++)
      {
      *(thisIds + i) = this->GetId(i);
      }
    for (this->Reset(), i=0; i < thisNumIds; i++) 
      {
      id = *(thisIds + i);
      if ( otherIds.IsId(id) != (-1) )
        {
        this->InsertNextId(id);
        }
      }
    delete [] thisIds;
    }
}
#undef VTK_TMP_ARRAY_SIZE

void vtkIdList::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Number of Ids: " << this->NumberOfIds << "\n";
}
