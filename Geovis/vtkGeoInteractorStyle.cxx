/*=============================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoInteractorStyle.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=============================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkGeoInteractorStyle.h"

#include "vtkCallbackCommand.h"
#include "vtkCompassWidget.h"
#include "vtkGeoCamera.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkInteractorStyleRubberBand3D.h"
#include "vtkObjectFactory.h"
#include "vtkUnsignedCharArray.h"
#include "vtkCamera.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkActor2DCollection.h"
#include "vtkWorldPointPicker.h"
#include "vtkMath.h"
#include "vtkIndent.h"
#include "vtkAssemblyPath.h"
#include "vtkProperty.h"
#include "vtkTimerLog.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkGeoMath.h"
#include "vtkCoordinate.h"

#include <float.h>

vtkStandardNewMacro(vtkGeoInteractorStyle);

namespace
{
  class vtkEventCommand : public vtkCommand
  {
  public:
    vtkEventCommand(vtkGeoInteractorStyle* selfptr) {this->Self = selfptr;}
    virtual void Execute(vtkObject *caller, unsigned long eventId,
                         void * /*callData*/)
    {
      if (eventId == vtkCommand::InteractionEvent)
        {
        this->Self->WidgetInteraction(caller);
        }
    }
    vtkGeoInteractorStyle* Self;
  };
}

//----------------------------------------------------------------------------
vtkGeoInteractorStyle::vtkGeoInteractorStyle()
{
  vtkEventCommand* rc = new vtkEventCommand(this);
  this->EventCommand = rc;
  rc->UnRegister(0);

  this->GeoCamera = vtkSmartPointer<vtkGeoCamera>::New();

  // setup the compass and its callbacks
  this->CompassWidget = vtkSmartPointer<vtkCompassWidget>::New();
  this->CompassWidget->CreateDefaultRepresentation();
  this->CompassWidget->AddObserver(vtkCommand::InteractionEvent,
                                   this->EventCommand);

  this->DraggingRubberBandBoxState = 0;
  this->StartPosition[0] = this->StartPosition[1] = 0;
  this->EndPosition[0] = this->EndPosition[1] = 0;
  this->PixelArray = vtkUnsignedCharArray::New();
  this->PixelDims[0] = this->PixelDims[1] = 0;
  this->MotionFactor   = 10.0;

  // Rubberband zoom has a verification stage.
  this->RubberBandExtent[0] = this->RubberBandExtent[1] = 0;
  this->RubberBandExtent[2] = this->RubberBandExtent[3] = 0;
  this->RubberBandExtentEnabled = 0;
  this->RenderCallbackTag = 0;
  this->LockHeading = false;
}

//-----------------------------------------------------------------------------
vtkGeoInteractorStyle::~vtkGeoInteractorStyle()
{
  // Too late because interactor is already 0. Keep this anyway.
  this->DisableRubberBandRedraw();
  this->PixelArray->Delete();
}

//-----------------------------------------------------------------------------
void vtkGeoInteractorStyle::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "LockHeading: " << this->GetLockHeading() << endl;
}

//-----------------------------------------------------------------------------
vtkGeoCamera* vtkGeoInteractorStyle::GetGeoCamera()
{
  return this->GeoCamera;
}

//-----------------------------------------------------------------------------
void vtkGeoInteractorStyle::OnMiddleButtonDown()
{
  this->FindPokedRenderer(this->Interactor->GetEventPosition()[0],
                          this->Interactor->GetEventPosition()[1]);

  if (this->RubberBandExtentEnabled)
    {
    if (this->InRubberBandRectangle(this->Interactor->GetEventPosition()[0],
                                    this->Interactor->GetEventPosition()[1]))
      {
      this->RubberBandZoom();
      return;
      }
    }

  double x = this->Interactor->GetEventPosition()[0];
  double y = this->Interactor->GetEventPosition()[1];
  this->FindPokedRenderer(static_cast<int>(x),static_cast<int>(y));
  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  this->StartPan();
}





//-----------------------------------------------------------------------------
void vtkGeoInteractorStyle::OnMiddleButtonUp()
{
  this->Interactor->GetRenderWindow()->SetCurrentCursor(VTK_CURSOR_DEFAULT);
  switch (this->State)
  {
    case VTKIS_DOLLY:
     this->EndDolly();
      break;

    case VTKIS_PAN:
      this->EndPan();
      break;
  }
}

