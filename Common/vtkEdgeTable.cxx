/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEdgeTable.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkEdgeTable.h"
#include "vtkPoints.h"
#include "vtkIdList.h"
#include "vtkVoidArray.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkEdgeTable);

// Instantiate object based on maximum point id.
vtkEdgeTable::vtkEdgeTable()
{
  this->Table = NULL;
  this->Attributes = NULL;
  this->PointerAttributes = NULL;
  this->Points = NULL;

  this->TableMaxId = -1;
  this->TableSize = 0;

  this->Position[0] = 0;
  this->Position[1] = -1;
  this->NumberOfEdges = 0;
}

// Free memory and return to instantiated state.
void vtkEdgeTable::Initialize()
{
  vtkIdType i;
  
  if ( this->Table )
    {
    for (i=0; i < this->TableSize; i++)
      {
      if ( this->Table[i] )
        {
        this->Table[i]->Delete();
        }
      }
    delete [] this->Table;
    this->Table = NULL;
    this->TableMaxId = -1;

    if ( this->StoreAttributes == 1 )
      {
      for (i=0; i < this->TableSize; i++)
        {
        if ( this->Attributes[i] )
          {
          this->Attributes[i]->Delete();
          }
        }
      delete [] this->Attributes;
      this->Attributes = NULL;
      }
    else if ( this->StoreAttributes == 2 )
      {
      for (i=0; i < this->TableSize; i++)
        {
        if ( this->PointerAttributes[i] )
          {
          this->PointerAttributes[i]->Delete();
          }
        }
      delete [] this->PointerAttributes;
      this->PointerAttributes = NULL;
      }
    }//if table defined

  if ( this->Points )
    {
    this->Points->Delete();
    this->Points = NULL;
    }
    
  this->TableSize = 0;
  this->NumberOfEdges = 0;
}

// Free memory and return to instantiated state.
void vtkEdgeTable::Reset()
{
  vtkIdType i;

  if ( this->Table )
    {
    for (i=0; i < this->TableSize; i++)
      {
      if ( this->Table[i] )
        {
        this->Table[i]->Reset();
        }
      }

    if ( this->StoreAttributes == 1 && this->Attributes )
      {
      for (i=0; i < this->TableSize; i++)
        {
        if ( this->Attributes[i] )
          {
          this->Attributes[i]->Reset();
          }
        }
      }
    else if ( this->StoreAttributes == 2 && this->PointerAttributes )
      {
      for (i=0; i < this->TableSize; i++)
        {
        if ( this->PointerAttributes[i] )
          {
          this->PointerAttributes[i]->Reset();
          }
        }
      }
    }//if table defined

  this->TableMaxId = -1;

  if ( this->Points )
    {
    this->Points->Reset();
    }
    
  this->NumberOfEdges = 0;
}

vtkEdgeTable::~vtkEdgeTable()
{
  this->Initialize();
}

int vtkEdgeTable::InitEdgeInsertion(vtkIdType numPoints, int storeAttributes)
{
  vtkIdType i;
  
  numPoints = (numPoints < 1 ? 1 : numPoints);

  // Discard old memory if not enough has been previously allocated
  this->StoreAttributes = storeAttributes;
  this->TableMaxId = -1;

  if ( numPoints > this->TableSize )
    {
    this->Initialize();
    this->Table = new vtkIdList *[numPoints];
    for (i=0; i < numPoints; i++)
      {
      this->Table[i] = NULL;
      }

    if (this->StoreAttributes == 1)
      {
      this->Attributes = new vtkIdList *[numPoints];
      for (i=0; i < numPoints; i++)
        {
        this->Attributes[i] = NULL;
        }
      }
    else if (this->StoreAttributes == 2)
      {
      this->PointerAttributes = new vtkVoidArray *[numPoints];
      for (i=0; i < numPoints; i++)
        {
        this->PointerAttributes[i] = NULL;
        }
      }
    this->TableSize = numPoints;
    }
  
  // Otherwise, reuse the old memory
  else
    {
    this->Reset();
    }

  this->Position[0] = 0;
  this->Position[1] = -1;

  this->NumberOfEdges = 0;

  return 1;
}

// Return non-negative if edge (p1,p2) is an edge; otherwise -1.
vtkIdType vtkEdgeTable::IsEdge(vtkIdType p1, vtkIdType p2)
{
  vtkIdType index, search;

  if ( p1 < p2 )
    {
    index = p1;
    search = p2;
    }
  else
    {
    index = p2;
    search = p1;
    }

  if ( this->Table[index] == NULL ) 
    {
    return (-1);
    }
  else
    {
    vtkIdType loc;
    if ( (loc=this->Table[index]->IsId(search)) == (-1) )
      {
      return (-1);
      }
    else
      {
      if ( this->StoreAttributes == 1 )
        {
        return this->Attributes[index]->GetId(loc);
        }
      else
        {
        return 1;
        }
      }
    }
}

