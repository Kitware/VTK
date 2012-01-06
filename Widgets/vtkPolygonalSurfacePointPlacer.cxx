/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolygonalSurfacePointPlacer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPolygonalSurfacePointPlacer.h"

#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkProp.h"
#include "vtkPropCollection.h"
#include "vtkPolyDataCollection.h"
#include "vtkCellPicker.h"
#include "vtkAssemblyPath.h"
#include "vtkAssemblyNode.h"
#include "vtkInteractorObserver.h"
#include "vtkMapper.h"
#include "vtkPolyData.h"
#include "vtkMath.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkIdList.h"

#include <vector>

class vtkPolygonalSurfacePointPlacerInternals
{

public:
  typedef std::vector<
    vtkPolygonalSurfacePointPlacerNode * > NodesContainerType;

  NodesContainerType Nodes;

  ~vtkPolygonalSurfacePointPlacerInternals()
    {
    for (unsigned int i = 0; i < this->Nodes.size(); i++)
      {
      delete this->Nodes[i];
      }
    this->Nodes.clear();
    }

  vtkPolygonalSurfacePointPlacerNode
    *GetNodeAtSurfaceWorldPosition( double worldPos[3] )
    {
    const double tolerance = 0.0005;
    for (unsigned int i = 0; i < this->Nodes.size(); i++)
      {
      if (vtkMath::Distance2BetweenPoints(
          this->Nodes[i]->SurfaceWorldPosition, worldPos ) < tolerance)
        {
        return this->Nodes[i];
        }
      }
    return NULL;
    }

  vtkPolygonalSurfacePointPlacerNode
    *GetNodeAtWorldPosition( double worldPos[3] )
    {
    const double tolerance = 0.0005;
    for (unsigned int i = 0; i < this->Nodes.size(); i++)
      {
      if (vtkMath::Distance2BetweenPoints(
          this->Nodes[i]->WorldPosition, worldPos ) < tolerance)
        {
        return this->Nodes[i];
        }
      }
    return NULL;
    }

    vtkPolygonalSurfacePointPlacerNode
      *InsertNodeAtCurrentPickPosition( vtkCellPicker *picker,
                                        const double distanceOffset,
                                        int snapToClosestPoint )
    {
    double worldPos[3];
    picker->GetPickPosition(worldPos);

    // Get a node at this position if one exists and overwrite it
    // with the current pick position. If one doesn't exist, add
    // a new node.
    vtkPolygonalSurfacePointPlacerNode
       * node = this->GetNodeAtSurfaceWorldPosition(worldPos);
    if (!node)
      {
      node = new vtkPolygonalSurfacePointPlacerNode;
      this->Nodes.push_back(node);
      }

    vtkMapper *mapper =
      vtkMapper::SafeDownCast(picker->GetMapper());
    if (!mapper)
      {
      return NULL;
      }

    // Get the underlying dataset
    vtkPolyData *pd = vtkPolyData::SafeDownCast(mapper->GetInput());
    if (!pd)
      {
      return NULL;
      }

    node->CellId = picker->GetCellId();
    picker->GetPCoords(node->ParametricCoords);

    // translate to the closest point on that cell, if requested

    if (snapToClosestPoint)
      {
      vtkIdList *ids = vtkIdList::New();
      pd->GetCellPoints( picker->GetCellId(), ids );
      double p[3], minDistance = VTK_DOUBLE_MAX;
      for (vtkIdType i = 0; i < ids->GetNumberOfIds(); ++i)
        {
        pd->GetPoints()->GetPoint(ids->GetId(i), p);
        const double dist2 = vtkMath::Distance2BetweenPoints(
            worldPos, pd->GetPoints()->GetPoint(ids->GetId(i)));
        if (dist2 < minDistance)
          {
          minDistance = dist2;
          worldPos[0] = p[0];
          worldPos[1] = p[1];
          worldPos[2] = p[2];
          }
        }
      ids->Delete();
      }

    node->SurfaceWorldPosition[0] = worldPos[0];
    node->SurfaceWorldPosition[1] = worldPos[1];
    node->SurfaceWorldPosition[2] = worldPos[2];
    node->PolyData = pd;
    double cellNormal[3];

    if (distanceOffset != 0.0)
      {
      pd->GetCellData()->GetNormals()->GetTuple( node->CellId, cellNormal );

      // Polyline can be drawn on polydata at a height offset.
      for (unsigned int i =0; i < 3; i++)
        {
        node->WorldPosition[i] =
          node->SurfaceWorldPosition[i] + cellNormal[i] * distanceOffset;
        }
      }
    else
      {
      for (unsigned int i =0; i < 3; i++)
        {
        node->WorldPosition[i] = node->SurfaceWorldPosition[i];
        }
      }
    return node;
    }