//-----------------------------------------------------------------------------
void vtkGeoInteractorStyle::OnRightButtonDown()
{
  this->Interactor->GetRenderWindow()->SetCurrentCursor(VTK_CURSOR_SIZENS);
  this->FindPokedRenderer(this->Interactor->GetEventPosition()[0],
                          this->Interactor->GetEventPosition()[1]);
  if (this->CurrentRenderer == NULL)
    {
    return;
    }
  this->StartDolly();
}

//-----------------------------------------------------------------------------
void vtkGeoInteractorStyle::OnRightButtonUp()
{
  this->Interactor->GetRenderWindow()->SetCurrentCursor(VTK_CURSOR_DEFAULT);
  if(this->State == VTKIS_DOLLY)
  {
    this->EndDolly();
  }
}

//-----------------------------------------------------------------------------
void vtkGeoInteractorStyle::OnLeftButtonDown()
{
  if (!this->Interactor)
    {
    return;
    }

  // don't turn on if already rubberbanding
  if (this->RubberBandExtentEnabled == 0)
    {
    // This rubber band state should be integrated with the
    this->DraggingRubberBandBoxState = 1;
    // State ivar.
    this->StartPosition[0] = this->Interactor->GetEventPosition()[0];
    this->StartPosition[1] = this->Interactor->GetEventPosition()[1];
    this->EndPosition[0] = this->StartPosition[0];
    this->EndPosition[1] = this->StartPosition[1];
    this->FindPokedRenderer(this->StartPosition[0], this->StartPosition[1]);
    }
}

//-----------------------------------------------------------------------------
void vtkGeoInteractorStyle::OnLeftButtonUp()
{
  if (!this->Interactor)
    {
    return;
    }

  if (this->RubberBandExtentEnabled)
    {
    int xx = this->Interactor->GetEventPosition()[0];
    int yy = this->Interactor->GetEventPosition()[1];
    if (this->InRubberBandRectangle(xx,yy))
      {
      this->Interactor->Render();
      }
    return;
  }

  // If we were dragging a rubber band rectangle.
  if (this->DraggingRubberBandBoxState)
    {
    this->DraggingRubberBandBoxState = 0;
    this->RubberBandExtentEnabled = 0;
    this->DisableRubberBandRedraw();
    this->Interactor->Render();

    unsigned int rect[5];
    rect[0] = this->StartPosition[0];
    rect[1] = this->StartPosition[1];
    rect[2] = this->EndPosition[0];
    rect[3] = this->EndPosition[1];
    if (this->Interactor->GetShiftKey())
      {
      rect[4] = vtkInteractorStyleRubberBand3D::SELECT_UNION;
      }
    else
      {
      rect[4] = vtkInteractorStyleRubberBand3D::SELECT_NORMAL;
      }
    this->InvokeEvent(vtkCommand::SelectionChangedEvent, reinterpret_cast<void*>(rect));
    this->Interactor->Render();
    }
}


//-----------------------------------------------------------------------------
bool vtkGeoInteractorStyle::InRubberBandRectangle( int x, int y)
{
  // disable the rubberband rectangle
  this->RubberBandExtentEnabled = 0;
  this->DisableRubberBandRedraw();

  // check if in rubberband
  if (x > this->RubberBandExtent[0] &&
      x < this->RubberBandExtent[1] &&
      y > this->RubberBandExtent[2] &&
      y < this->RubberBandExtent[3])
    {
    return true;
    }
  this->Interactor->Render();
  return false;
}

int vtkGeoInteractorStyle::GetRayIntersection(double origin[3],
                                              double direction[3],
                                              double intersection[3])
{
  double a, b, c;
  double r, k;

  a = direction[0]*direction[0] +
    direction[1]*direction[1] +
    direction[2]*direction[2];
  b = 2.0 * (direction[0]*origin[0] +
             direction[1]*origin[1] +
             direction[2]*origin[2]);
  c = origin[0]*origin[0] + origin[1]*origin[1] + origin[2]*origin[2]
    - vtkGeoMath::EarthRadiusMeters() * vtkGeoMath::EarthRadiusMeters();

  r = b*b - 4.0*a*c;
  if (r < 0.0)
    {
    // Set the intersection point to be the point on the ray closest to the
    // earth.  Derivative of distance ak + b = 0, k = -b/a
    k = -b / (2.0 * a);
    intersection[0] = origin[0] + k*direction[0];
    intersection[1] = origin[1] + k*direction[1];
    intersection[2] = origin[2] + k*direction[2];
    return VTK_ERROR;
    }
  k = (-b - sqrt(r)) / (2.0*a);

  intersection[0] = origin[0] + k*direction[0];
  intersection[1] = origin[1] + k*direction[1];
  intersection[2] = origin[2] + k*direction[2];

  if (k < 0.0)
    { // Intersection is behind ray.
    return VTK_ERROR;
    }

  return VTK_OK;
}

