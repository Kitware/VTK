/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPriorityQueue.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPriorityQueue - a list of ids arranged in priority order
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

#ifndef vtkPriorityQueue_h
#define vtkPriorityQueue_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"

#include "vtkIdTypeArray.h" // Needed for inline methods

class VTKCOMMONCORE_EXPORT vtkPriorityQueue : public vtkObject
{
public:
  //BTX
  class Item
  {
  public:
    double priority;
    vtkIdType id;
  };
  //ETX

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
  void Insert(double priority, vtkIdType id);

//BTX
  // Description:
  // Removes item at specified location from tree; then reorders and
  // balances tree. The location == 0 is the root of the tree. If queue
  // is exhausted, then a value < 0 is returned. (Note: the location
  // is not the same as deleting an id; id is mapped to location.)
  vtkIdType Pop(vtkIdType location, double &priority);
//ETX

  // Description:
  // Same as above but simplified for easier wrapping into interpreted
  // languages.
  vtkIdType Pop(vtkIdType location=0);

//BTX
  // Description:
  // Peek into the queue without actually removing anything. Returns the
  // id and the priority.
  vtkIdType Peek(vtkIdType location, double &priority);
//ETX

  // Description:
  // Peek into the queue without actually removing anything. Returns the
  // id.
  vtkIdType Peek(vtkIdType location=0);

  // Description:
  // Delete entry in queue with specified id. Returns priority value
  // associated with that id; or VTK_DOUBLE_MAX if not in queue.
  double DeleteId(vtkIdType id);

  // Description:
  // Get the priority of an entry in the queue with specified id. Returns
  // priority value of that id or VTK_DOUBLE_MAX if not in queue.
  double GetPriority(vtkIdType id);

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

  Item *Resize(const vtkIdType sz);

  vtkIdTypeArray *ItemLocation;
  Item *Array;
  vtkIdType Size;
  vtkIdType MaxId;
  vtkIdType Extend;
private:
  vtkPriorityQueue(const vtkPriorityQueue&);  // Not implemented.
  void operator=(const vtkPriorityQueue&);  // Not implemented.
};

inline double vtkPriorityQueue::DeleteId(vtkIdType id)
{
  double priority=VTK_DOUBLE_MAX;
  int loc;

  if ( id <= this->ItemLocation->GetMaxId() &&
  (loc=this->ItemLocation->GetValue(id)) != -1 )
    {
    this->Pop(loc,priority);
    }
  return priority;
}

inline double vtkPriorityQueue::GetPriority(vtkIdType id)
{
  int loc;

  if ( id <= this->ItemLocation->GetMaxId() &&
  (loc=this->ItemLocation->GetValue(id)) != -1 )
    {
    return this->Array[loc].priority;
    }
  return VTK_DOUBLE_MAX;
}

inline vtkIdType vtkPriorityQueue::Peek(vtkIdType location, double &priority)
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
