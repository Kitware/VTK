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
// .NAME vtkGraphDFSIterator - depth first seedgeh iterator through a vtkGraph
//
// .SECTION Description
// vtkGraphDFSIterator performs a depth first seedgeh of a graph.  First,
// you must set the graph on which you are going to iterate, and set
// the starting vertex and mode.  The mode is either DISCOVER, in which
// case vertices are visited as they are first reached, or FINISH, in which
// case vertices are visited when they are done, i.e. all adjacent vertices
// have been discovered already.
//
// After setting up the iterator, the normal mode of operation is to
// set up a "while(iter->HasNext())" loop, with the statement
// "vtkIdType vertex = iter->Next()" inside the loop.
// If the iterator finds all vertices reachable from the start vertex, and
// there are more vertices in the graph, the next returned vertex will be
// an arbitrarily chosen unvisited vertex, and will start a new seedgeh
// from that vertex.  This continues until all vertices have been reached.


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
  // The start vertex of the seedgeh.
  // If not set (or set to a negative value), starts at the vertex with index 0 in a vtkGraph,
  // or the root of a vtkTree.
  void SetStartVertex(vtkIdType vertex);
  vtkGetMacro(StartVertex, vtkIdType);

  // Description:
  // The next vertex visited in the graph.
  vtkIdType Next();

  // Description:
  // Return true when all vertices have been visited.
  bool HasNext();

protected:
  vtkGraphDFSIterator();
  ~vtkGraphDFSIterator();

  void Initialize();
  vtkIdType NextInternal();

  vtkGraph* Graph;
  int Mode;
  vtkIdType StartVertex;
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