int vtkGeoInteractorStyle::ViewportToWorld(double xMouse,
                                           double yMouse,
                                           double &wx, double &wy,
                                           double &wz)
{
  vtkRenderer *renderer = this->CurrentRenderer;
  vtkCamera* camera = renderer->GetActiveCamera();

  // Compute basis vectors (up and right).
  double up[3];
  double right[3];
  double position[3];
  double direction[3];
  camera->GetFocalPoint(direction);
  camera->GetPosition(position);
  double origin[3];
  this->GeoCamera->GetOrigin(origin);

  direction[0] = direction[0] - position[0];
  direction[1] = direction[1] - position[1];
  direction[2] = direction[2] - position[2];
  position[0] = position[0] + origin[0];
  position[1] = position[1] + origin[1];
  position[2] = position[2] + origin[2];
  camera->GetViewUp(up);
  vtkMath::Cross(direction, up, right);
  vtkMath::Normalize(right);
  // Up may not be orthogonalized.
  vtkMath::Cross(right, direction, up);
  vtkMath::Normalize(up);
  double dx, dy;
  int* size = renderer->GetSize();
  dx = xMouse - size[0]*0.5;
  dy = yMouse - size[1]*0.5;

  double angle = camera->GetViewAngle();
  double tmp = tan( vtkMath::RadiansFromDegrees( angle ) * 0.5 );
  vtkMath::Normalize(direction);
  // note the duplication of size[1] is intentional in the lines below
  direction[0] = direction[0] + tmp*2.0*dx*right[0]/size[1]
    + tmp*2.0*dy*up[0]/size[1];
  direction[1] = direction[1] + tmp*2.0*dx*right[1]/size[1]
    + tmp*2.0*dy*up[1]/size[1];
  direction[2] = direction[2] + tmp*2.0*dx*right[2]/size[1]
    + tmp*2.0*dy*up[2]/size[1];

  // OK, now we have a new direction.
  // Find an intersection with the world.
  double point[3];
  int result = this->GetRayIntersection(position, direction, point);
  wx = point[0];
  wy = point[1];
  wz = point[2];
  return result;
}

void vtkGeoInteractorStyle::WorldToLongLat(double wx, double wy,
                                           double wz,
                                           double &lon, double &lat)
{
  double r = sqrt(wx*wx + wy*wy + wz*wz);
  lat = double( vtkMath::DegreesFromRadians( asin(wz/r) ) );
  lon = double( vtkMath::DegreesFromRadians( atan2(wy,wx) ) - 90. );
}

void vtkGeoInteractorStyle::ViewportToLongLat(double x, double y,
                                              double &lon, double &lat)
{
  double wx, wy, wz;
  this->ViewportToWorld(x,y,wx,wy,wz);
  this->WorldToLongLat(wx,wy,wz,lon,lat);
}

//-----------------------------------------------------------------------------
// This is called when the left click verifies that the user wants to
// zoom to the rectangle selected.
void vtkGeoInteractorStyle::RubberBandZoom()
{
  // adjust lat, lon, and distance, heading and tilt are unchanged
  double RBCenter[2];
  double lat, lon;
  RBCenter[0] = (this->RubberBandExtent[0] + this->RubberBandExtent[1]) / 2.0;
  RBCenter[1] = (this->RubberBandExtent[2] + this->RubberBandExtent[3]) / 2.0;

  this->ViewportToLongLat(RBCenter[0], RBCenter[1], lon, lat);
  this->GeoCamera->SetLongitude(lon);
  this->GeoCamera->SetLatitude(lat);

  // compute the appropriate distance
  int* renSize;
  renSize = this->CurrentRenderer->GetSize();
  double scaleX = abs(this->RubberBandExtent[0] - this->RubberBandExtent[1])/
    static_cast<double>(renSize[0]);
  double scaleY = abs(this->RubberBandExtent[2] - this->RubberBandExtent[3])/
    static_cast<double>(renSize[1]);

  this->GeoCamera->SetDistance(this->GeoCamera->GetDistance()
                               *(scaleX + scaleY)/2.0);
  this->CompassWidget->SetDistance(this->GeoCamera->GetDistance());

  this->ResetCameraClippingRange();
  this->UpdateLights();

  this->Interactor->Render();
}

//-----------------------------------------------------------------------------
void vtkGeoInteractorStyle::OnEnter()
{
  // We can change the cursor here.
}

//-----------------------------------------------------------------------------
void vtkGeoInteractorStyle::OnLeave()
{
  // We can change the cursor here.
}


