/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeDFSIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// .NAME vtkTreeDFSIterator - depth first seedgeh iterator through a vtkGraph
//
// .SECTION Description
// vtkTreeDFSIterator performs a depth first seedgeh of a tree.  First,
// you must set the tree on which you are going to iterate, and set
// the starting vertex and mode.  The mode is either DISCOVER, in which
// case vertices are visited as they are first reached, or FINISH, in which
// case vertices are visited when they are done, i.e. all adjacent vertices
// have been discovered already.
//
// After setting up the iterator, the normal mode of operation is to
// set up a <code>while(iter->HasNext())</code> loop, with the statement
// <code>vtkIdType vertex = iter->Next()</code> inside the loop.


#ifndef __vtkTreeDFSIterator_h
#define __vtkTreeDFSIterator_h

#include "vtkObject.h"

class vtkTree;
class vtkTreeDFSIteratorInternals;
class vtkIntArray;
class vtkIdList;

class VTK_FILTERING_EXPORT vtkTreeDFSIterator : public vtkObject
{
public:
  static vtkTreeDFSIterator* New();
  vtkTypeMacro(vtkTreeDFSIterator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  enum ModeType
    {
    DISCOVER,
    FINISH
    };
  //ETX

  // Description:
  // Set the graph to iterate over.
  void SetTree(vtkTree* graph);

  // Description:
  // Set the visit mode of the iterator.  Mode can be
  //   DISCOVER (0): Order by discovery time
  //   FINISH   (1): Order by finish time
  // Default is DISCOVER.
  // Use DISCOVER for top-down algorithms where parents need to be processed before children.
  // Use FINISH for bottom-up algorithms where children need to be processed before parents.
  void SetMode(int mode);
  vtkGetMacro(Mode, int);

  // Description:
  // The start vertex of the seedgeh.
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

protected:
  vtkTreeDFSIterator();
  ~vtkTreeDFSIterator();

  void Initialize();
  vtkIdType NextInternal();

  vtkTree* Tree;
  int Mode;
  vtkIdType StartVertex;
  vtkIdType CurRoot;
  vtkTreeDFSIteratorInternals* Internals;
  vtkIntArray* Color;
  vtkIdType NextId;

  //BTX
  enum ColorType
    {
    WHITE,
    GRAY,
    BLACK
    };
  //ETX

private:
  vtkTreeDFSIterator(const vtkTreeDFSIterator &);  // Not implemented.
  void operator=(const vtkTreeDFSIterator &);        // Not implemented.
};


#endif

