/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitPlaneWidget.cxx
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
#include "vtkImplicitPlaneWidget.h"

#include "vtkAssemblyNode.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCellPicker.h"
#include "vtkSphereSource.h"
#include "vtkConeSource.h"
#include "vtkLineSource.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkOutlineFilter.h"
#include "vtkTubeFilter.h"
#include "vtkCutter.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkPlane.h"
#include "vtkFeatureEdges.h"
#include "vtkTransform.h"

vtkCxxRevisionMacro(vtkImplicitPlaneWidget, "1.2");
vtkStandardNewMacro(vtkImplicitPlaneWidget);

vtkImplicitPlaneWidget::vtkImplicitPlaneWidget() : vtkPolyDataSourceWidget()
{
  this->State = vtkImplicitPlaneWidget::Start;
  this->EventCallbackCommand->SetCallback(vtkImplicitPlaneWidget::ProcessEvents);
  
  this->NormalToXAxis = 0;
  this->NormalToYAxis = 0;
  this->NormalToZAxis = 0;

  // Build the representation of the widget
  // 
  this->Plane = vtkPlane::New();
  this->Plane->SetNormal(0,0,1);
  this->Plane->SetOrigin(0,0,0);

  this->Box = vtkImageData::New();
  this->Box->SetDimensions(2,2,2);
  this->Outline = vtkOutlineFilter::New();
  this->Outline->SetInput(this->Box);
  this->OutlineMapper = vtkPolyDataMapper::New();
  this->OutlineMapper->SetInput(this->Outline->GetOutput());
  this->OutlineActor = vtkActor::New();
  this->OutlineActor->SetMapper(this->OutlineMapper);
  
  this->Cutter = vtkCutter::New();
  this->Cutter->SetInput(this->Box);
  this->Cutter->SetCutFunction(this->Plane);
  this->CutMapper = vtkPolyDataMapper::New();
  this->CutMapper->SetInput(this->Cutter->GetOutput());
  this->CutActor = vtkActor::New();
  this->CutActor->SetMapper(this->CutMapper);
  
  this->Edges = vtkFeatureEdges::New();
  this->Edges->SetInput(this->Cutter->GetOutput());
  this->EdgesTuber = vtkTubeFilter::New();
  this->EdgesTuber->SetInput(this->Edges->GetOutput());
  this->EdgesTuber->SetNumberOfSides(12);
  this->EdgesMapper = vtkPolyDataMapper::New();
  this->EdgesMapper->SetInput(this->EdgesTuber->GetOutput());
  this->EdgesActor = vtkActor::New();
  this->EdgesActor->SetMapper(this->EdgesMapper);
  this->Tubing = 1; //control whether tubing is on

  // Create the + plane normal
  this->LineSource = vtkLineSource::New();
  this->LineSource->SetResolution(1);
  this->LineMapper = vtkPolyDataMapper::New();
  this->LineMapper->SetInput(this->LineSource->GetOutput());
  this->LineActor = vtkActor::New();
  this->LineActor->SetMapper(this->LineMapper);

  this->ConeSource = vtkConeSource::New();
  this->ConeSource->SetResolution(12);
  this->ConeSource->SetAngle(25.0);
  this->ConeMapper = vtkPolyDataMapper::New();
  this->ConeMapper->SetInput(this->ConeSource->GetOutput());
  this->ConeActor = vtkActor::New();
  this->ConeActor->SetMapper(this->ConeMapper);

  // Create the - plane normal
  this->LineSource2 = vtkLineSource::New();
  this->LineSource2->SetResolution(1);
  this->LineMapper2 = vtkPolyDataMapper::New();
  this->LineMapper2->SetInput(this->LineSource2->GetOutput());
  this->LineActor2 = vtkActor::New();
  this->LineActor2->SetMapper(this->LineMapper2);

  this->ConeSource2 = vtkConeSource::New();
  this->ConeSource2->SetResolution(12);
  this->ConeSource2->SetAngle(25.0);
  this->ConeMapper2 = vtkPolyDataMapper::New();
  this->ConeMapper2->SetInput(this->ConeSource2->GetOutput());
  this->ConeActor2 = vtkActor::New();
  this->ConeActor2->SetMapper(this->ConeMapper2);

  // Create the center handle
  this->Sphere = vtkSphereSource::New();
  this->Sphere->SetThetaResolution(16);
  this->Sphere->SetPhiResolution(8);
  this->SphereMapper = vtkPolyDataMapper::New();
  this->SphereMapper->SetInput(this->Sphere->GetOutput());
  this->SphereActor = vtkActor::New();
  this->SphereActor->SetMapper(this->SphereMapper);

  this->Transform = vtkTransform::New();

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
  this->Picker = vtkCellPicker::New();
  this->Picker->SetTolerance(0.005);
  this->Picker->AddPickList(this->CutActor);
  this->Picker->AddPickList(this->LineActor);
  this->Picker->AddPickList(this->ConeActor);
  this->Picker->AddPickList(this->LineActor2);
  this->Picker->AddPickList(this->ConeActor2);
  this->Picker->AddPickList(this->SphereActor);
  this->Picker->AddPickList(this->OutlineActor);
  this->Picker->PickFromListOn();
  
  // Set up the initial properties
  this->NormalProperty = NULL;
  this->SelectedNormalProperty = NULL;
  this->PlaneProperty = NULL;
  this->SelectedPlaneProperty = NULL;
  this->OutlineProperty = NULL;
  this->SelectedOutlineProperty = NULL;
  this->EdgesProperty = NULL;
  this->CreateDefaultProperties();
  
}

