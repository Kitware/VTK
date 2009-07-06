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
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
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
  // If non-null, propertyArr provides properties that will be attached
  // to this vertex. The values in propertyArr must match up with the
  // arrays in the vertex data retrieved by GetVertexData.
  // In a distributed graph, this routine cannot be used to add a vertex
  // if the vertices in the graph have pedigree IDs, because this routine
  // will always add the vertex locally, which may conflict with the
  // proper location of the vertex based on the distribution of the
  // pedigree IDs.
  vtkIdType AddVertex();

  // Description:
  // Adds a new vertex to the graph and returns the id of that vertex.
  // If non-null, propertyArr provides properties that will be attached
  // to this vertex. The values in propertyArr must match up with the
  // arrays in the vertex data retrieved by GetVertexData.
  // If the graph has pedigree IDs for its vertices, and a vertex with
  // the given pedigree ID already exists, its properties will be
  // overwritten with the properties in propertyArr and its ID will be
  // returned. Note that, in a distributed graph with pedigree IDs,
  // the vertex added or found might not be local. In this case,
  // AddVertex will wait until the vertex can be added or found
  // remotely, so that the proper vertex ID can be returned. If you
  // don't actually need to use the vertex ID, consider calling
  // LazyAddVertex, which provides better performance by eliminating
  // the delays associated with returning the vertex ID.
  vtkIdType AddVertex(vtkVariantArray *propertyArr);

  //BTX
  // Description:
  // Adds a vertex with the given pedigree ID to the graph (if a
  // vertex with that pedigree ID does not already exist) and returns
  // the id the vertex with that pedigree ID. If a vertex with the
  // given pedigree ID already exists, its ID will be returned. Note
  // that, in a distributed graph,
  // the vertex added or found might not be local. In this case,
  // AddVertex will wait until the vertex can be added or found
  // remotely, so that the proper vertex ID can be returned. If you
  // don't actually need to use the vertex ID, consider calling
  // LazyAddVertex, which provides better performance by eliminating
  // the delays associated with returning the vertex ID.
  vtkIdType AddVertex(const vtkVariant& pedigreeId);
  //ETX

  // Description:
  // Adds an undirected edge from u to v to the graph and returns a
  // vtkEdgeType structure for that edge. If provided, propertyArr
  // provides edge properties for the newly-created edge. The values
  // in propertyArr must match up with the arrays in the edge data
  // returned by GetEdgeData. Both u and v must refer to vertices
  // already in the graph.
//BTX
  vtkEdgeType AddEdge(vtkIdType u, vtkIdType v);
//ETX

  // Description:
  // Adds an undirected edge from u to v to the graph and returns a
  // vtkEdgeType structure for that edge. If provided, propertyArr
  // provides edge properties for the newly-created edge. The values
  // in propertyArr must match up with the arrays in the edge data
  // returned by GetEdgeData. Both u and v must refer to vertices
  // already in the graph.
//BTX
  vtkEdgeType AddEdge(vtkIdType u, vtkIdType v,
                      vtkVariantArray *propertyArr);