//-----------------------------------------------------------------------------
void vtkGeoInteractorStyle::ResetCamera()
{
  this->GeoCamera->SetLongitude(0.0);
  this->GeoCamera->SetLatitude(0.0);
  this->GeoCamera->SetDistance(5.0*vtkGeoMath::EarthRadiusMeters());
  this->CompassWidget->SetDistance(5.0*vtkGeoMath::EarthRadiusMeters());
  this->GeoCamera->SetTilt(90.0);
  this->CompassWidget->SetTilt(90);
  this->GeoCamera->SetHeading(0.0);
  this->CompassWidget->SetHeading(0);
  this->ResetCameraClippingRange();
}

//-----------------------------------------------------------------------------
void vtkGeoInteractorStyle::OnChar()
{
  vtkRenderWindowInteractor *rwi = this->Interactor;

  switch (rwi->GetKeyCode())
    {
    case 'a' :
      {
      break;
      }
    case 'q' :
      {
      break;
      }
    case 'r' :
    case 'R' :
      {
      this->FindPokedRenderer(rwi->GetEventPosition()[0],
                              rwi->GetEventPosition()[1]);
      this->ResetCamera();
      this->UpdateLights();
      rwi->Render();
      break;
      }
    case 'w' :
    case 'W' :
      {
      vtkActorCollection *ac;
      vtkActor *anActor, *aPart;
      vtkAssemblyPath *path;
      this->FindPokedRenderer(rwi->GetEventPosition()[0],
                              rwi->GetEventPosition()[1]);
      ac = this->CurrentRenderer->GetActors();
      vtkCollectionSimpleIterator ait;
      for (ac->InitTraversal(ait); (anActor = ac->GetNextActor(ait)); )
        {
        for (anActor->InitPathTraversal(); (path=anActor->GetNextPath()); )
          {
          aPart=vtkActor::SafeDownCast(path->GetLastNode()->GetViewProp());
          if(aPart)
            {
            aPart->GetProperty()->SetRepresentationToWireframe();
            }
          }
        }
      rwi->Render();
      }
      break;

    case 's' :
    case 'S' :
      {
      vtkActorCollection *ac;
      vtkActor *anActor, *aPart;
      vtkAssemblyPath *path;
      this->FindPokedRenderer(rwi->GetEventPosition()[0],
                              rwi->GetEventPosition()[1]);
      ac = this->CurrentRenderer->GetActors();
      vtkCollectionSimpleIterator ait;
      for (ac->InitTraversal(ait); (anActor = ac->GetNextActor(ait)); )
        {
        for (anActor->InitPathTraversal(); (path=anActor->GetNextPath()); )
          {

          aPart=vtkActor::SafeDownCast(path->GetLastNode()->GetViewProp());
          if(aPart)
            {
            aPart->GetProperty()->SetRepresentationToSurface();
            }
          }
        }
      rwi->Render();
      }
      break;
    }
}

//-----------------------------------------------------------------------------
void vtkGeoInteractorStyle::OnMouseMove()
{

  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  switch (this->State)
    {
    case VTKIS_PAN:
      this->FindPokedRenderer(x, y);
      this->Pan();
      this->InvokeEvent(vtkCommand::InteractionEvent, NULL);
      break;

    case VTKIS_DOLLY:
      this->FindPokedRenderer(x, y);
      this->Dolly();
      this->InvokeEvent(vtkCommand::InteractionEvent, NULL);
      break;
    }
  //rubber band zoom case:
  if (this->Interactor && this->DraggingRubberBandBoxState)
    {
    // Get rid of selected extent from previous cycle.
    if (this->RubberBandExtentEnabled)
      {
      this->DisableRubberBandRedraw();
      this->Interactor->Render();
      this->RubberBandExtentEnabled = 0;
      }
    this->EndPosition[0] = this->Interactor->GetEventPosition()[0];
    this->EndPosition[1] = this->Interactor->GetEventPosition()[1];

    // Transfer the position to the extent.
    if (this->StartPosition[0] < this->EndPosition[0])
      {
      this->RubberBandExtent[0] = this->StartPosition[0];
      this->RubberBandExtent[1] = this->EndPosition[0];
      }
    else
      {
      this->RubberBandExtent[0] = this->EndPosition[0];
      this->RubberBandExtent[1] = this->StartPosition[0];
      }
    if (this->StartPosition[1] < this->EndPosition[1])
      {
      this->RubberBandExtent[2] = this->StartPosition[1];
      this->RubberBandExtent[3] = this->EndPosition[1];
      }
    else
      {
      this->RubberBandExtent[2] = this->EndPosition[1];
      this->RubberBandExtent[3] = this->StartPosition[1];
      }

    this->DrawRectangle();
    }
}

