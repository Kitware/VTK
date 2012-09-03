/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSphereWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSphereWidget.h"

#include "vtkActor.h"
#include "vtkAssemblyNode.h"
#include "vtkAssemblyPath.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCellPicker.h"
#include "vtkDoubleArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPickingManager.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphere.h"
#include "vtkSphereSource.h"

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
  this->SphereMapper->SetInputConnection(
    this->SphereSource->GetOutputPort());
  this->SphereActor = vtkActor::New();
  this->SphereActor->SetMapper(this->SphereMapper);

  // controls
  this->Translation = 1;
  this->Scale = 1;

  // handles
  this->HandleVisibility = 0;
  this->HandleDirection[0] = 1.0;
  this->HandleDirection[1] = 0.0;
  this->HandleDirection[2] = 0.0;
  this->HandleSource = vtkSphereSource::New();
  this->HandleSource->SetThetaResolution(16);
  this->HandleSource->SetPhiResolution(8);
  this->HandleMapper = vtkPolyDataMapper::New();
  this->HandleMapper->SetInputConnection(
    this->HandleSource->GetOutputPort());
  this->HandleActor = vtkActor::New();
  this->HandleActor->SetMapper(this->HandleMapper);

  // Define the point coordinates
  double bounds[6];
  bounds[0] = -0.5;
  bounds[1] = 0.5;
  bounds[2] = -0.5;
  bounds[3] = 0.5;
  bounds[4] = -0.5;
  bounds[5] = 0.5;

  // Initial creation of the widget, serves to initialize it
  this->PlaceWidget(bounds);

  //Manage the picking stuff
  this->Picker = vtkCellPicker::New();
  this->Picker->SetTolerance(0.005); //need some fluff
  this->Picker->AddPickList(this->SphereActor);
  this->Picker->AddPickList(this->HandleActor);
  this->Picker->PickFromListOn();

  // Set up the initial properties
  this->SphereProperty = NULL;
  this->SelectedSphereProperty = NULL;
  this->HandleProperty = NULL;
  this->SelectedHandleProperty = NULL;
  this->CreateDefaultProperties();
}

vtkSphereWidget::~vtkSphereWidget()
{
  this->SphereActor->Delete();
  this->SphereMapper->Delete();
  this->SphereSource->Delete();

  this->Picker->Delete();

  this->HandleSource->Delete();
  this->HandleMapper->Delete();
  this->HandleActor->Delete();

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

    if ( ! this->CurrentRenderer )
      {
      this->SetCurrentRenderer(
        this->Interactor->FindPokedRenderer(
          this->Interactor->GetLastEventPosition()[0],
          this->Interactor->GetLastEventPosition()[1]));
      if (this->CurrentRenderer == NULL)
        {
        return;
        }
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

    this->CurrentRenderer->AddActor(this->HandleActor);
    this->HandleActor->SetProperty(this->HandleProperty);
    this->SelectRepresentation();
    this->SizeHandles();

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
    this->CurrentRenderer->RemoveActor(this->HandleActor);

    this->InvokeEvent(vtkCommand::DisableEvent,NULL);
    this->SetCurrentRenderer(NULL);
    }

  this->Interactor->Render();
}

void vtkSphereWidget::ProcessEvents(vtkObject* vtkNotUsed(object),
                                    unsigned long event,
                                    void* clientdata,
                                    void* vtkNotUsed(calldata))
{
  vtkSphereWidget* self = reinterpret_cast<vtkSphereWidget *>( clientdata );

  //okay, let's do the right thing
  switch(event)
    {
    case vtkCommand::LeftButtonPressEvent:
      self->OnLeftButtonDown();
      break;
    case vtkCommand::LeftButtonReleaseEvent:
      self->OnLeftButtonUp();
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

  os << indent << "Translation: " << (this->Translation ? "On\n" : "Off\n");
  os << indent << "Scale: " << (this->Scale ? "On\n" : "Off\n");

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
}

void vtkSphereWidget::SelectRepresentation()
{
  if ( ! this->HandleVisibility )
    {
    this->CurrentRenderer->RemoveActor(this->HandleActor);
    }

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
    this->ValidPick = 1;
    this->Picker->GetPickPosition(this->LastPickPosition);
    this->SphereActor->SetProperty(this->SelectedSphereProperty);
    }
  else
    {
    this->SphereActor->SetProperty(this->SphereProperty);
    }
}

void vtkSphereWidget::HighlightHandle(int highlight)
{
  if ( highlight )
    {
    this->ValidPick = 1;
    this->Picker->GetPickPosition(this->LastPickPosition);
    this->HandleActor->SetProperty(this->SelectedHandleProperty);
    }
  else
    {
    this->HandleActor->SetProperty(this->HandleProperty);
    }
}

