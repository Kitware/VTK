/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolygonalSurfaceContourLineInterpolator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPolygonalSurfaceContourLineInterpolator.h"

#include "vtkObjectFactory.h"
#include "vtkContourRepresentation.h"
#include "vtkPolyData.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkCellArray.h"
#include "vtkCell.h"
#include "vtkMath.h"
#include "vtkCellLocator.h"
#include "vtkPolygonalSurfacePointPlacer.h"
#include "vtkIdList.h"
#include "vtkDijkstraGraphGeodesicPath.h"

vtkStandardNewMacro(vtkPolygonalSurfaceContourLineInterpolator);

//----------------------------------------------------------------------
vtkPolygonalSurfaceContourLineInterpolator
::vtkPolygonalSurfaceContourLineInterpolator()
{
  this->LastInterpolatedVertexIds[0] = -1;
  this->LastInterpolatedVertexIds[1] = -1;
  this->DistanceOffset               = 0.0;
  this->DijkstraGraphGeodesicPath = vtkDijkstraGraphGeodesicPath::New();
}

//----------------------------------------------------------------------
vtkPolygonalSurfaceContourLineInterpolator
::~vtkPolygonalSurfaceContourLineInterpolator()
{
  this->DijkstraGraphGeodesicPath->Delete();
}

//----------------------------------------------------------------------
int vtkPolygonalSurfaceContourLineInterpolator::UpdateNode(
    vtkRenderer *, vtkContourRepresentation *,
    double * vtkNotUsed(node), int vtkNotUsed(idx) )
{
  return 0;
}

