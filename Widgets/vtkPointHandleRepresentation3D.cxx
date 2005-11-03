/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointHandleRepresentation3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPointHandleRepresentation3D.h"
#include "vtkCursor3D.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkCellPicker.h"
#include "vtkRenderer.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkAssemblyPath.h"
#include "vtkMath.h"
#include "vtkInteractorObserver.h"
#include "vtkLine.h"
#include "vtkCoordinate.h"
#include "vtkRenderWindow.h"

vtkCxxRevisionMacro(vtkPointHandleRepresentation3D, "1.2");
vtkStandardNewMacro(vtkPointHandleRepresentation3D);

vtkCxxSetObjectMacro(vtkPointHandleRepresentation3D,Property,vtkProperty);
vtkCxxSetObjectMacro(vtkPointHandleRepresentation3D,SelectedProperty,vtkProperty);


//----------------------------------------------------------------------
vtkPointHandleRepresentation3D::vtkPointHandleRepresentation3D()
{
  // Initialize state
  this->InteractionState = vtkHandleRepresentation::Outside;
  
  // Represent the line
  this->Cursor3D = vtkCursor3D::New();
  this->Cursor3D->AllOff();
  this->Cursor3D->AxesOn();
  
  this->Mapper = vtkPolyDataMapper::New();
  this->Mapper->SetInput(this->Cursor3D->GetOutput());

  // Set up the initial properties
  this->CreateDefaultProperties();

  this->Actor = vtkActor::New();
  this->Actor->SetMapper(this->Mapper);
  this->Actor->SetProperty(this->Property);

  //Manage the picking stuff
  this->CursorPicker = vtkCellPicker::New();
  this->CursorPicker->PickFromListOn();
  this->CursorPicker->AddPickList(this->Actor);
  this->CursorPicker->SetTolerance(0.01); //need some fluff

  // Override superclass'
  this->PlaceFactor = 1.0;
  
  // The size of the hot spot
  this->HotSpotSize = 0.05;
  this->WaitingForMotion = 0;
  this->ConstraintAxis = -1;
}

//----------------------------------------------------------------------
vtkPointHandleRepresentation3D::~vtkPointHandleRepresentation3D()
{
  this->Cursor3D->Delete();
  this->CursorPicker->Delete();
  this->Mapper->Delete();
  this->Actor->Delete();
  this->Property->Delete();
  this->SelectedProperty->Delete();
}

//-------------------------------------------------------------------------
void vtkPointHandleRepresentation3D::PlaceWidget(double bds[6])
{
  int i;
  double bounds[6], center[3];

  this->AdjustBounds(bds, bounds, center);
  
  this->Cursor3D->SetModelBounds(bounds);
  this->SetWorldPosition(center);
  this->Cursor3D->Update();

  for (i=0; i<6; i++)
    {
    this->InitialBounds[i] = bounds[i];
    }
  this->InitialLength = sqrt((bounds[1]-bounds[0])*(bounds[1]-bounds[0]) +
                             (bounds[3]-bounds[2])*(bounds[3]-bounds[2]) +
                             (bounds[5]-bounds[4])*(bounds[5]-bounds[4]));
}

//-------------------------------------------------------------------------
void vtkPointHandleRepresentation3D::SetWorldPosition(double p[3])
{
  this->Cursor3D->SetFocalPoint(p); //this may clamp the point
  this->Superclass::SetWorldPosition(this->Cursor3D->GetFocalPoint());
}

//-------------------------------------------------------------------------
void vtkPointHandleRepresentation3D::SetDisplayPosition(double p[3])
{
  this->Superclass::SetDisplayPosition(p);
  this->SetWorldPosition(this->WorldPosition->GetValue());
}

//-------------------------------------------------------------------------
int vtkPointHandleRepresentation3D::ComputeInteractionState(int X, int Y, int vtkNotUsed(modify))
{
  this->VisibilityOn(); //actor must be on to be picked
  this->CursorPicker->Pick(X,Y,0.0,this->Renderer);
  vtkAssemblyPath *path = this->CursorPicker->GetPath();
  if ( path != NULL )
    {
    this->InteractionState = vtkHandleRepresentation::Nearby;
    }
  else
    {
    this->InteractionState = vtkHandleRepresentation::Outside;
    if ( this->ActiveRepresentation )
      {
      this->VisibilityOff();
      }
    }

  return this->InteractionState;
}

