// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDataSetCollection
 * @brief   maintain an unordered list of dataset objects
 *
 * vtkDataSetCollection is an object that creates and manipulates ordered
 * lists of datasets. See also vtkCollection and subclasses.
 */

#ifndef vtkDataSetCollection_h
#define vtkDataSetCollection_h

#include "vtkCollection.h"
#include "vtkCommonDataModelModule.h" // For export macro

#include "vtkDataSet.h" // Needed for inline methods.

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONDATAMODEL_EXPORT vtkDataSetCollection : public vtkCollection
{
public:
  static vtkDataSetCollection* New();
  vtkTypeMacro(vtkDataSetCollection, vtkCollection);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Add a dataset to the bottom of the list.
   */
  void AddItem(vtkDataSet* ds) { this->vtkCollection::AddItem(ds); }

  ///@{
  /**
   * Get the next dataset in the list.
   */
  vtkDataSet* GetNextItem() { return static_cast<vtkDataSet*>(this->GetNextItemAsObject()); }
  vtkDataSet* GetNextDataSet() { return static_cast<vtkDataSet*>(this->GetNextItemAsObject()); }
  ///@}

  ///@{
  /**
   * Get the ith dataset in the list.
   */
  vtkDataSet* GetItem(int i) { return static_cast<vtkDataSet*>(this->GetItemAsObject(i)); }
  vtkDataSet* GetDataSet(int i) { return static_cast<vtkDataSet*>(this->GetItemAsObject(i)); }
  ///@}

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  vtkDataSet* GetNextDataSet(vtkCollectionSimpleIterator& cookie)
  {
    return static_cast<vtkDataSet*>(this->GetNextItemAsObject(cookie));
  }

protected:
  vtkDataSetCollection() = default;
  ~vtkDataSetCollection() override = default;

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject* o) { this->vtkCollection::AddItem(o); }

  vtkDataSetCollection(const vtkDataSetCollection&) = delete;
  void operator=(const vtkDataSetCollection&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
