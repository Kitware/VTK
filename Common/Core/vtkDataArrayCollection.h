// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDataArrayCollection
 * @brief   maintain an ordered list of dataarray objects
 *
 * vtkDataArrayCollection is an object that creates and manipulates lists of
 * datasets. See also vtkCollection and subclasses.
 */

#ifndef vtkDataArrayCollection_h
#define vtkDataArrayCollection_h

#include "vtkCollection.h"
#include "vtkCommonCoreModule.h" // For export macro

#include "vtkDataArray.h" // Needed for inline methods

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCORE_EXPORT vtkDataArrayCollection : public vtkCollection
{
public:
  static vtkDataArrayCollection* New();
  vtkTypeMacro(vtkDataArrayCollection, vtkCollection);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Add a dataarray to the bottom of the list.
   */
  void AddItem(vtkDataArray* ds) { this->vtkCollection::AddItem(ds); }

  /**
   * Get the next dataarray in the list.
   */
  vtkDataArray* GetNextItem() { return static_cast<vtkDataArray*>(this->GetNextItemAsObject()); }

  /**
   * Get the ith dataarray in the list.
   */
  vtkDataArray* GetItem(int i) { return static_cast<vtkDataArray*>(this->GetItemAsObject(i)); }

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  vtkDataArray* GetNextDataArray(vtkCollectionSimpleIterator& cookie)
  {
    return static_cast<vtkDataArray*>(this->GetNextItemAsObject(cookie));
  }

protected:
  vtkDataArrayCollection() = default;
  ~vtkDataArrayCollection() override = default;

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject* o) { this->vtkCollection::AddItem(o); }

  vtkDataArrayCollection(const vtkDataArrayCollection&) = delete;
  void operator=(const vtkDataArrayCollection&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
