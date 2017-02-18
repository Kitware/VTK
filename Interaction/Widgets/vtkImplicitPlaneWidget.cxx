/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitPlaneWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImplicitPlaneWidget.h"

#include "vtkActor.h"
#include "vtkAssemblyNode.h"
#include "vtkAssemblyPath.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCellPicker.h"
#include "vtkConeSource.h"
#include "vtkCutter.h"
#include "vtkFeatureEdges.h"
#include "vtkImageData.h"
#include "vtkLineSource.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkOutlineFilter.h"
#include "vtkPickingManager.h"
#include "vtkPlane.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTransform.h"
#include "vtkTubeFilter.h"

vtkStandardNewMacro(vtkImplicitPlaneWidget);

//----------------------------------------------------------------------------
vtkImplicitPlaneWidget::vtkImplicitPlaneWidget() : vtkPolyDataSourceWidget()
{
  this->DiagonalRatio = 0.3;
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
  this->Outline->SetInputData(this->Box);
  this->OutlineMapper = vtkPolyDataMapper::New();
  this->OutlineMapper->SetInputConnection(
    this->Outline->GetOutputPort());
  this->OutlineActor = vtkActor::New();
  this->OutlineActor->SetMapper(this->OutlineMapper);
  this->OutlineTranslation = 1;
  this->ScaleEnabled = 1;
  this->OutsideBounds = 1;

  this->Cutter = vtkCutter::New();
  this->Cutter->SetInputData(this->Box);
  this->Cutter->SetCutFunction(this->Plane);
  this->CutMapper = vtkPolyDataMapper::New();
  this->CutMapper->SetInputConnection(
    this->Cutter->GetOutputPort());
  this->CutActor = vtkActor::New();
  this->CutActor->SetMapper(this->CutMapper);
  this->DrawPlane = 1;

  this->Edges = vtkFeatureEdges::New();
  this->Edges->SetInputConnection(
    this->Cutter->GetOutputPort());
  this->EdgesTuber = vtkTubeFilter::New();
  this->EdgesTuber->SetInputConnection(
    this->Edges->GetOutputPort());
  this->EdgesTuber->SetNumberOfSides(12);
  this->EdgesMapper = vtkPolyDataMapper::New();
  this->EdgesMapper->SetInputConnection(
    this->EdgesTuber->GetOutputPort());
  this->EdgesActor = vtkActor::New();
  this->EdgesActor->SetMapper(this->EdgesMapper);
  this->Tubing = 1; //control whether tubing is on

  // Create the + plane normal
  this->LineSource = vtkLineSource::New();
  this->LineSource->SetResolution(1);
  this->LineMapper = vtkPolyDataMapper::New();
  this->LineMapper->SetInputConnection(
    this->LineSource->GetOutputPort());
  this->LineActor = vtkActor::New();
  this->LineActor->SetMapper(this->LineMapper);

  this->ConeSource = vtkConeSource::New();
  this->ConeSource->SetResolution(12);
  this->ConeSource->SetAngle(25.0);
  this->ConeMapper = vtkPolyDataMapper::New();
  this->ConeMapper->SetInputConnection(
    this->ConeSource->GetOutputPort());
  this->ConeActor = vtkActor::New();
  this->ConeActor->SetMapper(this->ConeMapper);

  // Create the - plane normal
  this->LineSource2 = vtkLineSource::New();
  this->LineSource2->SetResolution(1);
  this->LineMapper2 = vtkPolyDataMapper::New();
  this->LineMapper2->SetInputConnection(
    this->LineSource2->GetOutputPort());
  this->LineActor2 = vtkActor::New();
  this->LineActor2->SetMapper(this->LineMapper2);

  this->ConeSource2 = vtkConeSource::New();
  this->ConeSource2->SetResolution(12);
  this->ConeSource2->SetAngle(25.0);
  this->ConeMapper2 = vtkPolyDataMapper::New();
  this->ConeMapper2->SetInputConnection(
    this->ConeSource2->GetOutputPort());
  this->ConeActor2 = vtkActor::New();
  this->ConeActor2->SetMapper(this->ConeMapper2);

  // Create the origin handle
  this->Sphere = vtkSphereSource::New();
  this->Sphere->SetThetaResolution(16);
  this->Sphere->SetPhiResolution(8);
  this->SphereMapper = vtkPolyDataMapper::New();
  this->SphereMapper->SetInputConnection(
    this->Sphere->GetOutputPort());
  this->SphereActor = vtkActor::New();
  this->SphereActor->SetMapper(this->SphereMapper);
  this->OriginTranslation = 1;

  this->Transform = vtkTransform::New();

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
  this->CreateDefaultProperties();
}

