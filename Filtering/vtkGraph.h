/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraph.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGraph - A graph containing vertices and edges
//
// .SECTION Description
// A graph is a collection of vertices and a collection of edges connecting
// the vertices.  Both vertices and edges are represented by numeric identifiers
// where vertices are in the range [0 number_of_vertices) and edges are in the range
// [0 number_of_edges).  vtkGraph allows abitrary addition or removal of 
// vertices and edges.  When an element (vertex or edge) is removed, the final element
// identifier changes from number_of_elements - 1 to the identifier of the
// deleted element.
//
// This class may represent a directed or undirected graph.  For directed
// graphs, the in and out vertex/edge lists are distinct; for undirected graphs
// these functions both return the full list of adjacent vertices/edges.
//
// .SECTION Thanks
// Thanks to Patricia Crossno, Ken Moreland, Andrew Wilson and Brian Wylie from
// Sandia National Laboratories for their help in developing this class API.

#ifndef __vtkGraph_h
#define __vtkGraph_h

#include "vtkAbstractGraph.h"
class vtkVertexLinks;

class VTK_FILTERING_EXPORT vtkGraph : public vtkAbstractGraph
{
public:
  static vtkGraph* New();
  vtkTypeRevisionMacro(vtkGraph,vtkAbstractGraph);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return what type of dataset this is.
  int GetDataObjectType() {return VTK_GRAPH;}

  // Description:
  // The number of vertices in the graph.
  virtual vtkIdType GetNumberOfVertices();

  // Description:
  // The number of edges in the graph.
  virtual vtkIdType GetNumberOfEdges();

  // Description:
  // Fill vertices with the vertex IDs of every vertex adjacent to a certain vertex.
  // For an undirected graph, these all return the same vertices.
  virtual void GetAdjacentVertices(vtkIdType vertex, vtkGraphIdList* vertices);
  virtual void GetInVertices(vtkIdType vertex, vtkGraphIdList* vertices);
  virtual void GetOutVertices(vtkIdType vertex, vtkGraphIdList* vertices);

  // Description:
  // Fill edges with the edge IDs of every edge incident to a certain vertex.
  // For an undirected graph, these all return the same edges.
  virtual void GetIncidentEdges(vtkIdType vertex, vtkGraphIdList* edges);
  virtual void GetInEdges(vtkIdType vertex, vtkGraphIdList* edges);
  virtual void GetOutEdges(vtkIdType vertex, vtkGraphIdList* edges);

  // Description:
  // Get the total, or number of incoming or outgoing edges incident to a vertex.
  // For an undirected graph, these all return the same value.
  virtual vtkIdType GetDegree(vtkIdType vertex);
  virtual vtkIdType GetInDegree(vtkIdType vertex);
  virtual vtkIdType GetOutDegree(vtkIdType vertex);

  // Description:
  // Return the source vertex of an edge.
  virtual vtkIdType GetSourceVertex(vtkIdType edge);

  // Description:
  // Return the target vertex of an edge.
  virtual vtkIdType GetTargetVertex(vtkIdType edge);

  // Description:
  // Return the other vertex adjacent to an edge.
  virtual vtkIdType GetOppositeVertex(vtkIdType edge, vtkIdType vertex);

  // Description:
  // Create a deep copy of the graph.
  virtual void DeepCopy(vtkDataObject* object);

  // Description:
  // Create a shallow copy of the graph.
  virtual void ShallowCopy(vtkDataObject* object);

  // Description:
  // Fast access to incident edges of a graph.
  void GetIncidentEdges(vtkIdType vertex, vtkIdType& nedges, const vtkIdType*& edges);
  void GetInEdges(vtkIdType vertex, vtkIdType& nedges, const vtkIdType*& edges);
  void GetOutEdges(vtkIdType vertex, vtkIdType& nedges, const vtkIdType*& edges);

  // Description:
  // Set the number of vertices in the graph.
  void SetNumberOfVertices(vtkIdType vertices);

  // Description:
  // Set or get whether the graph should be considered directed or undirected.
  vtkGetMacro(Directed, bool);
  vtkSetMacro(Directed, bool);
  vtkBooleanMacro(Directed, bool);

  // Description:
  // Add a vertex to the graph and return the ID of the new vertex.
  vtkIdType AddVertex();

  // Description:
  // Add an edge from a source vertex to a target vertex and return
  // the ID of the new edge.
  vtkIdType AddEdge(vtkIdType source, vtkIdType target);

  // Description:
  // Remove the vertex with the corresponding ID.
  void RemoveVertex(vtkIdType vertex);

  // Description:
  // Remove all edges incident to the vertex.
  void ClearVertex(vtkIdType vertex);

  // Description:
  // Remove the edge with the corresponding ID.
  void RemoveEdge(vtkIdType edge);

  // Description:
  // Remove a collection of vertices or edges.
  void RemoveVertices(vtkIdType* vertices, vtkIdType size);
  void RemoveEdges(vtkIdType* edges, vtkIdType size);

  // Description:
  // Initialize to an empty graph.
  virtual void Initialize();

  // Description:
  // Retrieve a graph from an information vector.
  static vtkGraph* GetData(vtkInformation* info);
  static vtkGraph* GetData(vtkInformationVector* v, int i=0);

protected:
  vtkGraph();
  ~vtkGraph();

  vtkIdTypeArray* Edges;          // Size: number of edges
  vtkVertexLinks*   VertexLinks;

  bool Directed;
private:
  vtkGraph(const vtkGraph &);  // Not implemented.
  void operator=(const vtkGraph &);  // Not implemented.
};


#endif
