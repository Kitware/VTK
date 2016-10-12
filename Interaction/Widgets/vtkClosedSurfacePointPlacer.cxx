/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClosedSurfacePointPlacer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkClosedSurfacePointPlacer.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"
#include "vtkPlane.h"
#include "vtkPlanes.h"
#include "vtkPlaneCollection.h"
#include "vtkRenderer.h"
#include "vtkInteractorObserver.h"
#include "vtkLine.h"
#include "vtkCamera.h"

#include <algorithm>
#include <vector>

vtkStandardNewMacro(vtkClosedSurfacePointPlacer);
vtkCxxSetObjectMacro(vtkClosedSurfacePointPlacer, BoundingPlanes,vtkPlaneCollection);

//----------------------------------------------------------------------
// Place holder structure to find the two planes that would best cut
// a line with a plane. We do this freaky stuff because we cannot use
// absolute tolerances. Sometimes a point may be intersected by two planes
// when it is on a corner etc... Believe me, I found this necessary.
//
// Plane   : The plane that we found had intersected the line in question
// p       : The intersection point of the line and the plane.
// Distance: Distance of the point "p" from the object. Negative distances
//           mean that it is outside.
struct vtkClosedSurfacePointPlacerNode
{
  typedef            vtkClosedSurfacePointPlacerNode Self;
  mutable vtkPlane * Plane;
  double             Distance;
  double             p[3];
  static bool Sort( const Self &a, const Self &b )
     { return a.Distance > b.Distance; }
  bool operator==(const Self &a) const { return a.Plane == this->Plane; }
  bool operator!=(const Self &a) const { return a.Plane != this->Plane; }
  vtkClosedSurfacePointPlacerNode()
    { Plane = NULL; Distance = VTK_DOUBLE_MIN; }
};

//----------------------------------------------------------------------
vtkClosedSurfacePointPlacer::vtkClosedSurfacePointPlacer()
{
  this->BoundingPlanes      = NULL;
  this->MinimumDistance     = 0.0;
  this->InnerBoundingPlanes = vtkPlaneCollection::New();
}

//----------------------------------------------------------------------
vtkClosedSurfacePointPlacer::~vtkClosedSurfacePointPlacer()
{
  this->RemoveAllBoundingPlanes();
  if (this->BoundingPlanes)
  {
    this->BoundingPlanes->UnRegister(this);
  }
  this->InnerBoundingPlanes->Delete();
}

//----------------------------------------------------------------------
void vtkClosedSurfacePointPlacer::AddBoundingPlane(vtkPlane *plane)
{
  if (this->BoundingPlanes == NULL)
  {
    this->BoundingPlanes = vtkPlaneCollection::New();
    this->BoundingPlanes->Register(this);
    this->BoundingPlanes->Delete();
  }

  this->BoundingPlanes->AddItem(plane);
}

//----------------------------------------------------------------------
void vtkClosedSurfacePointPlacer::RemoveBoundingPlane(vtkPlane *plane)
{
  if (this->BoundingPlanes )
  {
    this->BoundingPlanes->RemoveItem(plane);
  }
}

//----------------------------------------------------------------------
void vtkClosedSurfacePointPlacer::RemoveAllBoundingPlanes()
{
  if ( this->BoundingPlanes )
  {
    this->BoundingPlanes->RemoveAllItems();
    this->BoundingPlanes->Delete();
    this->BoundingPlanes = NULL;
  }
}
//----------------------------------------------------------------------

void vtkClosedSurfacePointPlacer::SetBoundingPlanes(vtkPlanes *planes)
{
  if (!planes)
  {
    return;
  }

  vtkPlane *plane;
  int numPlanes = planes->GetNumberOfPlanes();

  this->RemoveAllBoundingPlanes();
  for (int i=0; i<numPlanes ; i++)
  {
    plane = vtkPlane::New();
    planes->GetPlane(i, plane);
    this->AddBoundingPlane(plane);
    plane->Delete();
  }
}

