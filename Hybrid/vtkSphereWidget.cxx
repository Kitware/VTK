/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSphereWidget.cxx
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
#include "vtkSphereWidget.h"
#include "vtkMath.h"
#include "vtkPolyDataMapper.h"
#include "vtkSphereSource.h"
#include "vtkCellPicker.h"
#include "vtkAssemblyNode.h"
#include "vtkDoubleArray.h"
#include "vtkSphere.h"
#include "vtkCallbackCommand.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkSphereWidget, "1.2");
vtkStandardNewMacro(vtkSphereWidget);

vtkSphereWidget::vtkSphereWidget()
{
  this->State = vtkSphereWidget::Start;
  this->EventCallbackCommand->SetCallback(vtkSphereWidget::ProcessEvents);
  
  this->Representation = VTK_SPHERE_WIREFRAME;

  //Build the representation of the widget
  // Represent the sphere
  this->SphereSource = vtkSphereSource::New();
  this->SphereSource->SetThetaResolution(16);
  this->SphereSource->SetPhiResolution(8);
  this->SphereSource->LatLongTessellationOn();
  this->SphereMapper = vtkPolyDataMapper::New();
  this->SphereMapper->SetInput(this->SphereSource->GetOutput());
  this->SphereActor = vtkActor::New();
  this->SphereActor->SetMapper(this->SphereMapper);

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
  this->SpherePicker = vtkCellPicker::New();
  this->SpherePicker->SetTolerance(0.005); //need some fluff
  this->SpherePicker->AddPickList(this->SphereActor);
  this->SpherePicker->PickFromListOn();
  
  // Set up the initial properties
  this->SphereProperty = NULL;
  this->SelectedSphereProperty = NULL;
  this->CreateDefaultProperties();
}

vtkSphereWidget::~vtkSphereWidget()
{
  this->SphereActor->Delete();
  this->SphereMapper->Delete();
  this->SphereSource->Delete();

  this->SpherePicker->Delete();

  if ( this->SphereProperty )
    {
    this->SphereProperty->Delete();
    }
  if ( this->SelectedSphereProperty )
    {
    this->SelectedSphereProperty->Delete();
    }
}

void vtkSphereWidget::SetEnabled(int enabling)
{
  if ( ! this->Interactor )
    {
    vtkErrorMacro(<<"The interactor must be set prior to enabling/disabling widget");
    return;
    }

  if ( enabling ) //----------------------------------------------------------
    {
    vtkDebugMacro(<<"Enabling sphere widget");

    if ( this->Enabled ) //already enabled, just return
      {
      return;
      }
    
    this->CurrentRenderer = this->Interactor->FindPokedRenderer(this->OldX,this->OldY);
    if (this->CurrentRenderer == NULL)
      {
      return;
      }

    this->Enabled = 1;

    // listen for the following events
    vtkRenderWindowInteractor *i = this->Interactor;
    i->AddObserver(vtkCommand::MouseMoveEvent, 
                   this->EventCallbackCommand, this->Priority);
    i->AddObserver(vtkCommand::LeftButtonPressEvent, 
                   this->EventCallbackCommand, this->Priority);
    i->AddObserver(vtkCommand::LeftButtonReleaseEvent, 
                   this->EventCallbackCommand, this->Priority);
    i->AddObserver(vtkCommand::RightButtonPressEvent, 
                   this->EventCallbackCommand, this->Priority);
    i->AddObserver(vtkCommand::RightButtonReleaseEvent, 
                   this->EventCallbackCommand, this->Priority);

    // Add the sphere
    this->CurrentRenderer->AddActor(this->SphereActor);
    this->SphereActor->SetProperty(this->SphereProperty);
    this->SelectRepresentation();

    this->InvokeEvent(vtkCommand::EnableEvent,NULL);
    }
  
  else //disabling----------------------------------------------------------
    {
    vtkDebugMacro(<<"Disabling sphere widget");

    if ( ! this->Enabled ) //already disabled, just return
      {
      return;
      }
    
    this->Enabled = 0;

    // don't listen for events any more
    this->Interactor->RemoveObserver(this->EventCallbackCommand);

    // turn off the sphere
    this->CurrentRenderer->RemoveActor(this->SphereActor);

    this->InvokeEvent(vtkCommand::DisableEvent,NULL);
    }

  this->Interactor->Render();
}

