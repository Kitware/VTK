/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIdList.h
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
// .NAME vtkIdList - list of point or cell ids
// .SECTION Description
// vtkIdList is used to represent and pass data id's between
// objects. vtkIdList may represent any type of integer id, but
// usually represents point and cell ids.

#ifndef __vtkIdList_h
#define __vtkIdList_h

#include "vtkObject.h"

class VTK_EXPORT vtkIdList : public vtkObject
{
public:
  static vtkIdList *New();

  void Initialize();
  int Allocate(const int sz, const int strategy=0);
  vtkTypeMacro(vtkIdList,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return the number of id's in the list.
  vtkIdType GetNumberOfIds() {return this->NumberOfIds;};
  
  // Description:
  // Return the id at location i.
  vtkIdType GetId(const int i) {return this->Ids[i];};
  
  // Description:
  // Specify the number of ids for this object to hold. Does an
  // allocation as well as setting the number of ids.
  void SetNumberOfIds(const vtkIdType number);

  // Description:
  // Set the id at location i. Doesn't do range checking so it's a bit
  // faster than InsertId. Make sure you use SetNumberOfIds() to allocate
  // memory prior to using SetId().
  void SetId(const vtkIdType i, const vtkIdType id) {this->Ids[i] = id;};

  // Description:
  // Set the id at location i. Does range checking and allocates memory
  // as necessary.
  void InsertId(const vtkIdType i, const vtkIdType id);

  // Description:
  // Add the id specified to the end of the list. Range checking is performed.
  vtkIdType InsertNextId(const vtkIdType id);

  // Description:
  // If id is not already in list, insert it and return location in
  // list. Otherwise return just location in list.
  vtkIdType InsertUniqueId(const vtkIdType id);

  // Description:
  // Get a pointer to a particular data index.
  vtkIdType *GetPointer(const vtkIdType i) {return this->Ids + i;};

  // Description:
  // Get a pointer to a particular data index. Make sure data is allocated
  // for the number of items requested. Set MaxId according to the number of
  // data values requested.
  vtkIdType *WritePointer(const vtkIdType i, const vtkIdType number);

  // Description:
  // Reset to an empty state.
  void Reset() {this->NumberOfIds = 0;};

  // Description:
  // Free any unused memory.
  void Squeeze() {this->Resize(this->NumberOfIds);};

  // Description:
  // Copy an id list by explicitly copying the internal array.
  void DeepCopy(vtkIdList *ids);

  // Description:
  // Delete specified id from list. Will remove all occurrences of id in list.
  void DeleteId(vtkIdType id);

  // Description:
  // Return -1 if id specified is not contained in the list; otherwise return
  // the position in the list.
  vtkIdType IsId(vtkIdType id);

  // Description:
  // Intersect this list with another vtkIdList. Updates current list according
  // to result of intersection operation.
  void IntersectWith(vtkIdList& otherIds);

protected:
  vtkIdList();
  ~vtkIdList();
  vtkIdList(const vtkIdList&) {};
  void operator=(const vtkIdList&) {};

  vtkIdType NumberOfIds;
  vtkIdType Size; 
  vtkIdType *Ids;

  vtkIdType *Resize(const vtkIdType sz);
};

// In-lined for performance
inline vtkIdType vtkIdList::InsertNextId(const vtkIdType id)
{
  if ( this->NumberOfIds >= this->Size )
    {
    this->Resize(this->NumberOfIds+1);
    }
  this->Ids[this->NumberOfIds++] = id;
  return this->NumberOfIds-1;
}

inline vtkIdType vtkIdList::IsId(vtkIdType id)
{
  vtkIdType *ptr, i;
  for (ptr=this->Ids, i=0; i<this->NumberOfIds; i++, ptr++)
    {
    if ( id == *ptr )
      {
      return i;
      }
    }
  return (-1);
}

#endif
