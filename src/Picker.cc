/*=========================================================================

  Program:   Visualization Library
  Module:    Picker.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "Picker.hh"
#include "Camera.hh"
#include "vlMath.hh"
#include "Vertex.hh"

static vlMath math;

// Description:
// Construct object with initial tolerance of 1/40th of window.
vlPicker::vlPicker()
{
  this->RenderWindow = NULL;

  this->SelectionPoint[0] = 0.0;
  this->SelectionPoint[1] = 0.0;
  this->SelectionPoint[2] = 0.0;

  this->Tolerance = 0.025; // 1/40th of the renderer window

  this->PickPosition[0] = 0.0;
  this->PickPosition[1] = 0.0;
  this->PickPosition[2] = 0.0;

  this->MapperPosition[0] = 0.0;
  this->MapperPosition[1] = 0.0;
  this->MapperPosition[2] = 0.0;

  this->Actor = NULL;
  this->Mapper = NULL;
  this->GlobalTMin = LARGE_FLOAT;
}

// Update state when actor is picked.
void vlPicker::MarkPicked(vlActor *actor, vlMapper *mapper, float tMin, 
                          float mapperPos[3])
{
  int i;
  float mapperHPosition[4];
  float *worldHPosition;

  this->Actor = actor;
  this->Mapper = mapper;
  this->GlobalTMin = tMin;
  for (i=0; i < 3; i++) 
    {
    this->MapperPosition[i] = mapperPos[i];
    mapperHPosition[i] = mapperPos[i];
    }
  mapperHPosition[3] = 0.0;
//
// The point has to be transformed back into world coordinates.
// Note: it is assumed that the transform is in the correct state.
//
  this->Transform.SetPoint(mapperHPosition);
  worldHPosition = this->Transform.GetPoint();

  for (i=0; i < 3; i++) this->PickPosition[i] = worldHPosition[i];
}

// Description:
// Perform pick operation with selection point provided. Normally the 
// first two values for the selection point are x-y pixel coordinate, and
// the third value is =0. Return non-zero if something was successfully picked.
int vlPicker::Pick(float selectionPt[3])
{
  int i;
  vlRendererCollection *renderers;
  vlRenderer *renderer;
  vlActorCollection *actors;
  vlActor *actor;
  vlCamera *camera;
  vlMapper *mapper;
  float p1World[4], p2World[4], p1Mapper[4], p2Mapper[4];
  static vlVertex cell; // use to take advantage of Hitbbox() method
  int picked=0;
  int *winSize;
  float x, y, t;
  float *viewport;
  float cameraPos[4], cameraFP[4];
  float *displayCoords, *worldCoords;
  float *clipRange;
  float ray[3], rayLength;
  float transparency;
  int visible, pickable;
  float *windowLowerLeft, *windowUpperRight;
  float *bounds, tol;
  float tF, tB;
  float hitPosition[3];
//
//  Initialize picking process
//
  for (i=0; i<3; i++) this->SelectionPoint[i] = selectionPt[i];
  this->Initialize();

  if ( !this->RenderWindow )
    {
    vlErrorMacro(<<"Must specify render window!");
    return 0;
    }
// 
// Determine which renderer we are dealing with (since more than one renderer
// may be rendering into renderer window).
//
  winSize = this->RenderWindow->GetSize();
  x = selectionPt[0] / winSize[0];
  y = selectionPt[1] / winSize[1];

  renderers = this->RenderWindow->GetRenderers();
  for ( renderers->InitTraversal(); renderer=renderers->GetNextItem(); )
    {
    viewport = renderer->GetViewport();
    if ( viewport[0] <= x && x <= viewport[2] &&
    viewport[1] <= y && y <= viewport[3] )
      break;
    }

  if ( !renderer )
    {
    vlErrorMacro(<<"No renderer selected!");
    return 0;
    }
//
// Get camera focal point and position. Convert to display (screen) 
// coordinates. We need a depth value for z-buffer.
//
  camera = renderer->GetActiveCamera();
  camera->GetPosition(cameraPos); cameraPos[3] = 1.0;
  camera->GetFocalPoint(cameraFP); cameraFP[3] = 1.0;

  renderer->SetWorldPoint(cameraFP);
  renderer->WorldToDisplay();
  displayCoords = renderer->GetDisplayPoint();
  selectionPt[2] = displayCoords[2];
//
// Convert the selection point into world coordinates.
//
  renderer->SetDisplayPoint(selectionPt);
  renderer->DisplayToWorld();
  worldCoords = renderer->GetWorldPoint();
  for (i=0; i < 3; i++) this->PickPosition[i] = worldCoords[i];
//
//  Compute the ray endpoints.  The ray is along the line running from
//  the camera position to the selection point, starting where this line
//  intersects the front clipping plane, and terminating where this
//  line intersects the back clipping plane.
//
  for (i=0; i<3; i++) ray[i] = this->PickPosition[i] - cameraPos[i];

  if (( rayLength = math.Dot(ray,ray)) == 0.0 ) 
    {
    vlWarningMacro("Cannot process points");
    return 0;
    } 
  else rayLength = sqrt (rayLength);

  clipRange = camera->GetClippingRange();

  tF = clipRange[0] / rayLength;
  tB = clipRange[1] / rayLength;
  for (i=0; i<3; i++) 
    {
    p1World[i] = cameraPos[i] + tF*ray[i];
    p2World[i] = cameraPos[i] + tB*ray[i];
    }
  p1World[3] = p2World[3] = 1.0;
//
// Compute the tolerance in world coordinates.  Do this by
// determining the world coordinates of the diagonal points of the
// window, computing the width of the window in world coordinates, and 
// multiplying by the this tolerance.
//
  x = winSize[0] * viewport[0];
  y = winSize[1] * viewport[1];
  renderer->SetDisplayPoint(x, y, selectionPt[2]);
  renderer->DisplayToWorld();
  windowLowerLeft = renderer->GetWorldPoint();

  x = winSize[0] * viewport[2];
  y = winSize[1] * viewport[3];
  renderer->SetDisplayPoint(x, y, selectionPt[2]);
  renderer->DisplayToWorld();
  windowUpperRight = renderer->GetWorldPoint();

  for (tol=0.0,i=0; i<3; i++) 
    tol += (windowUpperRight[i] - windowLowerLeft[i])*(windowUpperRight[i] - windowLowerLeft[i]);

  tol = sqrt (tol) * this->Tolerance;
//
//  Loop over all actors.  Transform ray (defined from position of
//  camera to selection point) into coordinates of mapper (not
//  transformed to actors coordinates!  Reduces overall computation!!!).
//
  actors = renderer->GetActors();
  for ( actors->InitTraversal(); actor=actors->GetNextItem(); )
    {
    visible = actor->GetVisibility();
    pickable = actor->GetPickable();
    transparency = actor->GetProperty()->GetTransparency();
//
//  If actor can be picked, get its composite matrix, invert it, and
//  use the inverted matrix to transform the ray points into mapper
//  coordinates. 
//
    if (visible && pickable && transparency != 0.0) 
      {
      this->Transform.SetMatrix(actor->GetMatrix());
      this->Transform.Push();
      this->Transform.Inverse();

      this->Transform.SetPoint(p1World);
      this->Transform.GetPoint(p1Mapper);

      this->Transform.SetPoint(p2World);
      this->Transform.GetPoint(p2Mapper);

      for (i=0; i<3; i++) ray[i] = p2Mapper[i] - p1Mapper[i];

      this->Transform.Pop();
//
//  Have the ray endpoints in mapper space, now need to compare this
//  with the mapper bounds to see whether intersection is possible.
//
      if ( (mapper = actor->GetMapper()) != NULL )
        {
//
//  Get the bounding box of the modeller.  Note that the tolerance is
//  added to the bounding box to make sure things on the edge of the
//  bounding box are picked correctly.
//
        bounds = mapper->GetBounds();
        if ( cell.HitBBox(bounds, p1Mapper, ray, hitPosition, t) )
          {
          picked = 1;
          this->IntersectWithLine(p1Mapper,p2Mapper,tol,actor,mapper);
          this->Actors.AddItem(actor);
          }
	}
      }
    }

  return picked;
}

// Intersect data with specified ray.
void vlPicker::IntersectWithLine(float p1[3], float p2[3], float tol, 
                                 vlActor *actor, vlMapper *mapper)
{
  int i;
  float *center, t, ray[3], rayFactor;
//
// Get the data from the modeller
//
  center = mapper->GetCenter();

  for (i=0; i<3; i++) ray[i] = p2[i] - p1[i];
  if (( rayFactor = math.Dot(ray,ray)) == 0.0 ) return;
//
// Project the center point onto the ray and determine its parametric value
//
  t = (ray[0]*(center[0]-p1[0]) + ray[1]*(center[1]-p1[1])
          + ray[2]*(center[2]-p1[2])) / rayFactor;

  if ( t >= 0.0 && t <= 1.0 && t < this->GlobalTMin ) 
    {
    this->MarkPicked(actor, mapper, t, center);
    }
}

// Initialize the picking process.
void vlPicker::Initialize()
{
  this->Actors.RemoveAllItems();

  this->PickPosition[0] = 0.0;
  this->PickPosition[1] = 0.0;
  this->PickPosition[2] = 0.0;

  this->MapperPosition[0] = 0.0;
  this->MapperPosition[1] = 0.0;
  this->MapperPosition[2] = 0.0;

  this->Actor = NULL;
  this->Mapper = NULL;
  this->GlobalTMin = LARGE_FLOAT;
}

