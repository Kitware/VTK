/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPriorityQueue.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkPriorityQueue.h"

// Description:
// Instantiate priority queue with default size and extension size of 1000.
vtkPriorityQueue::vtkPriorityQueue()
{
  this->Size = this->Extend = 1000;
  this->Array = new vtkPriorityItem[this->Size];
  this->MaxId = -1;

  this->ItemLocation.Allocate(this->Size,this->Extend);
  for (int i=0; i < this->Size; i++) this->ItemLocation.SetValue(i,-1);
}

// Description:
// Instantiate priority queue with specified size and amount to extend
// queue (if reallocation required).
vtkPriorityQueue::vtkPriorityQueue(const int sz, const int ext) :
ItemLocation(sz,ext)
{
  for (int i=0; i < sz; i++) this->ItemLocation.SetValue(i,-1);

  this->Size = ( sz > 0 ? sz : 1);
  this->Array = new vtkPriorityItem[sz];
  this->Extend = ( ext > 0 ? ext : 1);
  this->MaxId = -1;
}

vtkPriorityQueue::~vtkPriorityQueue(const int sz, const int ext)
{
  if ( this->Array != NULL ) delete [] this->Array;
}

// Description:
// Insert id with priority specified.
void vtkPriorityQueue::Insert(float priority, int id)
{
  int i, idx;
  static vtkPriorityItem temp;

  // check and make sure item hasn't been inserted before
  if ( id <= this->ItemLocation.GetMaxId() && 
  this->ItemLocation.GetValue(id) != -1 ) 
    return;

  // start by placing new entry at bottom of tree
  if ( ++this->MaxId >= this->Size ) this->Resize(this->MaxId);
  this->Array[this->MaxId].priority = priority;
  this->Array[this->MaxId].id = id;
  if ( id >= this->ItemLocation.GetSize() ) //might have to resize and initialize
    {
    int oldSize = this->ItemLocation.GetSize();
    this->ItemLocation.InsertValue(id,this->MaxId); 
    for (i=oldSize; i < this->ItemLocation.GetSize(); i++) 
      this->ItemLocation.SetValue(i, -1);
    this->ItemLocation.SetValue(id,this->MaxId);
    }

  this->ItemLocation.InsertValue(id,this->MaxId);

  // now begin percolating towards top of tree
  for ( i=this->MaxId; 
  i > 0 && this->Array[i].priority < this->Array[(idx=(i-1)/2)].priority; 
  i=idx)
    {
    temp = this->Array[i];

    this->ItemLocation.SetValue(temp.id,idx);
    this->Array[i] = this->Array[idx];

    this->ItemLocation.SetValue(this->Array[idx].id,i);
    this->Array[idx] = temp;
    }
}
// Description:
// Removes item at specified location from tree; then reorders and
// balances tree. The location == 0 is the root of the tree.
int vtkPriorityQueue::Pop(float &priority, int location)
{
  int id, i, j, idx;
  static vtkPriorityItem temp;

  if ( this->MaxId < 0 ) return -1;
 
  id = this->Array[location].id;
  priority = this->Array[location].priority;

  // move the last item to the location specified and push into the tree
  this->Array[location].id = this->Array[this->MaxId].id;
  this->Array[location].priority = this->Array[this->MaxId].priority;
  this->ItemLocation.SetValue(this->Array[location].id,location);

  if ( --this->MaxId <= 0 ) return id;
  this->ItemLocation.SetValue(id,-1);

  // percolate into the tree
  for ( i=location; i <= (this->MaxId-1)/2; i=j )
    {
    idx = 2*i + 1;

    if ( this->Array[idx].priority < this->Array[idx+1].priority || 
    idx == this->MaxId )
        j = idx;
    else
        j = idx + 1;

    if ( this->Array[i].priority > this->Array[j].priority )
      {
      temp = this->Array[i];

      this->ItemLocation.SetValue(temp.id,j);
      this->Array[i] = this->Array[j];

      this->ItemLocation.SetValue(this->Array[j].id,i);
      this->Array[j] = temp;
      }
    else break;
    }
  
  return id;
}

// Protected method reallocates queue.
vtkPriorityItem *vtkPriorityQueue::Resize(const int sz)
{
  vtkPriorityItem *newArray;
  int newSize;

  if (sz >= this->Size) newSize = this->Size + 
    this->Extend*(((sz-this->Size)/this->Extend)+1);
  else newSize = sz;

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

void vtkPriorityQueue::Reset()
{
  this->MaxId = -1;
  for (int i=0; i < this->ItemLocation.GetSize(); i++)
    this->ItemLocation.SetValue(i,-1);
  this->ItemLocation.Reset();
}

void vtkPriorityQueue::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  os << indent << "Number Of Entries: " << this->MaxId + 1 << "\n";
  os << indent << "Size: " << this->Size << "\n";
  os << indent << "Extend size: " << this->Extend << "\n";
}

