/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPriorityQueue.h
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
// .NAME vtkPriorityQueue - an list of ids arranged in priority order
// .SECTION Description
// vtkPriorityQueue is a general object for creating and manipulating lists
// of object ids (e.g., point or cell ids). Object ids are sorted acccording
// to a user-specified priority, where entries at the top of the queue have
// the smallest values.
//
// This implementation provides a feature beyond the usual ability to insert
// and retrieve (or pop) values from the queue. It is also possible to
// pop any item in the queue given its id number. This allows you to delete
// entries in the queue which can useful for reinserting an item into the
// queue. 
// .SECTION Caveats
// This implementation is a variation of the priority queue described in
// "Data Structures & Algorithms" by Aho, Hopcroft, Ullman. It creates 
// a balanced, partially ordered binary tree implemented as an ordered
// array. This avoids the overhead associated with parent/child pointers,
// and frequent memory allocation and deallocation.

#ifndef __vtkPriorityQueue_h
#define __vtkPriorityQueue_h

#include "vtkIntArray.h"

typedef struct _vtkPriorityItem
  {
  float priority;
  int id;
  } vtkPriorityItem;

class VTK_EXPORT vtkPriorityQueue : public vtkObject
{
public:
  vtkPriorityQueue();
  vtkPriorityQueue(const int sz, const int ext=1000);
  ~vtkPriorityQueue();
  void PrintSelf(ostream& os, vtkIndent indent);

  int Pop(float &priority, int location=0);
  float Delete(int id);
  float GetPriority(int id);
  void Insert(float priority, int id);
  int GetNumberOfItems() {return this->MaxId+1;};

  void Reset();

protected:
  vtkPriorityItem *Resize(const int sz);

  vtkIntArray *ItemLocation;
  vtkPriorityItem *Array;
  int Size;
  int MaxId;
  int Extend;
};

// Description:
// Delete entry in queue with specified id. Returns priority value
// associated with that id; or VTK_LARGE_FLOAT if not in queue.
inline float vtkPriorityQueue::Delete(int id)
{
  float priority=VTK_LARGE_FLOAT;
  int loc;

  if ( id <= this->ItemLocation->GetMaxId() &&  
  (loc=this->ItemLocation->GetValue(id)) != -1 )
    {
    this->Pop(priority,loc);
    }
  return priority;
};

// Description:
// Get the priority of an entry in the queue with specified id. Returns priority
// value of that id or VTK_LARGE_FLOAT if not in queue.
inline float vtkPriorityQueue::GetPriority(int id)
{
  int loc;

  if ( id <= this->ItemLocation->GetMaxId() &&  
  (loc=this->ItemLocation->GetValue(id)) != -1 )
    {
    return this->Array[loc].priority;
    }
  return VTK_LARGE_FLOAT;
};

#endif
