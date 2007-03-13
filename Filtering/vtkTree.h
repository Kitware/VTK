/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTree.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTree - A graph representing a hieredgehical tree.
//
// .SECTION Description
// The tree contains a root vertex, and each vertex may contain one or more
// children vertices.
//
// .SECTION Thanks
// Thanks to Patricia Crossno, Ken Moreland, Andrew Wilson and Brian Wylie from
// Sandia National Laboratories for their help in developing this class API.

#ifndef __vtkTree_h
#define __vtkTree_h

#include "vtkAbstractGraph.h"

class vtkIdList;
class vtkVertexLinks;

class VTK_FILTERING_EXPORT vtkTree : public vtkAbstractGraph
{
public:
  static vtkTree* New();
  vtkTypeRevisionMacro(vtkTree,vtkAbstractGraph);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return what type of dataset this is.
  int GetDataObjectType() {return VTK_TREE;}

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
  virtual void GetAdjacentVertices(vtkIdType vertex, vtkIdType& nverts, const vtkIdType* verts);
  virtual void GetInVertices(vtkIdType vertex, vtkIdType& nverts, const vtkIdType* verts);
  virtual void GetOutVertices(vtkIdType vertex, vtkIdType& nverts, const vtkIdType* verts);

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
  // A tree is always considered to have edges directed from the parent
  // to the child, so return true.
  bool GetDirected() { return true; }

  // Description:
  // Get the ID of the root vertex of the tree.
  vtkIdType GetRoot();

  // Description:
  // Get an ID list storing the children of a parent vertex.
  void GetChildren(vtkIdType parent, vtkIdType& nchildren, const vtkIdType*& children);

  // Description:
  // Get the number of children for this vertex.
  vtkIdType GetNumberOfChildren(vtkIdType parent);

  // Description:
  // Get the child vertex ID at a particular index.
  // The index may range from 0 to GetNumberOfChildern(parent)-1.
  vtkIdType GetChild(vtkIdType parent, vtkIdType index);

  // Description:
  // Reorders the children relative to the parent.
  // The children array must have length GetNumberOfChildren(parent),
  // and must contain the parent's existing child ids.
  void ReorderChildren(vtkIdType parent, vtkIdList* children);

  // Description:
  // Get the ID of the parent of a child vertex.
  // The parent of the root vertex is defined to be the root vertex itself.
  vtkIdType GetParent(vtkIdType child);

  // Description:
  // Get the level of the vertex in the tree.  The root vertex has level 0.
  vtkIdType GetLevel(vtkIdType vertex);

  // Description:
  // Return whether the tree is a leaf (i.e. if it has no children).
  bool IsLeaf(vtkIdType vertex);

  // Description:
  // Add a root vertex to the tree, and return the ID of the root.
  // The root must be the first vertex added to the tree.
  vtkIdType AddRoot();

  // Description:
  // Add a child to a parent vertex and return the ID of the child.
  vtkIdType AddChild(vtkIdType parent);

  // Description:
  // Remove a subtree of the tree rooted at a certain vertex.
  void RemoveVertexAndDescendants(vtkIdType vertex);

  // Description:
  // Detach a child vertex from its current parent, and attach
  // it to a new parent.
  void SetParent(vtkIdType child, vtkIdType parent);

  // Description:
  // Set the root of the tree by reversing edges on the path
  // from the new root to the old root.
  void SetRoot(vtkIdType root);

  // Description:
  // Reset the tree to be empty.
  virtual void Initialize();

  // Description:
  // Create a shallow copy of the tree.
  virtual void ShallowCopy(vtkDataObject* object);

  // Description:
  // Create a deep copy of the tree.
  virtual void DeepCopy(vtkDataObject* object);

  // Description:
  // Copy the geometric and topological structure of the tree.
  virtual void CopyStructure(vtkDataSet* ds);

  // Description:
  // Get the id of the edge linking the vertex to its parent.
  vtkIdType GetParentEdge(vtkIdType vertex);

  //BTX
  // Description:
  // Retrieve the tree from vtkInformation.
  static vtkTree* GetData(vtkInformation* info);
  static vtkTree* GetData(vtkInformationVector* v, int i=0);
  //ETX

protected:
  vtkTree();
  ~vtkTree();

  vtkVertexLinks* VertexLinks;

  vtkIdType AddVertex();
  void RemoveVertex(vtkIdType vertex);
  void RemoveVertices(vtkIdType* vertices, vtkIdType size);


  vtkIdType Root;
private:
  vtkTree(const vtkTree &);         // Not implemented.
  void operator=(const vtkTree &);  // Not implemented.
};


#endif