//-----------------------------------------------------------------------------
//<cough> compute a good screen coordinate to base pan operations off
//of. Brute force approach. Sample the screen and take the weighted average
//of hits
void vtkGeoInteractorStyle::GetPanCenter(double &px, double &py)
{
  vtkRenderer *renderer = this->CurrentRenderer;
  vtkCamera* camera = renderer->GetActiveCamera();

  // Compute basis vectors (up and right).
  double up[3];
  double right[3];
  double position[3];
  double direction[3];
  double direction2[3];
  camera->GetPosition(position);
  camera->GetFocalPoint(direction);
  double origin[3];
  this->GeoCamera->GetOrigin(origin);
  direction[0] = direction[0]+origin[0] - position[0];
  direction[1] = direction[1]+origin[1] - position[1];
  direction[2] = direction[2]+origin[2] - position[2];
  camera->GetViewUp(up);
  vtkMath::Cross(direction, up, right);
  vtkMath::Normalize(right);

  // Up may not be orthogonalized.
  vtkMath::Cross(right, direction, up);
  vtkMath::Normalize(up);
  double dx, dy;
  double angle = camera->GetViewAngle();
  double tmp = tan( vtkMath::RadiansFromDegrees( angle ) * 0.5 );
  int* size = renderer->GetSize();
  tmp = tmp*2.0/size[1];
  vtkMath::Normalize(direction);
  double point[3];

  double resx = 0;
  double resy = 0;
  int hits = 0;
  unsigned int ix, iy;
  for (ix = 0; ix < 9; ++ix)
    {
    dx = size[0]*ix/8.0 - size[0]*0.5;
    for (iy = 0; iy < 9; ++iy)
      {
      dy = size[1]*iy/8.0 - size[1]*0.5;

      // note the duplication of size[1] is intentional in the lines below
      direction2[0] = direction[0] + dx*tmp*right[0] + tmp*dy*up[0];
      direction2[1] = direction[1] + dx*tmp*right[1] + tmp*dy*up[1];
      direction2[2] = direction[2] + dx*tmp*right[2] + tmp*dy*up[2];

      // OK, now we have a new direction.
      // Find an intersection with the world.
      if (this->GetRayIntersection(position, direction2, point) != VTK_ERROR)
        {
        resx += dx;
        resy += dy;
        hits++;
        }
      }
    }

  // compute the result
  px = size[0]*0.5;
  py = size[1]*0.5;
  if (hits)
    {
    px += resx/hits;
    py += resy/hits;
    }
}

//-----------------------------------------------------------------------------
void vtkGeoInteractorStyle::Pan()
{
  // just change the lat lon
  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  vtkRenderWindowInteractor *rwi = this->Interactor;
  double dxMouse;
  double dyMouse;

  // It appears like the y is already flipped here.
  dxMouse=static_cast<double>(rwi->GetEventPosition()[0]-rwi->GetLastEventPosition()[0]);
  dyMouse=static_cast<double>(rwi->GetEventPosition()[1]-rwi->GetLastEventPosition()[1]);

  // use the center of the screen area covered by the earth
  // to determine the amount of lat long adjustment. The end
  // result is a x,y view position about which to pan
  double px, py;
  this->GetPanCenter(px,py);

  double lonlat1[2];
  double lonlat2[2];
  this->ViewportToLongLat(px, py,
                          lonlat1[0], lonlat1[1]);
  this->ViewportToLongLat(px - dxMouse,
                          py - dyMouse,
                          lonlat2[0], lonlat2[1]);

  if (!this->LockHeading)
    {
    this->GeoCamera->LockHeadingOff();
    }
  this->GeoCamera->SetLongitude(this->GeoCamera->GetLongitude() +
                                lonlat2[0] - lonlat1[0]);
  this->GeoCamera->SetLatitude(this->GeoCamera->GetLatitude() +
                               lonlat2[1] - lonlat1[1]);
  if (!this->LockHeading)
    {
    this->GeoCamera->LockHeadingOn();
    this->CompassWidget->SetHeading(this->GeoCamera->GetHeading()/360.0);
    }

  this->ResetCameraClippingRange();
  this->UpdateLights();

  rwi->Render();
}


//-----------------------------------------------------------------------------
void vtkGeoInteractorStyle::Dolly()
{
  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  vtkRenderWindowInteractor *rwi = this->Interactor;

  // These computations assume a perfect sphere.
  int dy = rwi->GetEventPosition()[1] - rwi->GetLastEventPosition()[1];
  int* size = this->CurrentRenderer->GetSize();
  double factor = 1.0 - ( static_cast<double>(dy) / size[1]);

  this->Dolly(factor);
}

