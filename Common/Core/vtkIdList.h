// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCORE_EXPORT VTK_MARSHALAUTO vtkIdList : public vtkObject
{
public:
  ///@{
  /**
   * Standard methods for instantiation, type information, and printing.
   */
  static vtkIdList* New();
  vtkTypeMacro(vtkIdList, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Release memory and restore to unallocated state.
   */
  void Initialize();

  /**
   * Allocate a capacity for sz ids in the list and
   * set the number of stored ids in the list to 0.
   * strategy is not used.
   */
  int Allocate(vtkIdType sz, int strategy = 0);

  /**
   * Return the number of id's in the list.
   */
  vtkIdType GetNumberOfIds() const noexcept { return this->NumberOfIds; }

  /**
   * Return the id at location i.
   */
  vtkIdType GetId(vtkIdType i) VTK_EXPECTS(0 <= i && i < GetNumberOfIds()) { return this->Ids[i]; }

  /**
   * Find the location i of the provided id.
   */
  vtkIdType FindIdLocation(const vtkIdType id)
  {
    for (int i = 0; i < this->NumberOfIds; i++)
      if (this->Ids[i] == id)
        return i;
    return -1;
  }

  /**
   * Specify the number of ids for this object to hold. Does an
   * allocation as well as setting the number of ids.
   */
  void SetNumberOfIds(vtkIdType number);

  /**
   * Set the id at location i. Doesn't do range checking so it's a bit
   * faster than InsertId. Make sure you use SetNumberOfIds() to allocate
   * memory prior to using SetId().
   */
  void SetId(vtkIdType i, vtkIdType vtkid) VTK_EXPECTS(0 <= i && i < GetNumberOfIds())
  {
    this->Ids[i] = vtkid;
  }

  /**
   * Set the id at location i. Does range checking and allocates memory
   * as necessary.
   */
  void InsertId(vtkIdType i, vtkIdType vtkid) VTK_EXPECTS(0 <= i);

  /**
   * Add the id specified to the end of the list. Range checking is performed.
   */
  vtkIdType InsertNextId(vtkIdType vtkid);

  /**
   * If id is not already in list, insert it and return location in
   * list. Otherwise return just location in list.
   */
  vtkIdType InsertUniqueId(vtkIdType vtkid);

  /**
   * Sort the ids in the list in ascending id order. This method uses
   * vtkSMPTools::Sort() so it can be sped up if built properly.
   */
  void Sort();

  /**
   * Fill the ids with the input value. This method uses
   * vtkSMPTools::Fill() so it can be sped up if built properly.
   */
  void Fill(vtkIdType value);

  /**
   * Get a pointer to a particular data index.
   */
  vtkIdType* GetPointer(vtkIdType i) { return this->Ids + i; }

  /**
   * Get a pointer to a particular data index. Make sure data is allocated
   * for the number of items requested. Set MaxId according to the number of
   * data values requested.
   */
  vtkIdType* WritePointer(vtkIdType i, vtkIdType number);

  /**
   * Specify an array of vtkIdType to use as the id list. This replaces the
   * underlying array. This instance of vtkIdList takes ownership of the
   * array, meaning that it deletes it on destruction (using delete[]).
   */
  void SetArray(vtkIdType* array, vtkIdType size, bool save = true);

  /**
   * Reset to an empty state but retain previously allocated memory.
   */
  void Reset() { this->NumberOfIds = 0; }

  /**
   * Free any unused memory.
   */
  void Squeeze() { this->Resize(this->NumberOfIds); }

  /**
   * Copy an id list by explicitly copying the internal array.
   */
  void DeepCopy(vtkIdList* ids);

  /**
   * Delete specified id from list. Will remove all occurrences of id in list.
   */
  void DeleteId(vtkIdType vtkid);

  /**
   * Return -1 if id specified is not contained in the list; otherwise return
   * the position in the list.
   */
  vtkIdType IsId(vtkIdType vtkid) VTK_FUTURE_CONST;

  /**
   * Intersect this list with another vtkIdList. Updates current list according
   * to result of intersection operation.
   */
  void IntersectWith(vtkIdList* otherIds);

  /**
   * Adjust the size of the id list while maintaining its content (except
   * when being truncated).
   */
  vtkIdType* Resize(vtkIdType sz);

#ifndef __VTK_WRAP__
  /**
   * This releases the ownership of the internal vtkIdType array and returns the
   * pointer to it. The caller is responsible of calling `delete []` on the
   * returned value. This vtkIdList will be set to initialized state after this
   * call.
   */
  vtkIdType* Release();
#endif

  ///@{
  /**
   * To support range-based `for` loops
   */
  vtkIdType* begin() { return this->Ids; }
  vtkIdType* end() { return this->Ids + this->NumberOfIds; }
  const vtkIdType* begin() const { return this->Ids; }
  const vtkIdType* end() const { return this->Ids + this->NumberOfIds; }
  ///@}
protected:
  vtkIdList();
  ~vtkIdList() override;

  /**
   * Allocate ids and set the number of ids.
   */
  bool AllocateInternal(vtkIdType sz, vtkIdType numberOfIds);
  /**
   * Release memory.
   */
  void InitializeMemory();

  vtkIdType NumberOfIds;
  vtkIdType Size;
  vtkIdType* Ids;
  bool ManageMemory;

private:
  vtkIdList(const vtkIdList&) = delete;
  void operator=(const vtkIdList&) = delete;
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
  if (this->NumberOfIds >= this->Size)
  {
    if (!this->Resize(2 * this->NumberOfIds + 1)) // grow by factor of 2
    {
      return this->NumberOfIds - 1;
    }
  }
  this->Ids[this->NumberOfIds++] = vtkid;
  return this->NumberOfIds - 1;
}

inline vtkIdType vtkIdList::IsId(vtkIdType vtkid) VTK_FUTURE_CONST
{
  vtkIdType *ptr, i;
  for (ptr = this->Ids, i = 0; i < this->NumberOfIds; i++, ptr++)
  {
    if (vtkid == *ptr)
    {
      return i;
    }
  }
  return (-1);
}

VTK_ABI_NAMESPACE_END
#endif
