/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadricDecimation.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkQuadricDecimation - reduce the number of triangles in a mesh
// .SECTION Description
// vtkQuadricDecimation is a filter to reduce the number of triangles in
// a triangle mesh, forming a good approximation to the original geometry. 
// The input to vtkQuadricDecimation is a vtkPolyData object, and only
// triangles are treated. If you desire to decimate polygonal meshes, first
// triangulate the polygons with vtkTriangleFilter object.
//
// The quadric error metric used is the one outlined in Hughues Hoppe's
// Vis '99 paper, "New Quadric Metric for Simplifying Meshes with Appearance
// Attributes."


#ifndef __vtkQuadricDecimation_h
#define __vtkQuadricDecimation_h

#include "vtkPolyDataToPolyDataFilter.h"
#include "vtkEdgeTable.h"
#include "vtkPriorityQueue.h"
#include "vtkIdList.h"

typedef struct {
  float *Quadric;
} VTK_ERROR_QUADRIC;

class VTK_GRAPHICS_EXPORT vtkQuadricDecimation : public vtkPolyDataToPolyDataFilter
{
public:
  vtkTypeRevisionMacro(vtkQuadricDecimation, vtkPolyDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkQuadricDecimation *New();

  // Description:
  // Set/Get the maximum allowable cost of collapsing an edge.
  vtkSetMacro(MaximumCost, float);
  vtkGetMacro(MaximumCost, float);
  
  // Description:
  // Set/Get the maximum number of edges to collapse.
  vtkSetMacro(MaximumCollapsedEdges, int);
  vtkGetMacro(MaximumCollapsedEdges, int);

  // Description:
  // For debugging: the last edge / triangles that were collapsed.
  vtkPolyData *GetTestOutput() {return this->GetOutput(1);}
  
protected:
  vtkQuadricDecimation();
  ~vtkQuadricDecimation();

  void Execute();

  // Description:
  // Compute quadric for this vertex.
  void ComputeQuadric(vtkIdType pointId);

  // Description:
  // Add the quadrics for these 2 points since the edge between them has
  // been collapsed.
  void AddQuadric(vtkIdType oldPtId, vtkIdType newPtId);
  
  // Description:
  // Compute cost for contracting this edge and the point that gives us this
  // cost.
  float ComputeCost(vtkIdType edgeId, float x[3], vtkPointData *pd);

  // Description:
  // Find all edges that will have an endpoint change ids because of an edge
  // collapse.  p1Id and p2Id are the endpoints of the edge.  p2Id is the
  // pointId being removed.
  void FindAffectedEdges(vtkIdType p1Id, vtkIdType p2Id, vtkIdList *edges);
  
  // Description:
  // Find a cell that uses this edge.
  vtkIdType GetEdgeCellId(vtkIdType p1Id, vtkIdType p2Id);
  
  // Description:
  // Find out how many components there are for each attribute for this
  // poly data.
  void GetAttributeComponents();
  
  float MaximumCost;
  int MaximumCollapsedEdges;
  int NumberOfCollapsedEdges;
  vtkEdgeTable *Edges;
  vtkIdList *EndPoint1List;
  vtkIdList *EndPoint2List;
  vtkPriorityQueue *EdgeCosts;
  VTK_ERROR_QUADRIC *ErrorQuadrics;
  int AttributeComponents[6];
  int NumberOfComponents;
  vtkPolyData *Mesh;
private:
  vtkQuadricDecimation(const vtkQuadricDecimation&);  // Not implemented.
  void operator=(const vtkQuadricDecimation&);  // Not implemented.
};

#endif
