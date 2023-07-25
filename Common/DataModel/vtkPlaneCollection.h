// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPlaneCollection
 * @brief   maintain a list of planes
 *
 * vtkPlaneCollection is an object that creates and manipulates
 * lists of objects of type vtkPlane.
 * @sa
 * vtkCollection
 */

#ifndef vtkPlaneCollection_h
#define vtkPlaneCollection_h

#include "vtkCollection.h"
#include "vtkCommonDataModelModule.h" // For export macro

#include "vtkPlane.h" // Needed for inline methods

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONDATAMODEL_EXPORT vtkPlaneCollection : public vtkCollection
{
public:
  vtkTypeMacro(vtkPlaneCollection, vtkCollection);
  static vtkPlaneCollection* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Add a plane to the list.
   */
  void AddItem(vtkPlane*);

  /**
   * Get the next plane in the list.
   */
  vtkPlane* GetNextItem();

  /**
   * Get the ith plane in the list.
   */
  vtkPlane* GetItem(int i) { return static_cast<vtkPlane*>(this->GetItemAsObject(i)); }

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  vtkPlane* GetNextPlane(vtkCollectionSimpleIterator& cookie);

protected:
  vtkPlaneCollection() = default;
  ~vtkPlaneCollection() override = default;

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject* o) { this->vtkCollection::AddItem(o); }

  vtkPlaneCollection(const vtkPlaneCollection&) = delete;
  void operator=(const vtkPlaneCollection&) = delete;
};

inline void vtkPlaneCollection::AddItem(vtkPlane* f)
{
  this->vtkCollection::AddItem(f);
}

inline vtkPlane* vtkPlaneCollection::GetNextItem()
{
  return static_cast<vtkPlane*>(this->GetNextItemAsObject());
}

VTK_ABI_NAMESPACE_END
#endif
