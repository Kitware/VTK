// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCollection
 * @brief   create and manipulate ordered lists of objects
 *
 * vtkCollection is a general object for creating and manipulating lists
 * of objects. The lists are ordered and allow duplicate entries.
 * vtkCollection also serves as a base class for lists of specific types
 * of objects.
 *
 * @sa
 * vtkActorCollection vtkAssemblyPaths vtkDataSetCollection
 * vtkImplicitFunctionCollection vtkLightCollection vtkPolyDataCollection
 * vtkRenderWindowCollection vtkRendererCollection
 * vtkStructuredPointsCollection vtkTransformCollection vtkVolumeCollection
 */

#ifndef vtkCollection_h
#define vtkCollection_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkDeprecation.h"      // For VTK_DEPRECATED_IN_9_6_0
#include "vtkObject.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkCollectionElement //;prevents pick-up by man page generator
{
public:
  vtkCollectionElement()
    : Item(nullptr)
    , Next(nullptr)
  {
  }
  vtkObject* Item;
  vtkCollectionElement* Next;
};
typedef void* vtkCollectionSimpleIterator;

class vtkCollectionIterator;

class VTKCOMMONCORE_EXPORT VTK_MARSHALAUTO vtkCollection : public vtkObject
{
public:
  vtkTypeMacro(vtkCollection, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct an empty collection.
   */
  static vtkCollection* New();

  /**
   * Add given item to the bottom (end) of the collection. Does not prevent duplicate entries.
   * Given item must not be nullptr.
   *
   * Note: it is undefined behaviour to invoke this during traversal of the collection.
   */
  void AddItem(vtkObject*);

  /**
   * Insert given item into the collection after the i'th item. Does not prevent duplicate entries.
   * If the collection is empty, does nothing (regardless of parameters).
   * If i < 0 the given item is placed at the top (beginning) of the collection.
   * Given item must not be nullptr.
   *
   * Note: it is undefined behaviour to invoke this during traversal of the collection.
   */
  void InsertItem(int i, vtkObject*);

  /**
   * Replace the i'th item in the collection with the given item.
   * If i is out-of-range, this function does nothing.
   * Given item must not be nullptr.
   *
   * Note: It is well-defined to replace an item during traversal of the collection.
   */
  void ReplaceItem(int i, vtkObject*);

  /**
   * Remove the i'th item in the collection. If i is out-of-range, this function does nothing.
   *
   * Note: It is well-defined to remove items during traversal of the collection.
   */
  void RemoveItem(int i);

  /**
   * Remove the first occurrence of the given item from the collection.
   * Removes only the first occurrence found, not all occurrences.
   * If no occurrence is found, the collection is unaffected.
   * If given item is nullptr, does nothing.
   *
   * Note: It is well-defined to remove items during traversal of the collection.
   */
  void RemoveItem(vtkObject*);

  /**
   * Remove all items from the collection.
   *
   * Note: It is well-defined to remove items during traversal of the collection.
   */
  void RemoveAllItems();

  /**
   * Search for the given item and return the 1-based index of its first occurrence in the
   * collection. If the item is not found, the return value is 0. If the item is found, the return
   * value is its first location + 1 (a 1-based index). If given item is nullptr, returns 0.
   */
  int IsItemPresent(vtkObject* a) VTK_FUTURE_CONST;

  /**
   * Just calls IndexOfFirstOccurrence.
   */
  VTK_DEPRECATED_IN_9_6_0("Use correctly spelled IndexOfFirstOccurrence instead.")
  int IndexOfFirstOccurence(vtkObject* a) VTK_FUTURE_CONST;

  /**
   * Search for the given item and return the 0-based index of its first occurrence in the
   * collection. If the item is not found, the return value is -1. If the item is found, the return
   * value is its first location (a 0-based index). If given item is nullptr, returns -1.
   */
  int IndexOfFirstOccurrence(vtkObject* a) const;

  /**
   * Return the number of items in the collection.
   */
  int GetNumberOfItems() VTK_FUTURE_CONST { return this->NumberOfItems; }

  /**
   * Get the i'th item in the collection. nullptr is returned if i is out
   * of range.
   */
  vtkObject* GetItemAsObject(int i) VTK_FUTURE_CONST;

  /**
   * Initialize the traversal of the collection. This means the next call to GetNextItemAsObject()
   * will return the first object in the collection.
   */
  void InitTraversal() { this->Current = this->Top; }

  /**
   * A reentrant safe way to iterate through a collection.
   * Just pass the same cookie value around each time.
   */
  void InitTraversal(vtkCollectionSimpleIterator& cookie)
  {
    cookie = static_cast<vtkCollectionSimpleIterator>(this->Top);
  }

  /**
   * Get the next item in the collection. nullptr is returned if the collection
   * is exhausted.
   */
  vtkObject* GetNextItemAsObject();

  /**
   * A reentrant safe way to get the next item as a collection. Just pass the
   * same cookie back and forth.
   */
  vtkObject* GetNextItemAsObject(vtkCollectionSimpleIterator& cookie) VTK_FUTURE_CONST;

  /**
   * Get an iterator to traverse the items in this collection.
   */
  VTK_NEWINSTANCE vtkCollectionIterator* NewIterator();

  /**
   * Add support for C++11 range-based for loops.
   */
  struct Iterator
  {
    vtkCollectionElement* current;
    vtkObject* operator*() const { return current->Item; }
    Iterator& operator++()
    {
      current = current->Next;
      return *this;
    }
    bool operator!=(const Iterator& other) const { return current != other.current; }
  };
  Iterator begin() const { return { this->Top }; }
  Iterator end() const { return { nullptr }; }

  ///@{
  /**
   * Participate in garbage collection.
   */
  bool UsesGarbageCollector() const override { return true; }
  ///@}

protected:
  vtkCollection();
  ~vtkCollection() override;

  virtual void RemoveElement(vtkCollectionElement* element, vtkCollectionElement* previous);
  virtual void DeleteElement(vtkCollectionElement*);
  int NumberOfItems;
  vtkCollectionElement* Top;
  vtkCollectionElement* Bottom;
  vtkCollectionElement* Current;

  friend class vtkCollectionIterator;

  // See vtkGarbageCollector.h:
  void ReportReferences(vtkGarbageCollector* collector) override;

private:
  vtkCollection(const vtkCollection&) = delete;
  void operator=(const vtkCollection&) = delete;
};

inline vtkObject* vtkCollection::GetNextItemAsObject()
{
  vtkCollectionElement* elem = this->Current;

  if (elem != nullptr)
  {
    this->Current = elem->Next;
    return elem->Item;
  }
  else
  {
    return nullptr;
  }
}

inline vtkObject* vtkCollection::GetNextItemAsObject(
  vtkCollectionSimpleIterator& cookie) VTK_FUTURE_CONST
{
  vtkCollectionElement* elem = static_cast<vtkCollectionElement*>(cookie);

  if (elem != nullptr)
  {
    cookie = static_cast<vtkCollectionSimpleIterator>(elem->Next);
    return elem->Item;
  }
  else
  {
    return nullptr;
  }
}

VTK_ABI_NAMESPACE_END
#endif
