/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIdList.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkIdList - list of point or cell ids
// .SECTION Description
// vtkIdList is used to represent and pass data id's between
// objects. vtkIdList may represent any type of integer id, but
// usually represents point and cell ids.

#ifndef __vtkIdList_h
#define __vtkIdList_h

#include "vtkObject.h"

class VTK_COMMON_EXPORT vtkIdList : public vtkObject
{
public:
  static vtkIdList *New();

  void Initialize();
  int Allocate(const int sz, const int strategy=0);
  vtkTypeRevisionMacro(vtkIdList,vtkObject);
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

  vtkIdType NumberOfIds;
  vtkIdType Size; 
  vtkIdType *Ids;

  vtkIdType *Resize(const vtkIdType sz);
private:
  vtkIdList(const vtkIdList&);  // Not implemented.
  void operator=(const vtkIdList&);  // Not implemented.
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
