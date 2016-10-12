/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkTreeIterator
 * @brief   Abstract class for iterator over a vtkTree.
 *
 *
 * The base class for tree iterators vtkTreeBFSIterator and vtkTreeDFSIterator.
 *
 * After setting up the iterator, the normal mode of operation is to
 * set up a <code>while(iter->HasNext())</code> loop, with the statement
 * <code>vtkIdType vertex = iter->Next()</code> inside the loop.
 *
 * @sa
 * vtkTreeBFSIterator vtkTreeDFSIterator
*/

#ifndef vtkTreeIterator_h
#define vtkTreeIterator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

class vtkTree;

class VTKCOMMONDATAMODEL_EXPORT vtkTreeIterator : public vtkObject
{
public:
  vtkTypeMacro(vtkTreeIterator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Set/get the graph to iterate over.
   */
  void SetTree(vtkTree* graph);
  vtkGetMacro(Tree, vtkTree*);
  //@}

  //@{
  /**
   * The start vertex of the traversal.
   * The tree iterator will only iterate over the subtree rooted at vertex.
   * If not set (or set to a negative value), starts at the root of the tree.
   */
  void SetStartVertex(vtkIdType vertex);
  vtkGetMacro(StartVertex, vtkIdType);
  //@}

  /**
   * The next vertex visited in the graph.
   */
  vtkIdType Next();

  /**
   * Return true when all vertices have been visited.
   */
  bool HasNext();

  /**
   * Reset the iterator to its start vertex.
   */
  void Restart();

protected:
  vtkTreeIterator();
  ~vtkTreeIterator() VTK_OVERRIDE;

  virtual void Initialize() = 0;
  virtual vtkIdType NextInternal() = 0;

  vtkTree* Tree;
  vtkIdType StartVertex;
  vtkIdType NextId;

private:
  vtkTreeIterator(const vtkTreeIterator &) VTK_DELETE_FUNCTION;
  void operator=(const vtkTreeIterator &) VTK_DELETE_FUNCTION;
};

#endif
