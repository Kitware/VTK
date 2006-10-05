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
// .NAME vtkTreeDFSIterator - depth first search iterator through a vtkGraph
//
// .SECTION Description
//


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
  vtkTypeRevisionMacro(vtkTreeDFSIterator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  enum ModeType
    {
    DISCOVER,
    FINISH,
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

  // Description:
  // The start node of the search.
  // The tree iterator will only iterate over the subtree rooted at node.
  // If not set (or set to a negative value), starts at the root of the tree.
  void SetStartNode(vtkIdType node);

  vtkIdType Next();
  bool HasNext();
protected:
  vtkTreeDFSIterator();
  ~vtkTreeDFSIterator();

  void Initialize();
  vtkIdType NextInternal();

  vtkTree* Tree;
  int Mode;
  vtkIdType StartNode;
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

