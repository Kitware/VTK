/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIdList.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkIdList
 * @brief   list of point or cell ids
 *
 * vtkIdList is used to represent and pass data id's between
 * objects. vtkIdList may represent any type of integer id, but
 * usually represents point and cell ids.
*/

#ifndef vtkIdList_h
#define vtkIdList_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"

class VTKCOMMONCORE_EXPORT vtkIdList : public vtkObject
{
public:
  static vtkIdList *New();

  void Initialize();

  /**
   * Allocate a capacity for sz ids in the list and
   * set the number of stored ids in the list to 0.
   * strategy is not used.
   */
  int Allocate(const vtkIdType sz, const int strategy=0);

  vtkTypeMacro(vtkIdList,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Return the number of id's in the list.
   */
  vtkIdType GetNumberOfIds() {return this->NumberOfIds;};

  /**
   * Return the id at location i.
   */
  vtkIdType GetId(const vtkIdType i) {return this->Ids[i];};

  /**
   * Specify the number of ids for this object to hold. Does an
   * allocation as well as setting the number of ids.
   */
  void SetNumberOfIds(const vtkIdType number);

  /**
   * Set the id at location i. Doesn't do range checking so it's a bit
   * faster than InsertId. Make sure you use SetNumberOfIds() to allocate
   * memory prior to using SetId().
   */
  void SetId(const vtkIdType i, const vtkIdType vtkid) {this->Ids[i] = vtkid;};

  /**
   * Set the id at location i. Does range checking and allocates memory
   * as necessary.
   */
  void InsertId(const vtkIdType i, const vtkIdType vtkid);

  /**
   * Add the id specified to the end of the list. Range checking is performed.
   */
  vtkIdType InsertNextId(const vtkIdType vtkid);

  /**
   * If id is not already in list, insert it and return location in
   * list. Otherwise return just location in list.
   */
  vtkIdType InsertUniqueId(const vtkIdType vtkid);

  /**
   * Get a pointer to a particular data index.
   */
  vtkIdType *GetPointer(const vtkIdType i) {return this->Ids + i;};

  /**
   * Get a pointer to a particular data index. Make sure data is allocated
   * for the number of items requested. Set MaxId according to the number of
   * data values requested.
   */
  vtkIdType *WritePointer(const vtkIdType i, const vtkIdType number);

  /**
   * Specify an array of vtkIdType to use as the id list. This replaces the
   * underlying array. This instance of vtkIdList takes ownership of the
   * array, meaning that it deletes it on destruction (using delete[]).
   */
  void SetArray(vtkIdType *array, vtkIdType size);

  /**
   * Reset to an empty state.
   */
  void Reset() {this->NumberOfIds = 0;};

  /**
   * Free any unused memory.
   */
  void Squeeze() {this->Resize(this->NumberOfIds);};

  /**
   * Copy an id list by explicitly copying the internal array.
   */
  void DeepCopy(vtkIdList *ids);

  /**
   * Delete specified id from list. Will remove all occurrences of id in list.
   */
  void DeleteId(vtkIdType vtkid);

  /**
   * Return -1 if id specified is not contained in the list; otherwise return
   * the position in the list.
   */
  vtkIdType IsId(vtkIdType vtkid);

  /**
   * Intersect this list with another vtkIdList. Updates current list according
   * to result of intersection operation.
   */
  void IntersectWith(vtkIdList* otherIds);

  /**
   * Adjust the size of the id list while maintaining its content (except
   * when being truncated).
   */
  vtkIdType *Resize(const vtkIdType sz);

  // This method should become legacy
  void IntersectWith(vtkIdList& otherIds) {
    this->IntersectWith(&otherIds); };

protected:
  vtkIdList();
  ~vtkIdList() VTK_OVERRIDE;

  vtkIdType NumberOfIds;
  vtkIdType Size;
  vtkIdType *Ids;

private:
  vtkIdList(const vtkIdList&) VTK_DELETE_FUNCTION;
  void operator=(const vtkIdList&) VTK_DELETE_FUNCTION;
};

// In-lined for performance
inline void vtkIdList::InsertId(const vtkIdType i, const vtkIdType vtkid)
{
  if (i >= this->Size)
  {
    this->Resize(i + 1);
  }
  this->Ids[i] = vtkid;
  if (i >= this->NumberOfIds)
  {
    this->NumberOfIds = i + 1;
  }
}

// In-lined for performance
inline vtkIdType vtkIdList::InsertNextId(const vtkIdType vtkid)
{
  if ( this->NumberOfIds >= this->Size )
  {
    if (!this->Resize(2*this->NumberOfIds+1)) //grow by factor of 2
    {
      return this->NumberOfIds-1;
    }
  }
  this->Ids[this->NumberOfIds++] = vtkid;
  return this->NumberOfIds-1;
}

inline vtkIdType vtkIdList::IsId(vtkIdType vtkid)
{
  vtkIdType *ptr, i;
  for (ptr=this->Ids, i=0; i<this->NumberOfIds; i++, ptr++)
  {
    if ( vtkid == *ptr )
    {
      return i;
    }
  }
  return (-1);
}

#endif
