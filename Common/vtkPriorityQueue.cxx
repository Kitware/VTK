/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPriorityQueue.cxx
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
#include "vtkPriorityQueue.h"
#include "vtkObjectFactory.h"

//------------------------------------------------------------------------
vtkPriorityQueue* vtkPriorityQueue::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPriorityQueue");
  if(ret)
    {
    return (vtkPriorityQueue*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPriorityQueue;
}

// Instantiate priority queue with default size and extension size of 1000.
vtkPriorityQueue::vtkPriorityQueue()
{
  this->Size = 0;
  this->Extend = 1000;
  this->Array = NULL;
  this->MaxId = -1;
  this->ItemLocation = NULL;
}

// Allocate priority queue with specified size and amount to extend
// queue (if reallocation required).
void vtkPriorityQueue::Allocate(const vtkIdType sz, const vtkIdType ext)
{
  this->ItemLocation = vtkIdTypeArray::New();
  this->ItemLocation->Allocate(sz,ext);
  for (vtkIdType i=0; i < sz; i++)
    {
    this->ItemLocation->SetValue(i,-1);
    }

  this->Size = ( sz > 0 ? sz : 1);
  if ( this->Array != NULL )
    {
    delete [] this->Array;
    }
  this->Array = new vtkPriorityItem[sz];
  this->Extend = ( ext > 0 ? ext : 1);
  this->MaxId = -1;
}

// Destructor for the vtkPriorityQueue class
vtkPriorityQueue::~vtkPriorityQueue()
{
  if ( this->ItemLocation )
    {
    this->ItemLocation->Delete();
    }
  if ( this->Array )
    {
    delete [] this->Array;
    }
}

// Insert id with priority specified.
void vtkPriorityQueue::Insert(float priority, vtkIdType id)
{
  vtkIdType i, idx;
  vtkPriorityItem temp;

  // check and make sure item hasn't been inserted before
  if ( id <= this->ItemLocation->GetMaxId() && 
       this->ItemLocation->GetValue(id) != -1 )
    {
    return;
    }

  // start by placing new entry at bottom of tree
  if ( ++this->MaxId >= this->Size )
    {
    this->Resize(this->MaxId + 1);
    }
  this->Array[this->MaxId].priority = priority;
  this->Array[this->MaxId].id = id;
  if ( id >= this->ItemLocation->GetSize() ) //might have to resize and initialize
    {
    vtkIdType oldSize = this->ItemLocation->GetSize();
    this->ItemLocation->InsertValue(id,this->MaxId); 
    for (i=oldSize; i < this->ItemLocation->GetSize(); i++) 
      {
      this->ItemLocation->SetValue(i, -1);
      }
    this->ItemLocation->SetValue(id,this->MaxId);
    }

  this->ItemLocation->InsertValue(id,this->MaxId);

  // now begin percolating towards top of tree
  for ( i=this->MaxId; 
  i > 0 && this->Array[i].priority < this->Array[(idx=(i-1)/2)].priority; 
  i=idx)
    {
    temp = this->Array[i];

    this->ItemLocation->SetValue(temp.id,idx);
    this->Array[i] = this->Array[idx];

    this->ItemLocation->SetValue(this->Array[idx].id,i);
    this->Array[idx] = temp;
    }
}

// Simplified call for easier wrapping for Tcl.
vtkIdType vtkPriorityQueue::Pop(vtkIdType location)
{
  float priority;
  return this->Pop(priority, location);
}

// Removes item at specified location from tree; then reorders and
// balances tree. The location == 0 is the root of the tree.
vtkIdType vtkPriorityQueue::Pop(float &priority, vtkIdType location)
{
  vtkIdType id, i, j, idx;
  vtkPriorityItem temp;

  if ( this->MaxId < 0 )
    {
    return -1;
    }
 
  id = this->Array[location].id;
  priority = this->Array[location].priority;

  // move the last item to the location specified and push into the tree
  this->Array[location].id = this->Array[this->MaxId].id;
  this->Array[location].priority = this->Array[this->MaxId].priority;

  this->ItemLocation->SetValue(this->Array[location].id,location);
  this->ItemLocation->SetValue(id,-1);

  if ( --this->MaxId <= 0 )
    {
    return id;
    }

  // percolate into the tree
  for ( j=0, i=location; i <= (this->MaxId-1)/2; i=j )
    {
    idx = 2*i + 1;

    if ( this->Array[idx].priority < this->Array[idx+1].priority || 
	 idx == this->MaxId )
      {
      j = idx;
      }
    else
      {
      j = idx + 1;
      }

    if ( this->Array[i].priority > this->Array[j].priority )
      {
      temp = this->Array[i];

      this->ItemLocation->SetValue(temp.id,j);
      this->Array[i] = this->Array[j];

      this->ItemLocation->SetValue(this->Array[j].id,i);
      this->Array[j] = temp;
      }
    else
      {
      break;
      }
    }
  
  return id;
}

// Protected method reallocates queue.
vtkPriorityItem *vtkPriorityQueue::Resize(const vtkIdType sz)
{
  vtkPriorityItem *newArray;
  vtkIdType newSize;

  if (sz >= this->Size)
    {
    newSize = this->Size + sz;
    }
  else
    {
    newSize = sz;
    }

  if (newSize <= 0)
    {
    newSize = 1;
    }

  newArray = new vtkPriorityItem[newSize];

  if (this->Array)
    {
    memcpy(newArray, this->Array,
	   (sz < this->Size ? sz : this->Size) * sizeof(vtkPriorityItem));
    delete [] this->Array;
    }

  this->Size = newSize;
  this->Array = newArray;

  return this->Array;
}

// Reset all of the entries in the queue so they don not have a priority
void vtkPriorityQueue::Reset()
{
  this->MaxId = -1;
  if ( this->ItemLocation != NULL )
    {
    for (int i=0; i <= this->ItemLocation->GetMaxId(); i++)
      {
      this->ItemLocation->SetValue(i,-1);
      }
    this->ItemLocation->Reset();
    }
}

void vtkPriorityQueue::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  os << indent << "Number Of Entries: " << this->MaxId + 1 << "\n";
  os << indent << "Size: " << this->Size << "\n";
  os << indent << "Extend size: " << this->Extend << "\n";
}

