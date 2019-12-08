/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSphereHandleRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSphereHandleRepresentation.h"
#include "vtkActor.h"
#include "vtkAssemblyPath.h"
#include "vtkCellPicker.h"
#include "vtkCoordinate.h"
#include "vtkInteractorObserver.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPickingManager.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"

vtkStandardNewMacro(vtkSphereHandleRepresentation);
vtkCxxSetObjectMacro(vtkSphereHandleRepresentation, SelectedProperty, vtkProperty);

//----------------------------------------------------------------------
vtkSphereHandleRepresentation::vtkSphereHandleRepresentation()
{
  // Initialize state
  this->InteractionState = vtkHandleRepresentation::Outside;

  // Represent the line
  this->Sphere = vtkSphereSource::New();
  this->Sphere->SetThetaResolution(16);
  this->Sphere->SetPhiResolution(8);

  this->Mapper = vtkPolyDataMapper::New();
  this->Mapper->SetInputConnection(this->Sphere->GetOutputPort());

  // Set up the initial properties
  this->CreateDefaultProperties();

  this->Actor = vtkActor::New();
  this->Actor->SetMapper(this->Mapper);
  this->Actor->SetProperty(this->Property);

  // Manage the picking stuff
  this->CursorPicker = vtkCellPicker::New();
  this->CursorPicker->PickFromListOn();
  this->CursorPicker->AddPickList(this->Actor);
  this->CursorPicker->SetTolerance(0.01); // need some fluff

  // Override superclass'
  this->PlaceFactor = 1.0;

  // The size of the hot spot
  this->HotSpotSize = 0.05;
  this->WaitingForMotion = 0;

  // Current handle size
  this->HandleSize = 15.0; // in pixels
  this->CurrentHandleSize = this->HandleSize;

  // Translation control
  this->TranslationMode = 1;
}

//----------------------------------------------------------------------
vtkSphereHandleRepresentation::~vtkSphereHandleRepresentation()
{
  this->Sphere->Delete();
  this->CursorPicker->Delete();
  this->Mapper->Delete();
  this->Actor->Delete();
  this->Property->Delete();
  this->SelectedProperty->Delete();
}

//----------------------------------------------------------------------
void vtkSphereHandleRepresentation::RegisterPickers()
{
  vtkPickingManager* pm = this->GetPickingManager();
  if (!pm)
  {
    return;
  }
  pm->AddPicker(this->CursorPicker, this);
}

//-------------------------------------------------------------------------
void vtkSphereHandleRepresentation::PlaceWidget(double bds[6])
{
  int i;
  double bounds[6], center[3];

  this->AdjustBounds(bds, bounds, center);

  //  this->Sphere->SetModelBounds(bounds);
  this->SetWorldPosition(center);

  for (i = 0; i < 6; i++)
  {
    this->InitialBounds[i] = bounds[i];
  }
  this->InitialLength = sqrt((bounds[1] - bounds[0]) * (bounds[1] - bounds[0]) +
    (bounds[3] - bounds[2]) * (bounds[3] - bounds[2]) +
    (bounds[5] - bounds[4]) * (bounds[5] - bounds[4]));
}

//-------------------------------------------------------------------------
void vtkSphereHandleRepresentation::SetSphereRadius(double radius)
{
  if (radius == this->Sphere->GetRadius())
  {
    return;
  }

  this->Sphere->SetRadius(radius);
  this->Modified();
}

//-------------------------------------------------------------------------
double vtkSphereHandleRepresentation::GetSphereRadius()
{
  return this->Sphere->GetRadius();
}

//-------------------------------------------------------------------------
double* vtkSphereHandleRepresentation::GetBounds()
{
  static double bounds[6];
  double center[3];
  double radius = this->Sphere->GetRadius();
  this->Sphere->GetCenter(center);

  bounds[0] = this->PlaceFactor * (center[0] - radius);
  bounds[1] = this->PlaceFactor * (center[0] + radius);
  bounds[2] = this->PlaceFactor * (center[1] - radius);
  bounds[3] = this->PlaceFactor * (center[1] + radius);
  bounds[4] = this->PlaceFactor * (center[2] - radius);
  bounds[5] = this->PlaceFactor * (center[2] + radius);

  return bounds;
}