void vtkSphereWidget::ProcessEvents(vtkObject* object, unsigned long event,
                                  void* clientdata, void* vtkNotUsed(calldata))
{
  vtkSphereWidget* self = reinterpret_cast<vtkSphereWidget *>( clientdata );
  vtkRenderWindowInteractor* rwi = static_cast<vtkRenderWindowInteractor *>( object );
  int* XY = rwi->GetEventPosition();

  //okay, let's do the right thing
  switch(event)
    {
    case vtkCommand::LeftButtonPressEvent:
      self->OnLeftButtonDown(rwi->GetControlKey(), rwi->GetShiftKey(), XY[0], XY[1]);
      break;
    case vtkCommand::LeftButtonReleaseEvent:
      self->OnLeftButtonUp(rwi->GetControlKey(), rwi->GetShiftKey(), XY[0], XY[1]);
      break;
    case vtkCommand::RightButtonPressEvent:
      self->OnRightButtonDown(rwi->GetControlKey(), rwi->GetShiftKey(), XY[0], XY[1]);
      break;
    case vtkCommand::RightButtonReleaseEvent:
      self->OnRightButtonUp(rwi->GetControlKey(), rwi->GetShiftKey(), XY[0], XY[1]);
      break;
    case vtkCommand::MouseMoveEvent:
      self->OnMouseMove(rwi->GetControlKey(), rwi->GetShiftKey(), XY[0], XY[1]);
      break;
    }
}

void vtkSphereWidget::PrintSelf(ostream& os, vtkIndent indent)
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

  int thetaRes = this->SphereSource->GetThetaResolution();
  int phiRes = this->SphereSource->GetPhiResolution();
  float *center = this->SphereSource->GetCenter();
  float r = this->SphereSource->GetRadius();

  os << indent << "Theta Resolution: " << thetaRes << "\n";
  os << indent << "Phi Resolution: " << phiRes << "\n";
  os << indent << "Center: (" << center[0] << ", "
                               << center[1] << ", "
                               << center[2] << ")\n";
  os << indent << "Radius: " << r << "\n";
}

void vtkSphereWidget::SelectRepresentation()
{
  if ( this->Representation == VTK_SPHERE_OFF )
    {
    this->CurrentRenderer->RemoveActor(this->SphereActor);
    }
  else if ( this->Representation == VTK_SPHERE_WIREFRAME )
    {
    this->CurrentRenderer->RemoveActor(this->SphereActor);
    this->CurrentRenderer->AddActor(this->SphereActor);
    this->SphereProperty->SetRepresentationToWireframe();
    this->SelectedSphereProperty->SetRepresentationToWireframe();
    }
  else //if ( this->Representation == VTK_SPHERE_SURFACE )
    {
    this->CurrentRenderer->RemoveActor(this->SphereActor);
    this->CurrentRenderer->AddActor(this->SphereActor);
    this->SphereProperty->SetRepresentationToSurface();
    this->SelectedSphereProperty->SetRepresentationToSurface();
    }
}

void vtkSphereWidget::GetSphere(vtkSphere *sphere)
{
  sphere->SetRadius(this->SphereSource->GetRadius());
  sphere->SetCenter(this->SphereSource->GetCenter());
}

void vtkSphereWidget::HighlightSphere(int highlight)
{
  if ( highlight )
    {
    this->SphereActor->SetProperty(this->SelectedSphereProperty);
    }
  else
    {
    this->SphereActor->SetProperty(this->SphereProperty);
    }
}

void vtkSphereWidget::OnLeftButtonDown (int vtkNotUsed(ctrl), 
                                        int vtkNotUsed(shift), 
                                        int X, int Y)
{
  // We're only here is we are enabled
  this->State = vtkSphereWidget::Moving;

  // Okay, we can process this. Try to pick handles first;
  // if no handles picked, then try to pick the sphere.
  vtkAssemblyPath *path;
  this->Interactor->FindPokedRenderer(X,Y);
  this->SpherePicker->Pick(X,Y,0.0,this->CurrentRenderer);
  path = this->SpherePicker->GetPath();
  if ( path != NULL )
    {
    this->HighlightSphere(1);
    }
  else
    {
    this->State = vtkSphereWidget::Outside;
    return;
    }
  
  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  this->Interactor->Render();

  this->OldX = X;
  this->OldY = Y;
}

