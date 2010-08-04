/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkTreeBFSIterator.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkTreeBFSIterator - breadth first search iterator through a vtkTree
//
// .SECTION Description
// vtkTreeBFSIterator performs a breadth first search of a tree.
//
// After setting up the iterator, the normal mode of operation is to
// set up a <code>while(iter->HasNext())</code> loop, with the statement
// <code>vtkIdType vertex = iter->Next()</code> inside the loop.
//
// .SECTION Thanks
// Thanks to David Doria for submitting this class.

#ifndef __vtkTreeBFSIterator_h
#define __vtkTreeBFSIterator_h

#include "vtkObject.h"

class vtkTree;
class vtkTreeBFSIteratorInternals;
class vtkIntArray;
class vtkIdList;

class VTK_FILTERING_EXPORT vtkTreeBFSIterator : public vtkObject
{
public:
  static vtkTreeBFSIterator* New();
  vtkTypeMacro(vtkTreeBFSIterator, vtkObject);
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
  vtkTreeBFSIterator();
  ~vtkTreeBFSIterator();

  void Initialize();
  vtkIdType NextInternal();

  vtkTree* Tree;
  int Mode;
  vtkIdType StartVertex;
  vtkIdType CurRoot;
  vtkTreeBFSIteratorInternals* Internals;
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
  vtkTreeBFSIterator(const vtkTreeBFSIterator &);  // Not implemented.
  void operator=(const vtkTreeBFSIterator &);        // Not implemented.
};

#endif