//----------------------------------------------------------------------
int vtkPolygonalSurfaceContourLineInterpolator::InterpolateLine(
                          vtkRenderer *,
                          vtkContourRepresentation *rep,
                          int idx1, int idx2 )
{
  vtkPolygonalSurfacePointPlacer *placer =
    vtkPolygonalSurfacePointPlacer::SafeDownCast(rep->GetPointPlacer());
  if (!placer)
    {
    return 1;
    }

  double p1[3], p2[3], p[3];
  rep->GetNthNodeWorldPosition( idx1, p1 );
  rep->GetNthNodeWorldPosition( idx2, p2 );

  typedef vtkPolygonalSurfacePointPlacer::Node NodeType;
  NodeType *nodeBegin = placer->GetNodeAtWorldPosition(p1);
  NodeType *nodeEnd   = placer->GetNodeAtWorldPosition(p2);
  if (nodeBegin->PolyData != nodeEnd->PolyData)
    {
    return 1;
    }

  // Find the starting and ending point id's
  vtkIdType beginVertId = -1, endVertId = -1;
  double minDistance;
  if (nodeBegin->CellId == -1)
    {
    // If no cell is specified, use the pointid instead
    beginVertId = nodeBegin->PointId;
    }
  else
    {
    vtkCell *cellBegin = nodeBegin->PolyData->GetCell(nodeBegin->CellId);
    vtkPoints *cellBeginPoints = cellBegin->GetPoints();

    minDistance = VTK_DOUBLE_MAX;
    for (int i = 0; i < cellBegin->GetNumberOfPoints(); i++)
      {
      cellBeginPoints->GetPoint(i, p);
      double distance = vtkMath::Distance2BetweenPoints( p, p1 );
      if (distance < minDistance)
        {
        beginVertId = cellBegin->GetPointId(i);
        minDistance = distance;
        }
      }
    }

    if (nodeEnd->CellId == -1)
      {
      // If no cell is specified, use the pointid instead
      endVertId = nodeEnd->PointId;
      }
    else
      {
      vtkCell *cellEnd   = nodeEnd->PolyData->GetCell(nodeEnd->CellId);
      vtkPoints *cellEndPoints   = cellEnd->GetPoints();

      minDistance = VTK_DOUBLE_MAX;
      for (int i = 0; i < cellEnd->GetNumberOfPoints(); i++)
        {
        cellEndPoints->GetPoint(i, p);
        double distance = vtkMath::Distance2BetweenPoints( p, p2 );
        if (distance < minDistance)
          {
          endVertId = cellEnd->GetPointId(i);
          minDistance = distance;
          }
        }
      }

  if (beginVertId == -1 || endVertId == -1)
    {
    // Could not find the starting and ending cells. We can't interpolate.
    return 0;
    }


  // Now compute the shortest path through the surface mesh along its
  // edges using Dijkstra.

  this->DijkstraGraphGeodesicPath->SetInput( nodeBegin->PolyData );
  this->DijkstraGraphGeodesicPath->SetStartVertex( endVertId );
  this->DijkstraGraphGeodesicPath->SetEndVertex( beginVertId );
  this->DijkstraGraphGeodesicPath->Update();

  vtkPolyData *pd = this->DijkstraGraphGeodesicPath->GetOutput();

  // We assume there's only one cell of course
  vtkIdType npts = 0, *pts = NULL;
  pd->GetLines()->InitTraversal();
  pd->GetLines()->GetNextCell( npts, pts );

  // Get the vertex normals if there is a height offset. The offset at
  // each node of the graph is in the direction of the vertex normal.

  vtkIdList *vertexIds = this->DijkstraGraphGeodesicPath->GetIdList();
  double vertexNormal[3];
  vtkDataArray *vertexNormals = NULL;
  if (this->DistanceOffset != 0.0)
    {
    vertexNormals = nodeBegin->PolyData->GetPointData()->GetNormals();
    }

  for (int n = 0; n < npts; n++)
    {
    pd->GetPoint( pts[n], p );

    // This is the id of the point on the polygonal surface.
    const vtkIdType ptId = vertexIds->GetId(n);

    // Offset the point in the direction of the normal, if a distance
    // offset is specified.
    if (vertexNormals)
      {
      vertexNormals->GetTuple( ptId, vertexNormal );
      p[0] += vertexNormal[0] * this->DistanceOffset;
      p[1] += vertexNormal[1] * this->DistanceOffset;
      p[2] += vertexNormal[2] * this->DistanceOffset;
      }

    // Add this point as an intermediate node of the contour. Store tehe
    // ptId if necessary.
    rep->AddIntermediatePointWorldPosition( idx1, p, ptId );
    }

  this->LastInterpolatedVertexIds[0] = beginVertId;
  this->LastInterpolatedVertexIds[1] = endVertId;

  // Also set the start and end node on the contour rep
  rep->GetNthNode(idx1)->PointId = beginVertId;
  rep->GetNthNode(idx2)->PointId = endVertId;

  return 1;
}

//----------------------------------------------------------------------
void vtkPolygonalSurfaceContourLineInterpolator
::GetContourPointIds( vtkContourRepresentation *rep, vtkIdList *ids )
{
  // Get the number of points in the contour and pre-allocate size

  const int nNodes = rep->GetNumberOfNodes();

  vtkIdType nPoints = 0;
  for (int i = 0; i < nNodes; i++)
    {
    // 1 for the node and then the number of points.
    nPoints += (rep->GetNthNode(i)->Points.size() + 1);
    }

  ids->SetNumberOfIds(nPoints);

  // Now fill the point ids

  int idx = 0;
  for (int i = 0; i < nNodes; i++)
    {
    vtkContourRepresentationNode *node = rep->GetNthNode(i);
    ids->SetId(idx++, node->PointId);
    const int nIntermediatePts = static_cast< int >(node->Points.size());

    for (int j = 0; j < nIntermediatePts; j++)
      {
      ids->SetId(idx++, node->Points[j]->PointId);
      }
    }
}

//----------------------------------------------------------------------
void vtkPolygonalSurfaceContourLineInterpolator::PrintSelf(
                              ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "DistanceOffset: " << this->DistanceOffset << endl;
}
