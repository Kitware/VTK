/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEdgeTable.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


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
  int i;
  
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
    this->TableSize = 0;

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
    
  this->NumberOfEdges = 0;
  this->StoreAttributes = 0;
}

// Free memory and return to instantiated state.
void vtkEdgeTable::Reset()
{
  int i;

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

int vtkEdgeTable::InitEdgeInsertion(int numPoints, int storeAttributes)
{
  int i;
  
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
int vtkEdgeTable::IsEdge(int p1, int p2)
{
  int index, search;

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
    int loc;
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
int vtkEdgeTable::InsertEdge(int p1, int p2)
{
  int index, search;

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

  this->Table[index]->InsertNextId(search);
  if ( this->StoreAttributes )
    {
    this->Attributes[index]->InsertNextId(this->NumberOfEdges);
    }
  this->NumberOfEdges++;

  return (this->NumberOfEdges - 1);
}

void vtkEdgeTable::InsertEdge(int p1, int p2, int attributeId)
{
  int index, search;

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
int vtkEdgeTable::GetNextEdge(int &p1, int &p2)
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

vtkIdList **vtkEdgeTable::Resize(int sz)
{
  vtkIdList **newTableArray;
  vtkIdList **newAttributeArray;
  int newSize, i;
  int extend=this->TableSize/2 + 1;

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

int vtkEdgeTable::InitPointInsertion(vtkPoints *newPts, int estSize)
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
  this->Points = newPts;
  this->Points->Register(this);

  // Set up the edge insertion
  this->InitEdgeInsertion(estSize,1);

  return 1;
}

int vtkEdgeTable::InsertUniquePoint(int p1, int p2, float x[3], int &ptId)
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
