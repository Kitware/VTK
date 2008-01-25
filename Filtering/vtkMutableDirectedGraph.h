/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMutableDirectedGraph.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
// .NAME vtkMutableDirectedGraph - An editable directed graph.
//
// .SECTION Description
// vtkMutableDirectedGraph is a directed graph which has additional methods
// for adding edges and vertices. AddChild() is a convenience method for
// constructing trees. ShallowCopy(), DeepCopy(), CheckedShallowCopy() and 
// CheckedDeepCopy() will succeed for instances of vtkDirectedGraph, 
// vtkMutableDirectedGraph and vtkTree.
//
// .SECTION See Also
// vtkDirectedGraph vtkGraph vtkTree

#ifndef __vtkMutableDirectedGraph_h
#define __vtkMutableDirectedGraph_h

#include "vtkDirectedGraph.h"

class vtkEdgeListIterator;
class vtkGraphEdge;

class VTK_FILTERING_EXPORT vtkMutableDirectedGraph : public vtkDirectedGraph
{
public:
  static vtkMutableDirectedGraph *New();
  vtkTypeRevisionMacro(vtkMutableDirectedGraph, vtkDirectedGraph);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Adds a vertex to the graph, and returns the id of that vertex.
  vtkIdType AddVertex();

  //BTX
  // Description:
  // Adds a directed edge from u to v to the graph and returns
  // a vtkEdgeType structure for that edge.
  vtkEdgeType AddEdge(vtkIdType u, vtkIdType v);
  //ETX

  // Description:
  // Version of AddEdge that returns a heavyweight vtkGraphEdge
  // for use with wrappers.
  // The graph owns the reference of the edge and will replace
  // its contents on the next call to AddGraphEdge.
  vtkGraphEdge *AddGraphEdge(vtkIdType u, vtkIdType v);

  // Description:
  // Convenience method for creating trees.
  // Returns the newly created vertex id.
  // Shortcut for
  // <code>
  // vtkIdType v = g->AddVertex();
  // g->AddEdge(parent, v);
  // </code>
  vtkIdType AddChild(vtkIdType parent);

protected:
  vtkMutableDirectedGraph();
  ~vtkMutableDirectedGraph();

  // Description:
  // Graph edge that is reused of AddGraphEdge calls.
  vtkGraphEdge *GraphEdge;

private:
  vtkMutableDirectedGraph(const vtkMutableDirectedGraph&);  // Not implemented.
  void operator=(const vtkMutableDirectedGraph&);  // Not implemented.
};

#endif