    vtkPolygonalSurfacePointPlacerNode
      *InsertNodeAtCurrentPickPosition( vtkPolyData *pd,
                                        double worldPos[3],
                                        vtkIdType cellId,
                                        vtkIdType pointId,
                                        const double vtkNotUsed(distanceOffset),
                                        int vtkNotUsed(snapToClosestPoint) )
    {

    // Get a node at this position if one exists and overwrite it
    // with the current pick position. If one doesn't exist, add
    // a new node.
    vtkPolygonalSurfacePointPlacerNode
       * node = this->GetNodeAtSurfaceWorldPosition(worldPos);
    if (!node)
      {
      node = new vtkPolygonalSurfacePointPlacerNode;
      this->Nodes.push_back(node);
      }

    node->CellId = cellId;
    node->PointId = pointId;

    node->SurfaceWorldPosition[0] = worldPos[0];
    node->SurfaceWorldPosition[1] = worldPos[1];
    node->SurfaceWorldPosition[2] = worldPos[2];
    node->PolyData = pd;

    for (unsigned int i =0; i < 3; i++)
      {
      node->WorldPosition[i] = node->SurfaceWorldPosition[i];
      }
    return node;
    }
   //ashish
};

vtkStandardNewMacro(vtkPolygonalSurfacePointPlacer);

//----------------------------------------------------------------------
vtkPolygonalSurfacePointPlacer::vtkPolygonalSurfacePointPlacer()
{
  this->Polys           = vtkPolyDataCollection::New();
  this->CellPicker      = vtkCellPicker::New();
  this->CellPicker->PickFromListOn();
  this->CellPicker->SetTolerance(0.005); // need some fluff

  this->Internals       = new vtkPolygonalSurfacePointPlacerInternals;
  this->DistanceOffset    = 0.0;
  this->SnapToClosestPoint = 0;
}

//----------------------------------------------------------------------
vtkPolygonalSurfacePointPlacer::~vtkPolygonalSurfacePointPlacer()
{
  this->CellPicker->Delete();
  this->Polys->Delete();
  delete this->Internals;
}

//----------------------------------------------------------------------
void vtkPolygonalSurfacePointPlacer::AddProp(vtkProp *prop)
{
  this->SurfaceProps->AddItem(prop);
  this->CellPicker->AddPickList(prop);
}

//----------------------------------------------------------------------
void vtkPolygonalSurfacePointPlacer::RemoveViewProp(vtkProp *prop)
{
  this->Superclass::RemoveViewProp( prop );
  this->CellPicker->DeletePickList( prop );
}

//----------------------------------------------------------------------
void vtkPolygonalSurfacePointPlacer::RemoveAllProps()
{
  this->Superclass::RemoveAllProps();
  this->CellPicker->InitializePickList();
}

//----------------------------------------------------------------------
int vtkPolygonalSurfacePointPlacer::ComputeWorldPosition( vtkRenderer *ren,
                                        double  displayPos[2],
                                        double *vtkNotUsed(refWorldPos),
                                        double  worldPos[3],
                                        double  worldOrient[9] )
{
  return this->ComputeWorldPosition(ren, displayPos, worldPos, worldOrient);
}

