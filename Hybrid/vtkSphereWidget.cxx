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

#include "vtkAssemblyNode.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCellPicker.h"
#include "vtkDoubleArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSphere.h"
#include "vtkSphereSource.h"
#include "vtkTransform.h"
#include "vtkTransformPolyDataFilter.h"

vtkCxxRevisionMacro(vtkSphereWidget, "1.16");
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
  this->SphereSource->SetRadius(1.0);
  this->SphereTransform = vtkTransform::New();
  this->TransformSphereFilter = vtkTransformPolyDataFilter::New();
  this->TransformSphereFilter->SetTransform(this->SphereTransform);
  this->TransformSphereFilter->SetInput(this->SphereSource->GetOutput());
  this->SphereMapper = vtkPolyDataMapper::New();
  this->SphereMapper->SetInput(this->TransformSphereFilter->GetOutput());
  this->SphereActor = vtkActor::New();
  this->SphereActor->SetMapper(this->SphereMapper);

  // handles
  this->HandleDirection[0] = 1.0;
  this->HandleDirection[1] = 0.0;
  this->HandleDirection[2] = 0.0;
  this->HandleSource = vtkSphereSource::New();
  this->HandleSource->SetThetaResolution(16);
  this->HandleSource->SetPhiResolution(8);
  this->HandleSource->SetRadius(1.0);
  this->HandleTransform = vtkTransform::New();
  this->TransformHandleFilter = vtkTransformPolyDataFilter::New();
  this->TransformHandleFilter->SetTransform(this->HandleTransform);
  this->TransformHandleFilter->SetInput(this->HandleSource->GetOutput());
  this->HandleMapper = vtkPolyDataMapper::New();
  this->HandleMapper->SetInput(this->TransformHandleFilter->GetOutput());
  this->HandleActor = vtkActor::New();
  this->HandleActor->SetMapper(this->HandleMapper);

  // controls
  this->Translation = 1;
  this->Scale = 1;
  this->Rotation = 1;
  this->HandleVisibility = 0;

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
  this->SpherePicker->AddPickList(this->HandleActor);
  this->SpherePicker->PickFromListOn();
  
  // Set up the initial properties
  this->SphereProperty = NULL;
  this->SelectedSphereProperty = NULL;
  this->HandleProperty = NULL;
  this->SelectedHandleProperty = NULL;
  this->CreateDefaultProperties();
}

