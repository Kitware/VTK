/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPriorityQueue.h
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
// .NAME vtkPriorityQueue - an list of ids arranged in priority order
// .SECTION Description
// vtkPriorityQueue is a general object for creating and manipulating lists
// of object ids (e.g., point or cell ids). Object ids are sorted according
// to a user-specified priority, where entries at the top of the queue have
// the smallest values.
//
// This implementation provides a feature beyond the usual ability to insert
// and retrieve (or pop) values from the queue. It is also possible to
// pop any item in the queue given its id number. This allows you to delete
// entries in the queue which can useful for reinserting an item into the
// queue. 
//
// .SECTION Caveats
// This implementation is a variation of the priority queue described in
// "Data Structures & Algorithms" by Aho, Hopcroft, Ullman. It creates 
// a balanced, partially ordered binary tree implemented as an ordered
// array. This avoids the overhead associated with parent/child pointers,
// and frequent memory allocation and deallocation.

#ifndef __vtkPriorityQueue_h
#define __vtkPriorityQueue_h

#include "vtkObject.h"
#include "vtkIdTypeArray.h"

//BTX
typedef struct _vtkPriorityItem
  {
  float priority;
  vtkIdType id;
  } vtkPriorityItem;
//ETX

class VTK_EXPORT vtkPriorityQueue : public vtkObject
{
public:
  // Description:
  // Instantiate priority queue with default size and extension size of 1000.
  static vtkPriorityQueue *New();

  vtkTypeMacro(vtkPriorityQueue,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Allocate initial space for priority queue.
  void Allocate(const vtkIdType sz, const vtkIdType ext=1000);

  // Description:
  // Insert id with priority specified. The id is generally an
  // index like a point id or cell id.
  void Insert(float priority, vtkIdType id);

  // Description:
  // Removes item at specified location from tree; then reorders and
  // balances tree. The location == 0 is the root of the tree. If queue
  // is exhausted, then a value < 0 is returned. (Note: the location
  // is not the same as deleting an id; id is mapped to location.)
  vtkIdType Pop(float &priority, vtkIdType location=0);
  
  // Description:
  // Same as above but simplified for easier wrapping into interpreted
  // languages.
  vtkIdType Pop(vtkIdType location=0);
  
  // Description:
  // Peek into the queue without actually removing anything. Returns the
  // id and the priority.
  vtkIdType Peek(float &priority, vtkIdType location=0);
  
  // Description:
  // Peek into the queue without actually removing anything. Returns the
  // id.
  vtkIdType Peek(vtkIdType location=0);
  
  // Description:
  // Delete entry in queue with specified id. Returns priority value
  // associated with that id; or VTK_LARGE_FLOAT if not in queue.
  float DeleteId(vtkIdType id);

  // Description:
  // Get the priority of an entry in the queue with specified id. Returns
  // priority value of that id or VTK_LARGE_FLOAT if not in queue.
  float GetPriority(vtkIdType id);

  // Description:
  // Return the number of items in this queue.
  vtkIdType GetNumberOfItems() {return this->MaxId+1;};

  // Description:
  // Empty the queue but without releasing memory. This avoids the
  // overhead of memory allocation/deletion.
  void Reset();

protected:
  vtkPriorityQueue();
  ~vtkPriorityQueue();
  vtkPriorityQueue(const vtkPriorityQueue&);
  void operator=(const vtkPriorityQueue&);
  
  vtkPriorityItem *Resize(const vtkIdType sz);

  vtkIdTypeArray *ItemLocation;
  vtkPriorityItem *Array;
  vtkIdType Size;
  vtkIdType MaxId;
  vtkIdType Extend;
};

inline float vtkPriorityQueue::DeleteId(vtkIdType id)
{
  float priority=VTK_LARGE_FLOAT;
  int loc;

  if ( id <= this->ItemLocation->GetMaxId() &&  
  (loc=this->ItemLocation->GetValue(id)) != -1 )
    {
    this->Pop(priority,loc);
    }
  return priority;
}

inline float vtkPriorityQueue::GetPriority(vtkIdType id)
{
  int loc;

  if ( id <= this->ItemLocation->GetMaxId() &&  
  (loc=this->ItemLocation->GetValue(id)) != -1 )
    {
    return this->Array[loc].priority;
    }
  return VTK_LARGE_FLOAT;
}

inline vtkIdType vtkPriorityQueue::Peek(float &priority, vtkIdType location)
{
  if ( this->MaxId < 0 )
    {
    return -1;
    }
  else
    {
    priority = this->Array[location].priority;
    return this->Array[location].id;
    }
}

inline vtkIdType vtkPriorityQueue::Peek(vtkIdType location)
{
  if ( this->MaxId < 0 )
    {
    return -1;
    }
  else
    {
    return this->Array[location].id;
    }
}

#endif