//----------------------------------------------------------------------------
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

  this->NormalProperty->Delete();
  this->SelectedNormalProperty->Delete();
  this->PlaneProperty->Delete();
  this->SelectedPlaneProperty->Delete();
  this->OutlineProperty->Delete();
  this->SelectedOutlineProperty->Delete();
  this->EdgesProperty->Delete();
}

//----------------------------------------------------------------------------
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

    if ( ! this->CurrentRenderer )
    {
      this->SetCurrentRenderer(this->Interactor->FindPokedRenderer(
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
    this->EdgesActor->SetProperty(this->EdgesProperty);

    // add the normal vector
    this->CurrentRenderer->AddActor(this->LineActor);
    this->LineActor->SetProperty(this->NormalProperty);
    this->CurrentRenderer->AddActor(this->ConeActor);
    this->ConeActor->SetProperty(this->NormalProperty);

    this->CurrentRenderer->AddActor(this->LineActor2);
    this->LineActor2->SetProperty(this->NormalProperty);
    this->CurrentRenderer->AddActor(this->ConeActor2);
    this->ConeActor2->SetProperty(this->NormalProperty);

    // add the origin handle
    this->CurrentRenderer->AddActor(this->SphereActor);
    this->SphereActor->SetProperty(this->NormalProperty);

    // add the plane (if desired)
    if ( this->DrawPlane )
    {
      this->CurrentRenderer->AddActor(this->CutActor);
    }
    this->CutActor->SetProperty(this->PlaneProperty);

    this->UpdateRepresentation();
    this->SizeHandles();
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
    this->SetCurrentRenderer(NULL);
  }

  this->Interactor->Render();
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
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

  if ( this->OutlineProperty )
  {
    os << indent << "Outline Property: " << this->OutlineProperty << "\n";
  }
  else
  {
    os << indent << "Outline Property: (none)\n";
  }
  if ( this->SelectedOutlineProperty )
  {
    os << indent << "Selected Outline Property: "
       << this->SelectedOutlineProperty << "\n";
  }
  else
  {
    os << indent << "Selected Outline Property: (none)\n";
  }

  if ( this->EdgesProperty )
  {
    os << indent << "Edges Property: " << this->EdgesProperty << "\n";
  }
  else
  {
    os << indent << "Edges Property: (none)\n";
  }

  os << indent << "Normal To X Axis: "
     << (this->NormalToXAxis ? "On" : "Off") << "\n";
  os << indent << "Normal To Y Axis: "
     << (this->NormalToYAxis ? "On" : "Off") << "\n";
  os << indent << "Normal To Z Axis: "
     << (this->NormalToZAxis ? "On" : "Off") << "\n";

  os << indent << "Tubing: " << (this->Tubing ? "On" : "Off") << "\n";
  os << indent << "Origin Translation: "
     << (this->OriginTranslation ? "On" : "Off") << "\n";
  os << indent << "Outline Translation: "
     << (this->OutlineTranslation ? "On" : "Off") << "\n";
  os << indent << "Outside Bounds: "
     << (this->OutsideBounds ? "On" : "Off") << "\n";
  os << indent << "Scale Enabled: "
     << (this->ScaleEnabled ? "On" : "Off") << "\n";
  os << indent << "Draw Plane: " << (this->DrawPlane ? "On" : "Off") << "\n";

  os << indent << "Diagonal Ratio: " << this->DiagonalRatio << "\n";
}


//----------------------------------------------------------------------------
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


//----------------------------------------------------------------------------
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


//----------------------------------------------------------------------------
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


//----------------------------------------------------------------------------
void vtkImplicitPlaneWidget::OnLeftButtonDown()
{
  // We're only here if we are enabled
  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // Okay, we can process this. See if we've picked anything.
  // Make sure it's in the activated renderer
  if (!this->CurrentRenderer || !this->CurrentRenderer->IsInViewport(X, Y))
  {
    this->State = vtkImplicitPlaneWidget::Outside;
    return;
  }

  vtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->Picker);

  if ( path == NULL ) //not picking this widget
  {
    this->HighlightPlane(0);
    this->HighlightNormal(0);
    this->HighlightOutline(0);
    this->State = vtkImplicitPlaneWidget::Outside;
    return;
  }

  vtkProp *prop = path->GetFirstNode()->GetViewProp();
  this->ValidPick = 1;
  this->Picker->GetPickPosition(this->LastPickPosition);
  if ( prop == this->ConeActor || prop == this->LineActor ||
       prop == this->ConeActor2 || prop == this->LineActor2 )
  {
    this->HighlightPlane(1);
    this->HighlightNormal(1);
    this->State = vtkImplicitPlaneWidget::Rotating;
  }
  else if ( prop == this->CutActor )
  {
    this->HighlightPlane(1);
    this->State = vtkImplicitPlaneWidget::Pushing;
  }
  else if ( prop == this->SphereActor )
  {
    if ( this->OriginTranslation )
    {
      this->HighlightNormal(1);
      this->State = vtkImplicitPlaneWidget::MovingOrigin;
    }
    else
    {
      return;
    }
  }
  else
  {
    if ( this->OutlineTranslation )
    {
      this->HighlightOutline(1);
      this->State = vtkImplicitPlaneWidget::MovingOutline;
    }
    else
    {
      return;
    }
  }

  this->EventCallbackCommand->SetAbortFlag(1);
  this->StartInteraction();
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  this->Interactor->Render();
}

