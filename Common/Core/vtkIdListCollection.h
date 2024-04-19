// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkIdListCollection
 * @brief   maintain an ordered list of IdList objects
 *
 * vtkIdListCollection is an object that creates and manipulates lists of
 * IdLists. See also vtkCollection and subclasses.
 */

#ifndef vtkIdListCollection_h
#define vtkIdListCollection_h

#include "vtkCollection.h"
#include "vtkCommonCoreModule.h" // For export macro

#include "vtkIdList.h" // Needed for inline methods

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCORE_EXPORT vtkIdListCollection : public vtkCollection
{
public:
  static vtkIdListCollection* New();
  vtkTypeMacro(vtkIdListCollection, vtkCollection);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Add an IdList to the bottom of the list.
   */
  void AddItem(vtkIdList* ds) { this->vtkCollection::AddItem(ds); }

  /**
   * Get the next IdList in the list.
   */
  vtkIdList* GetNextItem() { return static_cast<vtkIdList*>(this->GetNextItemAsObject()); }

  /**
   * Get the ith IdList in the list.
   */
  vtkIdList* GetItem(int i) { return static_cast<vtkIdList*>(this->GetItemAsObject(i)); }

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  vtkIdList* GetNextIdList(vtkCollectionSimpleIterator& cookie)
  {
    return static_cast<vtkIdList*>(this->GetNextItemAsObject(cookie));
  }

protected:
  vtkIdListCollection() = default;
  ~vtkIdListCollection() override = default;

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject* o) { this->vtkCollection::AddItem(o); }

  vtkIdListCollection(const vtkIdListCollection&) = delete;
  void operator=(const vtkIdListCollection&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