//-------------------------------------------------------------------------
void vtkSphereHandleRepresentation::SetWorldPosition(double p[3])
{
  this->Sphere->SetCenter(p); // this may clamp the point
  this->Superclass::SetWorldPosition(this->Sphere->GetCenter());
}

//-------------------------------------------------------------------------
void vtkSphereHandleRepresentation::SetDisplayPosition(double p[3])
{
  this->Superclass::SetDisplayPosition(p);
  this->SetWorldPosition(this->WorldPosition->GetValue());
}

//-------------------------------------------------------------------------
void vtkSphereHandleRepresentation::SetHandleSize(double size)
{
  this->Superclass::SetHandleSize(size);
  this->CurrentHandleSize = this->HandleSize;
}

//-------------------------------------------------------------------------
int vtkSphereHandleRepresentation::ComputeInteractionState(int X, int Y, int vtkNotUsed(modify))
{
  this->VisibilityOn(); // actor must be on to be picked

  vtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->CursorPicker);

  if (path != nullptr)
  {
    //    this->InteractionState = vtkHandleRepresentation::Nearby;
    this->InteractionState = vtkHandleRepresentation::Selecting;
  }
  else
  {
    this->InteractionState = vtkHandleRepresentation::Outside;
    if (this->ActiveRepresentation)
    {
      this->VisibilityOff();
    }
  }

  return this->InteractionState;
}

//----------------------------------------------------------------------
// Record the current event position, and the rectilinear wipe position.
void vtkSphereHandleRepresentation::StartWidgetInteraction(double startEventPos[2])
{
  this->StartEventPosition[0] = startEventPos[0];
  this->StartEventPosition[1] = startEventPos[1];
  this->StartEventPosition[2] = 0.0;

  this->LastEventPosition[0] = startEventPos[0];
  this->LastEventPosition[1] = startEventPos[1];

  vtkAssemblyPath* path =
    this->GetAssemblyPath(startEventPos[0], startEventPos[1], 0., this->CursorPicker);

  if (path != nullptr)
  {
    //    this->InteractionState = vtkHandleRepresentation::Nearby;
    this->InteractionState = vtkHandleRepresentation::Selecting;
    this->CursorPicker->GetPickPosition(this->LastPickPosition);
  }
  else
  {
    this->InteractionState = vtkHandleRepresentation::Outside;
    this->ConstraintAxis = -1;
  }
}

//----------------------------------------------------------------------
// Based on the displacement vector (computed in display coordinates) and
// the cursor state (which corresponds to which part of the widget has been
// selected), the widget points are modified.
// First construct a local coordinate system based on the display coordinates
// of the widget.
void vtkSphereHandleRepresentation::WidgetInteraction(double eventPos[2])
{
  // Do different things depending on state
  // Calculations everybody does
  double focalPoint[4], pickPoint[4], prevPickPoint[4];
  double z;

  // Compute the two points defining the motion vector
  vtkInteractorObserver::ComputeWorldToDisplay(this->Renderer, this->LastPickPosition[0],
    this->LastPickPosition[1], this->LastPickPosition[2], focalPoint);
  z = focalPoint[2];
  vtkInteractorObserver::ComputeDisplayToWorld(
    this->Renderer, this->LastEventPosition[0], this->LastEventPosition[1], z, prevPickPoint);
  vtkInteractorObserver::ComputeDisplayToWorld(
    this->Renderer, eventPos[0], eventPos[1], z, pickPoint);

  // Process the motion
  if (this->InteractionState == vtkHandleRepresentation::Selecting ||
    this->InteractionState == vtkHandleRepresentation::Translating)
  {
    if (!this->WaitingForMotion || this->WaitCount++ > 3)
    {
      if (this->InteractionState == vtkHandleRepresentation::Selecting && !this->TranslationMode)
      {
        this->MoveFocus(prevPickPoint, pickPoint);
      }
      else
      {
        this->Translate(prevPickPoint, pickPoint);
      }
    }
  }

  else if (this->InteractionState == vtkHandleRepresentation::Scaling)
  {
    this->Scale(prevPickPoint, pickPoint, eventPos);
  }

  // Book keeping
  this->LastEventPosition[0] = eventPos[0];
  this->LastEventPosition[1] = eventPos[1];

  this->Modified();
}
/*
//----------------------------------------------------------------------
void vtkSphereHandleRepresentation::MoveFocus(const double *p1, const double *p2)
{
  //Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  double focus[3];
  this->Sphere->GetCenter(focus);
  if ( this->ConstraintAxis >= 0 )
  {
    focus[this->ConstraintAxis] += v[this->ConstraintAxis];
  }
  else
  {
    focus[0] += v[0];
    focus[1] += v[1];
    focus[2] += v[2];
  }

  this->SetWorldPosition(focus);
}

//----------------------------------------------------------------------
// Translate everything
void vtkSphereHandleRepresentation::Translate(const double *p1, const double *p2)
{
  //Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  double *pos = this->Sphere->GetCenter();
  double newFocus[3];

  if ( this->ConstraintAxis >= 0 )
  {//move along axis
    for (int i=0; i<3; i++)
    {
      if ( i != this->ConstraintAxis )
      {
        v[i] = 0.0;
      }
    }
  }

  for (int i=0; i<3; i++)
  {
    newFocus[i] = pos[i] + v[i];
  }
  this->SetWorldPosition(newFocus);

  double radius = this->SizeHandlesInPixels(1.0,newFocus);
  radius *= this->CurrentHandleSize / this->HandleSize;

  this->Sphere->SetRadius(radius);
}*/