//----------------------------------------------------------------------------
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
  this->SizeHandles();

  this->EventCallbackCommand->SetAbortFlag(1);
  this->EndInteraction();
  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  this->Interactor->Render();
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneWidget::OnMiddleButtonDown()
{
  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // Okay, we can process this. See if we've picked anything.
  // Make sure it's in the activated renderer
  if (!this->CurrentRenderer || !this->CurrentRenderer->IsInViewport(X, Y))
  {
    this->State = vtkImplicitPlaneWidget::Outside;
    return;
  }

  // Okay, we can process this.
  vtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->Picker);

  if ( path == NULL ) //nothing picked
  {
    this->State = vtkImplicitPlaneWidget::Outside;
    return;
  }

  this->ValidPick = 1;
  this->Picker->GetPickPosition(this->LastPickPosition);
  this->State = vtkImplicitPlaneWidget::MovingPlane;
  this->HighlightNormal(1);
  this->HighlightPlane(1);

  this->EventCallbackCommand->SetAbortFlag(1);
  this->StartInteraction();
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  this->Interactor->Render();
}

//----------------------------------------------------------------------------
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
  this->SizeHandles();

  this->EventCallbackCommand->SetAbortFlag(1);
  this->EndInteraction();
  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  this->Interactor->Render();
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneWidget::OnRightButtonDown()
{
  if ( this->ScaleEnabled )
  {
    this->State = vtkImplicitPlaneWidget::Scaling;

    int X = this->Interactor->GetEventPosition()[0];
    int Y = this->Interactor->GetEventPosition()[1];

    // Okay, we can process this. See if we've picked anything.
    // Make sure it's in the activated renderer
    if (!this->CurrentRenderer || !this->CurrentRenderer->IsInViewport(X, Y))
    {
      this->State = vtkImplicitPlaneWidget::Outside;
      return;
    }

    // Okay, we can process this. Try to pick handles first;
    // if no handles picked, then pick the bounding box.
    vtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->Picker);

    if ( path == NULL ) //nothing picked
    {
      this->State = vtkImplicitPlaneWidget::Outside;
      return;
    }

    this->ValidPick = 1;
    this->Picker->GetPickPosition(this->LastPickPosition);
    this->HighlightPlane(1);
    this->HighlightOutline(1);
    this->HighlightNormal(1);

    this->EventCallbackCommand->SetAbortFlag(1);
    this->StartInteraction();
    this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
    this->Interactor->Render();
  }
}

//----------------------------------------------------------------------------
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
  this->SizeHandles();

  this->EventCallbackCommand->SetAbortFlag(1);
  this->EndInteraction();
  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  this->Interactor->Render();
}

