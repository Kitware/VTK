/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPriorityQueue.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPriorityQueue.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPriorityQueue);

// Instantiate priority queue with default size and extension size of 1000.
vtkPriorityQueue::vtkPriorityQueue()
{
  this->Size = 0;
  this->Extend = 1000;
  this->Array = NULL;
  this->MaxId = -1;
  this->ItemLocation = vtkIdTypeArray::New();
}

// Allocate priority queue with specified size and amount to extend
// queue (if reallocation required).
void vtkPriorityQueue::Allocate(const vtkIdType sz, const vtkIdType ext)
{
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
  this->Array = new vtkPriorityQueue::Item[sz];
  this->Extend = ( ext > 0 ? ext : 1);
  this->MaxId = -1;
}

// Destructor for the vtkPriorityQueue class
vtkPriorityQueue::~vtkPriorityQueue()
{
  this->ItemLocation->Delete();
  if ( this->Array )
    {
    delete [] this->Array;
    }
}

// Insert id with priority specified.
void vtkPriorityQueue::Insert(double priority, vtkIdType id)
{
  vtkIdType i, idx;
  vtkPriorityQueue::Item temp;

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
  double priority;
  return this->Pop(location, priority);
}

// Removes item at specified location from tree; then reorders and
// balances tree. The location == 0 is the root of the tree.
vtkIdType vtkPriorityQueue::Pop(vtkIdType location, double &priority)
{
  vtkIdType id, i, j, idx;
  vtkPriorityQueue::Item temp;

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

  // percolate down the tree from the specified location
  int lastNodeToCheck = (this->MaxId-1)/2;
  for ( j=0, i=location; i <= lastNodeToCheck; i=j )
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
 
  // percolate up the tree from the specified location
  for ( idx=0, i=location; i > 0; i=idx )
    {
    idx = (i-1)/2;

    if ( this->Array[i].priority < this->Array[idx].priority )
      {
      temp = this->Array[i];

      this->ItemLocation->SetValue(temp.id,idx);
      this->Array[i] = this->Array[idx];

      this->ItemLocation->SetValue(this->Array[idx].id,i);
      this->Array[idx] = temp;
      }
    else
      {
      break;
      }
    }

  return id;
}

// Protected method reallocates queue.
vtkPriorityQueue::Item *vtkPriorityQueue::Resize(const vtkIdType sz)
{
  vtkPriorityQueue::Item *newArray;
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

  newArray = new vtkPriorityQueue::Item[newSize];

  if (this->Array)
    {
    memcpy(newArray, this->Array,
           (sz < this->Size ? sz : this->Size) * sizeof(vtkPriorityQueue::Item));
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
 
  for (int i=0; i <= this->ItemLocation->GetMaxId(); i++)
    {
    this->ItemLocation->SetValue(i,-1);
    }
  this->ItemLocation->Reset();
}

void vtkPriorityQueue::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Number Of Entries: " << this->MaxId + 1 << "\n";
  os << indent << "Size: " << this->Size << "\n";
  os << indent << "Extend size: " << this->Extend << "\n";
}

