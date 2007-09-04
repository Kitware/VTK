/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBoundedPlanePointPlacer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBoundedPlanePointPlacer.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"
#include "vtkPlane.h"
#include "vtkPlanes.h"
#include "vtkPlaneCollection.h"
#include "vtkRenderer.h"
#include "vtkInteractorObserver.h"
#include "vtkLine.h"
#include "vtkCamera.h"
#include <vtkstd/vector>

vtkCxxRevisionMacro(vtkBoundedPlanePointPlacer, "1.5");
vtkStandardNewMacro(vtkBoundedPlanePointPlacer);

vtkCxxSetObjectMacro(vtkBoundedPlanePointPlacer, ObliquePlane, vtkPlane);
vtkCxxSetObjectMacro(vtkBoundedPlanePointPlacer, BoundingPlanes,vtkPlaneCollection);

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
struct vtkBoundedPlanePointPlacerNode
{
  typedef            vtkBoundedPlanePointPlacerNode Self;
  mutable vtkPlane * Plane;
  double             Distance;
  double             p[3];
  static bool Sort( const Self &a, const Self &b ) 
     { return a.Distance > b.Distance; }
  bool operator==(const Self &a) const { return a.Plane == this->Plane; }
  bool operator!=(const Self &a) const { return a.Plane != this->Plane; }
  vtkBoundedPlanePointPlacerNode() 
    { Plane = NULL; Distance = VTK_DOUBLE_MIN; }
};

//----------------------------------------------------------------------
vtkBoundedPlanePointPlacer::vtkBoundedPlanePointPlacer()
{
  this->ProjectionPosition = 0;
  this->ObliquePlane       = NULL;
  this->ProjectionNormal   = vtkBoundedPlanePointPlacer::ZAxis;  
  this->BoundingPlanes     = NULL;
}

//----------------------------------------------------------------------
vtkBoundedPlanePointPlacer::~vtkBoundedPlanePointPlacer()
{
  this->RemoveAllBoundingPlanes();
  
  if ( this->ObliquePlane )
    {
    this->ObliquePlane->UnRegister(this);
    this->ObliquePlane = NULL;
    }
  
  if (this->BoundingPlanes)
    {
    this->BoundingPlanes->UnRegister(this);
    }
}

//----------------------------------------------------------------------
void vtkBoundedPlanePointPlacer::SetProjectionPosition(double position)
{
  if ( this->ProjectionPosition != position )
    {
    this->ProjectionPosition = position;
    this->Modified();
    }
}

//----------------------------------------------------------------------
void vtkBoundedPlanePointPlacer::AddBoundingPlane(vtkPlane *plane)
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
void vtkBoundedPlanePointPlacer::RemoveBoundingPlane(vtkPlane *plane)
{
  if (this->BoundingPlanes )
    {
    this->BoundingPlanes->RemoveItem(plane);
    }
}

//----------------------------------------------------------------------
void vtkBoundedPlanePointPlacer::RemoveAllBoundingPlanes()
{
  if ( this->BoundingPlanes )
    {
    this->BoundingPlanes->RemoveAllItems();
    this->BoundingPlanes->Delete();
    this->BoundingPlanes = NULL;
    }
}
//----------------------------------------------------------------------

