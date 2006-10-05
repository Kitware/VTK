/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractGraph.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAbstractGraph - The base class for graph classes (i.e. graph and tree)
//
// .SECTION Description
// This class provides a common data structure and read-access API for
// graphs.  The write-access API is left to the subclasses so each may
// restrict the structure of the graph as needed.
//
// .SECTION Thanks
// Thanks to Patricia Crossno, Ken Moreland, Andrew Wilson and Brian Wylie from
// Sandia National Laboratories for their help in developing this class API.

#ifndef __vtkAbstractGraph_h
#define __vtkAbstractGraph_h

#include "vtkPointSet.h"
#include "vtkCellType.h"
#include "vtkIdTypeArray.h"

class vtkGraphIdList;
class vtkPointData;
class vtkCellData;
class vtkLine;
class vtkDynamicHeap;

class VTK_FILTERING_EXPORT vtkAbstractGraph : public vtkPointSet
{
public:
  vtkTypeRevisionMacro(vtkAbstractGraph,vtkPointSet);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The number of points is the same as the number of nodes.
  vtkIdType GetNumberOfPoints() { return this->GetNumberOfNodes(); }

  // Description:
  // These methods return the point (0,0,0) until the points structure
  // is created.
  double *GetPoint(vtkIdType ptId);
  void GetPoint(vtkIdType ptId, double x[3]);

  // Description:
  // Returns the points array for this graph.
  // If points is not yet constructed, generates and returns 
  // a new points array filled with (0,0,0) coordinates.
  vtkPoints* GetPoints();

  // Description:
  // The number of cells is the same as the number of arcs.
  vtkIdType GetNumberOfCells() { return this->GetNumberOfArcs(); }

  // Description:
  // The cells associated with a point corresponds to the
  // arcs adjacent to a node, so this method is identical to GetNodeArcs().
  void GetPointCells(vtkIdType ptId, vtkIdList* cellIds);

  // Description:
  // All arcs have two endpoints, so the maximum cell size is two.
  int GetMaxCellSize() { return 2; }

  // Description:
  // All arcs are represented by VTK_LINE cells.
  int GetCellType(vtkIdType vtkNotUsed(cellId)) { return VTK_LINE; }

  // Description:
  // For an arc, get a line from the source to the target.
  vtkCell* GetCell(vtkIdType cellId);
  void GetCell(vtkIdType cellId, vtkGenericCell * cell);

  // Description:
  // For an arc, fill ptIds with the source and target IDs.
  void GetCellPoints(vtkIdType cellId, vtkIdList* ptIds);

  // Description:
  // The node data of a graph is the same as the point data of the data set.
  vtkPointData* GetNodeData() { return this->GetPointData(); }

  // Description:
  // The arc data of a graph is the same as the cell data of the data set.
  vtkCellData* GetArcData() { return this->GetCellData(); }

  // Description:
  // The number of nodes in the graph.
  virtual vtkIdType GetNumberOfNodes() = 0;

  // Description:
  // The number of arcs in the graph.
  virtual vtkIdType GetNumberOfArcs() = 0;

  // Description:
  // Fill nodes with the node IDs of every node adjacent to a certain node.
  // For an undirected graph, these all return the same nodes.
  virtual void GetAdjacentNodes(vtkIdType node, vtkGraphIdList* nodes) = 0;
  virtual void GetInNodes(vtkIdType node, vtkGraphIdList* nodes) = 0;
  virtual void GetOutNodes(vtkIdType node, vtkGraphIdList* nodes) = 0;

  // Description:
  // Fill arcs with the arc IDs of every arc incident to a certain node.
  // For an undirected graph, these all return the same arcs.
  virtual void GetIncidentArcs(vtkIdType node, vtkGraphIdList* arcs) = 0;
  virtual void GetInArcs(vtkIdType node, vtkGraphIdList* arcs) = 0;
  virtual void GetOutArcs(vtkIdType node, vtkGraphIdList* arcs) = 0;

  // Description:
  // Get the total, or number of incoming or outgoing arcs incident to a node.
  // For an undirected graph, these all return the same value.
  virtual vtkIdType GetDegree(vtkIdType node) = 0;
  virtual vtkIdType GetInDegree(vtkIdType node) = 0;
  virtual vtkIdType GetOutDegree(vtkIdType node) = 0;

  // Description:
  // Return the source node of an arc.
  virtual vtkIdType GetSourceNode(vtkIdType arc) = 0;

  // Description:
  // Return the target node of an arc.
  virtual vtkIdType GetTargetNode(vtkIdType arc) = 0;

  // Description:
  // Return the other node adjacent to an arc.
  virtual vtkIdType GetOppositeNode(vtkIdType arc, vtkIdType node) = 0;

  // Description:
  // Return whether the graph is directed.
  // This method must be implemented in all subclasses of vtkAbstractGraph.
  virtual bool GetDirected() = 0;

  // Description:
  // Initialize the graph to an empty graph.
  virtual void Initialize();

  // Description:
  // Create a deep copy of the graph.
  virtual void DeepCopy(vtkDataObject* object);

  // Description:
  // Create a shallow copy of the graph.
  virtual void ShallowCopy(vtkDataObject* object);

protected:
  vtkAbstractGraph();
  ~vtkAbstractGraph();

private:
  vtkAbstractGraph(const vtkAbstractGraph &);  // Not implemented.
  void operator=(const vtkAbstractGraph &);    // Not implemented.

  vtkLine* Line;
  static double DefaultPoint[3];
};

#endif

