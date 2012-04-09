/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkTreeIterator.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkTreeIterator - Abstract class for iterator over a vtkTree.
//
// .SECTION Description
// The base class for tree iterators vtkTreeBFSIterator and vtkTreeDFSIterator.
//
// After setting up the iterator, the normal mode of operation is to
// set up a <code>while(iter->HasNext())</code> loop, with the statement
// <code>vtkIdType vertex = iter->Next()</code> inside the loop.
//
// .SECTION See Also
// vtkTreeBFSIterator vtkTreeDFSIterator

#ifndef __vtkTreeIterator_h
#define __vtkTreeIterator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

class vtkTree;

class VTKCOMMONDATAMODEL_EXPORT vtkTreeIterator : public vtkObject
{
public:
  vtkTypeMacro(vtkTreeIterator, vtkObject);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/get the graph to iterate over.
  void SetTree(vtkTree* graph);
  vtkGetMacro(Tree, vtkTree*);

  // Description:
  // The start vertex of the traversal.
  // The tree iterator will only iterate over the subtree rooted at vertex.
  // If not set (or set to a negative value), starts at the root of the tree.
  void SetStartVertex(vtkIdType vertex);
  vtkGetMacro(StartVertex, vtkIdType);

  // Description:
  // The next vertex visited in the graph.
  vtkIdType Next();

  // Description:
  // Return true when all vertices have been visited.
  bool HasNext();

  // Description:
  // Reset the iterator to its start vertex.
  void Restart();

protected:
  vtkTreeIterator();
  ~vtkTreeIterator();

  virtual void Initialize() = 0;
  virtual vtkIdType NextInternal() = 0;

  vtkTree* Tree;
  vtkIdType StartVertex;
  vtkIdType NextId;

private:
  vtkTreeIterator(const vtkTreeIterator &);  // Not implemented.
  void operator=(const vtkTreeIterator &);        // Not implemented.
};

#endif
