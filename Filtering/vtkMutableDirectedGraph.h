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
class vtkVariant;

class VTK_FILTERING_EXPORT vtkMutableDirectedGraph : public vtkDirectedGraph
{
public:
  static vtkMutableDirectedGraph *New();
  vtkTypeRevisionMacro(vtkMutableDirectedGraph, vtkDirectedGraph);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Adds a vertex to the graph, and returns the id of that vertex.
  vtkIdType AddVertex();

  // Description:
  // Adds a vertex with the given pedigree ID to the graph (if a
  // vertex with that pedigree ID does not already exist) and returns
  // the id the vertex with that pedigree ID.
  vtkIdType AddVertex(const vtkVariant& pedigreeId);

  // Description:
  // Adds a vertex with the given pedigree ID to the graph (if a
  // vertex with that pedigree ID does not already exist). If vertex
  // is non-NULL, the ID of the vertex with the given pedigree ID will
  // be written into *vertex.  In a distributed graph, passing NULL
  // for the second argument can improve performance when adding
  // non-local edges.
  void AddVertex(const vtkVariant& pedigreeId, vtkIdType *vertex);

  //BTX
  // Description:
  // Adds a vertex, with properties, to the graph, and returns the id of that vertex.
  vtkIdType AddVertex(vtkVariantArray *variantValueArr);

  // Description:
  // Adds a directed edge from u to v to the graph and returns
  // a vtkEdgeType structure for that edge.
  vtkEdgeType AddEdge(vtkIdType u, vtkIdType v);
  
  // Description:
  // Adds a directed edge, with properties, from u to v to the graph and returns
  // a vtkEdgeType structure for that edge.
  vtkEdgeType AddEdge(vtkIdType u, vtkIdType v, vtkVariantArray *variantValueArr);

  // Description:
  // Adds a directed edge from u to v to the graph and returns a
  // vtkEdgeType structure for that edge. uPedigreeId is the pedigree
  // ID of a vertex, which will be automatically added if it does not
  // already exist.
  vtkEdgeType AddEdge(const vtkVariant& uPedigreeId, vtkIdType v);

  // Description:
  // Adds a directed edge from u to v to the graph and returns a
  // vtkEdgeType structure for that edge. vPedigreeId is the pedigree
  // ID of a vertex, which will be automatically added if it does not
  // already exist.
  vtkEdgeType AddEdge(vtkIdType u, const vtkVariant& vPedigreeId);

  // Description:
  // Adds a directed edge from u to v to the graph and returns a
  // vtkEdgeType structure for that edge. uPedigreeId and vPedigreeId
  // are the pedigree IDs of vertices u and v, which will be automatically
  // added if they do not already exist.
  vtkEdgeType AddEdge(const vtkVariant& uPedigreeId, const vtkVariant& vPedigreeId);

  // Description:
  // Adds a directed edge from u to v to the graph. If non-null, edge
  // will receive the newly-constructed edge. For distributed graphs, 
  // passing NULL for edge can improve performance when adding non-local
  // edges.
  void AddEdge(vtkIdType u, vtkIdType v, vtkEdgeType *edge);

  // Description:
  // Adds a directed edge from u to v to the graph. If non-null, edge
  // will receive the newly-constructed edge. For distributed graphs,
  // passing NULL for edge can improve performance when adding
  // non-local edges. uPedigreeId is the pedigree ID of vertex u,
  // which will be automatically added if it does not already exist.
  void AddEdge(const vtkVariant& uPedigreeId, vtkIdType v, vtkEdgeType *edge);

  // Description:
  // Adds a directed edge from u to v to the graph. If non-null, edge
  // will receive the newly-constructed edge. For distributed graphs,
  // passing NULL for edge can improve performance when adding
  // non-local edges. vPedigreeId is the pedigree ID of vertex v,
  // which will be automatically added if it does not already exist.
  void AddEdge(vtkIdType u, const vtkVariant& vPedigreeId, vtkEdgeType *edge);

  // Description:
  // Adds a directed edge from u to v to the graph. If non-null, edge
  // will receive the newly-constructed edge. For distributed graphs,
  // passing NULL for edge can improve performance when adding
  // non-local edges. uPedigreeId and vPedigreeId are the pedigree IDs
  // of vertices u and v, which will be automatically added if they do
  // not already exist.
  void AddEdge(const vtkVariant& uPedigreeId, const vtkVariant& vPedigreeId, 
               vtkEdgeType *edge);
  
  // Description:
  // Adds a directed edge, with properties, from u to v to the graph. If non-null, edge
  // will receive the newly-constructed edge. For distributed graphs, 
  // passing NULL for edge can improve performance when adding non-local
  // edges.
  void AddEdge(vtkIdType u, vtkIdType v, vtkEdgeType *edge, vtkVariantArray *variantValueArr);
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
