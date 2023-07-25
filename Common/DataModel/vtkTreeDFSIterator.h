// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkTreeDFSIterator
 * @brief   depth first iterator through a vtkGraph
 *
 *
 * vtkTreeDFSIterator performs a depth first search traversal of a tree.
 *
 * First, you must set the tree on which you are going to iterate, and then
 * optionally set the starting vertex and mode. The mode is either
 * DISCOVER (default), in which case vertices are visited as they are first
 * reached, or FINISH, in which case vertices are visited when they are
 * done, i.e. all adjacent vertices have been discovered already.
 *
 * After setting up the iterator, the normal mode of operation is to
 * set up a <code>while(iter->HasNext())</code> loop, with the statement
 * <code>vtkIdType vertex = iter->Next()</code> inside the loop.
 */

#ifndef vtkTreeDFSIterator_h
#define vtkTreeDFSIterator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkTreeIterator.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkTreeDFSIteratorInternals;
class vtkIntArray;

class VTKCOMMONDATAMODEL_EXPORT vtkTreeDFSIterator : public vtkTreeIterator
{
public:
  static vtkTreeDFSIterator* New();
  vtkTypeMacro(vtkTreeDFSIterator, vtkTreeIterator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum ModeType
  {
    DISCOVER,
    FINISH
  };

  ///@{
  /**
   * Set the visit mode of the iterator.  Mode can be
   * DISCOVER (0): Order by discovery time
   * FINISH   (1): Order by finish time
   * Default is DISCOVER.
   * Use DISCOVER for top-down algorithms where parents need to be processed before children.
   * Use FINISH for bottom-up algorithms where children need to be processed before parents.
   */
  void SetMode(int mode);
  vtkGetMacro(Mode, int);
  ///@}

protected:
  vtkTreeDFSIterator();
  ~vtkTreeDFSIterator() override;

  void Initialize() override;
  vtkIdType NextInternal() override;

  int Mode;
  vtkIdType CurRoot;
  vtkTreeDFSIteratorInternals* Internals;
  vtkIntArray* Color;

  enum ColorType
  {
    WHITE,
    GRAY,
    BLACK
  };

private:
  vtkTreeDFSIterator(const vtkTreeDFSIterator&) = delete;
  void operator=(const vtkTreeDFSIterator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