void vtkSphereWidget::OnLeftButtonDown()
{
  if (!this->Interactor)
    {
    return;
    }

  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!this->CurrentRenderer || !this->CurrentRenderer->IsInViewport(X, Y))
    {
    this->State = vtkSphereWidget::Outside;
    return;
    }

  // Okay, we can process this. Try to pick handles first;
  // if no handles picked, then try to pick the sphere.
  vtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->Picker);

  if ( path == NULL )
    {
    this->State = vtkSphereWidget::Outside;
    return;
    }
  else if (path->GetFirstNode()->GetViewProp() == this->SphereActor )
    {
    this->State = vtkSphereWidget::Moving;
    this->HighlightSphere(1);
    }
  else if (path->GetFirstNode()->GetViewProp() == this->HandleActor )
    {
    this->State = vtkSphereWidget::Positioning;
    this->HighlightHandle(1);
    }

  this->EventCallbackCommand->SetAbortFlag(1);
  this->StartInteraction();
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkSphereWidget::OnMouseMove()
{
  // See whether we're active
  if ( this->State == vtkSphereWidget::Outside ||
       this->State == vtkSphereWidget::Start )
    {
    return;
    }

  if (!this->Interactor)
    {
    return;
    }

  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // Do different things depending on state
  // Calculations everybody does
  double focalPoint[4], pickPoint[4], prevPickPoint[4];
  double z;

  vtkCamera *camera = this->CurrentRenderer->GetActiveCamera();
  if ( !camera )
    {
    return;
    }

  // Compute the two points defining the motion vector
  camera->GetFocalPoint(focalPoint);
  this->ComputeWorldToDisplay(focalPoint[0], focalPoint[1],
                              focalPoint[2], focalPoint);
  z = focalPoint[2];
  this->ComputeDisplayToWorld(double(this->Interactor->GetLastEventPosition()[0]),
                              double(this->Interactor->GetLastEventPosition()[1]),
                              z,
                              prevPickPoint);
  this->ComputeDisplayToWorld(double(X), double(Y), z, pickPoint);

  // Process the motion
  if ( this->State == vtkSphereWidget::Moving )
    {
    this->Translate(prevPickPoint, pickPoint);
    }
  else if ( this->State == vtkSphereWidget::Scaling )
    {
    this->ScaleSphere(prevPickPoint, pickPoint, X, Y);
    }
  else if ( this->State == vtkSphereWidget::Positioning )
    {
    this->MoveHandle(prevPickPoint, pickPoint, X, Y);
    }

  // Interact, if desired
  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::InteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkSphereWidget::OnLeftButtonUp()
{
  if ( this->State == vtkSphereWidget::Outside )
    {
    return;
    }

  this->State = vtkSphereWidget::Start;
  this->HighlightSphere(0);
  this->HighlightHandle(0);
  this->SizeHandles();

  this->EventCallbackCommand->SetAbortFlag(1);
  this->EndInteraction();
  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  if (this->Interactor)
    {
    this->Interactor->Render();
    }
}

void vtkSphereWidget::OnRightButtonDown()
{
  if (!this->Interactor)
    {
    return;
    }

  this->State = vtkSphereWidget::Scaling;

  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!this->CurrentRenderer || !this->CurrentRenderer->IsInViewport(X, Y))
    {
    this->State = vtkSphereWidget::Outside;
    return;
    }

  // Okay, we can process this. Try to pick handles first;
  // if no handles picked, then pick the bounding box.
  vtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->Picker);

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
  this->StartInteraction();
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkSphereWidget::OnRightButtonUp()
{
  if ( this->State == vtkSphereWidget::Outside )
    {
    return;
    }

  this->State = vtkSphereWidget::Start;
  this->HighlightSphere(0);
  this->HighlightHandle(0);
  this->SizeHandles();

  this->EventCallbackCommand->SetAbortFlag(1);
  this->EndInteraction();
  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  if (this->Interactor)
    {
    this->Interactor->Render();
    }
}

// Loop through all points and translate them
void vtkSphereWidget::Translate(double *p1, double *p2)
{
  if ( !this->Translation )
    {
    return;
    }

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

  this->SelectRepresentation();
}

void vtkSphereWidget::ScaleSphere(double *p1, double *p2,
                                  int vtkNotUsed(X), int Y)
{
  if ( !this->Scale )
    {
    return;
    }

  //Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  double radius = this->SphereSource->GetRadius();
  double *c = this->SphereSource->GetCenter();

  // Compute the scale factor
  double sf = vtkMath::Norm(v) / radius;
  if ( Y > this->Interactor->GetLastEventPosition()[1] )
    {
    sf = 1.0 + sf;
    }
  else
    {
    sf = 1.0 - sf;
    }

  this->SphereSource->SetRadius(sf*radius);
  this->HandlePosition[0] = c[0]+sf*(this->HandlePosition[0]-c[0]);
  this->HandlePosition[1] = c[1]+sf*(this->HandlePosition[1]-c[1]);
  this->HandlePosition[2] = c[2]+sf*(this->HandlePosition[2]-c[2]);
  this->HandleSource->SetCenter(this->HandlePosition);

  this->SelectRepresentation();
}

void vtkSphereWidget::MoveHandle(double *p1, double *p2,
                                 int vtkNotUsed(X), int vtkNotUsed(Y))
{
  //Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  // Compute the new location of the sphere
  double *center = this->SphereSource->GetCenter();
  double radius = this->SphereSource->GetRadius();

  // set the position of the sphere
  double p[3];
  for (int i=0; i<3; i++)
    {
    p[i] = this->HandlePosition[i] + v[i];
    this->HandleDirection[i] = p[i] - center[i];
    }

  this->PlaceHandle(center,radius);

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
}

void vtkSphereWidget::PlaceWidget(double bds[6])
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

  this->SizeHandles();
}

void vtkSphereWidget::PlaceHandle(double *center, double radius)
{
  double sf = radius / vtkMath::Norm(this->HandleDirection);

  this->HandlePosition[0] = center[0] + sf*this->HandleDirection[0];
  this->HandlePosition[1] = center[1] + sf*this->HandleDirection[1];
  this->HandlePosition[2] = center[2] + sf*this->HandleDirection[2];
  this->HandleSource->SetCenter(this->HandlePosition);
}

void vtkSphereWidget::SizeHandles()
{
  double radius = this->vtk3DWidget::SizeHandles(1.25);
  this->HandleSource->SetRadius(radius);
}

//------------------------------------------------------------------------------
void vtkSphereWidget::RegisterPickers()
{
  this->Interactor->GetPickingManager()->AddPicker(this->Picker, this);
}

void vtkSphereWidget::GetPolyData(vtkPolyData *pd)
{
  pd->ShallowCopy(this->SphereSource->GetOutput());
}
