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

#include <algorithm>
#include <vector>

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
int vtkBoundedPlanePointPlacer::ComputeWorldPosition( vtkRenderer *ren,
                                                      double displayPos[2],
                                                      double vtkNotUsed(refWorldPos)[3],
                                                      double worldPos[3],
                                                      double worldOrient[9] )
{
  return this->ComputeWorldPosition( ren, displayPos, worldPos, worldOrient );
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
  
  os << indent << "Projection Normal: ";
  if ( this->ProjectionNormal == vtkBoundedPlanePointPlacer::XAxis )
    {
    os << "XAxis\n";
    }
  else if ( this->ProjectionNormal == vtkBoundedPlanePointPlacer::YAxis )
    {
    os << "YAxis\n";
    }
  else if ( this->ProjectionNormal == vtkBoundedPlanePointPlacer::ZAxis )
    {
    os << "ZAxis\n";
    }
  else //if ( this->ProjectionNormal == vtkBoundedPlanePointPlacer::Oblique )
    {
    os << "Oblique\n";
    }
  
  os << indent << "Projection Position: " << this->ProjectionPosition << "\n";

  os << indent << "Bounding Planes:\n";
  if ( this->BoundingPlanes )
    {
    this->BoundingPlanes->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << " (none)\n";
    }
  os << indent << "Oblique plane:\n";
  if ( this->ObliquePlane )
    {
    this->ObliquePlane->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << " (none)\n";
    }
}