vtkSphereWidget::~vtkSphereWidget()
{
  this->SphereTransform->Delete();
  this->TransformSphereFilter->Delete();
  this->HandleTransform->Delete();
  this->TransformHandleFilter->Delete();

  this->SphereActor->Delete();
  this->SphereMapper->Delete();
  this->SphereSource->Delete();

  this->SpherePicker->Delete();

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
    
    this->CurrentRenderer = 
      this->Interactor->FindPokedRenderer(this->Interactor->GetLastEventPosition()[0],
                                          this->Interactor->GetLastEventPosition()[1]);
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
    i->AddObserver(vtkCommand::MiddleButtonPressEvent, 
                   this->EventCallbackCommand, this->Priority);
    i->AddObserver(vtkCommand::MiddleButtonReleaseEvent, 
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
    this->BuildRepresentation();

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

void vtkSphereWidget::OnLeftButtonDown()
{
  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // Okay, we can process this. Try to pick handles first;
  // if no handles picked, then try to pick the sphere.
  vtkAssemblyPath *path;
  this->Interactor->FindPokedRenderer(X,Y);
  this->SpherePicker->Pick(X,Y,0.0,this->CurrentRenderer);
  path = this->SpherePicker->GetPath();
  if ( path == NULL )
    {
    this->State = vtkSphereWidget::Outside;
    return;
    }
  else if (path->GetFirstNode()->GetProp() == this->SphereActor )
    {
    this->HighlightSphere(1);
    if ( ! this->Interactor->GetShiftKey() && this->Rotation )
      {
      this->State = vtkSphereWidget::Rotating;
      }
    else
      {
      this->State = vtkSphereWidget::Translating;
      }
    }
  else //if (path->GetFirstNode()->GetProp() == this->HandleActor )
    {
    this->State = vtkSphereWidget::Positioning;
    this->HighlightHandle(1);
    }
  
  this->EventCallbackCommand->SetAbortFlag(1);
  this->StartInteraction();
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
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

  this->EventCallbackCommand->SetAbortFlag(1);
  this->EndInteraction();
  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkSphereWidget::OnMiddleButtonDown()
{
  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

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
    this->State = vtkSphereWidget::Translating;
    this->HighlightSphere(1);
    }
  
  this->EventCallbackCommand->SetAbortFlag(1);
  this->StartInteraction();
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkSphereWidget::OnMiddleButtonUp()
{
  if ( this->State == vtkSphereWidget::Outside )
    {
    return;
    }

  this->State = vtkSphereWidget::Start;
  this->HighlightSphere(0);
  this->HighlightHandle(0);
  
  this->EventCallbackCommand->SetAbortFlag(1);
  this->EndInteraction();
  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkSphereWidget::OnRightButtonDown()
{
  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

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
    this->State = vtkSphereWidget::Scaling;
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
  
  this->EventCallbackCommand->SetAbortFlag(1);
  this->EndInteraction();
  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
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
  this->ComputeDisplayToWorld(double(this->Interactor->GetLastEventPosition()[0]),
                              double(this->Interactor->GetLastEventPosition()[1]),
                              z, 
                              prevPickPoint);
  this->ComputeDisplayToWorld(double(X), double(Y), z, pickPoint);

  // Process the motion
  if ( this->State == vtkSphereWidget::Rotating )
    {
    double vpn[3];
    camera->GetViewPlaneNormal(vpn);
    this->Rotate(X, Y, prevPickPoint, pickPoint, vpn);
    }
  else if ( this->State == vtkSphereWidget::Translating )
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

void vtkSphereWidget::Rotate(int X, int Y, double *p1, double *p2, double *vpn)
{
  if ( !this->Rotation )
    {
    return;
    }

  float *center = this->SphereSource->GetCenter();
  double v[3]; //vector of motion
  double axis[3]; //axis of rotation
  double theta; //rotation angle

  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  // Create axis of rotation and angle of rotation
  vtkMath::Cross(vpn,v,axis);
  if ( vtkMath::Normalize(axis) == 0.0 )
    {
    return;
    }
  int *size = this->CurrentRenderer->GetSize();
  double l2 = (X-this->Interactor->GetLastEventPosition()[0])*(X-this->Interactor->GetLastEventPosition()[0]) + (Y-this->Interactor->GetLastEventPosition()[1])*(Y-this->Interactor->GetLastEventPosition()[1]);
  theta = 360.0 * sqrt(l2/((double)size[0]*size[0]+size[1]*size[1]));

  //Manipulate the transform to reflect the rotation
  vtkTransform *trans = vtkTransform::New();
  trans->Translate(-center[0],-center[1],-center[2]);
  trans->RotateWXYZ(theta,axis);
  trans->Translate(center[0],center[1],center[2]);
  trans->GetOrientation(this->SphereOrientation);
  
  this->BuildRepresentation();

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
  
  float *center = this->GetCenter();

  float center1[3];
  for (int i=0; i<3; i++)
    {
    center1[i] = center[i] + v[i];
    }
  
  this->SetCenter(center1);
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

  float radius = this->GetRadius();

  // Compute the scale factor
  float sf = vtkMath::Norm(v) / radius;
  if ( Y > this->Interactor->GetLastEventPosition()[1] )
    {
    sf = 1.0 + sf;
    }
  else
    {
    sf = 1.0 - sf;
    }
  
  this->SetRadius(sf*radius);
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
  float *center = this->SphereSource->GetCenter();
  float radius = this->SphereSource->GetRadius();
  
  // set the position of the sphere
  float p[3];
  for (int i=0; i<3; i++)
    {
    p[i] = this->HandlePosition[i] + v[i];
    this->HandleDirection[i] = p[i] - center[i];
    }

  this->BuildRepresentation();
}

void vtkSphereWidget::BuildRepresentation()
{
  if ( this->CurrentRenderer )
    {
    if ( ! this->HandleVisibility )
      {
      this->CurrentRenderer->RemoveActor(this->HandleActor);
      }

    // Control the representation of the sphere
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

  // Now create the transformations for the sphere.
  // Note that the sphere is always located at the origin.
  this->SphereTransform->Identity();
  this->SphereTransform->PostMultiply();  

  // scale
  this->SphereTransform->Scale(this->SphereScale[0],
                               this->SphereScale[1],
                               this->SphereScale[2]);

  // rotate
  this->SphereTransform->RotateY(this->SphereOrientation[1]);
  this->SphereTransform->RotateX(this->SphereOrientation[0]);
  this->SphereTransform->RotateZ(this->SphereOrientation[2]);

  // move back from origin to center of sphere
  this->SphereTransform->Translate(this->SphereCenter[0],
                                   this->SphereCenter[1],
                                   this->SphereCenter[2]);

  // Now create the transformations for the handle
  //
  this->HandleTransform->Identity();
  this->HandleTransform->PostMultiply();  


  // scale
  this->HandleTransform->Scale(0.075*this->HandleScale[0],
                               0.075*this->HandleScale[1],
                               0.075*this->HandleScale[2]);

  // rotate
  this->HandleTransform->RotateY(this->HandleDirection[1]);
  this->HandleTransform->RotateX(this->HandleDirection[0]);
  this->HandleTransform->RotateZ(this->HandleDirection[2]);

  // move back from origin to center of sphere
  this->HandleTransform->Translate(this->HandleCenter[0],
                                   this->HandleCenter[1],
                                   this->HandleCenter[2]);

  this->TransformHandleFilter->Update();
  this->TransformSphereFilter->Update();
}

void vtkSphereWidget::GetSphere(vtkSphere *sphere)
{
  sphere->SetRadius(this->GetRadius());
  sphere->SetCenter(this->GetCenter());

//  vtkWarningMacro( <<"Radius:(" << this->GetRadius() );
//  vtkWarningMacro(<<"Center:(" <<this->SphereCenter[0] << " "
//  << this->SphereCenter[1] << " "
//  << this->SphereCenter[2] << ")");

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

void vtkSphereWidget::HighlightHandle(int highlight)
{
  if ( highlight )
    {
    this->HandleActor->SetProperty(this->SelectedHandleProperty);
    }
  else
    {
    this->HandleActor->SetProperty(this->HandleProperty);
    }
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
  
  this->PlaceHandle(center,radius);

  for (int i=0; i<6; i++)
    {
    this->InitialBounds[i] = bounds[i];
    }
  this->InitialLength = sqrt((bounds[1]-bounds[0])*(bounds[1]-bounds[0]) +
                             (bounds[3]-bounds[2])*(bounds[3]-bounds[2]) +
                             (bounds[5]-bounds[4])*(bounds[5]-bounds[4]));

  // Create the initial transformations
  this->SphereCenter[0] = center[0];
  this->SphereCenter[1] = center[1];
  this->SphereCenter[2] = center[2];
  this->SphereScale[0] = radius;
  this->SphereScale[1] = radius;
  this->SphereScale[2] = radius;
  this->SphereOrientation[0] = 0.0;
  this->SphereOrientation[1] = 0.0;
  this->SphereOrientation[2] = 0.0;

  this->HandleCenter[0] = center[0] + radius;
  this->HandleCenter[1] = center[1];
  this->HandleCenter[2] = center[2];
  this->HandleScale[0] = 0.0075*this->InitialLength;
  this->HandleScale[1] = 0.0075*this->InitialLength;
  this->HandleScale[2] = 0.0075*this->InitialLength;
  
  this->BuildRepresentation();
}

void vtkSphereWidget::PlaceHandle(float *center, float radius)
{
  float sf = radius / vtkMath::Norm(this->HandleDirection);

  this->HandlePosition[0] = center[0] + sf*this->HandleDirection[0];
  this->HandlePosition[1] = center[1] + sf*this->HandleDirection[1];
  this->HandlePosition[2] = center[2] + sf*this->HandleDirection[2];
  this->HandleSource->SetCenter(this->HandlePosition);
}

void vtkSphereWidget::GetPolyData(vtkPolyData *pd)
{ 
  pd->ShallowCopy(this->TransformSphereFilter->GetOutput()); 
}

void vtkSphereWidget::GetTransform(vtkTransform *t)
{
  // The transformation is relative to the initial bounds.
  // Initial bounds are set when PlaceWidget() is invoked.
  t->Identity();
  t->Concatenate(this->SphereTransform);
}

void vtkSphereWidget::SetRadius(float r)
{
  r = ( r <= 0 ? 0.00001 : r);
  this->SphereScale[0] = r;
  this->SphereScale[1] = r;
  this->SphereScale[2] = r;
  this->BuildRepresentation();
}

  
// The sphere is set up with an initial radius of 1.0.
float vtkSphereWidget::GetRadius()
{
  return this->SphereScale[0];
}

void vtkSphereWidget::SetCenter(float x, float y, float z)
{
  this->SphereCenter[0] = x;
  this->SphereCenter[1] = y;
  this->SphereCenter[2] = z;
  this->BuildRepresentation();
}

void vtkSphereWidget::GetCenter(float xyz[3])
{
  xyz[0] = this->SphereCenter[0];
  xyz[1] = this->SphereCenter[1];
  xyz[2] = this->SphereCenter[2];
  return;
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
  float *center = this->SphereSource->GetCenter();
  float r = this->SphereSource->GetRadius();

  os << indent << "Theta Resolution: " << thetaRes << "\n";
  os << indent << "Phi Resolution: " << phiRes << "\n";
  os << indent << "Center: (" << center[0] << ", "
     << center[1] << ", " << center[2] << ")\n";
  os << indent << "Radius: " << r << "\n";
}

