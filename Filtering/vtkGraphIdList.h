/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphIdList.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGraphIdList - read-only list of vertex or edge ids
//
// .SECTION Description
// vtkGraphIdList is used to represent and pass data id's between
// objects. vtkGraphIdList may represent any type of integer id, but
// usually represents vertex and edge ids.
// vtkGraphIdList provides read-only access to all classes except
// for vtkGraph, vtkTree, which are responsible for creating instances
// of this class.
//
// vtkGraphIdList provides two modes for creating lists:
// (1) Fill a list from scratch using InsertNextId.  Edits on the list
//     perform normally in this case.
// (2) Set a list array pointer directly using SetArray.  If the "save"
//     option is set, the data which the pointer points to is never allowed
//     to be modified or deleted.  If the "save" flag is set, and a
//     edit is made on the list, the entire list is copied before the change.

#ifndef __vtkGraphIdList_h
#define __vtkGraphIdList_h

#include "vtkObject.h"

class VTK_FILTERING_EXPORT vtkGraphIdList : public vtkObject
{
public:
  static vtkGraphIdList *New();

  void Initialize();
  int Allocate(const int sz, const int strategy=0);
  vtkTypeRevisionMacro(vtkGraphIdList,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return the number of id's in the list.
  vtkIdType GetNumberOfIds() {return this->NumberOfIds;};
  
  // Description:
  // Return the id at location i.
  vtkIdType GetId(const int i) {return this->Ids[i];};

  // Description:
  // Return -1 if id specified is not contained in the list; otherwise return
  // the position in the list.
  vtkIdType IsId(vtkIdType id);

  // Description:
  // Get a pointer to a particular data index.
  const vtkIdType *GetPointer(const vtkIdType i) {return this->Ids + i;};

protected:
  // Description:
  // Specify the number of ids for this object to hold. Does an
  // allocation as well as setting the number of ids.
  void SetNumberOfIds(const vtkIdType number);

  // Description:
  // Set the id at location i. Doesn't do range checking so it's a bit
  // faster than InsertId. Make sure you use SetNumberOfIds() to allocate
  // memory prior to using SetId().
  void SetId(const vtkIdType i, const vtkIdType id);

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
  // Set a pointer to the data.  Indicate whether to save the array.
  void SetArray(vtkIdType* ids, vtkIdType size, bool save);

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
  void DeepCopy(vtkGraphIdList *ids);

  // Description:
  // Delete specified id from list. Will remove all occurrences of id in list.
  void DeleteId(vtkIdType id);

  // Description:
  // Intersect this list with another vtkGraphIdList. Updates current list according
  // to result of intersection operation.
  void IntersectWith(vtkGraphIdList& otherIds);

  vtkGraphIdList();
  ~vtkGraphIdList();

  vtkIdType NumberOfIds;
  vtkIdType Size; 
  vtkIdType *Ids;
  vtkIdType SaveUserArray;

  vtkIdType *Resize(const vtkIdType sz);
  void CopyArray();

  //BTX
  friend class vtkAbstractGraph;
  friend class vtkGraph;
  friend class vtkTree;
  //ETX

private:
  vtkGraphIdList(const vtkGraphIdList&);  // Not implemented.
  void operator=(const vtkGraphIdList&);  // Not implemented.
};

// In-lined for performance
inline vtkIdType vtkGraphIdList::InsertNextId(const vtkIdType id)
{
  if (this->SaveUserArray)
    {
    this->CopyArray();
    }

  if ( this->NumberOfIds >= this->Size )
    {
    this->Resize(this->NumberOfIds+1);
    }
  this->Ids[this->NumberOfIds++] = id;
  return this->NumberOfIds-1;
}

inline vtkIdType vtkGraphIdList::IsId(vtkIdType id)
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