vtkImplicitPlaneWidget::~vtkImplicitPlaneWidget()
{  
  this->Plane->Delete();
  this->Box->Delete();
  this->Outline->Delete();
  this->OutlineMapper->Delete();
  this->OutlineActor->Delete();
  
  this->Cutter->Delete();
  this->CutMapper->Delete();
  this->CutActor->Delete();
  
  this->Edges->Delete();
  this->EdgesTuber->Delete();
  this->EdgesMapper->Delete();
  this->EdgesActor->Delete();
  
  this->LineSource->Delete();
  this->LineMapper->Delete();
  this->LineActor->Delete();

  this->ConeSource->Delete();
  this->ConeMapper->Delete();
  this->ConeActor->Delete();

  this->LineSource2->Delete();
  this->LineMapper2->Delete();
  this->LineActor2->Delete();

  this->ConeSource2->Delete();
  this->ConeMapper2->Delete();
  this->ConeActor2->Delete();

  this->Sphere->Delete();
  this->SphereMapper->Delete();
  this->SphereActor->Delete();

  this->Transform->Delete();

  this->Picker->Delete();
    this->SphereActor->SetProperty(this->SelectedNormalProperty);

  if ( this->NormalProperty )
    {
    this->NormalProperty->Delete();
    }
  if ( this->SelectedNormalProperty )
    {
    this->SelectedNormalProperty->Delete();
    }

  if ( this->PlaneProperty )
    {
    this->PlaneProperty->Delete();
    }
  if ( this->SelectedPlaneProperty )
    {
    this->SelectedPlaneProperty->Delete();
    }

  if ( this->OutlineProperty )
    {
    this->OutlineProperty->Delete();
    }
  
  if ( this->EdgesProperty )
    {
    this->EdgesProperty->Delete();
    }
}