//----------------------------------------------------------------------
void vtkClosedSurfacePointPlacer::BuildPlanes()
{
  if (this->InnerBoundingPlanes->GetMTime() > this->GetMTime() &&
      this->InnerBoundingPlanes->GetMTime() > this->BoundingPlanes->GetMTime())
  {
    return;
  }

  // Need to build planes.. Bring them all in front by MinimumDistance.
  // Find the Inner bounding planes.

  this->InnerBoundingPlanes->RemoveAllItems();

  double normal[3], origin[3];
  vtkPlane *p;
  for (this->BoundingPlanes->InitTraversal();
      (p = this->BoundingPlanes->GetNextItem());  )
  {
    p->GetNormal(normal);
    p->GetOrigin(origin);
    for (int i = 0; i<3; i++)
    {
      origin[i] += this->MinimumDistance * normal[i];
    }

    vtkPlane * plane = vtkPlane::New();
    plane->SetOrigin(origin);
    plane->SetNormal(normal);
    this->InnerBoundingPlanes->AddItem(plane);
    plane->Delete();
  }
}

//----------------------------------------------------------------------
// Given a renderer, a display position and a reference position, "worldPos"
// is calculated as :
//   Consider the line "L" that passes through the supplied "displayPos" and
// is parallel to the direction of projection of the camera. Clip this line
// segment with the parallelopiped, let's call it "L_segment". The computed
// world position, "worldPos" will be the point on "L_segment" that is closest
// to refWorldPos.
int vtkClosedSurfacePointPlacer::ComputeWorldPosition(
    vtkRenderer * ren,
    double        displayPos[2],
    double        refWorldPos[3],
    double        worldPos[3],
    double      * vtkNotUsed(worldOrient) )
{
  this->BuildPlanes();

  if (!this->BoundingPlanes)
  {
    return 0;
  }

  double directionOfProjection[3], t, d[3],
         currentWorldPos[4], ls[2][3], fp[4];

  vtkInteractorObserver::ComputeWorldToDisplay( ren,
    refWorldPos[0], refWorldPos[1], refWorldPos[2], fp );

  ren->GetActiveCamera()->
        GetDirectionOfProjection(directionOfProjection);
  vtkInteractorObserver::ComputeDisplayToWorld( ren,
      displayPos[0], displayPos[1], fp[2], currentWorldPos);

  // The line "L" defined by two points, l0 and l1. The line-segment
  // end-points will be defined by points ls[2][3].
  double l0[3] = {currentWorldPos[0] - directionOfProjection[0],
                  currentWorldPos[1] - directionOfProjection[1],
                  currentWorldPos[2] - directionOfProjection[2] };
  double l1[3] = {currentWorldPos[0] + directionOfProjection[0],
                  currentWorldPos[1] + directionOfProjection[1],
                  currentWorldPos[2] + directionOfProjection[2] };

  // Traverse all the planes to clip the line.

  vtkPlaneCollection *pc = this->InnerBoundingPlanes;

  // Stores candidate intersections with the parallelopiped. This was found
  // necessary instead of a simple two point intersection test because of
  // tolerances in vtkPlane::EvaluatePosition when the handle was very close
  // to an edge.
  std::vector< vtkClosedSurfacePointPlacerNode > intersections;

  const int nPlanes = pc->GetNumberOfItems();

  // Loop over each plane.
  for ( int n = 0; n < nPlanes; n++ )
  {
    vtkPlane * plane = static_cast< vtkPlane * >(pc->GetItemAsObject(n));
    vtkClosedSurfacePointPlacerNode node;

    vtkPlane::IntersectWithLine( l0, l1,
          plane->GetNormal(), plane->GetOrigin(), t, node.p );

    // The IF below insures that the line and the plane aren't parallel.
    if (t != VTK_DOUBLE_MAX)
    {
      node.Plane    = plane;
      node.Distance = this->GetDistanceFromObject(
                         node.p, this->InnerBoundingPlanes, d);
      intersections.push_back(node);
      vtkDebugMacro( << "We aren't parallel to plane with normal: ("
        << plane->GetNormal()[0] << "," << plane->GetNormal()[1] << ","
        << plane->GetNormal()[2] << ")" );
      vtkDebugMacro( << "Size of inersections = " << intersections.size()
        << " Distance: " << node.Distance << " Plane: " << plane );
    }
  }

  std::sort( intersections.begin(),
                intersections.end(),
                vtkClosedSurfacePointPlacerNode::Sort);

  // Now pick the top two candidates, insuring that the line at least intersects
  // with the object. If we have fewer than 2 in the queue, or if the
  // top candidate is outsude, we have failed to intersect the object.

  std::vector< vtkClosedSurfacePointPlacerNode >
              ::const_iterator it = intersections.begin();
  if ( intersections.size() < 2 ||
         it ->Distance < (-1.0 * this->WorldTolerance) ||
      (++it)->Distance < (-1.0 * this->WorldTolerance))
  {
    // The display point points to a location outside the object. Just
    // return 0. In actuality, I'd like to return the closest point in the
    // object. For this I require an algorithm that can, given a point "p" and
    // an object "O", defined by a set of bounding planes, find the point on
    // "O" that is closest to "p"

    return 0;
  }

  it = intersections.begin();
  for (int i = 0; i < 2; i++, ++it)
  {
    ls[i][0] = it->p[0];
    ls[i][1] = it->p[1];
    ls[i][2] = it->p[2];
  }

  vtkLine::DistanceToLine( refWorldPos, ls[0], ls[1], t, worldPos );
  t = (t < 0.0 ? 0.0 : (t > 1.0 ? 1.0 : t));

  // the point "worldPos", now lies within the object and on the line from
  // the eye along the direction of projection.

  worldPos[0] = ls[0][0] * (1.0-t) + ls[1][0] * t;
  worldPos[1] = ls[0][1] * (1.0-t) + ls[1][1] * t;
  worldPos[2] = ls[0][2] * (1.0-t) + ls[1][2] * t;

  vtkDebugMacro( << "Reference Pos: (" << refWorldPos[0] << ","
    << refWorldPos[1] << "," << refWorldPos[2] << ")  Line segment from "
    << "the eye along the direction of projection, clipped by the object [("
    << ls[0][0] << "," << ls[0][1] << "," << ls[0][2] << ") - (" << ls[1][0]
    << "," << ls[1][1] << "," << ls[1][2] << ")] Computed position (that is "
    << "the closest point on this segment to ReferencePos: (" << worldPos[0]
    << "," << worldPos[1] << "," << worldPos[2] << ")" );

  return 1;
}