//----------------------------------------------------------------------------
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

  vtkCamera *camera = this->CurrentRenderer->GetActiveCamera();
  if ( !camera )
  {
    return;
  }

  // Compute the two points defining the motion vector
  this->ComputeWorldToDisplay(this->LastPickPosition[0], this->LastPickPosition[1],
                              this->LastPickPosition[2], focalPoint);
  z = focalPoint[2];
  this->ComputeDisplayToWorld(double(this->Interactor->GetLastEventPosition()[0]),
                              double(this->Interactor->GetLastEventPosition()[1]),
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

//----------------------------------------------------------------------------
void vtkImplicitPlaneWidget::Rotate(int X, int Y, double *p1, double *p2, double *vpn)
{
  double v[3]; //vector of motion
  double axis[3]; //axis of rotation
  double theta; //rotation angle

  // mouse motion vector in world space
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  double *origin = this->Plane->GetOrigin();
  double *normal = this->Plane->GetNormal();

  // Create axis of rotation and angle of rotation
  vtkMath::Cross(vpn,v,axis);
  if ( vtkMath::Normalize(axis) == 0.0 )
  {
    return;
  }
  int *size = this->CurrentRenderer->GetSize();
  double l2 = (X-this->Interactor->GetLastEventPosition()[0])
             *(X-this->Interactor->GetLastEventPosition()[0])
             +(Y-this->Interactor->GetLastEventPosition()[1])
             *(Y-this->Interactor->GetLastEventPosition()[1]);
  theta = 360.0 * sqrt(l2/(size[0]*size[0]+size[1]*size[1]));

  //Manipulate the transform to reflect the rotation
  this->Transform->Identity();
  this->Transform->Translate(origin[0],origin[1],origin[2]);
  this->Transform->RotateWXYZ(theta,axis);
  this->Transform->Translate(-origin[0],-origin[1],-origin[2]);

  //Set the new normal
  double nNew[3];
  this->Transform->TransformNormal(normal,nNew);
  this->Plane->SetNormal(nNew);

  this->UpdateRepresentation();
}

//----------------------------------------------------------------------------
// Loop through all points and translate them
void vtkImplicitPlaneWidget::TranslatePlane(double *p1, double *p2)
{
  //Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  //Translate the plane
  double oNew[3];
  double *origin = this->Plane->GetOrigin();
  oNew[0] = origin[0] + v[0];
  oNew[1] = origin[1] + v[1];
  oNew[2] = origin[2] + v[2];
  this->Plane->SetOrigin(oNew);

  this->UpdateRepresentation();
}

//----------------------------------------------------------------------------
// Loop through all points and translate them
void vtkImplicitPlaneWidget::TranslateOutline(double *p1, double *p2)
{
  //Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  //Translate the bounding box
  double *origin = this->Box->GetOrigin();
  double oNew[3];
  oNew[0] = origin[0] + v[0];
  oNew[1] = origin[1] + v[1];
  oNew[2] = origin[2] + v[2];
  this->Box->SetOrigin(oNew);

  //Translate the plane
  origin = this->Plane->GetOrigin();
  oNew[0] = origin[0] + v[0];
  oNew[1] = origin[1] + v[1];
  oNew[2] = origin[2] + v[2];
  this->Plane->SetOrigin(oNew);

  this->UpdateRepresentation();
}

//----------------------------------------------------------------------------
// Loop through all points and translate them
void vtkImplicitPlaneWidget::TranslateOrigin(double *p1, double *p2)
{
  //Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  //Add to the current point, project back down onto plane
  double *o = this->Plane->GetOrigin();
  double *n = this->Plane->GetNormal();
  double newOrigin[3];

  newOrigin[0] = o[0] + v[0];
  newOrigin[1] = o[1] + v[1];
  newOrigin[2] = o[2] + v[2];

  vtkPlane::ProjectPoint(newOrigin,o,n,newOrigin);
  this->SetOrigin(newOrigin[0],newOrigin[1],newOrigin[2]);
  this->UpdateRepresentation();
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneWidget::Scale(double *p1, double *p2,
                                   int vtkNotUsed(X), int Y)
{
  //Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  double *o = this->Plane->GetOrigin();

  // Compute the scale factor
  double sf = vtkMath::Norm(v) / this->Outline->GetOutput()->GetLength();
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

  double *origin = this->Box->GetOrigin();
  double *spacing = this->Box->GetSpacing();
  double oNew[3], p[3], pNew[3];
  p[0] = origin[0] + spacing[0];
  p[1] = origin[1] + spacing[1];
  p[2] = origin[2] + spacing[2];

  this->Transform->TransformPoint(origin,oNew);
  this->Transform->TransformPoint(p,pNew);

  this->Box->SetOrigin(oNew);
  this->Box->SetSpacing( (pNew[0]-oNew[0]),
                         (pNew[1]-oNew[1]),
                         (pNew[2]-oNew[2]) );

  this->UpdateRepresentation();
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneWidget::Push(double *p1, double *p2)
{
  //Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  this->Plane->Push( vtkMath::Dot(v,this->Plane->GetNormal()) );
  this->SetOrigin(this->Plane->GetOrigin());
  this->UpdateRepresentation();
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneWidget::CreateDefaultProperties()
{
  // Normal properties
  this->NormalProperty = vtkProperty::New();
  this->NormalProperty->SetColor(1,1,1);
  this->NormalProperty->SetLineWidth(2);

  this->SelectedNormalProperty = vtkProperty::New();
  this->SelectedNormalProperty->SetColor(1,0,0);
  this->NormalProperty->SetLineWidth(2);

  // Plane properties
  this->PlaneProperty = vtkProperty::New();
  this->PlaneProperty->SetAmbient(1.0);
  this->PlaneProperty->SetAmbientColor(1.0,1.0,1.0);

  this->SelectedPlaneProperty = vtkProperty::New();
  this->SelectedPlaneProperty->SetAmbient(1.0);
  this->SelectedPlaneProperty->SetAmbientColor(0.0,1.0,0.0);
  this->SelectedPlaneProperty->SetOpacity(0.25);

  // Outline properties
  this->OutlineProperty = vtkProperty::New();
  this->OutlineProperty->SetAmbient(1.0);
  this->OutlineProperty->SetAmbientColor(1.0,1.0,1.0);

  this->SelectedOutlineProperty = vtkProperty::New();
  this->SelectedOutlineProperty->SetAmbient(1.0);
  this->SelectedOutlineProperty->SetAmbientColor(0.0,1.0,0.0);

  // Edge property
  this->EdgesProperty = vtkProperty::New();
}

//------------------------------------------------------------------------------
void vtkImplicitPlaneWidget::RegisterPickers()
{
  this->Interactor->GetPickingManager()->AddPicker(this->Picker, this);
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneWidget::PlaceWidget(double bds[6])
{
  int i;
  double bounds[6], origin[3];

  this->AdjustBounds(bds, bounds, origin);

  // Set up the bounding box
  this->Box->SetOrigin(bounds[0],bounds[2],bounds[4]);
  this->Box->SetSpacing((bounds[1]-bounds[0]),(bounds[3]-bounds[2]),
                        (bounds[5]-bounds[4]));
  this->Outline->Update();

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

  for (i=0; i<6; i++)
  {
    this->InitialBounds[i] = bounds[i];
  }

  this->InitialLength = sqrt((bounds[1]-bounds[0])*(bounds[1]-bounds[0]) +
                             (bounds[3]-bounds[2])*(bounds[3]-bounds[2]) +
                             (bounds[5]-bounds[4])*(bounds[5]-bounds[4]));

  this->UpdateRepresentation();

  this->SizeHandles();
}

//----------------------------------------------------------------------------
// Description:
// Set the origin of the plane.
void vtkImplicitPlaneWidget::SetOrigin(double x, double y, double z)
{
  double origin[3];
  origin[0] = x;
  origin[1] = y;
  origin[2] = z;
  this->SetOrigin(origin);
}

//----------------------------------------------------------------------------
// Description:
// Set the origin of the plane.
void vtkImplicitPlaneWidget::SetOrigin(double x[3])
{
  const double *bounds = this->Outline->GetOutput()->GetBounds();
  for (int i=0; i<3; i++)
  {
    if ( x[i] < bounds[2*i] )
    {
      x[i] = bounds[2*i];
    }
    else if ( x[i] > bounds[2*i+1] )
    {
      x[i] = bounds[2*i+1];
    }
  }
  this->Plane->SetOrigin(x);
  this->UpdateRepresentation();
}

//----------------------------------------------------------------------------
// Description:
// Get the origin of the plane.
double* vtkImplicitPlaneWidget::GetOrigin()
{
  return this->Plane->GetOrigin();
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneWidget::GetOrigin(double xyz[3])
{
  this->Plane->GetOrigin(xyz);
}

//----------------------------------------------------------------------------
// Description:
// Set the normal to the plane.
void vtkImplicitPlaneWidget::SetNormal(double x, double y, double z)
{
  double n[3];
  n[0] = x;
  n[1] = y;
  n[2] = z;
  vtkMath::Normalize(n);
  this->Plane->SetNormal(n);
  this->UpdateRepresentation();
}

//----------------------------------------------------------------------------
// Description:
// Set the normal to the plane.
void vtkImplicitPlaneWidget::SetNormal(double n[3])
{
  this->SetNormal(n[0], n[1], n[2]);
}

//----------------------------------------------------------------------------
// Description:
// Get the normal to the plane.
double* vtkImplicitPlaneWidget::GetNormal()
{
  return this->Plane->GetNormal();
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneWidget::GetNormal(double xyz[3])
{
  this->Plane->GetNormal(xyz);
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneWidget::SetDrawPlane(int drawPlane)
{
  if ( drawPlane == this->DrawPlane )
  {
    return;
  }

  this->Modified();
  this->DrawPlane = drawPlane;
  if ( this->Enabled )
  {
    if ( drawPlane )
    {
      this->CurrentRenderer->AddActor(this->CutActor);
    }
    else
    {
      this->CurrentRenderer->RemoveActor(this->CutActor);
    }
    this->Interactor->Render();
  }
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneWidget::SetNormalToXAxis (int var)
{
  if (this->NormalToXAxis != var)
  {
    this->NormalToXAxis = var;
    this->Modified();
  }
  if (var)
  {
    this->NormalToYAxisOff();
    this->NormalToZAxisOff();
  }
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneWidget::SetNormalToYAxis (int var)
{
  if (this->NormalToYAxis != var)
  {
    this->NormalToYAxis = var;
    this->Modified();
  }
  if (var)
  {
    this->NormalToXAxisOff();
    this->NormalToZAxisOff();
  }
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneWidget::SetNormalToZAxis (int var)
{
  if (this->NormalToZAxis != var)
  {
    this->NormalToZAxis = var;
    this->Modified();
  }
  if (var)
  {
    this->NormalToXAxisOff();
    this->NormalToYAxisOff();
  }
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneWidget::GetPolyData(vtkPolyData *pd)
{
  pd->ShallowCopy(this->Cutter->GetOutput());
}

//----------------------------------------------------------------------------
vtkPolyDataAlgorithm *vtkImplicitPlaneWidget::GetPolyDataAlgorithm()
{
  return this->Cutter;
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneWidget::GetPlane(vtkPlane *plane)
{
  if ( plane == NULL )
  {
    return;
  }

  plane->SetNormal(this->Plane->GetNormal());
  plane->SetOrigin(this->Plane->GetOrigin());
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneWidget::UpdatePlacement()
{
  this->Outline->Update();
  this->Cutter->Update();
  this->Edges->Update();
  this->UpdateRepresentation();
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneWidget::UpdateRepresentation()
{
  if ( ! this->CurrentRenderer )
  {
    return;
  }

  double *origin = this->Plane->GetOrigin();
  double *normal = this->Plane->GetNormal();
  double p2[3];
  if( !this->OutsideBounds )
  {
    const double *bounds = this->GetInput()->GetBounds();
    for (int i=0; i<3; i++)
    {
      if ( origin[i] < bounds[2*i] )
      {
        origin[i] = bounds[2*i];
      }
      else if ( origin[i] > bounds[2*i+1] )
      {
        origin[i] = bounds[2*i+1];
      }
    }
  }

  // Setup the plane normal
  double d = this->Outline->GetOutput()->GetLength();

  const double ratio = this->DiagonalRatio;
  p2[0] = origin[0] + ratio * d * normal[0];
  p2[1] = origin[1] + ratio * d * normal[1];
  p2[2] = origin[2] + ratio * d * normal[2];

  this->LineSource->SetPoint1(origin);
  this->LineSource->SetPoint2(p2);
  this->ConeSource->SetCenter(p2);
  this->ConeSource->SetDirection(normal);

  p2[0] = origin[0] - ratio * d * normal[0];
  p2[1] = origin[1] - ratio * d * normal[1];
  p2[2] = origin[2] - ratio * d * normal[2];

  this->LineSource2->SetPoint1(origin[0],origin[1],origin[2]);
  this->LineSource2->SetPoint2(p2);
  this->ConeSource2->SetCenter(p2);
  this->ConeSource2->SetDirection(normal[0],normal[1],normal[2]);

  // Set up the position handle
  this->Sphere->SetCenter(origin[0],origin[1],origin[2]);

  // Control the look of the edges
  if ( this->Tubing )
  {
    this->EdgesMapper->SetInputConnection(
      this->EdgesTuber->GetOutputPort());
  }
  else
  {
    this->EdgesMapper->SetInputConnection(
      this->Edges->GetOutputPort());
  }
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneWidget::SizeHandles()
{
  double radius = this->vtk3DWidget::SizeHandles(1.35);

  this->ConeSource->SetHeight(2.0*radius);
  this->ConeSource->SetRadius(radius);
  this->ConeSource2->SetHeight(2.0*radius);
  this->ConeSource2->SetRadius(radius);

  this->Sphere->SetRadius(radius);

  this->EdgesTuber->SetRadius(0.25*radius);
}