//-----------------------------------------------------------------------------
void vtkGeoInteractorStyle::Dolly(double factor)
{
  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  vtkRenderWindowInteractor *rwi = this->Interactor;

  double distance = this->GeoCamera->GetDistance();
  distance = distance / factor;

  // The Long,Lat,Alt API makes it difficult to Dolly.
  // Leave it to the GeoCamera.
  this->GeoCamera->SetDistance(distance);
  this->CompassWidget->SetDistance(distance);

  this->UpdateLights();
  this->ResetCameraClippingRange();
  rwi->Render();
}

//-----------------------------------------------------------------------------
// Called by the renderer start event.
// Redraw the rectangle.
void vtkGeoInteractorStyleRenderCallback(vtkObject *caller,
                                 unsigned long vtkNotUsed(event),
                                 void *clientData, void *)
{
  caller = caller;
  vtkGeoInteractorStyle *self
    = static_cast<vtkGeoInteractorStyle *>(clientData);

  self->RedrawRectangle();
}

//-----------------------------------------------------------------------------
// If anything causes a render we will loose the rubber band rectangle.
// The method gets a new background image and redraws the rectangle.
void vtkGeoInteractorStyle::RedrawRectangle()
{
  // Do we need a new background image allocated.
  int numPixels = 0;
  if (this->PixelArray)
    {
    numPixels = this->PixelArray->GetNumberOfTuples();
    }
  vtkRenderWindow *renWin = this->Interactor->GetRenderWindow();
  int *winSize = renWin->GetSize();
  if (winSize[0] * winSize[1] != numPixels)
    {
    this->PixelArray->Initialize();
    this->PixelArray->SetNumberOfComponents(3);
    this->PixelArray->SetNumberOfTuples(winSize[0]*winSize[1]);
    this->PixelDims[0] = winSize[0];
    this->PixelDims[1] = winSize[1];
    }
  // Could do some mTime checks but that would only catch refreshes.
  renWin->GetPixelData(0, 0, winSize[0]-1, winSize[1]-1, 1, this->PixelArray);

  // Make sure the extent still lies completely inside the window.
  if (this->RubberBandExtent[0] < 0)
    {
    this->RubberBandExtent[0] = 0;
    }
  if (this->RubberBandExtent[2] < 0)
    {
    this->RubberBandExtent[2] = 0;
    }
  if (this->RubberBandExtent[1] >= winSize[0])
    {
    this->RubberBandExtent[1] = winSize[0]-1;
    }
  if (this->RubberBandExtent[3] >= winSize[1])
    {
    this->RubberBandExtent[3] = winSize[1]-1;
    }

  // Now draw the rectangle.
  this->DrawRectangle();
}