//----------------------------------------------------------------------
int vtkClosedSurfacePointPlacer
::ComputeWorldPosition( vtkRenderer *,
                        double vtkNotUsed(displayPos)[2],
                        double vtkNotUsed(worldPos)[3],
                        double vtkNotUsed(worldOrient)[9] )
{
  vtkErrorMacro( << "This placer needs a reference world position.");
  return 0;
}

//----------------------------------------------------------------------
int vtkClosedSurfacePointPlacer::ValidateWorldPosition( double worldPos[3],
                                                       double* vtkNotUsed(worldOrient) )
{
  return this->ValidateWorldPosition( worldPos );
}

//----------------------------------------------------------------------
int vtkClosedSurfacePointPlacer::ValidateWorldPosition( double worldPos[3] )
{
  this->BuildPlanes();

  // Now check against the bounding planes
  if ( this->InnerBoundingPlanes )
  {
    vtkPlane *p;

    this->InnerBoundingPlanes->InitTraversal();

    while ( (p = this->InnerBoundingPlanes->GetNextItem()) )
    {
      if ( p->EvaluateFunction( worldPos ) < this->WorldTolerance )
      {
        return 0;
      }
    }
  }
  return 1;
}

//----------------------------------------------------------------------
// Calculate the distance of a point from the Object. Negative
// values imply that the point is outside. Positive values imply that it is
// inside. The closest point to the object is returned in closestPt.
double vtkClosedSurfacePointPlacer
::GetDistanceFromObject( double               pos[3],
                         vtkPlaneCollection * pc,
                         double               closestPt[3])
{
  vtkPlane *minPlane = NULL;
  double    minD     = VTK_DOUBLE_MAX;

  pc->InitTraversal();
  while ( vtkPlane * p = pc->GetNextItem() )
  {
    const double d = p->EvaluateFunction( pos );
    if (d < minD)
    {
      minD = d;
      minPlane = p;
    }
  }

  vtkPlane::ProjectPoint( pos, minPlane->GetOrigin(),
                          minPlane->GetNormal(), closestPt );
  return minD;
}

//----------------------------------------------------------------------
void vtkClosedSurfacePointPlacer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Bounding Planes:\n";
  if ( this->BoundingPlanes )
  {
    this->BoundingPlanes->PrintSelf(os,indent.GetNextIndent());
  }
  else
  {
    os << " (none)\n";
  }

  os << indent << "Minimum Distance: " << this->MinimumDistance << "\n";
}
