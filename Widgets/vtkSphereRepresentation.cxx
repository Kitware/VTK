/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSphereRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSphereRepresentation.h"
#include "vtkActor.h"
#include "vtkSphere.h"
#include "vtkSphereSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyData.h"
#include "vtkCallbackCommand.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkInteractorObserver.h"
#include "vtkMath.h"
#include "vtkCellArray.h"
#include "vtkCellPicker.h"
#include "vtkTransform.h"
#include "vtkDoubleArray.h"
#include "vtkSphere.h"
#include "vtkCamera.h"
#include "vtkAssemblyPath.h"
#include "vtkTextMapper.h"
#include "vtkActor2D.h"
#include "vtkTextProperty.h"
#include "vtkLineSource.h"
#include "vtkWindow.h"
#include "vtkObjectFactory.h"


vtkStandardNewMacro(vtkSphereRepresentation);

//----------------------------------------------------------------------------
vtkSphereRepresentation::vtkSphereRepresentation()
{
  // The initial state
  this->InteractionState = vtkSphereRepresentation::Outside;

  // Handle size is in pixels for this widget
  this->HandleSize = 10.0;

  // Set up the initial representation
  this->Representation = VTK_SPHERE_WIREFRAME;

  // Set up the initial properties
  this->SphereProperty = NULL;
  this->SelectedSphereProperty = NULL;
  this->HandleProperty = NULL;
  this->SelectedHandleProperty = NULL;
  this->HandleTextProperty = NULL;
  this->RadialLineProperty = NULL;
  this->CreateDefaultProperties();
  
  // Build the representation of the widget
  // Represent the sphere
  this->SphereSource = vtkSphereSource::New();
  this->SphereSource->SetThetaResolution(16);
  this->SphereSource->SetPhiResolution(8);
  this->SphereSource->LatLongTessellationOn();
  this->SphereMapper = vtkPolyDataMapper::New();
  this->SphereMapper->SetInput(this->SphereSource->GetOutput());
  this->SphereActor = vtkActor::New();
  this->SphereActor->SetMapper(this->SphereMapper);

  // the handle
  this->HandleVisibility = 0;
  this->HandleDirection[0] = 1.0;
  this->HandleDirection[1] = 0.0;
  this->HandleDirection[2] = 0.0;
  this->HandleSource = vtkSphereSource::New();
  this->HandleSource->SetThetaResolution(16);
  this->HandleSource->SetPhiResolution(8);
  this->HandleMapper = vtkPolyDataMapper::New();
  this->HandleMapper->SetInput(this->HandleSource->GetOutput());
  this->HandleActor = vtkActor::New();
  this->HandleActor->SetMapper(this->HandleMapper);
  

  // Manage the handle label
  this->HandleText = 1;
  this->HandleTextMapper = vtkTextMapper::New();
  this->HandleTextMapper->SetTextProperty(this->HandleTextProperty);
  this->HandleTextActor = vtkActor2D::New();
  this->HandleTextActor->SetMapper(this->HandleTextMapper);
  this->HandleTextActor->GetPositionCoordinate()->SetCoordinateSystemToDisplay();

  // Manage the radial line segment
  this->RadialLine = 1;
  this->RadialLineSource = vtkLineSource::New();
  this->RadialLineSource->SetResolution(1);
  this->RadialLineMapper = vtkPolyDataMapper::New();
  this->RadialLineMapper->SetInputConnection(this->RadialLineSource->GetOutputPort());
  this->RadialLineActor = vtkActor::New();
  this->RadialLineActor->SetMapper(this->RadialLineMapper);
  this->RadialLineActor->SetProperty(this->RadialLineProperty);
  
  // Define the point coordinates
  double bounds[6];
  bounds[0] = -0.5;
  bounds[1] = 0.5;
  bounds[2] = -0.5;
  bounds[3] = 0.5;
  bounds[4] = -0.5;
  bounds[5] = 0.5;

  // First creation of the widget, serves to initialize it
  this->PlaceWidget(bounds);

  // Manage the picking stuff
  this->HandlePicker = vtkCellPicker::New();
  this->HandlePicker->SetTolerance(0.005); //need some fluff
  this->HandlePicker->AddPickList(this->HandleActor);
  this->HandlePicker->PickFromListOn();
  
  this->SpherePicker = vtkCellPicker::New();
  this->SpherePicker->SetTolerance(0.005); 
  this->SpherePicker->AddPickList(this->SphereActor);
  this->SpherePicker->PickFromListOn();
  
  this->SphereActor->SetProperty(this->SphereProperty);
  this->HandleActor->SetProperty(this->HandleProperty);
}