void vtkImplicitPlaneWidget::SetEnabled(int enabling)
{
  if ( ! this->Interactor )
    {
    vtkErrorMacro(<<"The interactor must be set prior to enabling/disabling widget");
    return;
    }

  if ( enabling ) //------------------------------------------------------------
    {
    vtkDebugMacro(<<"Enabling plane widget");

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

    // add the outline
    this->CurrentRenderer->AddActor(this->OutlineActor);
    this->OutlineActor->SetProperty(this->OutlineProperty);

    // add the edges
    this->CurrentRenderer->AddActor(this->EdgesActor);
    this->OutlineActor->SetProperty(this->EdgesProperty);

    // add the normal vector
    this->CurrentRenderer->AddActor(this->LineActor);
    this->LineActor->SetProperty(this->NormalProperty);
    this->CurrentRenderer->AddActor(this->ConeActor);
    this->ConeActor->SetProperty(this->NormalProperty);

    this->CurrentRenderer->AddActor(this->LineActor2);
    this->LineActor2->SetProperty(this->NormalProperty);
    this->CurrentRenderer->AddActor(this->ConeActor2);
    this->ConeActor2->SetProperty(this->NormalProperty);
    
    // add the center handle
    this->CurrentRenderer->AddActor(this->SphereActor);
    this->SphereActor->SetProperty(this->NormalProperty);

    // add the plane
    this->CurrentRenderer->AddActor(this->CutActor);
    this->ConeActor->SetProperty(this->PlaneProperty);

    this->UpdateRepresentation();
    this->InvokeEvent(vtkCommand::EnableEvent,NULL);
    }
  
  else //disabling----------------------------------------------------------
    {
    vtkDebugMacro(<<"Disabling plane widget");

    if ( ! this->Enabled ) //already disabled, just return
      {
      return;
      }
    
    this->Enabled = 0;

    // don't listen for events any more
    this->Interactor->RemoveObserver(this->EventCallbackCommand);

    // turn off the various actors
    this->CurrentRenderer->RemoveActor(this->OutlineActor);
    this->CurrentRenderer->RemoveActor(this->EdgesActor);
    this->CurrentRenderer->RemoveActor(this->LineActor);
    this->CurrentRenderer->RemoveActor(this->ConeActor);
    this->CurrentRenderer->RemoveActor(this->LineActor2);
    this->CurrentRenderer->RemoveActor(this->ConeActor2);
    this->CurrentRenderer->RemoveActor(this->SphereActor);
    this->CurrentRenderer->RemoveActor(this->CutActor);

    this->InvokeEvent(vtkCommand::DisableEvent,NULL);
    }

  this->Interactor->Render();
}

void vtkImplicitPlaneWidget::ProcessEvents(vtkObject* vtkNotUsed(object), 
                                           unsigned long event,
                                           void* clientdata, 
                                           void* vtkNotUsed(calldata))
{
  vtkImplicitPlaneWidget* self = 
    reinterpret_cast<vtkImplicitPlaneWidget *>( clientdata );

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

void vtkImplicitPlaneWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if ( this->NormalProperty )
    {
    os << indent << "Normal Property: " << this->NormalProperty << "\n";
    }
  else
    {
    os << indent << "Normal Property: (none)\n";
    }
  if ( this->SelectedNormalProperty )
    {
    os << indent << "Selected Normal Property: " 
       << this->SelectedNormalProperty << "\n";
    }
  else
    {
    os << indent << "Selected Normal Property: (none)\n";
    }

  if ( this->PlaneProperty )
    {
    os << indent << "Plane Property: " << this->PlaneProperty << "\n";
    }
  else
    {
    os << indent << "Plane Property: (none)\n";
    }
  if ( this->SelectedPlaneProperty )
    {
    os << indent << "Selected Plane Property: " 
       << this->SelectedPlaneProperty << "\n";
    }
  else
    {
    os << indent << "Selected Plane Property: (none)\n";
    }

  if ( this->EdgesProperty )
    {
    os << indent << "Edges Property: " << this->EdgesProperty << "\n";
    }
  else
    {
    os << indent << "Edges Property: (none)\n";
    }

  if ( this->OutlineProperty )
    {
    os << indent << "Outline Property: " << this->OutlineProperty << "\n";
    }
  else
    {
    os << indent << "Outline Property: (none)\n";
    }

  os << indent << "Normal To X Axis: " 
     << (this->NormalToXAxis ? "On" : "Off") << "\n";
  os << indent << "Normal To Y Axis: " 
     << (this->NormalToYAxis ? "On" : "Off") << "\n";
  os << indent << "Normal To Z Axis: " 
     << (this->NormalToZAxis ? "On" : "Off") << "\n";
}


void vtkImplicitPlaneWidget::HighlightNormal(int highlight)
{
  if ( highlight )
    {
    this->LineActor->SetProperty(this->SelectedNormalProperty);
    this->ConeActor->SetProperty(this->SelectedNormalProperty);
    this->LineActor2->SetProperty(this->SelectedNormalProperty);
    this->ConeActor2->SetProperty(this->SelectedNormalProperty);
    this->SphereActor->SetProperty(this->SelectedNormalProperty);
    }
  else
    {
    this->LineActor->SetProperty(this->NormalProperty);
    this->ConeActor->SetProperty(this->NormalProperty);
    this->LineActor2->SetProperty(this->NormalProperty);
    this->ConeActor2->SetProperty(this->NormalProperty);
    this->SphereActor->SetProperty(this->NormalProperty);
    }
}


