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

#include "vtkAbstractArray.h"    // For vtkAbstractArray::DeleteMethod
#include "vtkBuffer.h"           // For vtkBuffer
#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCORE_EXPORT VTK_MARSHALAUTO vtkIdList : public vtkObject
{
public:
  enum DeleteMethod
  {
    VTK_DATA_ARRAY_FREE = vtkAbstractArray::VTK_DATA_ARRAY_FREE,
    VTK_DATA_ARRAY_DELETE = vtkAbstractArray::VTK_DATA_ARRAY_DELETE,
    VTK_DATA_ARRAY_ALIGNED_FREE = vtkAbstractArray::VTK_DATA_ARRAY_ALIGNED_FREE,
    VTK_DATA_ARRAY_USER_DEFINED = vtkAbstractArray::VTK_DATA_ARRAY_USER_DEFINED
  };

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
   * Allocate memory for this id list. Delete old storage only if necessary.
   * Note that strategy is no longer used.
   * This method will reset NumberOfIds to 0 and change the id list's capacity such that
   * this->Capacity >= size.
   * If size is 0, all memory will be freed.
   * Return 1 on success, 0 on failure.
   */
  VTK_DEPRECATED_IN_9_7_0("Use Reserve() to allocate or Initialize() to deallocate.")
  vtkTypeBool Allocate(vtkIdType size, int strategy = 0);

  /**
   * Reserve the id list to the requested number of ids and preserve data.
   *
   * Increasing the id list capacity may allocate extra memory beyond what was
   * requested. NumberOfIds will not be modified when increasing id list size.
   *
   * Decreasing the id list capacity is effectively a no-op.
   *
   * Returns 1 if resizing succeeded and 0 otherwise.
   */
  vtkTypeBool Reserve(vtkIdType size);

  /**
   * Return the number of id's in the list.
   */
  vtkIdType GetNumberOfIds() const noexcept { return this->NumberOfIds; }

  /**
   * Return the id at location i.
   */
  vtkIdType GetId(vtkIdType i) VTK_EXPECTS(0 <= i && i < GetNumberOfIds())
  {
    return this->Buffer->GetBuffer()[i];
  }

  /**
   * Find the location i of the provided id.
   */
  vtkIdType FindIdLocation(const vtkIdType id)
  {
    for (int i = 0; i < this->NumberOfIds; i++)
    {
      if (this->Buffer->GetBuffer()[i] == id)
      {
        return i;
      }
    }
    return -1;
  }

  /**
   * Specify the number of ids for this object to hold. Does an
   * allocation as well as setting the number of ids.
   * Preserves existing data.
   */
  void SetNumberOfIds(vtkIdType number);

  /**
   * Set the id at location i. Doesn't do range checking so it's a bit
   * faster than InsertId. Make sure you use SetNumberOfIds() to allocate
   * memory prior to using SetId().
   */
  void SetId(vtkIdType i, vtkIdType id) VTK_EXPECTS(0 <= i && i < GetNumberOfIds())
  {
    this->Buffer->GetBuffer()[i] = id;
  }

  /**
   * Set the id at location i. Does range checking and allocates memory
   * as necessary.
   */
  void InsertId(vtkIdType i, vtkIdType id) VTK_EXPECTS(0 <= i);

  /**
   * Add the id specified to the end of the list. Range checking is performed.
   */
  vtkIdType InsertNextId(vtkIdType id);

  /**
   * If id is not already in list, insert it and return location in
   * list. Otherwise return just location in list.
   */
  vtkIdType InsertUniqueId(vtkIdType id);

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
  vtkIdType* GetPointer(vtkIdType i) { return this->Buffer->GetBuffer() + i; }

  /**
   * Get a pointer to a particular data index. Make sure data is allocated
   * for the number of items requested. Set MaxId according to the number of
   * data values requested.
   */
  vtkIdType* WritePointer(vtkIdType i, vtkIdType number);

  /**
   * This method let's the user specify data to be held by the id list. The
   * array argument is a pointer to the data. Size is the size of the array
   * supplied by the user (as number of ids, not in bytes).
   * Set save to 1 to prevent the class from
   * deleting the array when it cleans up or reallocates memory.  The class
   * uses the actual array provided; it does not copy the data from the
   * supplied array. If specified, the delete method determines how the data
   * array will be deallocated. If the delete method is
   * VTK_DATA_ARRAY_FREE, free() will be used. If the delete method is
   * VTK_DATA_ARRAY_DELETE, delete[] will be used. If the delete method is
   * VTK_DATA_ARRAY_ALIGNED_FREE _aligned_free() will be used on Windows, while
   * free() will be used everywhere else. The default is VTK_DATA_ARRAY_DELETE.
   */
  void SetList(
    vtkIdType* array, vtkIdType size, bool save, int deleteMethod = VTK_DATA_ARRAY_DELETE);

  /**
   * This method does the same as SetList but the save and manageMemory are opposite.
   * It was deprecated to promote uniformity between vtkIdList and vtkAOSDataArrayTemplate APIs.
   */
  VTK_DEPRECATED_IN_9_7_0("Use SetList instead")
  void SetArray(vtkIdType* array, vtkIdType size, bool manageMemory = true);

  /**
   * Reset to an empty state but retain previously allocated memory.
   */
  void Reset() { this->NumberOfIds = 0; }

  /**
   * Free any unused memory.
   */
  void Squeeze();

  /**
   * Copy an id list by copying the internal buffer pointer.
   * Note that this is a shallow copy, so the internal buffer is shared between the two id lists.
   */
  void ShallowCopy(vtkIdList* list);

  /**
   * Copy an id list by explicitly copying the internal array.
   */
  void DeepCopy(vtkIdList* ids);

  /**
   * Delete specified id from list. Will remove all occurrences of id in list.
   */
  void DeleteId(vtkIdType id);

  /**
   * Return -1 if id specified is not contained in the list; otherwise return
   * the position in the list.
   */
  vtkIdType IsId(vtkIdType id) VTK_FUTURE_CONST;

  /**
   * Intersect this list with another vtkIdList. Updates current list according
   * to result of intersection operation.
   */
  void IntersectWith(vtkIdList* otherIds);

  /**
   * Adjust the size of the id list while maintaining its content (except
   * when being truncated).
   */
  VTK_DEPRECATED_IN_9_7_0("Use Reserve, Squeeze or Initialize")
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

  /**
   * Get the capacity of the id list.  This returns the number of id slots
   * in the id list's allocated storage.
   */
  vtkIdType GetCapacity() const { return this->Buffer->GetNumberOfElements(); }

  ///@{
  /**
   * To support range-based `for` loops
   */
  vtkIdType* begin() { return this->Buffer->GetBuffer(); }
  vtkIdType* end() { return this->Buffer->GetBuffer() + this->NumberOfIds; }
  const vtkIdType* begin() const { return this->Buffer->GetBuffer(); }
  const vtkIdType* end() const { return this->Buffer->GetBuffer() + this->NumberOfIds; }
  ///@}
protected:
  vtkIdList();
  ~vtkIdList() override;

  /**
   * Allocate ids and set the number of ids.
   */
  VTK_DEPRECATED_IN_9_7_0("Use Allocate and SetNumberOfIds instead")
  bool AllocateInternal(vtkIdType sz, vtkIdType numberOfIds);
  /**
   * Release memory.
   */
  VTK_DEPRECATED_IN_9_7_0("Use Allocate(0) instead")
  void InitializeMemory();

  vtkIdType NumberOfIds;
  vtkBuffer<vtkIdType>* Buffer;
  vtkIdType Size VTK_DEPRECATED_IN_9_7_0("Use GetCapacity() instead");

private:
  vtkIdList(const vtkIdList&) = delete;
  void operator=(const vtkIdList&) = delete;
};

// In-lined for performance
inline void vtkIdList::InsertId(const vtkIdType i, const vtkIdType id)
{
  if (i >= this->GetCapacity())
  {
    this->Reserve(i + 1);
  }
  this->Buffer->GetBuffer()[i] = id;
  if (i >= this->NumberOfIds)
  {
    this->NumberOfIds = i + 1;
  }
}

// In-lined for performance
inline vtkIdType vtkIdList::InsertNextId(const vtkIdType id)
{
  if (this->NumberOfIds >= this->GetCapacity())
  {
    if (!this->Reserve(this->NumberOfIds + 1))
    {
      return this->NumberOfIds - 1;
    }
  }
  this->Buffer->GetBuffer()[this->NumberOfIds++] = id;
  return this->NumberOfIds - 1;
}

inline vtkIdType vtkIdList::IsId(vtkIdType id) VTK_FUTURE_CONST
{
  for (vtkIdType i = 0; i < this->NumberOfIds; ++i)
  {
    if (this->Buffer->GetBuffer()[i] == id)
    {
      return i;
    }
  }
  return -1;
}

VTK_ABI_NAMESPACE_END
#endif
