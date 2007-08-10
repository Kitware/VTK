/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphIdList.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
#include "vtkGraphIdList.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkGraphIdList, "1.2");
vtkStandardNewMacro(vtkGraphIdList);

vtkGraphIdList::vtkGraphIdList()
{
  this->NumberOfIds = 0;
  this->Size = 0;
  this->Ids = NULL;
  this->SaveUserArray = false;
}

vtkGraphIdList::~vtkGraphIdList()
{
  if ( this->Ids != NULL && !this->SaveUserArray )
    {
    delete [] this->Ids;
    }
}

void vtkGraphIdList::Initialize()
{
  if ( this->Ids != NULL && !this->SaveUserArray )
    {
    delete [] this->Ids;
    }
  this->Ids = NULL;
  this->NumberOfIds = 0;
  this->Size = 0;
  this->SaveUserArray = false;
}

int vtkGraphIdList::Allocate(const int sz, const int vtkNotUsed(strategy))
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
  this->SaveUserArray = false;
  return 1;
}

void vtkGraphIdList::SetNumberOfIds(const vtkIdType number)
{
  this->Allocate(number,0);
  this->NumberOfIds = number;
}

void vtkGraphIdList::SetId(const vtkIdType i, const vtkIdType id)
{
  if (this->SaveUserArray)
    {
    this->CopyArray();
    }
  this->Ids[i] = id;
}

void vtkGraphIdList::InsertId(const vtkIdType i, const vtkIdType id)
{
  if ( i >= this->Size )
    {
    this->Resize(i+1);
    }
  if (this->SaveUserArray)
    {
    this->CopyArray();
    }
  this->Ids[i] = id;
  if ( i >= this->NumberOfIds )
    {
    this->NumberOfIds = i + 1;
    }
}

vtkIdType vtkGraphIdList::InsertUniqueId(const vtkIdType id)
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

void vtkGraphIdList::SetArray(vtkIdType* ids, vtkIdType size, bool save)
{
  if (!this->SaveUserArray)
    {
    delete this->Ids;
    }
  this->Ids = ids;
  this->NumberOfIds = size;
  this->Size = size;
  this->SaveUserArray = save;
}

vtkIdType *vtkGraphIdList::WritePointer(const vtkIdType i, const vtkIdType number)
{
  if (this->SaveUserArray)
    {
    this->CopyArray();
    }

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

void vtkGraphIdList::DeleteId(vtkIdType id)
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

void vtkGraphIdList::DeepCopy(vtkGraphIdList *ids)
{
  this->Initialize();
  this->NumberOfIds = ids->NumberOfIds;
  this->Size = ids->Size;
  this->Ids = new vtkIdType [ids->Size];
  this->SaveUserArray = false;
  for (vtkIdType i=0; i < ids->NumberOfIds; i++)
    {
    this->Ids[i] = ids->Ids[i];
    }
}

vtkIdType *vtkGraphIdList::Resize(const vtkIdType sz)
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
    if (!this->SaveUserArray)
      {
      delete [] this->Ids;
      }
    }

  this->Size = newSize;
  this->Ids = newIds;
  this->SaveUserArray = false;
  return this->Ids;
}


void vtkGraphIdList::CopyArray()
{
  vtkIdType* newIds;
  if ( (newIds = new vtkIdType[this->Size]) == NULL )
    { 
    vtkErrorMacro(<< "Cannot allocate memory\n");
    return;
    }

  if (this->Ids)
    {
    memcpy(newIds, this->Ids, this->Size*sizeof(vtkIdType));
    if (!this->SaveUserArray)
      {
      delete [] this->Ids;
      }
    }

  this->Ids = newIds;
  this->SaveUserArray = false;
}

#define VTK_TMP_ARRAY_SIZE 500
// Intersect this list with another vtkGraphIdList. Updates current list according
// to result of intersection operation.
void vtkGraphIdList::IntersectWith(vtkGraphIdList& otherIds)
{
  if (this->SaveUserArray)
    {
    this->CopyArray();
    }

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

void vtkGraphIdList::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Number of Ids: " << this->NumberOfIds << "\n";
}