// Return non-negative if edge (p1,p2) is an edge; otherwise -1.
void vtkEdgeTable::IsEdge(vtkIdType p1, vtkIdType p2, void* &ptr)
{
  vtkIdType index, search;

  if ( p1 < p2 )
    {
    index = p1;
    search = p2;
    }
  else
    {
    index = p2;
    search = p1;
    }

  if ( this->Table[index] == NULL ) 
    {
    ptr = NULL;
    }
  else
    {
    vtkIdType loc;
    if ( (loc=this->Table[index]->IsId(search)) == (-1) )
      {
      ptr = NULL;
      }
    else
      {
      if ( this->StoreAttributes == 2 )
        {
        ptr = this->PointerAttributes[index]->GetVoidPointer(loc);
        }
      else
        {
        ptr = NULL;
        }
      }
    }
}

// Insert the edge (p1,p2) into the table. It is the user's responsibility to
// check if the edge has already been inserted.
vtkIdType vtkEdgeTable::InsertEdge(vtkIdType p1, vtkIdType p2)
{
  vtkIdType index, search;

  if ( p1 < p2 )
    {
    index = p1;
    search = p2;
    }
  else
    {
    index = p2;
    search = p1;
    }

  if ( index >= this->TableSize )
    {
    this->Resize(index+1);
    }
  
  if ( index > this->TableMaxId )
    {
    this->TableMaxId = index;
    }

  if ( this->Table[index] == NULL ) 
    {
    this->Table[index] = vtkIdList::New();
    this->Table[index]->Allocate(6,12);
    if ( this->StoreAttributes == 1 )
      {
      if ( this->Attributes[index] )
        {
        this->Attributes[index]->Delete();
        }
      this->Attributes[index] = vtkIdList::New();
      this->Attributes[index]->Allocate(6,12);
      }
    }

  this->Table[index]->InsertNextId(search);
  if ( this->StoreAttributes == 1 )
    {
    this->Attributes[index]->InsertNextId(this->NumberOfEdges);
    }
  this->NumberOfEdges++;

  return (this->NumberOfEdges - 1);
}

void vtkEdgeTable::InsertEdge(vtkIdType p1, vtkIdType p2, 
                              vtkIdType attributeId)
{
  vtkIdType index, search;

  if ( p1 < p2 )
    {
    index = p1;
    search = p2;
    }
  else
    {
    index = p2;
    search = p1;
    }

  if ( index >= this->TableSize )
    {
    this->Resize(index+1);
    }

  if ( index > this->TableMaxId )
    {
    this->TableMaxId = index;
    }

  if ( this->Table[index] == NULL ) 
    {
    this->Table[index] = vtkIdList::New();
    this->Table[index]->Allocate(6,12);
    if ( this->StoreAttributes == 1 )
      {
      this->Attributes[index] = vtkIdList::New();
      this->Attributes[index]->Allocate(6,12);
      }
    }

  this->NumberOfEdges++;
  this->Table[index]->InsertNextId(search);
  if ( this->StoreAttributes )
    {
    this->Attributes[index]->InsertNextId(attributeId);
    }
}

void vtkEdgeTable::InsertEdge(vtkIdType p1, vtkIdType p2, void* ptr)
{
  vtkIdType index, search;

  if ( p1 < p2 )
    {
    index = p1;
    search = p2;
    }
  else
    {
    index = p2;
    search = p1;
    }

  if ( index >= this->TableSize )
    {
    this->Resize(index+1);
    }

  if ( index > this->TableMaxId )
    {
    this->TableMaxId = index;
    }

  if ( this->Table[index] == NULL ) 
    {
    this->Table[index] = vtkIdList::New();
    this->Table[index]->Allocate(6,12);
    if ( this->StoreAttributes == 2 )
      {
      this->PointerAttributes[index] = vtkVoidArray::New();
      this->PointerAttributes[index]->Allocate(6,12);
      }
    }

  this->NumberOfEdges++;
  this->Table[index]->InsertNextId(search);
  if ( this->StoreAttributes == 2 )
    {
    this->PointerAttributes[index]->InsertNextVoidPointer(ptr);
    }
}


// Intialize traversal of edges in table.
void vtkEdgeTable::InitTraversal()
{
  this->Position[0] = 0;
  this->Position[1] = -1;
}

