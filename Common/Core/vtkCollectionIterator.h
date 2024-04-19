// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCollectionIterator
 * @brief   iterator through a vtkCollection.
 *
 * vtkCollectionIterator provides an alternative way to traverse
 * through the objects in a vtkCollection.  Unlike the collection's
 * built in interface, this allows multiple iterators to
 * simultaneously traverse the collection.  If items are removed from
 * the collection, only the iterators currently pointing to those
 * items are invalidated.  Other iterators will still continue to
 * function normally.
 */

#ifndef vtkCollectionIterator_h
#define vtkCollectionIterator_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkCollection;
class vtkCollectionElement;

class VTKCOMMONCORE_EXPORT vtkCollectionIterator : public vtkObject
{
public:
  vtkTypeMacro(vtkCollectionIterator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkCollectionIterator* New();

  ///@{
  /**
   * Set/Get the collection over which to iterate.
   */
  virtual void SetCollection(vtkCollection*);
  vtkGetObjectMacro(Collection, vtkCollection);
  ///@}

  /**
   * Position the iterator at the first item in the collection.
   */
  void InitTraversal() { this->GoToFirstItem(); }

  /**
   * Position the iterator at the first item in the collection.
   */
  void GoToFirstItem();

  /**
   * Move the iterator to the next item in the collection.
   */
  void GoToNextItem();

  /**
   * Test whether the iterator is currently positioned at a valid item.
   * Returns 1 for yes, 0 for no.
   */
  int IsDoneWithTraversal();

  /**
   * Get the item at the current iterator position.  Valid only when
   * IsDoneWithTraversal() returns 1.
   */
  vtkObject* GetCurrentObject();

protected:
  vtkCollectionIterator();
  ~vtkCollectionIterator() override;

  // The collection over which we are iterating.
  vtkCollection* Collection;

  // The current iterator position.
  vtkCollectionElement* Element;

  vtkObject* GetObjectInternal();

private:
  vtkCollectionIterator(const vtkCollectionIterator&) = delete;
  void operator=(const vtkCollectionIterator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