void vtkSphereWidget::OnMouseMove (int vtkNotUsed(ctrl), 
                                 int vtkNotUsed(shift), int X, int Y)
{
  // See whether we're active
  if ( this->State == vtkSphereWidget::Outside || 
       this->State == vtkSphereWidget::Start )
    {
    return;
    }
  
  // Do different things depending on state
  // Calculations everybody does
  double focalPoint[4], pickPoint[4], prevPickPoint[4];
  double z;

  this->CurrentCamera = this->Interactor->FindPokedCamera(X,Y);
  if ( !this->CurrentCamera )
    {
    return;
    }

  // Compute the two points defining the motion vector
  this->CurrentCamera->GetFocalPoint(focalPoint);
  this->ComputeWorldToDisplay(focalPoint[0], focalPoint[1],
                              focalPoint[2], focalPoint);
  z = focalPoint[2];
  this->ComputeDisplayToWorld(double(this->OldX),double(this->OldY),
                              z, prevPickPoint);
  this->ComputeDisplayToWorld(double(X), double(Y), z, pickPoint);

  // Process the motion
  if ( this->State == vtkSphereWidget::Moving )
    {
    this->Translate(prevPickPoint, pickPoint);
    }
  else if ( this->State == vtkSphereWidget::Scaling )
    {
    this->Scale(prevPickPoint, pickPoint, X, Y);
    }

  // Interact, if desired
  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::InteractionEvent,NULL);
  
  this->Interactor->Render();
  this->OldX = X;
  this->OldY = Y;
}

void vtkSphereWidget::OnLeftButtonUp (int vtkNotUsed(ctrl), 
                                    int vtkNotUsed(shift), 
                                    int vtkNotUsed(X), int vtkNotUsed(Y))
{
  if ( this->State == vtkSphereWidget::Outside )
    {
    return;
    }

  this->State = vtkSphereWidget::Start;
  this->HighlightSphere(0);

  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkSphereWidget::OnRightButtonDown (int vtkNotUsed(ctrl), 
                                       int vtkNotUsed(shift), int X, int Y)
{
  this->State = vtkSphereWidget::Scaling;

  // Okay, we can process this. Try to pick handles first;
  // if no handles picked, then pick the bounding box.
  vtkAssemblyPath *path;
  this->Interactor->FindPokedRenderer(X,Y);
  this->SpherePicker->Pick(X,Y,0.0,this->CurrentRenderer);
  path = this->SpherePicker->GetPath();
  if ( path == NULL )
    {
    this->State = vtkSphereWidget::Outside;
    this->HighlightSphere(0);
    return;
    }
  else
    {
    this->HighlightSphere(1);
    }
  
  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  this->Interactor->Render();
  
  this->OldX = X;
  this->OldY = Y;
}

void vtkSphereWidget::OnRightButtonUp (int vtkNotUsed(ctrl), 
                                     int vtkNotUsed(shift), 
                                     int vtkNotUsed(X), int vtkNotUsed(Y))
{
  if ( this->State == vtkSphereWidget::Outside )
    {
    return;
    }

  this->State = vtkSphereWidget::Start;
  this->HighlightSphere(0);
  
  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  this->Interactor->Render();
}

// Loop through all points and translate them
void vtkSphereWidget::Translate(double *p1, double *p2)
{
  //Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];
  
  //int res = this->SphereSource->GetResolution();
  float *center = this->SphereSource->GetCenter();

  float center1[3];
  for (int i=0; i<3; i++)
    {
    center1[i] = center[i] + v[i];
    }
  
  this->SphereSource->SetCenter(center1);
  this->SphereSource->Update();

  this->SelectRepresentation();
}

void vtkSphereWidget::Scale(double *p1, double *p2, int vtkNotUsed(X), int Y)
{
  //Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  //int res = this->SphereSource->GetResolution();
  float radius = this->SphereSource->GetRadius();

  // Compute the scale factor
  float sf = vtkMath::Norm(v) / radius;
  if ( Y > this->OldY )
    {
    sf = 1.0 + sf;
    }
  else
    {
    sf = 1.0 - sf;
    }
  
  this->SphereSource->SetRadius(sf*radius);
  this->SphereSource->Update();

  this->SelectRepresentation();
}

void vtkSphereWidget::CreateDefaultProperties()
{
  if ( ! this->SphereProperty )
    {
    this->SphereProperty = vtkProperty::New();
    }
  if ( ! this->SelectedSphereProperty )
    {
    this->SelectedSphereProperty = vtkProperty::New();
    }
}

void vtkSphereWidget::PlaceWidget(float bds[6])
{
  float bounds[6], center[3], radius;

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

  for (int i=0; i<6; i++)
    {
    this->InitialBounds[i] = bounds[i];
    }
  this->InitialLength = sqrt((bounds[1]-bounds[0])*(bounds[1]-bounds[0]) +
                             (bounds[3]-bounds[2])*(bounds[3]-bounds[2]) +
                             (bounds[5]-bounds[4])*(bounds[5]-bounds[4]));

}