void vtkImplicitPlaneWidget::HighlightPlane(int highlight)
{
  if ( highlight )
    {
    this->CutActor->SetProperty(this->SelectedPlaneProperty);
    }
  else
    {
    this->CutActor->SetProperty(this->PlaneProperty);
    }
}


void vtkImplicitPlaneWidget::HighlightOutline(int highlight)
{
  if ( highlight )
    {
    this->OutlineActor->SetProperty(this->SelectedOutlineProperty);
    }
  else
    {
    this->OutlineActor->SetProperty(this->OutlineProperty);
    }
}


void vtkImplicitPlaneWidget::OnLeftButtonDown()
{
  // We're only here if we are enabled
  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // Okay, we can process this. See if we've picked anything.
  vtkAssemblyPath *path;
  this->Interactor->FindPokedRenderer(X,Y);
  this->Picker->Pick(X,Y,0.0,this->CurrentRenderer);
  path = this->Picker->GetPath();

  if ( path == NULL ) //not picking this widget
    {
    this->HighlightPlane(0);
    this->HighlightNormal(0);
    this->HighlightOutline(0);
    this->State = vtkImplicitPlaneWidget::Outside;
    return;
    }

  vtkProp *prop = path->GetFirstNode()->GetProp();
  if ( prop == this->ConeActor || prop == this->LineActor ||
       prop == this->ConeActor2 || prop == this->LineActor2 )
    {
    this->HighlightNormal(1);
    this->State = vtkImplicitPlaneWidget::Rotating;
    }
  else if ( prop == this->CutActor )
    {
    this->HighlightPlane(1);
    this->State = vtkImplicitPlaneWidget::MovingPlane;
    }
  else if ( prop == this->SphereActor )
    {
    this->HighlightNormal(1);
    this->State = vtkImplicitPlaneWidget::MovingOrigin;
    }
  else
    {
    this->HighlightOutline(1);
    this->State = vtkImplicitPlaneWidget::MovingOutline;
    }
  
  this->EventCallbackCommand->SetAbortFlag(1);
  this->StartInteraction();
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkImplicitPlaneWidget::OnLeftButtonUp()
{
  if ( this->State == vtkImplicitPlaneWidget::Outside )
    {
    return;
    }

  this->State = vtkImplicitPlaneWidget::Start;
  this->HighlightPlane(0);
  this->HighlightOutline(0);
  this->HighlightNormal(0);

  this->EventCallbackCommand->SetAbortFlag(1);
  this->EndInteraction();
  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkImplicitPlaneWidget::OnMiddleButtonDown()
{
  this->State = vtkImplicitPlaneWidget::Pushing;

  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // Okay, we can process this. If anything is picked, then we
  // can start pushing the plane.
  vtkAssemblyPath *path;
  this->Interactor->FindPokedRenderer(X,Y);
  this->Picker->Pick(X,Y,0.0,this->CurrentRenderer);
  path = this->Picker->GetPath();
  
  if ( path == NULL ) //nothing picked
    {
    this->State = vtkImplicitPlaneWidget::Outside;
    return;
    }

  this->HighlightNormal(1);
  this->HighlightPlane(1);
  
  this->EventCallbackCommand->SetAbortFlag(1);
  this->StartInteraction();
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkImplicitPlaneWidget::OnMiddleButtonUp()
{
  if ( this->State == vtkImplicitPlaneWidget::Outside )
    {
    return;
    }

  this->State = vtkImplicitPlaneWidget::Start;
  this->HighlightPlane(0);
  this->HighlightOutline(0);
  this->HighlightNormal(0);
  
  this->EventCallbackCommand->SetAbortFlag(1);
  this->EndInteraction();
  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkImplicitPlaneWidget::OnRightButtonDown()
{
  this->State = vtkImplicitPlaneWidget::Scaling;

  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // Okay, we can process this. Try to pick handles first;
  // if no handles picked, then pick the bounding box.
  vtkAssemblyPath *path;
  this->Interactor->FindPokedRenderer(X,Y);
  this->Picker->Pick(X,Y,0.0,this->CurrentRenderer);
  path = this->Picker->GetPath();
  if ( path == NULL ) //nothing picked
    {
    this->State = vtkImplicitPlaneWidget::Outside;
    return;
    }

  this->HighlightPlane(1);
  this->HighlightOutline(1);
  this->HighlightNormal(1);

  this->EventCallbackCommand->SetAbortFlag(1);
  this->StartInteraction();
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkImplicitPlaneWidget::OnRightButtonUp()
{
  if ( this->State == vtkImplicitPlaneWidget::Outside )
    {
    return;
    }

  this->State = vtkImplicitPlaneWidget::Start;
  this->HighlightPlane(0);
  this->HighlightOutline(0);
  this->HighlightNormal(0);
  
  this->EventCallbackCommand->SetAbortFlag(1);
  this->EndInteraction();
  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkImplicitPlaneWidget::OnMouseMove()
{
  // See whether we're active
  if ( this->State == vtkImplicitPlaneWidget::Outside || 
       this->State == vtkImplicitPlaneWidget::Start )
    {
    return;
    }
  
  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // Do different things depending on state
  // Calculations everybody does
  double focalPoint[4], pickPoint[4], prevPickPoint[4];
  double z, vpn[3];

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
  if ( this->State == vtkImplicitPlaneWidget::MovingPlane )
    {
    this->TranslatePlane(prevPickPoint, pickPoint);
    }
  else if ( this->State == vtkImplicitPlaneWidget::MovingOutline )
    {
    this->TranslateOutline(prevPickPoint, pickPoint);
    }
  else if ( this->State == vtkImplicitPlaneWidget::MovingOrigin )
    {
    this->TranslateOrigin(prevPickPoint, pickPoint);
    }
  else if ( this->State == vtkImplicitPlaneWidget::Pushing )
    {
    this->Push(prevPickPoint, pickPoint);
    }
  else if ( this->State == vtkImplicitPlaneWidget::Scaling )
    {
    this->Scale(prevPickPoint, pickPoint, X, Y);
    }
  else if ( this->State == vtkImplicitPlaneWidget::Rotating )
    {
    camera->GetViewPlaneNormal(vpn);
    this->Rotate(X, Y, prevPickPoint, pickPoint, vpn);
    }

  // Interact, if desired
  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::InteractionEvent,NULL);
  
  this->Interactor->Render();
}


void vtkImplicitPlaneWidget::Rotate(int X, int Y, double *p1, double *p2, double *vpn)
{
  double v[3]; //vector of motion
  double axis[3]; //axis of rotation
  double theta; //rotation angle

  // mouse motion vector in world space
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  float *center = this->Plane->GetOrigin();
  float *normal = this->Plane->GetNormal();

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
  this->Transform->Identity();
  this->Transform->Translate(center[0],center[1],center[2]);
  this->Transform->RotateWXYZ(theta,axis);
  this->Transform->Translate(-center[0],-center[1],-center[2]);

  //Set the new normal
  float nNew[3];
  this->Transform->TransformNormal(normal,nNew);
  this->Plane->SetNormal(nNew);
  
  this->UpdateRepresentation();
}

// Loop through all points and translate them
void vtkImplicitPlaneWidget::TranslatePlane(double *p1, double *p2)
{
  //Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];
  
}

// Loop through all points and translate them
void vtkImplicitPlaneWidget::TranslateOutline(double *p1, double *p2)
{
  //Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];
  
}

// Loop through all points and translate them
void vtkImplicitPlaneWidget::TranslateOrigin(double *p1, double *p2)
{
  //Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];
  
  //Add to the current point, project back down onto plane
  float *o = this->Plane->GetOrigin();
  float *n = this->Plane->GetNormal();
  float newOrigin[3];

  newOrigin[0] = o[0] + v[0];
  newOrigin[1] = o[1] + v[1];
  newOrigin[2] = o[2] + v[2];
  
  vtkPlane::ProjectPoint(newOrigin,o,n,newOrigin);
  this->Plane->SetOrigin(newOrigin);
  this->UpdateRepresentation();
}

void vtkImplicitPlaneWidget::Scale(double *p1, double *p2, 
                                   int vtkNotUsed(X), int Y)
{
  //Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  //int res = this->PlaneSource->GetXResolution();
  float *o = this->Plane->GetOrigin();

  // Compute the scale factor
  float sf = vtkMath::Norm(v) / this->Outline->GetOutput()->GetLength();
  if ( Y > this->Interactor->GetLastEventPosition()[1] )
    {
    sf = 1.0 + sf;
    }
  else
    {
    sf = 1.0 - sf;
    }
  
  this->Transform->Identity();
  this->Transform->Translate(o[0],o[1],o[2]);
  this->Transform->Scale(sf,sf,sf);
  this->Transform->Translate(-o[0],-o[1],-o[2]);

  float *origin = this->Box->GetOrigin();
  float *spacing = this->Box->GetSpacing();
  float oNew[3], p[3], pNew[3];
  p[0] = origin[0] + spacing[0];
  p[1] = origin[1] + spacing[1];
  p[2] = origin[2] + spacing[2];

  this->Transform->TransformPoint(origin,oNew);
  this->Transform->TransformPoint(p,pNew);

  this->Box->SetOrigin(oNew);
  this->Box->SetSpacing( (pNew[0]-oNew[0]), (pNew[1]-oNew[1]), (pNew[2]-oNew[2]) );

  this->UpdateRepresentation();
}

void vtkImplicitPlaneWidget::Push(double *p1, double *p2)
{
  //Get the motion vector
  float v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];
  
  this->Plane->Push( vtkMath::Dot(v,this->Plane->GetNormal()) );
  this->UpdateRepresentation();
}

void vtkImplicitPlaneWidget::CreateDefaultProperties()
{
  if ( ! this->NormalProperty )
    {
    this->NormalProperty = vtkProperty::New();
    this->NormalProperty->SetColor(1,1,1);
    }
  if ( ! this->SelectedNormalProperty )
    {
    this->SelectedNormalProperty = vtkProperty::New();
    this->SelectedNormalProperty->SetColor(1,0,0);
    }
  
  if ( ! this->PlaneProperty )
    {
    this->PlaneProperty = vtkProperty::New();
    this->PlaneProperty->SetAmbient(1.0);
    this->PlaneProperty->SetAmbientColor(1.0,1.0,1.0);
    }
  if ( ! this->SelectedPlaneProperty )
    {
    this->SelectedPlaneProperty = vtkProperty::New();
    this->SelectedPlaneProperty->SetAmbient(1.0);
    this->SelectedPlaneProperty->SetAmbientColor(0.0,1.0,0.0);
    }

  if ( ! this->OutlineProperty )
    {
    this->OutlineProperty = vtkProperty::New();
    this->OutlineProperty->SetAmbient(1.0);
    this->OutlineProperty->SetAmbientColor(1.0,1.0,1.0);
    }
  if ( ! this->SelectedOutlineProperty )
    {
    this->SelectedOutlineProperty = vtkProperty::New();
    this->SelectedOutlineProperty->SetAmbient(1.0);
    this->SelectedOutlineProperty->SetAmbientColor(0.0,1.0,0.0);
    }

  if ( ! this->EdgesProperty )
    {
    this->EdgesProperty = vtkProperty::New();
    this->EdgesProperty->SetAmbient(1.0);
    this->EdgesProperty->SetAmbientColor(1.0,1.0,1.0);
    }
}

void vtkImplicitPlaneWidget::PlaceWidget(float bds[6])
{
  int i;
  float bounds[6], center[3];

  this->AdjustBounds(bds, bounds, center);

  // Set up the bounding box
  this->Box->SetOrigin(bounds[0],bounds[2],bounds[4]);
  this->Box->SetSpacing((bounds[1]-bounds[0]),(bounds[3]-bounds[2]),
                        (bounds[5]-bounds[4]));
  this->Outline->Update();

  if (this->Input || this->Prop3D)
    {
    this->LineSource->SetPoint1(this->Plane->GetOrigin());
    if ( this->NormalToYAxis )
      {
      this->Plane->SetNormal(0,1,0);
      this->LineSource->SetPoint2(0,1,0);
      }
    else if ( this->NormalToZAxis )
      {
      this->Plane->SetNormal(0,0,1);
      this->LineSource->SetPoint2(0,0,1);
      }
    else //default or x-normal
      {
      this->Plane->SetNormal(1,0,0);
      this->LineSource->SetPoint2(1,0,0);
      }
    }

  for (i=0; i<6; i++)
    {
    this->InitialBounds[i] = bounds[i];
    }

  this->InitialLength = sqrt((bounds[1]-bounds[0])*(bounds[1]-bounds[0]) +
                             (bounds[3]-bounds[2])*(bounds[3]-bounds[2]) +
                             (bounds[5]-bounds[4])*(bounds[5]-bounds[4]));

  this->UpdateRepresentation();
}

// Description:
// Set the center of the plane.
void vtkImplicitPlaneWidget::SetCenter(float x, float y, float z) 
{
  this->Plane->SetOrigin(x, y, z);
  this->UpdateRepresentation();
}

// Description:
// Set the center of the plane.
void vtkImplicitPlaneWidget::SetCenter(float c[3]) 
{
  this->SetCenter(c[0], c[1], c[2]);
}

// Description:
// Get the center of the plane.
float* vtkImplicitPlaneWidget::GetCenter() 
{
  return this->Plane->GetOrigin();
}

void vtkImplicitPlaneWidget::GetCenter(float xyz[3]) 
{
  this->Plane->GetOrigin(xyz);
}

// Description:
// Set the normal to the plane.
void vtkImplicitPlaneWidget::SetNormal(float x, float y, float z) 
{
  float n[3];
  n[0] = x;
  n[1] = y;
  n[2] = z;
  vtkMath::Normalize(n);
  this->Plane->SetNormal(n);
  this->UpdateRepresentation();
}

// Description:
// Set the normal to the plane.
void vtkImplicitPlaneWidget::SetNormal(float n[3]) 
{
  this->SetNormal(n[0], n[1], n[2]);
}

// Description:
// Get the normal to the plane.
float* vtkImplicitPlaneWidget::GetNormal() 
{
  return this->Plane->GetNormal();
}

void vtkImplicitPlaneWidget::GetNormal(float xyz[3]) 
{
  this->Plane->GetNormal(xyz);
}

void vtkImplicitPlaneWidget::GetPolyData(vtkPolyData *pd)
{ 
  pd->ShallowCopy(this->Cutter->GetOutput()); 
}

vtkPolyDataSource *vtkImplicitPlaneWidget::GetPolyDataSource()
{
  return this->Cutter;
}

void vtkImplicitPlaneWidget::GetPlane(vtkPlane *plane)
{
  if ( plane == NULL )
    {
    return;
    }
  
  plane->SetNormal(this->Plane->GetNormal());
  plane->SetOrigin(this->Plane->GetOrigin());
}

void vtkImplicitPlaneWidget::UpdatePlacement(void)
{
  this->Outline->Update();
  this->Cutter->Update();
  this->Edges->Update();
}

void vtkImplicitPlaneWidget::UpdateRepresentation()
{
  if ( ! this->CurrentRenderer )
    {
    return;
    }

  float *center = this->Plane->GetOrigin();
  float *normal = this->Plane->GetNormal();
  float p2[3];

  // Setup the plane normal
  float d = this->Outline->GetOutput()->GetLength();

  p2[0] = center[0] + 0.30 * d * normal[0];
  p2[1] = center[1] + 0.30 * d * normal[1];
  p2[2] = center[2] + 0.30 * d * normal[2];

  this->LineSource->SetPoint1(center);
  this->LineSource->SetPoint2(p2);
  this->ConeSource->SetCenter(p2);
  this->ConeSource->SetDirection(normal);

  p2[0] = center[0] - 0.30 * d * normal[0];
  p2[1] = center[1] - 0.30 * d * normal[1];
  p2[2] = center[2] - 0.30 * d * normal[2];

  this->LineSource2->SetPoint1(center);
  this->LineSource2->SetPoint2(p2);
  this->ConeSource2->SetCenter(p2);
  this->ConeSource2->SetDirection(normal);

  this->ConeSource->SetHeight(0.060*this->InitialLength);
  this->ConeSource->SetRadius(0.025*this->InitialLength);  
  this->ConeSource2->SetHeight(0.060*this->InitialLength);
  this->ConeSource2->SetRadius(0.025*this->InitialLength);  
  
  // Set up the position handle
  this->Sphere->SetRadius(0.025*this->InitialLength);
  this->Sphere->SetCenter(center);

  // Control the look of the edges
  if ( this->Tubing )
    {
    this->EdgesMapper->SetInput(this->EdgesTuber->GetOutput());
    this->EdgesTuber->SetRadius(0.005*this->InitialLength);
    }
  else 
    {
    this->EdgesMapper->SetInput(this->Edges->GetOutput());
    }
}

