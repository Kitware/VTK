/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointWidget.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPointWidget.h"

#include "vtkAssemblyNode.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCellPicker.h"
#include "vtkMath.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderWindow.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkPointWidget, "1.1");
vtkStandardNewMacro(vtkPointWidget);

vtkPointWidget::vtkPointWidget()
{
  this->State = vtkPointWidget::Start;
  this->EventCallbackCommand->SetCallback(vtkPointWidget::ProcessEvents);
  
  // Represent the line
  this->Cursor3D = vtkCursor3D::New();
  this->Mapper = vtkPolyDataMapper::New();
  this->Mapper->SetInput(this->Cursor3D->GetOutput());
  this->Actor = vtkActor::New();
  this->Actor->SetMapper(this->Mapper);

  // Define the point coordinates
  float bounds[6];
  bounds[0] = -0.5;
  bounds[1] = 0.5;
  bounds[2] = -0.5;
  bounds[3] = 0.5;
  bounds[4] = -0.5;
  bounds[5] = 0.5;

  // Initial creation of the widget, serves to initialize it
  this->PlaceWidget(bounds);

  //Manage the picking stuff
  this->CursorPicker = vtkCellPicker::New();
  this->CursorPicker->AddPickList(this->Actor);
  this->CursorPicker->SetTolerance(0.005); //need some fluff

  // Set up the initial properties
  this->Property = NULL;
  this->SelectedProperty = NULL;
  this->CreateDefaultProperties();

  // Constraints not set
  this->ConstraintAxis = -1;

  // Override superclass'
  this->PlaceFactor = 1.0;
}

vtkPointWidget::~vtkPointWidget()
{
  if ( this->Enabled )
    {
    this->SetEnabled(0);
    }
  this->Actor->Delete();
  this->Mapper->Delete();
  this->Cursor3D->Delete();

  this->CursorPicker->Delete();

  if ( this->Property )
    {
    this->Property->Delete();
    }
  if ( this->SelectedProperty )
    {
    this->SelectedProperty->Delete();
    }
}

void vtkPointWidget::SetEnabled(int enabling)
{
  if ( ! this->Interactor )
    {
    vtkErrorMacro(<<"The interactor must be set prior to enabling/disabling widget");
    return;
    }

  if ( enabling ) //------------------------------------------------------------
    {
    vtkDebugMacro(<<"Enabling point widget");

    if ( this->Enabled ) //already enabled, just return
      {
      return;
      }
    
    this->CurrentRenderer = this->Interactor->FindPokedRenderer(
      this->Interactor->GetLastEventPosition()[0],
      this->Interactor->GetLastEventPosition()[1]);
    if (this->CurrentRenderer == NULL)
      {
      return;
      }

    this->Enabled = 1;

    // listen for the following events
    vtkRenderWindowInteractor *i = this->Interactor;
    i->AddObserver(vtkCommand::MouseMoveEvent, this->EventCallbackCommand, 
                   this->Priority);
    i->AddObserver(vtkCommand::LeftButtonPressEvent, 
                   this->EventCallbackCommand, this->Priority);
    i->AddObserver(vtkCommand::LeftButtonReleaseEvent, 
                   this->EventCallbackCommand, this->Priority);
    i->AddObserver(vtkCommand::MiddleButtonPressEvent, 
                   this->EventCallbackCommand, this->Priority);
    i->AddObserver(vtkCommand::MiddleButtonReleaseEvent, 
                   this->EventCallbackCommand, this->Priority);
    i->AddObserver(vtkCommand::RightButtonPressEvent, 
                   this->EventCallbackCommand, this->Priority);
    i->AddObserver(vtkCommand::RightButtonReleaseEvent, 
                   this->EventCallbackCommand, this->Priority);

    // Add the line
    this->CurrentRenderer->AddActor(this->Actor);
    this->Actor->SetProperty(this->Property);

    this->InvokeEvent(vtkCommand::EnableEvent,NULL);
    }
  
  else //disabling----------------------------------------------------------
    {
    vtkDebugMacro(<<"Disabling point widget");

    if ( ! this->Enabled ) //already disabled, just return
      {
      return;
      }
    
    this->Enabled = 0;

    // don't listen for events any more
    this->Interactor->RemoveObserver(this->EventCallbackCommand);

    // turn off the line
    this->CurrentRenderer->RemoveActor(this->Actor);

    this->InvokeEvent(vtkCommand::DisableEvent,NULL);
    }

  this->Interactor->Render();
}

void vtkPointWidget::ProcessEvents(vtkObject* vtkNotUsed(object), 
                                   unsigned long event,
                                   void* clientdata, 
                                   void* vtkNotUsed(calldata))
{
  vtkPointWidget* self = reinterpret_cast<vtkPointWidget *>( clientdata );

  //okay, let's do the right thing
  switch(event)
    {
    case vtkCommand::LeftButtonPressEvent:
      self->OnLeftButtonDown();
      break;
    case vtkCommand::LeftButtonReleaseEvent:
      self->OnLeftButtonUp();
      break;
    case vtkCommand::MiddleButtonPressEvent:
      self->OnMiddleButtonDown();
      break;
    case vtkCommand::MiddleButtonReleaseEvent:
      self->OnMiddleButtonUp();
      break;
    case vtkCommand::RightButtonPressEvent:
      self->OnRightButtonDown();
      break;
    case vtkCommand::RightButtonReleaseEvent:
      self->OnRightButtonUp();
      break;
    case vtkCommand::MouseMoveEvent:
      self->OnMouseMove();
      break;
    }
}

void vtkPointWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if ( this->Property )
    {
    os << indent << "Property: " << this->Property << "\n";
    }
  else
    {
    os << indent << "Property: (none)\n";
    }
  if ( this->SelectedProperty )
    {
    os << indent << "Selected Property: " << this->SelectedProperty << "\n";
    }
  else
    {
    os << indent << "Selected Property: (none)\n";
    }

  float *pos = this->Cursor3D->GetFocalPoint();
  os << indent << "Position: (" << pos[0] << ", "
               << pos[1] << ", " << pos[2] << ")\n";

  os << indent << "Outline: " << (this->GetOutline() ? "On\n" : "Off\n");
  os << indent << "XShadows: " << (this->GetXShadows() ? "On\n" : "Off\n");
  os << indent << "YShadows: " << (this->GetYShadows() ? "On\n" : "Off\n");
  os << indent << "ZShadows: " << (this->GetZShadows() ? "On\n" : "Off\n");
}

void vtkPointWidget::Highlight(int highlight)
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

void vtkPointWidget::OnMouseMove()
{
  // See whether we're active
  if ( this->State == vtkPointWidget::Outside || 
       this->State == vtkPointWidget::Start )
    {
    return;
    }
  
  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // Do different things depending on state
  // Calculations everybody does
  double focalPoint[4], pickPoint[4], prevPickPoint[4];
  double z;

  vtkRenderer *renderer = this->Interactor->FindPokedRenderer(X,Y);
  vtkCamera *camera = renderer->GetActiveCamera();
  if ( !camera )
    {
    return;
    }

  // Compute the two points defining the motion vector
  camera->GetFocalPoint(focalPoint);
  this->ComputeWorldToDisplay(focalPoint[0], focalPoint[1],
                              focalPoint[2], focalPoint);
  z = focalPoint[2];
  this->ComputeDisplayToWorld(double(this->Interactor->GetLastEventPosition()[0]),double(this->Interactor->GetLastEventPosition()[1]),
                              z, prevPickPoint);
  this->ComputeDisplayToWorld(double(X), double(Y), z, pickPoint);

  // Process the motion
  if ( this->State == vtkPointWidget::Moving )
    {
    this->MoveFocus(prevPickPoint, pickPoint);
    }
  else if ( this->State == vtkPointWidget::Scaling )
    {
    this->Scale(prevPickPoint, pickPoint, X, Y);
    }
  else if ( this->State == vtkPointWidget::Translating )
    {
    this->Translate(prevPickPoint, pickPoint);
    }

  // Interact, if desired
  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::InteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkPointWidget::OnLeftButtonDown()
{
  // We're only here is we are enabled
  this->State = vtkPointWidget::Moving;

  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // Okay, we can process this. Pick the cursor.
  vtkAssemblyPath *path;
  this->Interactor->FindPokedRenderer(X,Y);
  this->CursorPicker->Pick(X,Y,0.0,this->CurrentRenderer);
  path = this->CursorPicker->GetPath();
  if ( path != NULL )
    {
    int idx = this->CursorPicker->GetCellId();
    if ( idx >= 0 && idx < 3 )
      {
      this->ConstraintAxis = idx;
      }
    this->Highlight(1);
    }
  else
    {
    this->ConstraintAxis = -1;
    this->Highlight(0);
    this->State = vtkPointWidget::Outside;
    return;
    }
  
  this->EventCallbackCommand->SetAbortFlag(1);
  this->StartInteraction();
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkPointWidget::OnLeftButtonUp()
{
  if ( this->State == vtkPointWidget::Outside )
    {
    return;
    }

  this->State = vtkPointWidget::Start;
  this->Highlight(0);

  this->EventCallbackCommand->SetAbortFlag(1);
  this->EndInteraction();
  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkPointWidget::OnMiddleButtonDown()
{
  this->State = vtkPointWidget::Translating;

  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // Okay, we can process this. Pick 
  // if no handles picked, then pick the bounding box.
  vtkAssemblyPath *path;
  this->Interactor->FindPokedRenderer(X,Y);
  this->CursorPicker->Pick(X,Y,0.0,this->CurrentRenderer);
  path = this->CursorPicker->GetPath();
  if ( path != NULL )
    {
    int idx = this->CursorPicker->GetCellId();
    if ( idx >= 0 && idx < 3 )
      {
      this->ConstraintAxis = idx;
      }
    this->Highlight(1);
    }
  else
    {
    this->ConstraintAxis = -1;
    this->Highlight(0);
    this->State = vtkPointWidget::Outside;
    return;
    }
  
  this->EventCallbackCommand->SetAbortFlag(1);
  this->StartInteraction();
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkPointWidget::OnMiddleButtonUp()
{
  if ( this->State == vtkPointWidget::Outside )
    {
    return;
    }

  this->State = vtkPointWidget::Start;
  this->Highlight(0);
  
  this->EventCallbackCommand->SetAbortFlag(1);
  this->EndInteraction();
  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkPointWidget::OnRightButtonDown()
{
  this->State = vtkPointWidget::Scaling;

  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // Okay, we can process this. Pick the cursor.
  vtkAssemblyPath *path;
  this->Interactor->FindPokedRenderer(X,Y);
  this->CursorPicker->Pick(X,Y,0.0,this->CurrentRenderer);
  path = this->CursorPicker->GetPath();
  if ( path != NULL )
    {
    int idx = this->CursorPicker->GetCellId();
    if ( idx >= 0 && idx < 3 )
      {
      this->ConstraintAxis = idx;
      }
    this->Highlight(1);
    }
  else
    {
    this->ConstraintAxis = -1;
    this->Highlight(0);
    this->State = vtkPointWidget::Outside;
    return;
    }
  
  this->EventCallbackCommand->SetAbortFlag(1);
  this->StartInteraction();
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkPointWidget::OnRightButtonUp()
{
  if ( this->State == vtkPointWidget::Outside )
    {
    return;
    }

  this->State = vtkPointWidget::Start;
  this->Highlight(0);
  
  this->EventCallbackCommand->SetAbortFlag(1);
  this->EndInteraction();
  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkPointWidget::MoveFocus(double *p1, double *p2)
{
  //Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  float focus[3];
  this->Cursor3D->GetFocalPoint(focus);
  if ( this->Interactor->GetShiftKey() && this->ConstraintAxis >= 0 )
    {
    focus[this->ConstraintAxis] += v[this->ConstraintAxis];
    }
  else
    {
    focus[0] += v[0];
    focus[1] += v[1];
    focus[2] += v[2];
    }
  
  this->Cursor3D->SetFocalPoint(focus);
}

// Loop through all points and translate them
void vtkPointWidget::Translate(double *p1, double *p2)
{
  //Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];
  
  float *bounds = this->Cursor3D->GetModelBounds();
  float *pos = this->Cursor3D->GetFocalPoint();
  float newBounds[6], newFocus[3];
  int i;

  if ( this->Interactor->GetShiftKey() && this->ConstraintAxis >= 0 )
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
  this->Cursor3D->SetFocalPoint(newFocus);
  this->Cursor3D->Update();
}

void vtkPointWidget::Scale(double *p1, double *p2, int vtkNotUsed(X), int Y)
{
  //Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  //int res = this->Cursor3D->GetResolution();
  float *bounds = this->Cursor3D->GetModelBounds();
  float *focus = this->Cursor3D->GetFocalPoint();

  // Compute the scale factor
  float sf = vtkMath::Norm(v) / 
    sqrt( (bounds[1]-bounds[0])*(bounds[1]-bounds[0]) +
          (bounds[3]-bounds[2])*(bounds[3]-bounds[2]) +
          (bounds[5]-bounds[4])*(bounds[5]-bounds[4]));

  if ( Y > this->Interactor->GetLastEventPosition()[1] )
    {
    sf = 1.0 + sf;
    }
  else
    {
    sf = 1.0 - sf;
    }
  
  // Move the end points
  float newBounds[6];
  for (int i=0; i<3; i++)
    {
    newBounds[2*i] = sf * (bounds[2*i] - focus[i]) + focus[i];
    newBounds[2*i+1] = sf * (bounds[2*i+1] - focus[i]) + focus[i];
    }

  this->Cursor3D->SetModelBounds(newBounds);
  this->Cursor3D->Update();
}

void vtkPointWidget::CreateDefaultProperties()
{
  if ( ! this->Property )
    {
    this->Property = vtkProperty::New();
    this->Property->SetAmbient(1.0);
    this->Property->SetAmbientColor(1.0,1.0,1.0);
    this->Property->SetLineWidth(0.5);
    }
  if ( ! this->SelectedProperty )
    {
    this->SelectedProperty = vtkProperty::New();
    this->SelectedProperty->SetAmbient(1.0);
    this->SelectedProperty->SetAmbientColor(0.0,1.0,0.0);
    this->SelectedProperty->SetLineWidth(2.0);
    }
}

void vtkPointWidget::PlaceWidget(float bds[6])
{
  int i;
  float bounds[6], center[3];

  this->AdjustBounds(bds, bounds, center);
  
  this->Cursor3D->SetModelBounds(bounds);
  this->Cursor3D->SetFocalPoint(center);
  this->Cursor3D->Update();

  for (i=0; i<6; i++)
    {
    this->InitialBounds[i] = bounds[i];
    }
  this->InitialLength = sqrt((bounds[1]-bounds[0])*(bounds[1]-bounds[0]) +
                             (bounds[3]-bounds[2])*(bounds[3]-bounds[2]) +
                             (bounds[5]-bounds[4])*(bounds[5]-bounds[4]));

  this->InvokeEvent(vtkCommand::InteractionEvent,NULL);  
}