void vtkBoundedPlanePointPlacer::SetBoundingPlanes(vtkPlanes *planes)
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
// Given a renderer, a display position and a reference position, "worldPos"
// is calculated as :
//   Consider the line "L" that passes through the supplied "displayPos" and
// is parallel to the direction of projection of the camera. Clip this line
// segment with the parallelopiped, let's call it "L_segment". The computed 
// world position, "worldPos" will be the point on "L_segment" that is closest 
// to refWorldPos.
int vtkBoundedPlanePointPlacer::ComputeWorldPosition( 
    vtkRenderer * ren,
    double        displayPos[2],
    double        refWorldPos[3],
    double        worldPos[3],
    double      * vtkNotUsed(worldOrient) )
{
  if (!this->BoundingPlanes)
    {
    return 0;
    }

  double directionOfProjection[3], t, d[3],
         currentWorldPos[3], ls[2][3], fp[4];

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

  vtkPlaneCollection *pc = this->GetBoundingPlanes();
  
  // Stores candidate intersections with the parallelopiped. This was found
  // necessary instead of a simple two point intersection test because of
  // tolerances in vtkPlane::EvaluatePosition when the handle was very close
  // to an edge.
  vtkstd::vector< vtkBoundedPlanePointPlacerNode > intersections;

  const int nPlanes = pc->GetNumberOfItems();

  // Loop over each plane.
  for ( int n = 0; n < nPlanes; n++ )
    {
    vtkPlane * plane = static_cast< vtkPlane * >(pc->GetItemAsObject(n));
    vtkBoundedPlanePointPlacerNode node;

    vtkPlane::IntersectWithLine( l0, l1, 
          plane->GetNormal(), plane->GetOrigin(), t, node.p );
    
    // The IF below insures that the line and the plane aren't parallel.
    if (t != VTK_DOUBLE_MAX)
      {
      node.Plane    = plane;
      node.Distance = this->GetDistanceFromObject(
                         node.p, this->BoundingPlanes, d);
      intersections.push_back(node);
      vtkDebugMacro( << "We aren't parallel to plane with normal: (" 
        << plane->GetNormal()[0] << "," << plane->GetNormal()[1] << "," 
        << plane->GetNormal()[2] << ")" );
      vtkDebugMacro( << "Size of inersections = " << intersections.size() 
        << " Distance: " << node.Distance << " Plane: " << plane );
      }
    }

  vtkstd::sort( intersections.begin(), 
                intersections.end(), 
                vtkBoundedPlanePointPlacerNode::Sort);
  
  // Now pick the top two candidates, insuring that the line at least intersects
  // with the object. If we have fewer than 2 in the queue, or if the 
  // top candidate is outsude, we have failed to intersect the object.
  
  vtkstd::vector< vtkBoundedPlanePointPlacerNode >
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

  // Now that we have the end points to the line segment "L_segment" in 
  // ls[0] and ls[1], we will find the point on this segment that
  // refWorldPos is closest to. To do this we have the method 
  // vtkLine::DistanceToLine, (we shall clamp 0 < t < 1, to ensure that it
  // is within the line segment.

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
int vtkBoundedPlanePointPlacer::ComputeWorldPosition( vtkRenderer *ren,
                                                      double displayPos[2],
                                                      double worldPos[3],
                                                      double worldOrient[9] )
{
  double nearWorldPoint[4];
  double farWorldPoint[4];
  double tmp[3];
  
  tmp[0] = displayPos[0];
  tmp[1] = displayPos[1];
  tmp[2] = 0.0;  // near plane
  
  ren->SetDisplayPoint(tmp);
  ren->DisplayToWorld();
  ren->GetWorldPoint(nearWorldPoint);

  tmp[2] = 1.0;  // far plane
  ren->SetDisplayPoint(tmp);
  ren->DisplayToWorld();
  ren->GetWorldPoint(farWorldPoint);
  
  double normal[3];
  double origin[3];

  this->GetProjectionNormal( normal );
  this->GetProjectionOrigin( origin );
  
  double position[3];
  double distance;
  if ( vtkPlane::IntersectWithLine( nearWorldPoint,
                                    farWorldPoint,
                                    normal, origin,
                                    distance, position ) )
    {
    // Fill in the information now before validating it.
    // This is because we should return the best information
    // we can since this may be part of an UpdateWorldPosition
    // call - we need to do the best at updating the position
    // even if it is not valid.
    this->GetCurrentOrientation( worldOrient );
    worldPos[0] = position[0];
    worldPos[1] = position[1];
    worldPos[2] = position[2];
    
    // Now check against the bounding planes
    if ( this->BoundingPlanes )
      {
      vtkPlane *p;
      
      this->BoundingPlanes->InitTraversal();
      
      while ( (p = this->BoundingPlanes->GetNextItem()) )
        {
        if ( p->EvaluateFunction( position ) < this->WorldTolerance )
          {
          return 0;
          }
        }
      }
    return 1;
    }

  return 0;
}

//----------------------------------------------------------------------
int vtkBoundedPlanePointPlacer::ValidateWorldPosition( double worldPos[3],
                                                       double* vtkNotUsed(worldOrient) )
{
  return this->ValidateWorldPosition( worldPos );
}

//----------------------------------------------------------------------
int vtkBoundedPlanePointPlacer::ValidateWorldPosition( double worldPos[3] )
{
    // Now check against the bounding planes
    if ( this->BoundingPlanes )
      {
      vtkPlane *p;
      
      this->BoundingPlanes->InitTraversal();
      
      while ( (p = this->BoundingPlanes->GetNextItem()) )
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
int vtkBoundedPlanePointPlacer::UpdateWorldPosition( vtkRenderer *ren,
                                                     double worldPos[3],
                                                     double worldOrient[9] )
{
  double displayPoint[2];
  double tmp[4];
  
  tmp[0] = worldPos[0];
  tmp[1] = worldPos[1];
  tmp[2] = worldPos[2];
  tmp[3] = 1.0;
  
  ren->SetWorldPoint( tmp );
  ren->WorldToDisplay();
  ren->GetDisplayPoint( tmp );
  
  displayPoint[0] = tmp[0];
  displayPoint[1] = tmp[1];
  
  return this->ComputeWorldPosition( ren, displayPoint, 
                                     worldPos, worldOrient );
}
//----------------------------------------------------------------------
void vtkBoundedPlanePointPlacer::GetCurrentOrientation( double worldOrient[9] )
{
  double *x = worldOrient;
  double *y = worldOrient+3;
  double *z = worldOrient+6;
  
  this->GetProjectionNormal( z );
  
  double v[3];
  if ( fabs( z[0] ) >= fabs( z[1] ) &&
       fabs( z[0] ) >= fabs( z[2] ) )
    {
    v[0] = 0;
    v[1] = 1;
    v[2] = 0;
    }
  else
    {
    v[0] = 1;
    v[1] = 0;
    v[2] = 0;
    }
  
  vtkMath::Cross( z, v, y );
  vtkMath::Cross( y, z, x );
}

//----------------------------------------------------------------------
void vtkBoundedPlanePointPlacer::GetProjectionNormal( double normal[3] )
{
  switch ( this->ProjectionNormal )
    {
    case vtkBoundedPlanePointPlacer::XAxis:
      normal[0] = 1.0;
      normal[1] = 0.0;
      normal[2] = 0.0;
      break;
    case vtkBoundedPlanePointPlacer::YAxis:
      normal[0] = 0.0;
      normal[1] = 1.0;
      normal[2] = 0.0;
      break;
    case vtkBoundedPlanePointPlacer::ZAxis:
      normal[0] = 0.0;
      normal[1] = 0.0;
      normal[2] = 1.0;
      break;
    case vtkBoundedPlanePointPlacer::Oblique:
      this->ObliquePlane->GetNormal(normal);
      break;      
    }
}

//----------------------------------------------------------------------
void vtkBoundedPlanePointPlacer::GetProjectionOrigin( double origin[3] )
{
  switch ( this->ProjectionNormal )
    {
    case vtkBoundedPlanePointPlacer::XAxis:
      origin[0] = this->ProjectionPosition;
      origin[1] = 0.0;
      origin[2] = 0.0;
      break;
    case vtkBoundedPlanePointPlacer::YAxis:
      origin[0] = 0.0;
      origin[1] = this->ProjectionPosition;
      origin[2] = 0.0;
      break;
    case vtkBoundedPlanePointPlacer::ZAxis:
      origin[0] = 0.0;
      origin[1] = 0.0;
      origin[2] = this->ProjectionPosition;
      break;
    case vtkBoundedPlanePointPlacer::Oblique:
      this->ObliquePlane->GetOrigin(origin);
      break;      
    }
}

//----------------------------------------------------------------------
// Calculate the distance of a point from the Object. Negative 
// values imply that the point is outside. Positive values imply that it is
// inside. The closest point to the object is returned in closestPt. 
double vtkBoundedPlanePointPlacer
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
void vtkBoundedPlanePointPlacer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
}
