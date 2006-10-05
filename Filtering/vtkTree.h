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
// .NAME vtkTree - A graph representing a hierarchical tree.
//
// .SECTION Description
// The tree contains a root node, and each node may contain one or more
// children nodes.
//
// .SECTION Thanks
// Thanks to Patricia Crossno, Ken Moreland, Andrew Wilson and Brian Wylie from
// Sandia National Laboratories for their help in developing this class API.

#ifndef __vtkTree_h
#define __vtkTree_h

#include "vtkAbstractGraph.h"

class vtkNodeLinks;

class VTK_FILTERING_EXPORT vtkTree : public vtkAbstractGraph
{
public:
  static vtkTree* New();
  vtkTypeRevisionMacro(vtkTree,vtkAbstractGraph);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The number of nodes in the graph.
  virtual vtkIdType GetNumberOfNodes();

  // Description:
  // The number of arcs in the graph.
  virtual vtkIdType GetNumberOfArcs();

  // Description:
  // Fill nodes with the node IDs of every node adjacent to a certain node.
  // For an undirected graph, these all return the same nodes.
  virtual void GetAdjacentNodes(vtkIdType node, vtkGraphIdList* nodes);
  virtual void GetInNodes(vtkIdType node, vtkGraphIdList* nodes);
  virtual void GetOutNodes(vtkIdType node, vtkGraphIdList* nodes);

  // Description:
  // Fill arcs with the arc IDs of every arc incident to a certain node.
  // For an undirected graph, these all return the same arcs.
  virtual void GetIncidentArcs(vtkIdType node, vtkGraphIdList* arcs);
  virtual void GetInArcs(vtkIdType node, vtkGraphIdList* arcs);
  virtual void GetOutArcs(vtkIdType node, vtkGraphIdList* arcs);

  // Description:
  // Get the total, or number of incoming or outgoing arcs incident to a node.
  // For an undirected graph, these all return the same value.
  virtual vtkIdType GetDegree(vtkIdType node);
  virtual vtkIdType GetInDegree(vtkIdType node);
  virtual vtkIdType GetOutDegree(vtkIdType node);

  // Description:
  // Return the source node of an arc.
  virtual vtkIdType GetSourceNode(vtkIdType arc);

  // Description:
  // Return the target node of an arc.
  virtual vtkIdType GetTargetNode(vtkIdType arc);

  // Description:
  // Return the other node adjacent to an arc.
  virtual vtkIdType GetOppositeNode(vtkIdType arc, vtkIdType node);

  // Description:
  // A tree is always considered to have arcs directed from the parent
  // to the child, so return true.
  bool GetDirected() { return true; }

  // Description:
  // Get the ID of the root node of the tree.
  vtkIdType GetRoot();

  // Description:
  // Get an ID list storing the children of a parent node.
  void GetChildren(vtkIdType parent, vtkIdType& nchildren, const vtkIdType*& children);

  // Description:
  // Get the number of children for this node.
  vtkIdType GetNumberOfChildren(vtkIdType parent);

  // Description:
  // Get the ID of the parent of a child node.
  // The parent of the root node is defined to be the root node itself.
  vtkIdType GetParent(vtkIdType child);

  // Description:
  // Get the level of the node in the tree.  The root node has level 0.
  vtkIdType GetLevel(vtkIdType node);

  // Description:
  // Return whether the tree is a leaf (i.e. if it has no children).
  bool IsLeaf(vtkIdType node);

  // Description:
  // Add a root node to the tree, and return the ID of the root.
  // The root must be the first node added to the tree.
  vtkIdType AddRoot();

  // Description:
  // Add a child to a parent node and return the ID of the child.
  vtkIdType AddChild(vtkIdType parent);

  // Description:
  // Remove a subtree of the tree rooted at a certain node.
  void RemoveNodeAndDescendants(vtkIdType node);

  // Description:
  // Detach a child node from its current parent, and attach
  // it to a new parent.
  void SetParent(vtkIdType child, vtkIdType parent);

  // Description:
  // Set the root of the tree by reversing arcs on the path
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
  // Retrieve the tree from vtkInformation.
  static vtkTree* GetData(vtkInformation* info);
  static vtkTree* GetData(vtkInformationVector* v, int i=0);

protected:
  vtkTree();
  ~vtkTree();

  vtkNodeLinks* NodeLinks;

  vtkIdType GetParentArc(vtkIdType node);
  vtkIdType AddNode();
  void RemoveNode(vtkIdType node);
  void RemoveNodes(vtkIdType* nodes, vtkIdType size);


  vtkIdType Root;
private:
  vtkTree(const vtkTree &);         // Not implemented.
  void operator=(const vtkTree &);  // Not implemented.
};


#endif