//----------------------------------------------------------------------
void vtkSphereHandleRepresentation::MoveFocus(const double* p1, const double* p2)
{
  Superclass::Translate(p1, p2);
}

//----------------------------------------------------------------------
// Translate everything
void vtkSphereHandleRepresentation::Translate(const double* p1, const double* p2)
{
  double v[3];
  this->GetTranslationVector(p1, p2, v);
  double* pos = this->Sphere->GetCenter();
  double focus[3];
  for (int i = 0; i < 3; i++)
  {
    focus[i] = pos[i] + v[i];
  }
  this->SetWorldPosition(focus);

  double radius = this->SizeHandlesInPixels(1.0, focus);
  radius *= this->CurrentHandleSize / this->HandleSize;

  this->Sphere->SetRadius(radius);
}

//----------------------------------------------------------------------
void vtkSphereHandleRepresentation::SizeBounds()
{
  double center[3];
  this->Sphere->GetCenter(center);
  double radius = this->SizeHandlesInPixels(1.0, center);
  radius *= this->CurrentHandleSize / this->HandleSize;

  this->Sphere->SetRadius(radius);
}

//----------------------------------------------------------------------
void vtkSphereHandleRepresentation::Scale(
  const double* p1, const double* p2, const double eventPos[2])
{
  // Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  const double* bounds = this->GetBounds();

  // Compute the scale factor
  double sf = vtkMath::Norm(v) /
    sqrt((bounds[1] - bounds[0]) * (bounds[1] - bounds[0]) +
      (bounds[3] - bounds[2]) * (bounds[3] - bounds[2]) +
      (bounds[5] - bounds[4]) * (bounds[5] - bounds[4]));

  if (eventPos[1] > this->LastEventPosition[1])
  {
    sf = 1.0 + sf;
  }
  else
  {
    sf = 1.0 - sf;
  }

  this->CurrentHandleSize *= sf;
  this->CurrentHandleSize = (this->CurrentHandleSize < 0.001 ? 0.001 : this->CurrentHandleSize);

  this->SizeBounds();
}

//----------------------------------------------------------------------
void vtkSphereHandleRepresentation::Highlight(int highlight)
{
  if (highlight)
  {
    this->Actor->SetProperty(this->SelectedProperty);
  }
  else
  {
    this->Actor->SetProperty(this->Property);
  }
}

//----------------------------------------------------------------------
void vtkSphereHandleRepresentation::CreateDefaultProperties()
{
  this->Property = vtkProperty::New();
  this->Property->SetColor(1, 1, 1);

  this->SelectedProperty = vtkProperty::New();
  this->SelectedProperty->SetColor(0, 1, 0);
}