// Traverse list of edges in table. Return the edge as (p1,p2), where p1 and
// p2 are point id's. Method return value is <0 if the list is exhausted;
// otherwise a valid id >=0. The value of p1 is guaranteed to be <= p2. The
// return value is an id that can be used for accessing attributes.
vtkIdType vtkEdgeTable::GetNextEdge(vtkIdType &p1, vtkIdType &p2)
{
  for ( ; this->Position[0] <= this->TableMaxId; 
  this->Position[0]++, this->Position[1]=(-1) )
    {
    if ( this->Table[this->Position[0]] != NULL && 
    ++this->Position[1] < this->Table[this->Position[0]]->GetNumberOfIds() )
      {
      p1 = this->Position[0];
      p2 = this->Table[this->Position[0]]->GetId(this->Position[1]);
      if ( this->StoreAttributes == 1 )
        {
        return this->Attributes[this->Position[0]]->GetId(this->Position[1]);
        }
      else
        {
          return (-1);
        }
      }
    }

  return (-1);
}

// Traverse list of edges in table. Return the edge as (p1,p2), where p1 and
// p2 are point id's. The value of p1 is guaranteed to be <= p2. The
// return value is either 1 for success or 0 if the list is exhausted.
int vtkEdgeTable::GetNextEdge(vtkIdType &p1, vtkIdType &p2, void* &ptr)
{
  for ( ; this->Position[0] <= this->TableMaxId; 
  this->Position[0]++, this->Position[1]=(-1) )
    {
    if ( this->Table[this->Position[0]] != NULL && 
    ++this->Position[1] < this->Table[this->Position[0]]->GetNumberOfIds() )
      {
      p1 = this->Position[0];
      p2 = this->Table[this->Position[0]]->GetId(this->Position[1]);
      if ( this->StoreAttributes == 2 )
        {
          this->IsEdge(p1, p2, ptr);
        }
      else
        {
          ptr = NULL;
        }
      return 1;
      }
    }
  return 0;
}

vtkIdList **vtkEdgeTable::Resize(vtkIdType sz)
{
  vtkIdList **newTableArray;
  vtkIdList **newAttributeArray;
  vtkVoidArray **newPointerAttributeArray;
  vtkIdType newSize, i;
  vtkIdType extend=this->TableSize/2 + 1;

  if (sz >= this->TableSize)
    {
    newSize = this->TableSize + 
              extend*(((sz-this->TableSize)/extend)+1);
    }
  else
    {
    newSize = sz;
    }

  sz = (sz < this->TableSize ? sz : this->TableSize);
  newTableArray = new vtkIdList *[newSize];
  memcpy(newTableArray, this->Table, sz * sizeof(vtkIdList *));
  for (i=sz; i < newSize; i++)
    {
    newTableArray[i] = NULL;
    }
  this->TableSize = newSize;
  delete [] this->Table;
  this->Table = newTableArray;

  if ( this->StoreAttributes == 1 )
    {
    newAttributeArray = new vtkIdList *[newSize];
    memcpy(newAttributeArray, this->Attributes, sz * sizeof(vtkIdList *));
    for (i=sz; i < newSize; i++)
      {
      newAttributeArray[i] = NULL;
      }
    if ( this->Attributes )
      {
      delete [] this->Attributes;
      }
    this->Attributes = newAttributeArray;
    }
  else if ( this->StoreAttributes == 2 )
    {
    newPointerAttributeArray = new vtkVoidArray *[newSize];
    memcpy(newPointerAttributeArray, this->Attributes, sz * sizeof(vtkVoidArray *));
    for (i=sz; i < newSize; i++)
      {
      newPointerAttributeArray[i] = NULL;
      }
    if ( this->PointerAttributes )
      {
      delete [] this->PointerAttributes;
      }
    this->PointerAttributes = newPointerAttributeArray;
    }

  return this->Table;
}

int vtkEdgeTable::InitPointInsertion(vtkPoints *newPts, vtkIdType estSize)
{
  // Initialize
  if ( this->Table )
    {
    this->Initialize();
    }
  if ( newPts == NULL )
    {
    vtkErrorMacro(<<"Must define points for point insertion");
    return 0;
    }
  if (this->Points != NULL)
    {
    this->Points->Delete();
    }
  // Set up the edge insertion
  this->InitEdgeInsertion(estSize,1);

  this->Points = newPts;
  this->Points->Register(this);

  return 1;
}

int vtkEdgeTable::InsertUniquePoint(vtkIdType p1, vtkIdType p2, double x[3],
                                    vtkIdType &ptId)
{
  int loc = this->IsEdge(p1,p2);

  if ( loc != -1 )
    {
    ptId = loc;
    return 0;
    }
  else
    {
    ptId = this->InsertEdge(p1,p2);
    this->Points->InsertPoint(ptId,x);
    return 1;
    }
}

void vtkEdgeTable::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "NumberOfEdges: " << this->GetNumberOfEdges() << "\n";
}
