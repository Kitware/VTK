/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphDFSIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGraphDFSIterator - depth first search iterator through a vtkGraph
//
// .SECTION Description
// vtkGraphDFSIterator performs a depth first search of a graph.  First,
// you must set the graph on which you are going to iterate, and set
// the starting node and mode.  The mode is either DISCOVER, in which
// case nodes are visited as they are first reached, or FINISH, in which
// case nodes are visited when they are done, i.e. all adjacent nodes
// have been discovered already.
//
// After setting up the iterator, the normal mode of operation is to
// set up a "while(iter->HasNext())" loop, with the statement
// "vtkIdType node = iter->Next()" inside the loop.
// If the iterator finds all nodes reachable from the start node, and
// there are more nodes in the graph, the next returned node will be
// an arbitrarily chosen unvisited node, and will start a new search
// from that node.  This continues until all nodes have been reached.


#ifndef __vtkGraphDFSIterator_h
#define __vtkGraphDFSIterator_h

#include "vtkObject.h"

class vtkGraph;
class vtkGraphDFSIteratorInternals;
class vtkIntArray;
class vtkIdList;

class VTK_FILTERING_EXPORT vtkGraphDFSIterator : public vtkObject
{
public:
  static vtkGraphDFSIterator* New();
  vtkTypeRevisionMacro(vtkGraphDFSIterator, vtkObject);
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
  void SetGraph(vtkGraph* graph);

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
  // The start node of the search.
  // If not set (or set to a negative value), starts at the node with index 0 in a vtkGraph,
  // or the root of a vtkTree.
  void SetStartNode(vtkIdType node);
  vtkGetMacro(StartNode, vtkIdType);

  // Description:
  // The next node visited in the graph.
  vtkIdType Next();

  // Description:
  // Return true when all nodes have been visited.
  bool HasNext();

protected:
  vtkGraphDFSIterator();
  ~vtkGraphDFSIterator();

  void Initialize();
  vtkIdType NextInternal();

  vtkGraph* Graph;
  int Mode;
  vtkIdType StartNode;
  vtkIdType CurRoot;
  vtkGraphDFSIteratorInternals* Internals;
  vtkIntArray* Color;
  vtkIdType NumBlack;
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
  vtkGraphDFSIterator(const vtkGraphDFSIterator &);  // Not implemented.
  void operator=(const vtkGraphDFSIterator &);        // Not implemented.
};


#endif

