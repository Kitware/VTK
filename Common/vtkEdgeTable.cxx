/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEdgeTable.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkEdgeTable.h"
#include "vtkPoints.h"
#include "vtkIdList.h"
#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
vtkEdgeTable* vtkEdgeTable::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkEdgeTable");
  if(ret)
    {
    return (vtkEdgeTable*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkEdgeTable;
}




// Instantiate object based on maximum point id.
vtkEdgeTable::vtkEdgeTable()
{
  this->Table = NULL;
  this->Attributes = NULL;
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

    if ( this->StoreAttributes )
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
    }//if table defined

    if ( this->Points )
    {
    this->Points->Delete();
    this->Points = NULL;
    }
    
  this->TableSize = 0;
  this->NumberOfEdges = 0;
  this->StoreAttributes = 0;
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

    if ( this->StoreAttributes )
      {
      for (i=0; i < this->TableSize; i++)
        {
        if ( this->Attributes[i] )
          {
          this->Attributes[i]->Reset();
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
  this->StoreAttributes = 0;
}

vtkEdgeTable::~vtkEdgeTable()
{
  this->Initialize();
}

int vtkEdgeTable::InitEdgeInsertion(vtkIdType numPoints, int storeAttributes)
{
  vtkIdType i;
  
  if ( numPoints < 1 )
    {
    numPoints = 1;
    }

  // Discard old memory if not enough has benn previously allocated
  if ( numPoints > this->TableSize )
    {
    this->Initialize();
    this->Table = new vtkIdList *[numPoints];
    for (i=0; i < numPoints; i++)
      {
      this->Table[i] = NULL;
      }

    this->StoreAttributes = storeAttributes;
    if ( this->StoreAttributes )
      {
      this->Attributes = new vtkIdList *[numPoints];
      for (i=0; i < numPoints; i++)
        {
        this->Attributes[i] = NULL;
        }
      }
    this->TableMaxId = -1;
    this->TableSize = numPoints;
    }
  
  // Otherwise, reuse the old memory
  else
    {
    this->Reset();
    this->StoreAttributes = storeAttributes;
    this->TableMaxId = -1;
    }

  this->Position[0] = 0;
  this->Position[1] = -1;

  this->NumberOfEdges = 0;

  return 1;
}

// Return non-negative if edge (p1,p2) is an edge; otherwise -1.
int vtkEdgeTable::IsEdge(vtkIdType p1, vtkIdType p2)
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
      if ( this->StoreAttributes )
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
    if ( this->StoreAttributes )
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
  if ( this->StoreAttributes )
    {
    this->Attributes[index]->InsertNextId(this->NumberOfEdges);
    }
  this->NumberOfEdges++;

  return (this->NumberOfEdges - 1);
}

void vtkEdgeTable::InsertEdge(vtkIdType p1, vtkIdType p2, int attributeId)
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
    if ( this->StoreAttributes )
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


// Intialize traversal of edges in table.
void vtkEdgeTable::InitTraversal()
{
  this->Position[0] = 0;
  this->Position[1] = -1;
}

// Traverse list of edges in table. Return the edge as (p1,p2), where p1 and p2
// are point id's. Method return value is non-negative if list is exhausted; 
// >= 0 otherwise. The value of p1 is guaranteed to be <= p2. The return value
// is an id that can be used for accessing attributes.
int vtkEdgeTable::GetNextEdge(vtkIdType &p1, vtkIdType &p2)
{
  for ( ; this->Position[0] <= this->TableMaxId; 
  this->Position[0]++, this->Position[1]=(-1) )
    {
    if ( this->Table[this->Position[0]] != NULL && 
    ++this->Position[1] < this->Table[this->Position[0]]->GetNumberOfIds() )
      {
      p1 = this->Position[0];
      p2 = this->Table[this->Position[0]]->GetId(this->Position[1]);
      return this->Attributes[this->Position[0]]->GetId(this->Position[1]);
      }
    }

  return (-1);
}

vtkIdList **vtkEdgeTable::Resize(vtkIdType sz)
{
  vtkIdList **newTableArray;
  vtkIdList **newAttributeArray;
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

  if ( this->StoreAttributes )
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

int vtkEdgeTable::InsertUniquePoint(vtkIdType p1, vtkIdType p2, float x[3],
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
  vtkObject::PrintSelf(os,indent);

  os << indent << "NumberOfEdges: " << this->GetNumberOfEdges() << "\n";
}