//-----------------------------------------------------------------------------
// This assumes the extent has been properly constrained inside the window.
void vtkGeoInteractorStyle::DrawRectangle()
{
  // If this is the first time we are drawing,
  // we need to get the background image.
  // Use the callback tag to indicate whether this is the first time or not.
  if (this->RenderCallbackTag == 0)
    {
    vtkRenderWindow *renWin = this->Interactor->GetRenderWindow();

    this->PixelArray->Initialize();
    this->PixelArray->SetNumberOfComponents(3);
    int *size = renWin->GetSize();
    this->PixelDims[0] = size[0];
    this->PixelDims[1] = size[1];
    this->PixelArray->SetNumberOfTuples(this->PixelDims[0]*this->PixelDims[1]);

    renWin->GetPixelData(0, 0, this->PixelDims[0]-1, this->PixelDims[1]-1, 1,
                         this->PixelArray);

    // Add a callback (if not already added) that redraws the rectangle.
    this->EnableRubberBandRedraw();
    }

  vtkUnsignedCharArray *tmpPixelArray = vtkUnsignedCharArray::New();
  tmpPixelArray->DeepCopy(this->PixelArray);

  unsigned char *pixels = tmpPixelArray->GetPointer(0);

  // Make sure the extent still lies completely inside the window.
  if (this->RubberBandExtent[0] < 0)
    {
    this->RubberBandExtent[0] = 0;
    }
  if (this->RubberBandExtent[2] < 0)
    {
    this->RubberBandExtent[2] = 0;
    }
  if (this->RubberBandExtent[1] >= this->PixelDims[0])
    {
    this->RubberBandExtent[1] = this->PixelDims[0]-1;
    }
  if (this->RubberBandExtent[3] >= this->PixelDims[1])
    {
    this->RubberBandExtent[3] = this->PixelDims[1]-1;
    }

  int min[2], max[2];
  min[0] = this->RubberBandExtent[0];
  max[0] = this->RubberBandExtent[1];
  min[1] = this->RubberBandExtent[2];
  max[1] = this->RubberBandExtent[3];

  int i;
  int *size = this->PixelDims;
  for (i = min[0]; i <= max[0]; i++)
    {
    pixels[3*(min[1]*size[0]+i)] = 255 ^ pixels[3*(min[1]*size[0]+i)];
    pixels[3*(min[1]*size[0]+i)+1] = 255 ^ pixels[3*(min[1]*size[0]+i)+1];
    pixels[3*(min[1]*size[0]+i)+2] = 255 ^ pixels[3*(min[1]*size[0]+i)+2];
    pixels[3*(max[1]*size[0]+i)] = 255 ^ pixels[3*(max[1]*size[0]+i)];
    pixels[3*(max[1]*size[0]+i)+1] = 255 ^ pixels[3*(max[1]*size[0]+i)+1];
    pixels[3*(max[1]*size[0]+i)+2] = 255 ^ pixels[3*(max[1]*size[0]+i)+2];
    }
  for (i = min[1]+1; i < max[1]; i++)
    {
    pixels[3*(i*size[0]+min[0])] = 255 ^ pixels[3*(i*size[0]+min[0])];
    pixels[3*(i*size[0]+min[0])+1] = 255 ^ pixels[3*(i*size[0]+min[0])+1];
    pixels[3*(i*size[0]+min[0])+2] = 255 ^ pixels[3*(i*size[0]+min[0])+2];
    pixels[3*(i*size[0]+max[0])] = 255 ^ pixels[3*(i*size[0]+max[0])];
    pixels[3*(i*size[0]+max[0])+1] = 255 ^ pixels[3*(i*size[0]+max[0])+1];
    pixels[3*(i*size[0]+max[0])+2] = 255 ^ pixels[3*(i*size[0]+max[0])+2];
    }

  this->Interactor->GetRenderWindow()->SetPixelData(0, 0,
                                                    size[0]-1, size[1]-1,
                                                    pixels, 1);
  tmpPixelArray->Delete();
}

//-----------------------------------------------------------------------------
// If anything causes a render we will loose the rubber band rectangle.
// The callback will draw it again.
void vtkGeoInteractorStyle::EnableRubberBandRedraw()
{
  if (this->RenderCallbackTag != 0)
    { // Callback has already been added.
    return;
    }

  vtkRenderWindow* renWin = this->Interactor->GetRenderWindow();
  if (renWin == 0)
    {
    return;
    }

  // Watch for any render that will disable the rectangle.
  vtkCallbackCommand *cbc;

  cbc= vtkCallbackCommand::New();
  cbc->SetCallback(vtkGeoInteractorStyleRenderCallback);
  cbc->SetClientData(this);
  // Renderer will delete the cbc when the observer is removed.
  this->RenderCallbackTag = renWin->AddObserver(vtkCommand::EndEvent,cbc);
  cbc->Delete();
}

//-----------------------------------------------------------------------------
void vtkGeoInteractorStyle::DisableRubberBandRedraw()
{
  if (this->RenderCallbackTag == 0 || this->Interactor == 0)
    {
    return;
    }

  vtkRenderWindow *renWin = this->Interactor->GetRenderWindow();
  if (renWin == 0)
    {
    return;
    }

  renWin->RemoveObserver(this->RenderCallbackTag);
  this->RenderCallbackTag = 0;
}


//-----------------------------------------------------------------------------
// This works with the globe source.
//---------------------------------------------------------------------------
void vtkGeoInteractorStyle::ResetCameraClippingRange()
{
  // Do smart clipping such that the near clipping plane is at least
  // as close as halfway from the camera to the earth's surface.
  vtkCamera *camera;
  double position[3];

  if (!this->CurrentRenderer)
    {
    return;
    }
  camera = this->CurrentRenderer->GetActiveCamera();
  this->GeoCamera->GetPosition(position);

  double distAbove = vtkMath::Norm(position) - vtkGeoMath::EarthRadiusMeters();
  // when inside the earth use the default
  if (distAbove < 0)
    {
    this->CurrentRenderer->ResetCameraClippingRange();
    return;
    }
  this->CurrentRenderer->ResetCameraClippingRange();
  double rng[2];
  camera->GetClippingRange(rng);

  // When we are 1 unit away from the ground, place the near plane at 0.01
  // unit away from the camera.
  double nearDist = distAbove * 0.01;
  if (rng[0] > nearDist)
    {
    rng[0] = nearDist;
    rng[1] = distAbove + vtkGeoMath::EarthRadiusMeters() * 2.0 + 100.0;
    camera->SetClippingRange(rng);
    }
}

