/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPicker.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkPicker.h"
#include "vtkCamera.h"
#include "vtkMath.h"
#include "vtkVertex.h"
#include "vtkRenderWindow.h"

// Description:
// Construct object with initial tolerance of 1/40th of window.
vtkPicker::vtkPicker()
{
  this->Renderer = NULL;

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

  this->Assembly = NULL;
  this->Actor = NULL;
  this->Mapper = NULL;
  this->DataSet = NULL;
  this->GlobalTMin = VTK_LARGE_FLOAT;
}

// Update state when actor is picked.
void vtkPicker::MarkPicked(vtkActor *assem, vtkActor *actor, vtkMapper *mapper,
                           float tMin, float mapperPos[3])
{
  int i;
  float mapperHPosition[4];
  float *worldHPosition;

  this->Assembly = assem;
  this->Actor = actor;
  this->Mapper = mapper;
  this->DataSet = mapper->GetInput();
  this->GlobalTMin = tMin;
  for (i=0; i < 3; i++) 
    {
    this->MapperPosition[i] = mapperPos[i];
    mapperHPosition[i] = mapperPos[i];
    }
  mapperHPosition[3] = 1.0;
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
int vtkPicker::Pick(float selectionX, float selectionY, float selectionZ,
                   vtkRenderer *renderer)
{
  int i;
  vtkActorCollection *actors;
  vtkActor *actor, *part;
  vtkCamera *camera;
  vtkMapper *mapper;
  float p1World[4], p2World[4], p1Mapper[4], p2Mapper[4];
  int picked=0;
  int *winSize;
  float x, y, t;
  float *viewport;
  float cameraPos[4], cameraFP[4];
  float *displayCoords, *worldCoords;
  float *clipRange;
  float ray[3], rayLength;
  float opacity;
  int visible, pickable;
  float windowLowerLeft[4], windowUpperRight[4];
  float *bounds, tol;
  float tF, tB;
  float hitPosition[3];
//
//  Initialize picking process
//
  this->Renderer = renderer;

  this->SelectionPoint[0] = selectionX;
  this->SelectionPoint[1] = selectionY;
  this->SelectionPoint[2] = selectionZ;

  this->Initialize();

  if ( renderer == NULL )
    {
    vtkErrorMacro(<<"Must specify renderer!");
    return 0;
    }
//
// Get camera focal point and position. Convert to display (screen) 
// coordinates. We need a depth value for z-buffer.
//
  camera = renderer->GetActiveCamera();
  camera->GetPosition((float *)cameraPos); cameraPos[3] = 1.0;
  camera->GetFocalPoint((float *)cameraFP); cameraFP[3] = 1.0;

  renderer->SetWorldPoint(cameraFP);
  renderer->WorldToDisplay();
  displayCoords = renderer->GetDisplayPoint();
  selectionZ = displayCoords[2];
//
// Convert the selection point into world coordinates.
//
  renderer->SetDisplayPoint(selectionX, selectionY, selectionZ);
  renderer->DisplayToWorld();
  worldCoords = renderer->GetWorldPoint();
  if ( worldCoords[3] == 0.0 )
    {
    vtkErrorMacro(<<"Bad homogeneous coordinates");
    return 0;
    }
  for (i=0; i < 3; i++) 
    this->PickPosition[i] = worldCoords[i] / worldCoords[3];
//
//  Compute the ray endpoints.  The ray is along the line running from
//  the camera position to the selection point, starting where this line
//  intersects the front clipping plane, and terminating where this
//  line intersects the back clipping plane.
//
  for (i=0; i<3; i++) ray[i] = this->PickPosition[i] - cameraPos[i];

  if (( rayLength = vtkMath::Dot(ray,ray)) == 0.0 ) 
    {
    vtkWarningMacro("Cannot process points");
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
// multiplying by the tolerance.
//
  viewport = renderer->GetViewport();
  winSize = renderer->GetRenderWindow()->GetSize();
  x = winSize[0] * viewport[0];
  y = winSize[1] * viewport[1];
  renderer->SetDisplayPoint(x, y, selectionZ);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(windowLowerLeft);

  x = winSize[0] * viewport[2];
  y = winSize[1] * viewport[3];
  renderer->SetDisplayPoint(x, y, selectionZ);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(windowUpperRight);

  for (tol=0.0,i=0; i<3; i++) 
    tol += (windowUpperRight[i] - windowLowerLeft[i])*(windowUpperRight[i] - windowLowerLeft[i]);

  tol = sqrt (tol) * this->Tolerance;
//
//  Loop over all actors.  Transform ray (defined from position of
//  camera to selection point) into coordinates of mapper (not
//  transformed to actors coordinates!  Reduces overall computation!!!).
//
  actors = renderer->GetActors();
  this->Transform.PostMultiply();
  for ( actors->InitTraversal(); (actor=actors->GetNextItem()); )
    {
    for ( actor->InitPartTraversal(); (part=actor->GetNextPart()); )
      {
      visible = part->GetVisibility();
      pickable = part->GetPickable();
      opacity = part->GetProperty()->GetOpacity();
//
//  If actor can be picked, get its composite matrix, invert it, and
//  use the inverted matrix to transform the ray points into mapper
//  coordinates. 
//
      if ( visible && pickable && (opacity != 0.0) &&
      (mapper = part->GetMapper()) != NULL )
        {
        this->Transform.SetMatrix(part->vtkProp::GetMatrix());
        this->Transform.Push();
        this->Transform.Inverse();

        this->Transform.SetPoint(p1World);
        this->Transform.GetPoint(p1Mapper);

        this->Transform.SetPoint(p2World);
        this->Transform.GetPoint(p2Mapper);

        for (i=0; i<3; i++) 
          {
          p1Mapper[i] /= p1Mapper[3];
          p2Mapper[i] /= p2Mapper[3];
          ray[i] = p2Mapper[i] - p1Mapper[i];
          }

        this->Transform.Pop();
//
//  Have the ray endpoints in mapper space, now need to compare this
//  with the mapper bounds to see whether intersection is possible.
//
//
//  Get the bounding box of the modeller.  Note that the tolerance is
//  added to the bounding box to make sure things on the edge of the
//  bounding box are picked correctly.
//
        bounds = mapper->GetBounds();
        if ( vtkCell::HitBBox(bounds, (float *)p1Mapper, ray, hitPosition, t) )
          {
          picked = 1;
          this->IntersectWithLine((float *)p1Mapper, (float *)p2Mapper,tol,actor,part,mapper);
          this->Actors.AddItem(part);
          }

        }//if visible and pickable not transparent and has mapper
      }//for all parts
    }//for all actors

  return picked;
}

// Intersect data with specified ray.
void vtkPicker::IntersectWithLine(float p1[3], float p2[3], 
				  float vtkNotUsed(tol), vtkActor *assem, 
				  vtkActor *actor, vtkMapper *mapper)
{
  int i;
  float *center, t, ray[3], rayFactor;

  //
  // Get the data from the modeller
  //
  center = mapper->GetCenter();

  for (i=0; i<3; i++) ray[i] = p2[i] - p1[i];
  if (( rayFactor = vtkMath::Dot(ray,ray)) == 0.0 ) return;
  //
  // Project the center point onto the ray and determine its parametric value
  //
  t = (ray[0]*(center[0]-p1[0]) + ray[1]*(center[1]-p1[1])
          + ray[2]*(center[2]-p1[2])) / rayFactor;

  if ( t >= 0.0 && t <= 1.0 && t < this->GlobalTMin ) 
    {
    this->MarkPicked(assem, actor, mapper, t, center);
    }
}

// Initialize the picking process.
void vtkPicker::Initialize()
{
  this->Actors.RemoveAllItems();

  this->PickPosition[0] = 0.0;
  this->PickPosition[1] = 0.0;
  this->PickPosition[2] = 0.0;

  this->MapperPosition[0] = 0.0;
  this->MapperPosition[1] = 0.0;
  this->MapperPosition[2] = 0.0;

  this->Assembly = NULL;
  this->Actor = NULL;
  this->Mapper = NULL;
  this->GlobalTMin = VTK_LARGE_FLOAT;
}

void vtkPicker::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os,indent);

  os << indent << "Renderer: " << this->Renderer << "\n";
  os << indent << "Selection Point: (" <<  this->SelectionPoint[0] << ","
     << this->SelectionPoint[1] << ","
     << this->SelectionPoint[2] << ")\n";
  os << indent << "Tolerance: " << this->Tolerance << "\n";
  os << indent << "Pick Position: (" <<  this->PickPosition[0] << ","
     << this->PickPosition[1] << ","
     << this->PickPosition[2] << ")\n";
  os << indent << "Mapper Position: (" <<  this->MapperPosition[0] << ","
     << this->MapperPosition[1] << ","
     << this->MapperPosition[2] << ")\n";
  os << indent << "Assembly: " << this->Assembly << "\n";
  os << indent << "Actor: " << this->Actor << "\n";
  os << indent << "Mapper: " << this->Mapper << "\n";
}
