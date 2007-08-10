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
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
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
  // The number of points is the same as the number of vertices.
  vtkIdType GetNumberOfPoints() { return this->GetNumberOfVertices(); }

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
  // The number of cells is the same as the number of edges.
  vtkIdType GetNumberOfCells() { return this->GetNumberOfEdges(); }

  // Description:
  // The cells associated with a point corresponds to the
  // edges adjacent to a vertex, so this method is identical to GetVertexEdges().
  void GetPointCells(vtkIdType ptId, vtkIdList* cellIds);

  // Description:
  // All edges have two endpoints, so the maximum cell size is two.
  int GetMaxCellSize() { return 2; }

  // Description:
  // All edges are represented by VTK_LINE cells.
  int GetCellType(vtkIdType cellId);

  // Description:
  // For an edge, get a line from the source to the target.
  vtkCell* GetCell(vtkIdType cellId);
  void GetCell(vtkIdType cellId, vtkGenericCell * cell);

  // Description:
  // For an edge, fill ptIds with the source and target IDs.
  void GetCellPoints(vtkIdType cellId, vtkIdList* ptIds);

  // Description:
  // The vertex data of a graph is the same as the point data of the data set.
  vtkPointData* GetVertexData() { return this->GetPointData(); }

  // Description:
  // The edge data of a graph is the same as the cell data of the data set.
  vtkCellData* GetEdgeData() { return this->GetCellData(); }

  // Description:
  // The number of vertices in the graph.
  virtual vtkIdType GetNumberOfVertices() = 0;

  // Description:
  // The number of edges in the graph.
  virtual vtkIdType GetNumberOfEdges() = 0;

  // Description:
  // Fill vertices with the vertex IDs of every vertex adjacent to a certain vertex.
  // For an undirected graph, these all return the same vertices.
  virtual void GetAdjacentVertices(vtkIdType vertex, vtkGraphIdList* vertices) = 0;
  virtual void GetInVertices(vtkIdType vertex, vtkGraphIdList* vertices) = 0;
  virtual void GetOutVertices(vtkIdType vertex, vtkGraphIdList* vertices) = 0;

  // Description:
  // Fill edges with the edge IDs of every edge incident to a certain vertex.
  // For an undirected graph, these all return the same edges.
  virtual void GetIncidentEdges(vtkIdType vertex, vtkGraphIdList* edges) = 0;
  virtual void GetInEdges(vtkIdType vertex, vtkGraphIdList* edges) = 0;
  virtual void GetOutEdges(vtkIdType vertex, vtkGraphIdList* edges) = 0;

  // Description:
  // Get the total, or number of incoming or outgoing edges incident to a vertex.
  // For an undirected graph, these all return the same value.
  virtual vtkIdType GetDegree(vtkIdType vertex) = 0;
  virtual vtkIdType GetInDegree(vtkIdType vertex) = 0;
  virtual vtkIdType GetOutDegree(vtkIdType vertex) = 0;

  // Description:
  // Return the source vertex of an edge.
  virtual vtkIdType GetSourceVertex(vtkIdType edge) = 0;

  // Description:
  // Return the target vertex of an edge.
  virtual vtkIdType GetTargetVertex(vtkIdType edge) = 0;

  // Description:
  // Return the other vertex adjacent to an edge.
  virtual vtkIdType GetOppositeVertex(vtkIdType edge, vtkIdType vertex) = 0;

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

  //BTX
  // Description:
  // Retrieve an abstract graph from an information vector.
  static vtkAbstractGraph* GetData(vtkInformation* info);
  static vtkAbstractGraph* GetData(vtkInformationVector* v, int i=0);
  //ETX

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