//----------------------------------------------------------------------------
vtkSphereRepresentation::~vtkSphereRepresentation()
{  
  this->SphereActor->Delete();
  this->SphereMapper->Delete();
  this->SphereSource->Delete();

  this->HandlePicker->Delete();
  this->SpherePicker->Delete();

  this->HandleSource->Delete();
  this->HandleMapper->Delete();
  this->HandleActor->Delete();

  // the handle
  this->HandleTextProperty->Delete();
  this->HandleTextMapper->Delete();
  this->HandleTextActor->Delete();

  // the text
  this->RadialLineProperty->Delete();
  this->RadialLineSource->Delete();
  this->RadialLineMapper->Delete();
  this->RadialLineActor->Delete();

  if ( this->SphereProperty )
    {
    this->SphereProperty->Delete();
    }
  if ( this->SelectedSphereProperty )
    {
    this->SelectedSphereProperty->Delete();
    }
  if ( this->HandleProperty )
    {
    this->HandleProperty->Delete();
    }
  if ( this->SelectedHandleProperty )
    {
    this->SelectedHandleProperty->Delete();
    }
}

//----------------------------------------------------------------------
void vtkSphereRepresentation::GetPolyData(vtkPolyData *pd)
{
  pd->ShallowCopy(this->SphereSource->GetOutput()); 
}

//----------------------------------------------------------------------
void vtkSphereRepresentation::GetSphere(vtkSphere *sphere)
{
  sphere->SetRadius(this->SphereSource->GetRadius());
  sphere->SetCenter(this->SphereSource->GetCenter());
}

//----------------------------------------------------------------------
void vtkSphereRepresentation::HighlightSphere(int highlight)
{
  if ( highlight )
    {
    this->ValidPick = 1;
    this->SphereActor->SetProperty(this->SelectedSphereProperty);
    }
  else
    {
    this->SphereActor->SetProperty(this->SphereProperty);
    }
}

//----------------------------------------------------------------------
void vtkSphereRepresentation::HighlightHandle(int highlight)
{
  if ( highlight )
    {
    this->ValidPick = 1;
    this->HandleActor->SetProperty(this->SelectedHandleProperty);
    }
  else
    {
    this->HandleActor->SetProperty(this->HandleProperty);
    }
}

//----------------------------------------------------------------------
void vtkSphereRepresentation::Scale(double *p1, double *p2, 
                                    int vtkNotUsed(X), int Y)
{
  //Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  double radius = this->SphereSource->GetRadius();
  double *c = this->SphereSource->GetCenter();

  // Compute the scale factor
  double sf = vtkMath::Norm(v) / radius;
  if ( Y > this->LastEventPosition[1] )
    {
    sf = 1.0 + sf;
    }
  else
    {
    sf = 1.0 - sf;
    }
  
  // Make sure that the radius is valid; don't let it shrink further
  // but it can still grow in radius.
  if ( Y <= this->LastEventPosition[1] && sf*radius < 1.0e-06*this->InitialLength )
    {
    return;
    }

  // Need to prevent radius going to zero
  this->SphereSource->SetRadius(sf*radius);
  this->HandlePosition[0] = c[0]+sf*(this->HandlePosition[0]-c[0]);
  this->HandlePosition[1] = c[1]+sf*(this->HandlePosition[1]-c[1]);
  this->HandlePosition[2] = c[2]+sf*(this->HandlePosition[2]-c[2]);
  this->HandleSource->SetCenter(this->HandlePosition);
}

