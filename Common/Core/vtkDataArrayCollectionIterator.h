// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDataArrayCollectionIterator
 * @brief   iterator through a vtkDataArrayCollection.
 *
 * vtkDataArrayCollectionIterator provides an implementation of
 * vtkCollectionIterator which allows the items to be retrieved with
 * the proper subclass pointer type for vtkDataArrayCollection.
 */

#ifndef vtkDataArrayCollectionIterator_h
#define vtkDataArrayCollectionIterator_h

#include "vtkCollectionIterator.h"
#include "vtkCommonCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArray;
class vtkDataArrayCollection;

class VTKCOMMONCORE_EXPORT vtkDataArrayCollectionIterator : public vtkCollectionIterator
{
public:
  vtkTypeMacro(vtkDataArrayCollectionIterator, vtkCollectionIterator);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkDataArrayCollectionIterator* New();

  ///@{
  /**
   * Set the collection over which to iterate.
   */
  void SetCollection(vtkCollection*) override;
  void SetCollection(vtkDataArrayCollection*);
  ///@}

  /**
   * Get the item at the current iterator position.  Valid only when
   * IsDoneWithTraversal() returns 1.
   */
  vtkDataArray* GetDataArray();

protected:
  vtkDataArrayCollectionIterator();
  ~vtkDataArrayCollectionIterator() override;

private:
  vtkDataArrayCollectionIterator(const vtkDataArrayCollectionIterator&) = delete;
  void operator=(const vtkDataArrayCollectionIterator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
