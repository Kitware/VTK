// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkStructuredPointsCollection
 * @brief   maintain a list of structured points data objects
 *
 * vtkStructuredPointsCollection is an object that creates and manipulates
 * ordered lists of structured points datasets. See also vtkCollection and
 * subclasses.
 */

#ifndef vtkStructuredPointsCollection_h
#define vtkStructuredPointsCollection_h

#include "vtkCollection.h"
#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkStructuredPoints.h"      // Needed for static cast

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONDATAMODEL_EXPORT vtkStructuredPointsCollection : public vtkCollection
{
public:
  static vtkStructuredPointsCollection* New();
  vtkTypeMacro(vtkStructuredPointsCollection, vtkCollection);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Add a pointer to a vtkStructuredPoints to the bottom of the list.
   */
  void AddItem(vtkStructuredPoints* ds) { this->vtkCollection::AddItem(ds); }

  /**
   * Get the next item in the collection. nullptr is returned if the collection
   * is exhausted.
   */
  vtkStructuredPoints* GetNextItem()
  {
    return static_cast<vtkStructuredPoints*>(this->GetNextItemAsObject());
  }

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  vtkStructuredPoints* GetNextStructuredPoints(vtkCollectionSimpleIterator& cookie)
  {
    return static_cast<vtkStructuredPoints*>(this->GetNextItemAsObject(cookie));
  }

protected:
  vtkStructuredPointsCollection() = default;
  ~vtkStructuredPointsCollection() override = default;

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject* o) { this->vtkCollection::AddItem(o); }

  vtkStructuredPointsCollection(const vtkStructuredPointsCollection&) = delete;
  void operator=(const vtkStructuredPointsCollection&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