//----------------------------------------------------------------------
void vtkSphereRepresentation::StartWidgetInteraction(double e[2])
{
  // Store the start position
  this->StartEventPosition[0] = e[0];
  this->StartEventPosition[1] = e[1];
  this->StartEventPosition[2] = 0.0;

  // Store the start position
  this->LastEventPosition[0] = e[0];
  this->LastEventPosition[1] = e[1];
  this->LastEventPosition[2] = 0.0;

  this->ComputeInteractionState(static_cast<int>(e[0]),static_cast<int>(e[1]),0);
}

//----------------------------------------------------------------------
void vtkSphereRepresentation::WidgetInteraction(double e[2])
{
  // Convert events to appropriate coordinate systems
  vtkCamera *camera = this->Renderer->GetActiveCamera();
  if ( !camera )
    {
    return;
    }
  double focalPoint[4], pickPoint[4], prevPickPoint[4];
  double z, vpn[3];
  camera->GetViewPlaneNormal(vpn);

  // Compute the two points defining the motion vector
  vtkInteractorObserver::ComputeWorldToDisplay(this->Renderer,
                                               this->LastPickPosition[0], this->LastPickPosition[1], this->LastPickPosition[2], 
                                               focalPoint);
  z = focalPoint[2];
  vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer,this->LastEventPosition[0],
                                               this->LastEventPosition[1], z, prevPickPoint);
  vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer, e[0], e[1], z, pickPoint);

  // Process the motion
  if ( this->InteractionState == vtkSphereRepresentation::Translating )
    {
    this->Translate(prevPickPoint, pickPoint);
    }

  else if ( this->InteractionState == vtkSphereRepresentation::Scaling )
    {
    this->Scale(prevPickPoint, pickPoint, 
                static_cast<int>(e[0]), static_cast<int>(e[1]));
    }
  else if ( this->InteractionState == vtkSphereRepresentation::MovingHandle )
    {
    this->SpherePicker->Pick(static_cast<int>(e[0]),static_cast<int>(e[1]),0.0,
                             this->Renderer);
    vtkAssemblyPath *path = this->SpherePicker->GetPath();
    if ( path != NULL )
      {
      this->HandleSource->SetCenter(this->SpherePicker->GetPickPosition());
      this->SpherePicker->GetPickPosition(this->HandlePosition);
      }
    }

  // Store the position
  this->LastEventPosition[0] = e[0];
  this->LastEventPosition[1] = e[1];
  this->LastEventPosition[2] = 0.0;
}

//----------------------------------------------------------------------------
// Loop through all points and translate them
void vtkSphereRepresentation::Translate(double *p1, double *p2)
{
  //Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];
  
  //int res = this->SphereSource->GetResolution();
  double *center = this->SphereSource->GetCenter();

  double center1[3];
  for (int i=0; i<3; i++)
    {
    center1[i] = center[i] + v[i];
    this->HandlePosition[i] += v[i];
    }
  
  this->SphereSource->SetCenter(center1);
  this->HandleSource->SetCenter(HandlePosition);
}

//----------------------------------------------------------------------------
void vtkSphereRepresentation::CreateDefaultProperties()
{
  if ( ! this->SphereProperty )
    {
    this->SphereProperty = vtkProperty::New();
    }
  if ( ! this->SelectedSphereProperty )
    {
    this->SelectedSphereProperty = vtkProperty::New();
    }

  if ( ! this->HandleProperty )
    {
    this->HandleProperty = vtkProperty::New();
    this->HandleProperty->SetColor(1,1,1);
    }
  if ( ! this->SelectedHandleProperty )
    {
    this->SelectedHandleProperty = vtkProperty::New();
    this->SelectedHandleProperty->SetColor(1,0,0);
    }

  if ( ! this->HandleTextProperty )
    {
    this->HandleTextProperty = vtkTextProperty::New();
    this->HandleTextProperty->SetFontSize(12);
    this->HandleTextProperty->SetBold(1);
    this->HandleTextProperty->SetItalic(1);
    this->HandleTextProperty->SetShadow(1);
    this->HandleTextProperty->SetFontFamilyToArial();
    }
  
  if ( ! this->RadialLineProperty )
    {
    this->RadialLineProperty = vtkProperty::New();
    this->RadialLineProperty->SetColor(1,0,0);
    }
}