//----------------------------------------------------------------------
int vtkPolygonalSurfacePointPlacer::ComputeWorldPosition( vtkRenderer *ren,
                                      double displayPos[2],
                                      double worldPos[3],
                                      double vtkNotUsed(worldOrient)[9] )
{
  if ( this->CellPicker->Pick(displayPos[0],
                              displayPos[1], 0.0, ren) )
    {

    vtkMapper *mapper =
      vtkMapper::SafeDownCast(this->CellPicker->GetMapper());
    if (!mapper)
      {
      return 0;
      }

    // Get the underlying dataset
    vtkPolyData *pd = vtkPolyData::SafeDownCast(mapper->GetInput());
    if (!pd)
      {
      return 0;
      }

    if (vtkAssemblyPath *path = this->CellPicker->GetPath())
      {

      // We are checking if the prop present in the path is present
      // in the list supplied to us.. If it is, that prop will be picked.
      // If not, no prop will be picked.

      bool found = false;
      vtkAssemblyNode *node = NULL;
      vtkCollectionSimpleIterator sit;
      this->SurfaceProps->InitTraversal(sit);

      while (vtkProp *p = this->SurfaceProps->GetNextProp(sit))
        {
        vtkCollectionSimpleIterator psit;
        path->InitTraversal(psit);

        for ( int i = 0; i < path->GetNumberOfItems() && !found ; ++i )
          {
          node = path->GetNextNode(psit);
          found = ( node->GetViewProp() == p );
          }

        if (found)
          {
          vtkPolygonalSurfacePointPlacer::Node *contourNode
            = this->Internals->InsertNodeAtCurrentPickPosition(
                          this->CellPicker, this->DistanceOffset,
                          this->SnapToClosestPoint);
          if (contourNode)
            {
            worldPos[0] = contourNode->WorldPosition[0];
            worldPos[1] = contourNode->WorldPosition[1];
            worldPos[2] = contourNode->WorldPosition[2];
            return 1;
            }
          }
        }
      }
    }

  return 0;
}

//----------------------------------------------------------------------
int vtkPolygonalSurfacePointPlacer::ValidateWorldPosition( double worldPos[3],
                                           double *vtkNotUsed(worldOrient) )
{
  return this->ValidateWorldPosition( worldPos );
}

//----------------------------------------------------------------------
int vtkPolygonalSurfacePointPlacer::ValidateWorldPosition(
                     double vtkNotUsed(worldPos)[3] )
{
  return 1;
}

//----------------------------------------------------------------------
int vtkPolygonalSurfacePointPlacer::ValidateDisplayPosition( vtkRenderer *,
                                      double vtkNotUsed(displayPos)[2] )
{
  // We could check here to ensure that the display point picks one of the
  // terrain props, but the contour representation always calls
  // ComputeWorldPosition followed by
  // ValidateDisplayPosition/ValidateWorldPosition when it needs to
  // update a node...
  //
  // So that would be wasting CPU cycles to perform
  // the same check twice..  Just return 1 here.

  return 1;
}

//----------------------------------------------------------------------
vtkPolygonalSurfacePointPlacer::Node *vtkPolygonalSurfacePointPlacer
::GetNodeAtWorldPosition( double worldPos[3] )
{
  return this->Internals->GetNodeAtWorldPosition(worldPos);
}

//----------------------------------------------------------------------
int vtkPolygonalSurfacePointPlacer::
UpdateNodeWorldPosition( double worldPos[3], vtkIdType nodePointId )
{
  if( this->Polys->GetNumberOfItems() != 0 )
  {
    vtkPolyData *pd = vtkPolyData::SafeDownCast(
                this->Polys->GetItemAsObject(0));
    this->Internals->InsertNodeAtCurrentPickPosition( pd,
        worldPos,-1,nodePointId,this->DistanceOffset,this->SnapToClosestPoint);
    return 1;
  }
  else
  {
    vtkErrorMacro("PolyDataCollection has no items.");
    return 0;
  }
}

//----------------------------------------------------------------------
void vtkPolygonalSurfacePointPlacer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Cell Picker: " << this->CellPicker << endl;
  if (this->CellPicker)
    {
    this->CellPicker->PrintSelf(os, indent.GetNextIndent());
    }

  os << indent << "Surface Props: " << this->SurfaceProps << endl;
  if (this->SurfaceProps)
    {
    this->SurfaceProps->PrintSelf(os, indent.GetNextIndent());
    }

  os << indent << "Surface polygons: " << this->Polys << endl;
  if (this->Polys)
    {
    this->Polys->PrintSelf(os, indent.GetNextIndent());
    }

  os << indent << "Distance Offset: " << this->DistanceOffset << "\n";
  os << indent << "SnapToClosestPoint: " << this->SnapToClosestPoint << endl;
}