//-------------------------------------------------------------------------
int vtkPointHandleRepresentation3D::DetermineConstraintAxis(int constraint, double *x)
{
  // Look for trivial cases
  if ( ! this->Constrained )
    {
    return -1;
    }
  else if ( constraint >= 0 && constraint < 3 )
    {
    return constraint;
    }
  
  // Okay, figure out constraint. First see if the choice is
  // outside the hot spot
  if ( ! this->WaitingForMotion )
    {
    double p[3], d2, tol;
    this->CursorPicker->GetPickPosition(p);
    d2 = vtkMath::Distance2BetweenPoints(p,this->StartEventPosition);
    tol = this->HotSpotSize*this->InitialLength;
    if ( d2 > (tol*tol) )
      {
      this->WaitingForMotion = 0;
      return this->CursorPicker->GetCellId();
      }
    else
      {
      this->WaitingForMotion = 1;
      this->WaitCount = 0;
      return -1;
      }
    }
  else if ( this->WaitingForMotion && x ) 
    {
    double v[3];
    this->WaitingForMotion = 0;
    v[0] = fabs(x[0] - this->StartEventPosition[0]);
    v[1] = fabs(x[1] - this->StartEventPosition[1]);
    v[2] = fabs(x[2] - this->StartEventPosition[2]);
    return ( v[0]>v[1] ? (v[0]>v[2]?0:2) : (v[1]>v[2]?1:2));
    }
  else
    {
    return -1;
    }
}

//----------------------------------------------------------------------
// Record the current event position, and the rectilinear wipe position.
void vtkPointHandleRepresentation3D::StartWidgetInteraction(double startEventPos[2])
{
  this->StartEventPosition[0] = startEventPos[0];
  this->StartEventPosition[1] = startEventPos[1];
  this->StartEventPosition[2] = 0.0;

  this->LastEventPosition[0] = startEventPos[0];
  this->LastEventPosition[1] = startEventPos[1];

  vtkAssemblyPath *path;
  this->CursorPicker->Pick(startEventPos[0],startEventPos[1],0.0,this->Renderer);
  path = this->CursorPicker->GetPath();
  if ( path != NULL )
    {
    this->InteractionState = vtkHandleRepresentation::Nearby;
    this->ConstraintAxis = this->DetermineConstraintAxis(-1,NULL);
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
void vtkPointHandleRepresentation3D::WidgetInteraction(double eventPos[2])
{
  // Do different things depending on state
  // Calculations everybody does
  double focalPoint[4], pickPoint[4], prevPickPoint[4];
  double z;

  // Compute the two points defining the motion vector
  vtkInteractorObserver::ComputeWorldToDisplay(this->Renderer,
                                               this->LastPickPosition[0], 
                                               this->LastPickPosition[1],
                                               this->LastPickPosition[2], focalPoint);
  z = focalPoint[2];
  vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer, this->LastEventPosition[0], 
                                               this->LastEventPosition[1], z, prevPickPoint);
  vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer, eventPos[0], eventPos[1], z, pickPoint);

  // Process the motion
  if ( this->InteractionState == vtkHandleRepresentation::Selecting )
    {
    if ( !this->WaitingForMotion || this->WaitCount++ > 3 )
      {
      this->ConstraintAxis = 
        this->DetermineConstraintAxis(this->ConstraintAxis,pickPoint);
      this->MoveFocus(prevPickPoint, pickPoint);
      }
    }

  else if ( this->InteractionState == vtkHandleRepresentation::Scaling )
    {
    this->Scale(prevPickPoint, pickPoint, eventPos);
    }

  else if ( this->InteractionState == vtkHandleRepresentation::Translating )
    {
    if ( !this->WaitingForMotion || this->WaitCount++ > 3 )
      {
      this->ConstraintAxis = 
        this->DetermineConstraintAxis(this->ConstraintAxis,pickPoint);
      this->Translate(prevPickPoint, pickPoint);
      }
    }

  // Book keeping
  this->LastEventPosition[0] = eventPos[0];
  this->LastEventPosition[1] = eventPos[1];
  
  this->Modified();
}