//----------------------------------------------------------------------------
void vtkSphereRepresentation::PlaceWidget(double center[3], double handle[3])
{
  double r = vtkMath::Distance2BetweenPoints(center,handle);
  this->SphereSource->SetCenter(center);
  this->SphereSource->SetRadius(r);
  this->SphereSource->Update();

  this->HandlePosition[0] = handle[0];
  this->HandlePosition[1] = handle[1];
  this->HandlePosition[2] = handle[2];
  this->HandleSource->SetCenter(handle);
  this->HandleSource->Update();

  this->HandleDirection[0] = handle[0] - center[0];
  this->HandleDirection[1] = handle[1] - center[1];
  this->HandleDirection[2] = handle[2] - center[2];

  this->InitialLength = r;
  this->InitialBounds[0] = center[0] - r;
  this->InitialBounds[1] = center[0] + r;
  this->InitialBounds[2] = center[1] - r;
  this->InitialBounds[3] = center[1] + r;
  this->InitialBounds[4] = center[2] - r;
  this->InitialBounds[5] = center[2] + r;

  this->ValidPick = 1;
  this->BuildRepresentation();
}

//----------------------------------------------------------------------------
void vtkSphereRepresentation::SetCenter(double center[3])
{
  double c[3];
  this->SphereSource->GetCenter(c);
  if ( c[0] != center[0] || c[1] != center[1] || c[2] != center[2] )
    {
    double handle[3];
    this->SphereSource->SetCenter(center);

    if(this->GetHandleVisibility())
      {
      this->HandleSource->GetCenter(handle);
      this->HandleDirection[0] = handle[0] - center[0];
      this->HandleDirection[1] = handle[1] - center[1];
      this->HandleDirection[2] = handle[2] - center[2];
      double r = static_cast<double>(
        vtkMath::Distance2BetweenPoints(handle,center) );
      this->SphereSource->SetRadius(r);
      }

    this->SphereSource->Update();
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkSphereRepresentation::SetRadius(double r)
{
  r = (r <= this->InitialLength*1.0e-04 ? this->InitialLength*1.0e-04 : r);
  if ( r != this->SphereSource->GetRadius() )
    {
    double center[3];
    this->SphereSource->SetRadius(r); 
    this->SphereSource->GetCenter(center);
    this->PlaceHandle(center,r);
    this->SphereSource->Update();
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkSphereRepresentation::SetHandlePosition(double handle[3])
{
  double h[3];
  this->HandleSource->GetCenter(h);
  if ( h[0] != handle[0] || h[1] != handle[1] || h[2] != handle[2] )
    {
    double c[3];
    this->HandleSource->SetCenter(handle);
    this->SphereSource->GetCenter(c);
    this->HandleDirection[0] = handle[0] - c[0];
    this->HandleDirection[1] = handle[1] - c[1];
    this->HandleDirection[2] = handle[2] - c[2];
    double r = static_cast<double>(
      vtkMath::Distance2BetweenPoints(handle,c) );
    this->SphereSource->SetRadius(r);
    this->SphereSource->Update();
    this->HandleSource->Update();
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkSphereRepresentation::SetHandleDirection(double dir[3])
{
  double *d = this->HandleDirection;
  if ( d[0] != dir[0] || d[1] != dir[1] || d[2] != dir[2] )
    {
    double c[3], handle[3];
    this->SphereSource->GetCenter(c);
    handle[0] = c[0] + dir[0];
    handle[1] = c[1] + dir[1];
    handle[2] = c[2] + dir[2];
    this->HandleSource->SetCenter(handle);
    this->HandleDirection[0] = dir[0];
    this->HandleDirection[1] = dir[1];
    this->HandleDirection[2] = dir[2];
    double r = static_cast<double>(
      vtkMath::Distance2BetweenPoints(handle,c) );
    this->SphereSource->SetRadius(r);
    this->SphereSource->Update();
    this->HandleSource->Update();
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkSphereRepresentation::PlaceWidget(double bds[6])
{
  double bounds[6], center[3], radius;

  this->AdjustBounds(bds, bounds, center);
  
  radius = (bounds[1]-bounds[0]) / 2.0;
  if ( radius > ((bounds[3]-bounds[2])/2.0) )
    {
    radius = (bounds[3]-bounds[2])/2.0;
    }
  radius = (bounds[1]-bounds[0]) / 2.0;
  if ( radius > ((bounds[5]-bounds[4])/2.0) )
    {
    radius = (bounds[5]-bounds[4])/2.0;
    }
  
  this->SphereSource->SetCenter(center);
  this->SphereSource->SetRadius(radius);
  this->SphereSource->Update();

  // place the handle
  this->PlaceHandle(center,radius);

  for (int i=0; i<6; i++)
    {
    this->InitialBounds[i] = bounds[i];
    }
  this->InitialLength = sqrt((bounds[1]-bounds[0])*(bounds[1]-bounds[0]) +
                             (bounds[3]-bounds[2])*(bounds[3]-bounds[2]) +
                             (bounds[5]-bounds[4])*(bounds[5]-bounds[4]));

  this->ValidPick = 1; // since we have set up widget properly
  this->SizeHandles();
}

//----------------------------------------------------------------------------
void vtkSphereRepresentation::PlaceHandle(double *center, double radius)
{
  double sf = radius / vtkMath::Norm(this->HandleDirection);

  this->HandlePosition[0] = center[0] + sf*this->HandleDirection[0];
  this->HandlePosition[1] = center[1] + sf*this->HandleDirection[1];
  this->HandlePosition[2] = center[2] + sf*this->HandleDirection[2];
  this->HandleSource->SetCenter(this->HandlePosition);
  this->HandleSource->Update();
}

//----------------------------------------------------------------------------
int vtkSphereRepresentation::ComputeInteractionState(int X, int Y, int vtkNotUsed(modify))
{
  // Okay, we can process this. Try to pick handles first;
  // if no handles picked, then pick the bounding box.
  this->InteractionState = vtkSphereRepresentation::Outside;
  if (!this->Renderer || !this->Renderer->IsInViewport(X, Y))
    {
    return this->InteractionState;
    }
  
  // Try and pick a handle first. This allows the picking of the handle even
  // if it is "behind" the sphere.
  vtkAssemblyPath *path;
  int handlePicked = 0;
  if ( this->HandleVisibility || this->HandleText || this->RadialLine )
    {
    this->HandlePicker->Pick(X,Y,0.0,this->Renderer);
    path = this->HandlePicker->GetPath();
    if ( path != NULL )
      {
      this->ValidPick = 1;
      this->InteractionState = vtkSphereRepresentation::MovingHandle;
      this->HandleSource->GetCenter(this->LastPickPosition);
      this->HandleSource->GetCenter(this->HandlePosition);
      handlePicked = 1;
      }
    }
  
  if ( ! handlePicked )
    {
    this->SpherePicker->Pick(X,Y,0.0,this->Renderer);
    path = this->SpherePicker->GetPath();
    if ( path != NULL )
      {
      this->ValidPick = 1;
      this->InteractionState = vtkSphereRepresentation::OnSphere;
      this->SpherePicker->GetPickPosition(this->LastPickPosition);
      }
    }

  return this->InteractionState;
}

//----------------------------------------------------------------------
void vtkSphereRepresentation::SetInteractionState(int state)
{
  // Clamp to allowable values
  state = ( state < vtkSphereRepresentation::Outside ? vtkSphereRepresentation::Outside : 
            (state > vtkSphereRepresentation::Scaling ? vtkSphereRepresentation::Scaling : state) );
  
  // Depending on state, highlight appropriate parts of representation
  this->InteractionState = state;
}

//----------------------------------------------------------------------
double *vtkSphereRepresentation::GetBounds()
{
  this->BuildRepresentation();
  return this->SphereSource->GetOutput()->GetBounds();
}

//----------------------------------------------------------------------------
void vtkSphereRepresentation::BuildRepresentation()
{
  // Always rebuild, it's not worth keeping track of modified
  switch ( this->Representation )
    {
    case VTK_SPHERE_OFF:
      break;
    case VTK_SPHERE_WIREFRAME:
      this->SphereProperty->SetRepresentationToWireframe();
      this->SelectedSphereProperty->SetRepresentationToWireframe();
      break;
    case VTK_SPHERE_SURFACE:
      this->SphereProperty->SetRepresentationToSurface();
      this->SelectedSphereProperty->SetRepresentationToSurface();
      break;
    }
  this->SphereSource->Update();
  this->SizeHandles();

  // Now the annotations
  if ( this->RadialLine )
    {
    this->RadialLineSource->SetPoint1(this->SphereSource->GetCenter());
    this->RadialLineSource->SetPoint2(this->HandleSource->GetCenter());
    this->RadialLineSource->Update();
    }
  
  if ( this->HandleText && this->Renderer )
    {
    char str[256];
    double c[3], hc[3], tc[4];
    this->SphereSource->GetCenter(c);
    this->HandleSource->GetCenter(hc);
    double r = sqrt( static_cast<double>(vtkMath::Distance2BetweenPoints(c,hc)) );
    r = (r<=0.0 ? 1.0 : r);
    double theta = vtkMath::DegreesFromRadians( atan2( ( hc[1] - c[1] ), ( hc[0] - c[0] ) ) );
    double phi   = vtkMath::DegreesFromRadians( acos( ( hc[2] - c[2] ) / r ) );
    sprintf(str,"(%0.2g, %1.1f, %1.1f)", r, theta, phi);
    this->HandleTextMapper->SetInput(str);
    vtkInteractorObserver::ComputeWorldToDisplay(this->Renderer, hc[0], hc[1], hc[2], tc);
    this->HandleTextActor->GetPositionCoordinate()->SetValue(tc[0]+10,tc[1]+10);
    }
}

//----------------------------------------------------------------------------
void vtkSphereRepresentation::ReleaseGraphicsResources(vtkWindow *w)
{
  this->SphereActor->ReleaseGraphicsResources(w);
  this->HandleActor->ReleaseGraphicsResources(w);
  this->HandleTextActor->ReleaseGraphicsResources(w);
  this->RadialLineActor->ReleaseGraphicsResources(w);
}

//----------------------------------------------------------------------------
int vtkSphereRepresentation::RenderOpaqueGeometry(vtkViewport *v)
{
  int count=0;
  this->BuildRepresentation();
  
  if ( this->Representation != VTK_SPHERE_OFF )
    {
    count += this->SphereActor->RenderOpaqueGeometry(v);
    }
  if ( this->HandleVisibility )
    {
    count += this->HandleActor->RenderOpaqueGeometry(v);
    }
  if ( this->RadialLine )
    {
    count += this->RadialLineActor->RenderOpaqueGeometry(v);
    }

  return count;
}

//----------------------------------------------------------------------------
int vtkSphereRepresentation::RenderTranslucentPolygonalGeometry(vtkViewport *v)
{
  int count=0;
  
  if ( this->Representation != VTK_SPHERE_OFF )
    {
    count += this->SphereActor->RenderTranslucentPolygonalGeometry(v);
    }
  if ( this->HandleVisibility )
    {
    count += this->HandleActor->RenderTranslucentPolygonalGeometry(v);
    }
  if ( this->RadialLine )
    {
    count += this->RadialLineActor->RenderTranslucentPolygonalGeometry(v);
    }

  return count;
}

//----------------------------------------------------------------------------
int vtkSphereRepresentation::RenderOverlay(vtkViewport *v)
{
  int count=0;
  
  if ( this->HandleText )
    {
    count += this->HandleTextActor->RenderOverlay(v);
    }

  return count;
}

//----------------------------------------------------------------------------
int vtkSphereRepresentation::HasTranslucentPolygonalGeometry()
{
  int result=0;
  this->BuildRepresentation();

  if ( this->Representation != VTK_SPHERE_OFF )
    {
    result |= this->SphereActor->HasTranslucentPolygonalGeometry();
    }
  if ( this->HandleVisibility )
    {
    result |= this->HandleActor->HasTranslucentPolygonalGeometry();
    }
  if ( this->HandleText )
    {
    result |= this->HandleTextActor->HasTranslucentPolygonalGeometry();
    }
  if ( this->RadialLine )
    {
    result |= this->RadialLineActor->HasTranslucentPolygonalGeometry();
    }

  return result;
}


//----------------------------------------------------------------------------
void vtkSphereRepresentation::SizeHandles()
{
  double radius = this->vtkWidgetRepresentation::
    SizeHandlesInPixels(1.5,this->HandleSource->GetOutput()->GetCenter());
  this->HandleSource->SetRadius(radius);
}



//----------------------------------------------------------------------------
void vtkSphereRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Sphere Representation: ";
  if ( this->Representation == VTK_SPHERE_OFF )
    {
    os << "Off\n";
    }
  else if ( this->Representation == VTK_SPHERE_WIREFRAME )
    {
    os << "Wireframe\n";
    }
  else //if ( this->Representation == VTK_SPHERE_SURFACE )
    {
    os << "Surface\n";
    }

  if ( this->SphereProperty )
    {
    os << indent << "Sphere Property: " << this->SphereProperty << "\n";
    }
  else
    {
    os << indent << "Sphere Property: (none)\n";
    }
  if ( this->SelectedSphereProperty )
    {
    os << indent << "Selected Sphere Property: " 
       << this->SelectedSphereProperty << "\n";
    }
  else
    {
    os << indent << "Selected Sphere Property: (none)\n";
    }

  if ( this->HandleProperty )
    {
    os << indent << "Handle Property: " << this->HandleProperty << "\n";
    }
  else
    {
    os << indent << "Handle Property: (none)\n";
    }
  if ( this->SelectedHandleProperty )
    {
    os << indent << "Selected Handle Property: " 
       << this->SelectedHandleProperty << "\n";
    }
  else
    {
    os << indent << "Selected Handle Property: (none)\n";
    }

  os << indent << "Handle Visibility: "
     << (this->HandleVisibility ? "On\n" : "Off\n");
  os << indent << "Handle Direction: (" << this->HandleDirection[0] << ", "
     << this->HandleDirection[1] << ", " 
     << this->HandleDirection[2] << ")\n";
  os << indent << "Handle Position: (" << this->HandlePosition[0] << ", "
     << this->HandlePosition[1] << ", " 
     << this->HandlePosition[2] << ")\n";

  int thetaRes = this->SphereSource->GetThetaResolution();
  int phiRes = this->SphereSource->GetPhiResolution();
  double *center = this->SphereSource->GetCenter();
  double r = this->SphereSource->GetRadius();

  os << indent << "Theta Resolution: " << thetaRes << "\n";
  os << indent << "Phi Resolution: " << phiRes << "\n";
  os << indent << "Center: (" << center[0] << ", "
     << center[1] << ", " << center[2] << ")\n";
  os << indent << "Radius: " << r << "\n";

  os << indent << "Handle Text: " << this->HandleText << "\n";
  os << indent << "Radial Line: " << this->RadialLine << "\n";
  
  if ( this->HandleTextProperty )
    {
    os << indent << "Handle Text Property: " << this->HandleTextProperty << "\n";
    }
  else
    {
    os << indent << "Handle Text Property: (none)\n";
    }

  if ( this->RadialLineProperty )
    {
    os << indent << "Radial Line Property: " << this->RadialLineProperty << "\n";
    }
  else
    {
    os << indent << "Radial Line Property: (none)\n";
    }
}


