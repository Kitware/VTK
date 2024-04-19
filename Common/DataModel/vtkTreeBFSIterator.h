// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkTreeBFSIterator
 * @brief   breadth first search iterator through a vtkTree
 *
 *
 * vtkTreeBFSIterator performs a breadth first search traversal of a tree.
 *
 * After setting up the iterator, the normal mode of operation is to
 * set up a <code>while(iter->HasNext())</code> loop, with the statement
 * <code>vtkIdType vertex = iter->Next()</code> inside the loop.
 *
 * @par Thanks:
 * Thanks to David Doria for submitting this class.
 */

#ifndef vtkTreeBFSIterator_h
#define vtkTreeBFSIterator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkTreeIterator.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkTreeBFSIteratorInternals;
class vtkIntArray;

class VTKCOMMONDATAMODEL_EXPORT vtkTreeBFSIterator : public vtkTreeIterator
{
public:
  static vtkTreeBFSIterator* New();
  vtkTypeMacro(vtkTreeBFSIterator, vtkTreeIterator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkTreeBFSIterator();
  ~vtkTreeBFSIterator() override;

  void Initialize() override;
  vtkIdType NextInternal() override;

  vtkTreeBFSIteratorInternals* Internals;
  vtkIntArray* Color;

  enum ColorType
  {
    WHITE,
    GRAY,
    BLACK
  };

private:
  vtkTreeBFSIterator(const vtkTreeBFSIterator&) = delete;
  void operator=(const vtkTreeBFSIterator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
