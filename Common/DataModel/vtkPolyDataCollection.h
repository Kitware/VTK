// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPolyDataCollection
 * @brief   maintain a list of polygonal data objects
 *
 * vtkPolyDataCollection is an object that creates and manipulates ordered
 * lists of datasets of type vtkPolyData.
 *
 * @sa
 * vtkDataSetCollection vtkCollection
 */

#ifndef vtkPolyDataCollection_h
#define vtkPolyDataCollection_h

#include "vtkCollection.h"
#include "vtkCommonDataModelModule.h" // For export macro

#include "vtkPolyData.h" // Needed for static cast

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONDATAMODEL_EXPORT vtkPolyDataCollection : public vtkCollection
{
public:
  static vtkPolyDataCollection* New();
  vtkTypeMacro(vtkPolyDataCollection, vtkCollection);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Add a poly data to the bottom of the list.
   */
  void AddItem(vtkPolyData* pd) { this->vtkCollection::AddItem(pd); }

  /**
   * Get the next poly data in the list.
   */
  vtkPolyData* GetNextItem() { return static_cast<vtkPolyData*>(this->GetNextItemAsObject()); }

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  vtkPolyData* GetNextPolyData(vtkCollectionSimpleIterator& cookie)
  {
    return static_cast<vtkPolyData*>(this->GetNextItemAsObject(cookie));
  }

protected:
  vtkPolyDataCollection() = default;
  ~vtkPolyDataCollection() override = default;

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject* o) { this->vtkCollection::AddItem(o); }

  vtkPolyDataCollection(const vtkPolyDataCollection&) = delete;
  void operator=(const vtkPolyDataCollection&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
