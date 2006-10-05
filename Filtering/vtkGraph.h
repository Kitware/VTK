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
// .NAME vtkGraph - A graph containing nodes and arcs
//
// .SECTION Description
// A graph is a collection of nodes and a collection of arcs connecting
// the nodes.  Both nodes and arcs are represented by numeric identifiers
// where nodes are in the range [0 number_of_nodes) and arcs are in the range
// [0 number_of_arcs).  vtkGraph allows abitrary addition or removal of 
// nodes and arcs.  When an element (node or arc) is removed, the final element
// identifier changes from number_of_elements - 1 to the identifier of the
// deleted element.
//
// This class may represent a directed or undirected graph.  For directed
// graphs, the in and out node/arc lists are distinct; for undirected graphs
// these functions both return the full list of adjacent nodes/arcs.
//
// .SECTION Thanks
// Thanks to Patricia Crossno, Ken Moreland, Andrew Wilson and Brian Wylie from
// Sandia National Laboratories for their help in developing this class API.

#ifndef __vtkGraph_h
#define __vtkGraph_h

#include "vtkAbstractGraph.h"
class vtkNodeLinks;

class VTK_FILTERING_EXPORT vtkGraph : public vtkAbstractGraph
{
public:
  static vtkGraph* New();
  vtkTypeRevisionMacro(vtkGraph,vtkAbstractGraph);
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
  // Create a deep copy of the graph.
  virtual void DeepCopy(vtkDataObject* object);

  // Description:
  // Create a shallow copy of the graph.
  virtual void ShallowCopy(vtkDataObject* object);

  // Description:
  // Fast access to incident arcs of a graph.
  void GetIncidentArcs(vtkIdType node, vtkIdType& narcs, const vtkIdType*& arcs);
  void GetInArcs(vtkIdType node, vtkIdType& narcs, const vtkIdType*& arcs);
  void GetOutArcs(vtkIdType node, vtkIdType& narcs, const vtkIdType*& arcs);

  // Description:
  // Set the number of nodes in the graph.
  void SetNumberOfNodes(vtkIdType nodes);

  // Description:
  // Set or get whether the graph should be considered directed or undirected.
  vtkGetMacro(Directed, bool);
  vtkSetMacro(Directed, bool);
  vtkBooleanMacro(Directed, bool);

  // Description:
  // Add a node to the graph and return the ID of the new node.
  vtkIdType AddNode();

  // Description:
  // Add an arc from a source node to a target node and return
  // the ID of the new arc.
  vtkIdType AddArc(vtkIdType source, vtkIdType target);

  // Description:
  // Remove the node with the corresponding ID.
  void RemoveNode(vtkIdType node);

  // Description:
  // Remove all arcs incident to the node.
  void ClearNode(vtkIdType node);

  // Description:
  // Remove the arc with the corresponding ID.
  void RemoveArc(vtkIdType arc);

  // Description:
  // Remove a collection of nodes or arcs.
  void RemoveNodes(vtkIdType* nodes, vtkIdType size);
  void RemoveArcs(vtkIdType* arcs, vtkIdType size);

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

  vtkIdTypeArray* Arcs;          // Size: number of arcs
  vtkNodeLinks*   NodeLinks;

  bool Directed;
private:
  vtkGraph(const vtkGraph &);  // Not implemented.
  void operator=(const vtkGraph &);  // Not implemented.
};


#endif