//----------------------------------------------------------------------
void vtkSphereHandleRepresentation::BuildRepresentation()
{
  // The net effect is to resize the handle
  if (this->GetMTime() > this->BuildTime ||
    (this->Renderer && this->Renderer->GetVTKWindow() &&
      this->Renderer->GetVTKWindow()->GetMTime() > this->BuildTime))
  {
    if (!this->Placed)
    {
      this->ValidPick = 1;
      this->Placed = 1;
    }

    this->SizeBounds();
    this->Sphere->Update();
    this->BuildTime.Modified();
  }
}

//----------------------------------------------------------------------
void vtkSphereHandleRepresentation::ShallowCopy(vtkProp* prop)
{
  vtkSphereHandleRepresentation* rep = vtkSphereHandleRepresentation::SafeDownCast(prop);
  if (rep)
  {
    this->SetTranslationMode(rep->GetTranslationMode());
    this->SetProperty(rep->GetProperty());
    this->SetSelectedProperty(rep->GetSelectedProperty());
    this->SetHotSpotSize(rep->GetHotSpotSize());
  }
  this->Superclass::ShallowCopy(prop);
}

//----------------------------------------------------------------------
void vtkSphereHandleRepresentation::DeepCopy(vtkProp* prop)
{
  vtkSphereHandleRepresentation* rep = vtkSphereHandleRepresentation::SafeDownCast(prop);
  if (rep)
  {
    this->SetTranslationMode(rep->GetTranslationMode());
    this->Property->DeepCopy(rep->GetProperty());
    this->SelectedProperty->DeepCopy(rep->GetSelectedProperty());
    this->SetHotSpotSize(rep->GetHotSpotSize());
  }
  this->Superclass::DeepCopy(prop);
}

//----------------------------------------------------------------------
void vtkSphereHandleRepresentation::GetActors(vtkPropCollection* pc)
{
  this->Actor->GetActors(pc);
}

//----------------------------------------------------------------------
void vtkSphereHandleRepresentation::ReleaseGraphicsResources(vtkWindow* win)
{
  this->Actor->ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------
int vtkSphereHandleRepresentation::RenderOpaqueGeometry(vtkViewport* viewport)
{
  this->BuildRepresentation();
  return this->Actor->RenderOpaqueGeometry(viewport);
}

//----------------------------------------------------------------------
int vtkSphereHandleRepresentation ::RenderTranslucentPolygonalGeometry(vtkViewport* viewport)
{
  this->BuildRepresentation();
  return this->Actor->RenderTranslucentPolygonalGeometry(viewport);
}

//-----------------------------------------------------------------------------
vtkTypeBool vtkSphereHandleRepresentation::HasTranslucentPolygonalGeometry()
{
  return 0; // this->Actor->HasTranslucentPolygonalGeometry();
}

//----------------------------------------------------------------------
void vtkSphereHandleRepresentation::SetProperty(vtkProperty* p)
{
  vtkSetObjectBodyMacro(Property, vtkProperty, p);
  if (p)
  {
    this->Actor->SetProperty(p);
  }
}

//----------------------------------------------------------------------
void vtkSphereHandleRepresentation::SetVisibility(vtkTypeBool visible)
{
  this->Actor->SetVisibility(visible);
  // Forward to superclass
  this->Superclass::SetVisibility(visible);
}

//----------------------------------------------------------------------
void vtkSphereHandleRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  // Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Hot Spot Size: " << this->HotSpotSize << "\n";
  if (this->Property)
  {
    os << indent << "Property: " << this->Property << "\n";
  }
  else
  {
    os << indent << "Property: (none)\n";
  }
  if (this->SelectedProperty)
  {
    os << indent << "Selected Property: " << this->SelectedProperty << "\n";
  }
  else
  {
    os << indent << "Selected Property: (none)\n";
  }

  os << indent << "Translation Mode: " << (this->TranslationMode ? "On\n" : "Off\n");
  os << indent << "Sphere: " << this->Sphere << "\n";

  this->Sphere->PrintSelf(os, indent.GetNextIndent());
}