//----------------------------------------------------------------------
void vtkPointHandleRepresentation3D::MoveFocus(double *p1, double *p2)
{
  //Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  double focus[3];
  this->Cursor3D->GetFocalPoint(focus);
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
void vtkPointHandleRepresentation3D::Translate(double *p1, double *p2)
{
  //Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];
  
  double *bounds = this->Cursor3D->GetModelBounds();
  double *pos = this->Cursor3D->GetFocalPoint();
  double newBounds[6], newFocus[3];
  int i;

  if ( this->ConstraintAxis >= 0 )
    {//move along axis
    for (i=0; i<3; i++)
      {
      if ( i != this->ConstraintAxis )
        {
        v[i] = 0.0;
        }
      }
    }
  
  for (i=0; i<3; i++)
    {
    newBounds[2*i] = bounds[2*i] + v[i];
    newBounds[2*i+1] = bounds[2*i+1] + v[i];
    newFocus[i] = pos[i] + v[i];
    }
  
  this->Cursor3D->SetModelBounds(newBounds);
  this->SetWorldPosition(newFocus);
}

//----------------------------------------------------------------------
void vtkPointHandleRepresentation3D::Scale(double *p1, double *p2, double eventPos[2])
{
  //Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  //int res = this->Cursor3D->GetResolution();
  double *bounds = this->Cursor3D->GetModelBounds();
  double *focus = this->Cursor3D->GetFocalPoint();

  // Compute the scale factor
  double sf = vtkMath::Norm(v) / 
    sqrt( (bounds[1]-bounds[0])*(bounds[1]-bounds[0]) +
          (bounds[3]-bounds[2])*(bounds[3]-bounds[2]) +
          (bounds[5]-bounds[4])*(bounds[5]-bounds[4]));

  if ( eventPos[1] > this->LastEventPosition[1] )
    {
    sf = 1.0 + sf;
    }
  else
    {
    sf = 1.0 - sf;
    }
  
  // Move the end points
  double newBounds[6];
  for (int i=0; i<3; i++)
    {
    newBounds[2*i] = sf * (bounds[2*i] - focus[i]) + focus[i];
    newBounds[2*i+1] = sf * (bounds[2*i+1] - focus[i]) + focus[i];
    }

  this->Cursor3D->SetModelBounds(newBounds);
  this->Cursor3D->Update();
}

//----------------------------------------------------------------------
void vtkPointHandleRepresentation3D::Highlight(int highlight)
{
  if ( highlight )
    {
    this->Actor->SetProperty(this->SelectedProperty);
    }
  else
    {
    this->Actor->SetProperty(this->Property);
    }
}

//----------------------------------------------------------------------
void vtkPointHandleRepresentation3D::CreateDefaultProperties()
{
  this->Property = vtkProperty::New();
  this->Property->SetAmbient(1.0);
  this->Property->SetAmbientColor(1.0,1.0,1.0);
  this->Property->SetLineWidth(0.5);

  this->SelectedProperty = vtkProperty::New();
  this->SelectedProperty->SetAmbient(1.0);
  this->SelectedProperty->SetAmbientColor(0.0,1.0,0.0);
  this->SelectedProperty->SetLineWidth(2.0);
}

//----------------------------------------------------------------------
void vtkPointHandleRepresentation3D::BuildRepresentation()
{
  // The net effect is to resize the handle
  if ( this->GetMTime() > this->BuildTime || 
       (this->Renderer && this->Renderer->GetVTKWindow() &&
        this->Renderer->GetVTKWindow()->GetMTime() > this->BuildTime) )
    {
    // The hardest part is to determine the bounding box for the handle
    double radius, bounds[6];
    double windowLowerLeft[4], windowUpperRight[4];
    double *viewport = this->Renderer->GetViewport();
    int *winSize = this->Renderer->GetRenderWindow()->GetSize();

    double x = winSize[0] * viewport[0];
    double y = winSize[1] * viewport[1];
    vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer, x,y,0.0, windowLowerLeft);

    x = winSize[0] * viewport[2];
    y = winSize[1] * viewport[3];
    vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer, x,y,0.0, windowUpperRight);

    int i;
    for (radius=0.0, i=0; i<3; i++) 
      {
      radius += (windowUpperRight[i] - windowLowerLeft[i]) *
        (windowUpperRight[i] - windowLowerLeft[i]);
      }

    double apt[3];
    this->GetWorldPosition(apt);
    radius = this->HandleSize*sqrt(radius);
    bounds[0] = apt[0] - radius;
    bounds[1] = apt[0] + radius;
    bounds[2] = apt[1] - radius;
    bounds[3] = apt[1] + radius;
    bounds[4] = apt[2] - radius;
    bounds[5] = apt[2] + radius;

    this->PlaceWidget(bounds);
    this->BuildTime.Modified();
    }
}

//----------------------------------------------------------------------
void vtkPointHandleRepresentation3D::ShallowCopy(vtkProp *prop)
{
  vtkPointHandleRepresentation3D *rep = 
    vtkPointHandleRepresentation3D::SafeDownCast(prop);
  if ( rep )
    {
    this->SetOutline(rep->GetOutline());
    this->SetXShadows(rep->GetXShadows());
    this->SetYShadows(rep->GetYShadows());
    this->SetZShadows(rep->GetZShadows());
    this->SetTranslationMode(rep->GetTranslationMode());
    this->SetProperty(rep->GetProperty());
    this->SetSelectedProperty(rep->GetSelectedProperty());
    this->SetHotSpotSize(rep->GetHotSpotSize());
    }
  this->Superclass::ShallowCopy(prop);
}

//----------------------------------------------------------------------
void vtkPointHandleRepresentation3D::GetActors(vtkPropCollection *pc)
{
  this->Actor->GetActors(pc);
}

//----------------------------------------------------------------------
void vtkPointHandleRepresentation3D::ReleaseGraphicsResources(vtkWindow *win)
{
  this->Actor->ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------
int vtkPointHandleRepresentation3D::RenderOpaqueGeometry(vtkViewport *viewport)
{
  this->BuildRepresentation();
  return this->Actor->RenderOpaqueGeometry(viewport);
}

//----------------------------------------------------------------------
int vtkPointHandleRepresentation3D::RenderTranslucentGeometry(vtkViewport *viewport)
{
  this->BuildRepresentation();
  return this->Actor->RenderTranslucentGeometry(viewport);
}


//----------------------------------------------------------------------
void vtkPointHandleRepresentation3D::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);
  
}
