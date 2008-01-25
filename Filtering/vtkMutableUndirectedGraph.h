/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMutableUndirectedGraph.h

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
// .NAME vtkMutableUndirectedGraph - An editable undirected graph.
//
// .SECTION Description
// vtkMutableUndirectedGraph is an undirected graph with additional functions
// for adding vertices and edges. ShallowCopy(), DeepCopy(), CheckedShallowCopy(),
// and CheckedDeepCopy() will succeed when the argument is a vtkUndirectedGraph
// or vtkMutableUndirectedGraph.
//
// .SECTION See Also
// vtkUndirectedGraph vtkGraph

#ifndef __vtkMutableUndirectedGraph_h
#define __vtkMutableUndirectedGraph_h

#include "vtkUndirectedGraph.h"

class vtkEdgeListIterator;
class vtkGraphEdge;

class VTK_FILTERING_EXPORT vtkMutableUndirectedGraph : public vtkUndirectedGraph
{
public:
  static vtkMutableUndirectedGraph *New();
  vtkTypeRevisionMacro(vtkMutableUndirectedGraph, vtkUndirectedGraph);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Adds a new vertex to the graph and returns the id of that vertex.
  vtkIdType AddVertex();

  //BTX
  // Description:
  // Adds an undirected edge between u and v, and returns
  // a vtkEdgeType structure for that edge.
  // The returned vtkEdgeType indicates a Source and Target,
  // but these are in arbitrary order.
  vtkEdgeType AddEdge(vtkIdType u, vtkIdType v);
  //ETX

  // Description:
  // Version of AddEdge that returns a heavyweight vtkGraphEdge
  // for use with wrappers.
  // The graph owns the reference of the edge and will replace
  // its contents on the next call to AddGraphEdge.
  vtkGraphEdge *AddGraphEdge(vtkIdType u, vtkIdType v);

protected:
  vtkMutableUndirectedGraph();
  ~vtkMutableUndirectedGraph();

  // Description:
  // Graph edge that is reused of AddGraphEdge calls.
  vtkGraphEdge *GraphEdge;

private:
  vtkMutableUndirectedGraph(const vtkMutableUndirectedGraph&);  // Not implemented.
  void operator=(const vtkMutableUndirectedGraph&);  // Not implemented.
};

#endif