//-----------------------------------------------------------------------------
void vtkGeoInteractorStyle::OnTimer()
{
  vtkRenderWindowInteractor *rwi = this->Interactor;

  switch (this->State)
    {
    case VTKIS_NONE:
      if (this->AnimState == VTKIS_ANIM_ON)
        {
        if (this->UseTimers)
          {
          rwi->DestroyTimer();
          }
        rwi->Render();
        if (this->UseTimers)
          {
          rwi->CreateTimer(VTKI_TIMER_FIRST);
          }
        }
      break;

    case VTKIS_TIMER:
      rwi->Render();
      if (this->UseTimers)
        {
        rwi->CreateTimer(VTKI_TIMER_UPDATE);
        }
      break;

    default:
      break;
    }
}


//-----------------------------------------------------------------------------
// Change the light base on camera position.
void vtkGeoInteractorStyle::UpdateLights()
{
  if (!this->CurrentRenderer || !this->Interactor)
    {
    return;
    }

  vtkCamera *camera;
  vtkLightCollection* lights;
  vtkLight *light;
  double position[3];
  double focalPoint[3];

  this->Interactor->SetLightFollowCamera(0);
  this->CurrentRenderer->SetLightFollowCamera(0);

  // only update the light's geometry if this Renderer is tracking
  // this lights.  That allows one renderer to view the lights that
  // another renderer is setting up.
  camera = this->CurrentRenderer->GetActiveCamera();
  // Overhead light pointing to center of earth.
  camera->GetPosition(position);
  position[0] = position[0] * 2.0;
  position[1] = position[1] * 2.0;
  position[2] = position[2] * 2.0;
  focalPoint[0] = focalPoint[1] = focalPoint[2] = 0.0;
  //vtkMatrix4x4 *lightMatrix = camera->GetCameraLightTransformMatrix();

  lights = this->CurrentRenderer->GetLights();
  vtkCollectionSimpleIterator sit;
  for(lights->InitTraversal(sit);
      (light = lights->GetNextLight(sit)); )
    {
    light->SetPosition(position);
    light->SetFocalPoint(focalPoint);
    }
}

//----------------------------------------------------------------------------
// The only thing this does different than the superclass
// is use the old CreateTimer instead of the CreateRepeatingTimer.
// If we make CreateRepeatingTimer virtual, then we can get rid of this
// method.
void vtkGeoInteractorStyle::StartState(int newstate)
{
  this->State = newstate;
  if (this->AnimState == VTKIS_ANIM_OFF)
    {
    vtkRenderWindowInteractor *rwi = this->Interactor;
    rwi->GetRenderWindow()->SetDesiredUpdateRate(rwi->GetDesiredUpdateRate());
    this->InvokeEvent(vtkCommand::StartInteractionEvent, NULL);
    rwi->SetTimerEventDuration(this->TimerDuration);
    if ( this->UseTimers && !(this->TimerId=rwi->CreateTimer(VTKI_TIMER_FIRST)) )
      {
      vtkErrorMacro(<< "Timer start failed");
      this->State = VTKIS_NONE;
      }
    }
}

void vtkGeoInteractorStyle::WidgetInteraction(vtkObject *caller)
{
  if (caller == this->CompassWidget.GetPointer())
    {
    this->GeoCamera->SetHeading(this->CompassWidget->GetHeading()*360.0);
    this->GeoCamera->SetTilt(this->CompassWidget->GetTilt());
    this->GeoCamera->SetDistance(this->CompassWidget->GetDistance());
    this->ResetCameraClippingRange();
    this->UpdateLights();
    this->Interactor->Render();
    }
}


void vtkGeoInteractorStyle::SetInteractor
(vtkRenderWindowInteractor *interactor)
{
  this->Superclass::SetInteractor(interactor);
  this->CompassWidget->SetInteractor(interactor);
  if (interactor)
    {
    this->CompassWidget->SetEnabled(1);
    }
  else
    {
    this->CompassWidget->SetEnabled(0);
    }
}

void vtkGeoInteractorStyle::SetCurrentRenderer(vtkRenderer *ren)
{
  this->Superclass::SetCurrentRenderer(ren);
  if (ren)
    {
    ren->SetActiveCamera(this->GeoCamera->GetVTKCamera());
    }
  this->ResetCameraClippingRange();
  this->UpdateLights();
}