//ETX

  //BTX
  // Description:
  // Adds an undirected edge from u to v to the graph and returns a
  // vtkEdgeType structure for that edge. uPedigreeId is the pedigree
  // ID of a vertex, which will be automatically added if it does not
  // already exist. v must refer to a vertex already in the graph.
  // If provided, propertyArr provides edge properties
  // for the newly-created edge. The values in propertyArr must match
  // up with the arrays in the edge data returned by GetEdgeData.
  vtkEdgeType AddEdge(const vtkVariant& uPedigreeId, vtkIdType v,
                      vtkVariantArray *propertyArr = 0);

  // Description:
  // Adds an undirected edge from u to v to the graph and returns a
  // vtkEdgeType structure for that edge. vPedigreeId is the pedigree
  // ID of a vertex, which will be automatically added if it does not
  // already exist. u must refer to a vertex already in the graph.
  // If provided, propertyArr provides edge properties
  // for the newly-created edge. The values in propertyArr must match
  // up with the arrays in the edge data returned by GetEdgeData.
  vtkEdgeType AddEdge(vtkIdType u, const vtkVariant& vPedigreeId,
                      vtkVariantArray *propertyArr = 0);

  // Description:
  // Adds an undirected edge from u to v to the graph and returns a
  // vtkEdgeType structure for that edge. uPedigreeId and vPedigreeId
  // are the pedigree IDs of vertices u and v, which will be
  // automatically added if they do not already exist. If provided,
  // propertyArr provides edge properties for the newly-created
  // edge. The values in propertyArr must match up with the arrays in
  // the edge data returned by GetEdgeData.
  vtkEdgeType AddEdge(const vtkVariant& uPedigreeId,
                      const vtkVariant& vPedigreeId,
                      vtkVariantArray *propertyArr = 0);
  //ETX

  // Description:
  // Adds a vertex to the graph, and returns the id of that vertex.
  // If non-null, propertyArr provides properties that will be attached
  // to this vertex. The values in propertyArr must match up with the
  // arrays in the vertex data retrieved by GetVertexData.
  // In a distributed graph, this routine cannot be used to add a vertex
  // if the vertices in the graph have pedigree IDs, because this routine
  // will always add the vertex locally, which may conflict with the
  // proper location of the vertex based on the distribution of the
  // pedigree IDs.
  void LazyAddVertex();

  // Description:
  // Adds a vertex to the graph, and returns the id of that vertex.
  // If non-null, propertyArr provides properties that will be attached
  // to this vertex. The values in propertyArr must match up with the
  // arrays in the vertex data retrieved by GetVertexData.
  // If the graph has pedigree IDs for its vertices, and a vertex with
  // the given pedigree ID already exists, its properties will be
  // overwritten with the properties in propertyArr. Note that, in a
  // distributed graph with pedigree IDs, the vertex added or found
  // might not be local. In these cases, LazyAddVertex can be
  // significantly faster than AddVertex, because it does not have to
  // return the resulting vertex ID to the caller.
  void LazyAddVertex(vtkVariantArray *propertyArr);

  //BTX
  // Description:
  // Adds a vertex with the given pedigree ID to the graph (if a
  // vertex with that pedigree ID does not already exist). Note that,
  // in a distributed graph, the vertex added or found might not be
  // local. In these cases, LazyAddVertex can be
  // significantly faster than AddVertex, because it does not have to
  // return the resulting vertex ID to the caller.
  void LazyAddVertex(const vtkVariant& pedigreeId);
  //ETX

  // Description:
  // Adds an undirected edge from u to v to the graph. The edge may not
  // be added immediately, which provides more optimization
  // opportunities for distributed graphs; consequently, the edge
  // itself is not actually returned. u and v must refer to vertices
  // in the graph.
  void LazyAddEdge(vtkIdType u, vtkIdType v);

  // Description:
  // Adds an undirected edge from u to v to the graph. The edge may not
  // be added immediately, which provides more optimization
  // opportunities for distributed graphs; consequently, the edge
  // itself is not actually returned. u and v must refer to vertices
  // in the graph. If non-NULL, propertyArr
  // provides edge properties for the newly-created edge. The values
  // in propertyArr must match up with the arrays in the edge data
  // returned by GetEdgeData.
  void LazyAddEdge(vtkIdType u, vtkIdType v, vtkVariantArray *propertyArr);

  //BTX
  // Description:
  // Adds an undirected edge from u to v to the graph. The edge may not
  // be added immediately, which provides more optimization
  // opportunities for distributed graphs; consequently, the edge
  // itself is not actually returned. uPedigreeId is the pedigree
  // ID of a vertex, which will be automatically added if it does not
  // already exist. v must refer to a vertex in the graph. If
  // provided, propertyArr provides edge properties
  // for the newly-created edge. The values in propertyArr must match
  // up with the arrays in the edge data returned by GetEdgeData.
  void LazyAddEdge(const vtkVariant& uPedigreeId, vtkIdType v,
                   vtkVariantArray *propertyArr = 0);

  // Description:
  // Adds an undirected edge from u to v to the graph. The edge may not
  // be added immediately, which provides more optimization
  // opportunities for distributed graphs; consequently, the edge
  // itself is not actually returned. vPedigreeId is the pedigree
  // ID of a vertex, which will be automatically added if it does not
  // already exist. u must refer to a vertex in the graph. If
  // provided, propertyArr provides edge properties
  // for the newly-created edge. The values in propertyArr must match
  // up with the arrays in the edge data returned by GetEdgeData.
  void LazyAddEdge(vtkIdType u, const vtkVariant& vPedigreeId,
                   vtkVariantArray *propertyArr = 0);

  // Description:
  // Adds an undirected edge from u to v to the graph. The edge may not
  // be added immediately, which provides more optimization
  // opportunities for distributed graphs; consequently, the edge
  // itself is not actually returned. uPedigreeId and vPedigreeId
  // are the pedigree IDs of vertices u and v, which will be
  // automatically added if they do not already exist. If provided,
  // propertyArr provides edge properties for the newly-created
  // edge. The values in propertyArr must match up with the arrays in
  // the edge data returned by GetEdgeData.
  void LazyAddEdge(const vtkVariant& uPedigreeId,
                   const vtkVariant& vPedigreeId,
                   vtkVariantArray *propertyArr = 0);
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
