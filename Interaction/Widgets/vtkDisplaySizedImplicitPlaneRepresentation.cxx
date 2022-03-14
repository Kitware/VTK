/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDisplaySizedImplicitPlaneRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDisplaySizedImplicitPlaneRepresentation.h"

#include "vtkActor.h"
#include "vtkAssemblyPath.h"
#include "vtkBox.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCellPicker.h"
#include "vtkConeSource.h"
#include "vtkCutter.h"
#include "vtkDiskSource.h"
#include "vtkEventData.h"
#include "vtkFeatureEdges.h"
#include "vtkHardwarePicker.h"
#include "vtkImageData.h"
#include "vtkInteractorObserver.h"
#include "vtkLineSource.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkOutlineFilter.h"
#include "vtkPickingManager.h"
#include "vtkPlane.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkTransform.h"
#include "vtkTubeFilter.h"
#include "vtkWindow.h"

#include <cfloat> //for FLT_EPSILON

vtkStandardNewMacro(vtkDisplaySizedImplicitPlaneRepresentation);

static constexpr double DefaultPickTol = 0.001;

//------------------------------------------------------------------------------
vtkDisplaySizedImplicitPlaneRepresentation::vtkDisplaySizedImplicitPlaneRepresentation()
{
  this->NormalToXAxis = 0;
  this->NormalToYAxis = 0;
  this->NormalToZAxis = 0;

  this->SnappedOrientation = false;
  this->SnapToAxes = false;
  this->PickCameraFocalInfo = false;

  this->LockNormalToCamera = 0;

  // Handle size is in pixels for this widget
  this->HandleSize = 5.0;

  // Pushing operation
  this->BumpDistance = 0.01;

  // Build the representation of the widget
  this->Plane->SetNormal(0, 0, 1);
  this->Plane->SetOrigin(0, 0, 0);

  this->Box->SetDimensions(2, 2, 2);
  this->Outline->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
  this->Outline->SetInputData(this->Box);
  this->OutlineMapper->SetInputConnection(this->Outline->GetOutputPort());
  this->OutlineActor->SetMapper(this->OutlineMapper);
  this->OutlineTranslation = 1;
  this->ScaleEnabled = 1;
  this->OutsideBounds = 1;
  this->ConstrainToWidgetBounds = 0;
  this->ConstrainMaximumSizeToWidgetBounds = 0;
  this->DrawOutline = 0;

  this->RadiusMultiplier = 1.0;
  this->DiskPlaneSource->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
  this->DiskPlaneSource->SetCircumferentialResolution(64);
  this->DiskPlaneSource->SetInnerRadius(0);
  this->PlaneMapper->SetInputConnection(this->DiskPlaneSource->GetOutputPort());
  this->PlaneActor->SetMapper(this->PlaneMapper);
  this->DrawPlane = 1;

  this->Edges->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
  this->Edges->SetInputConnection(this->DiskPlaneSource->GetOutputPort());
  this->EdgesTuber->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
  this->EdgesTuber->SetInputConnection(this->Edges->GetOutputPort());
  this->EdgesTuber->SetNumberOfSides(12);
  this->EdgesMapper->SetInputConnection(this->EdgesTuber->GetOutputPort());
  this->EdgesActor->SetMapper(this->EdgesMapper);

  this->Cutter->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
  this->Cutter->SetInputData(this->Box);
  this->Cutter->SetCutFunction(this->Plane);
  this->IntersectionEdges->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
  this->IntersectionEdges->SetInputConnection(this->Cutter->GetOutputPort());
  this->IntersectionEdgesTuber->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
  this->IntersectionEdgesTuber->SetInputConnection(this->IntersectionEdges->GetOutputPort());
  this->IntersectionEdgesTuber->SetNumberOfSides(12);
  this->IntersectionEdgesMapper->SetInputConnection(this->IntersectionEdgesTuber->GetOutputPort());
  this->IntersectionEdgesActor->SetMapper(this->IntersectionEdgesMapper);
  this->DrawIntersectionEdges = 0;

  // Create the +- plane normal
  this->LineSource->SetResolution(1);
  this->LineSource->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
  this->LineMapper->SetInputConnection(this->LineSource->GetOutputPort());
  this->LineActor->SetMapper(this->LineMapper);

  this->ConeSource->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
  this->ConeSource->SetResolution(12);
  this->ConeSource->SetAngle(25.0);
  this->ConeMapper->SetInputConnection(this->ConeSource->GetOutputPort());
  this->ConeActor->SetMapper(this->ConeMapper);

  this->ConeSource2->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
  this->ConeSource2->SetResolution(12);
  this->ConeSource2->SetAngle(25.0);
  this->ConeMapper2->SetInputConnection(this->ConeSource2->GetOutputPort());
  this->ConeActor2->SetMapper(this->ConeMapper2);

  // Create the origin handle
  this->Sphere->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
  this->Sphere->SetThetaResolution(16);
  this->Sphere->SetPhiResolution(8);
  this->SphereMapper->SetInputConnection(this->Sphere->GetOutputPort());
  this->SphereActor->SetMapper(this->SphereMapper);

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

  // Manage the picking stuff
  this->CellPicker->SetTolerance(DefaultPickTol);
  this->CellPicker->AddPickList(this->OutlineActor);
  this->CellPicker->AddPickList(this->PlaneActor);
  this->CellPicker->AddPickList(this->EdgesActor);
  this->CellPicker->AddPickList(this->LineActor);
  this->CellPicker->AddPickList(this->ConeActor);
  this->CellPicker->AddPickList(this->ConeActor2);
  this->CellPicker->AddPickList(this->SphereActor);
  this->CellPicker->PickFromListOn();
  this->HardwarePicker->PickFromListOff();

  // Set up the initial properties
  this->CreateDefaultProperties();

  // Pass the initial properties to the actors.
  this->OutlineActor->SetProperty(this->OutlineProperty);
  this->LineActor->SetProperty(this->NormalProperty);
  this->ConeActor->SetProperty(this->NormalProperty);
  this->ConeActor2->SetProperty(this->NormalProperty);
  this->SphereActor->SetProperty(this->SphereProperty);
  this->PlaneActor->SetProperty(this->PlaneProperty);
  this->HighlightEdges(0);
  this->IntersectionEdgesActor->SetProperty(this->IntersectionEdgesProperty);
  this->SetIntersectionEdgesColor(this->IntersectionEdgesActor->GetProperty()->GetColor());

  this->RepresentationState = vtkDisplaySizedImplicitPlaneRepresentation::Outside;

  this->TranslationAxis = Axis::NONE;
  this->AlwaysSnapToNearestAxis = false;
}

//------------------------------------------------------------------------------
vtkDisplaySizedImplicitPlaneRepresentation::~vtkDisplaySizedImplicitPlaneRepresentation() = default;

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::SetLockNormalToCamera(vtkTypeBool lock)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting " << LockNormalToCamera
                << " to " << lock);
  if (lock == this->LockNormalToCamera)
  {
    return;
  }

  if (lock)
  {
    this->CellPicker->DeletePickList(this->LineActor);
    this->CellPicker->DeletePickList(this->ConeActor);
    this->CellPicker->DeletePickList(this->ConeActor2);
    this->CellPicker->DeletePickList(this->SphereActor);

    this->SetNormalToCamera();
  }
  else
  {
    this->CellPicker->AddPickList(this->LineActor);
    this->CellPicker->AddPickList(this->ConeActor);
    this->CellPicker->AddPickList(this->ConeActor2);
    this->CellPicker->AddPickList(this->SphereActor);
  }

  this->LockNormalToCamera = lock;
  this->Modified();
}

//------------------------------------------------------------------------------
int vtkDisplaySizedImplicitPlaneRepresentation::ComputeInteractionState(
  int X, int Y, int vtkNotUsed(modify))
{
  // See if anything has been selected
  this->ComputeAdaptivePickerTolerance();
  vtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->CellPicker);

  if (path == nullptr) // Not picking this widget
  {
    this->SetRepresentationState(vtkDisplaySizedImplicitPlaneRepresentation::Outside);
    this->InteractionState = vtkDisplaySizedImplicitPlaneRepresentation::Outside;
    return this->InteractionState;
  }

  // Something picked, continue
  this->ValidPick = 1;

  // Depending on the interaction state (set by the widget) we modify
  // this state based on what is picked.
  if (this->InteractionState == vtkDisplaySizedImplicitPlaneRepresentation::Moving)
  {
    vtkProp* prop = path->GetFirstNode()->GetViewProp();
    if (prop == this->ConeActor || prop == this->LineActor || prop == this->ConeActor2)
    {
      this->InteractionState = vtkDisplaySizedImplicitPlaneRepresentation::Rotating;
      this->SetRepresentationState(vtkDisplaySizedImplicitPlaneRepresentation::Rotating);
    }
    else if (prop == this->EdgesActor)
    {
      this->InteractionState = vtkDisplaySizedImplicitPlaneRepresentation::ResizeDiskRadius;
      this->SetRepresentationState(vtkDisplaySizedImplicitPlaneRepresentation::ResizeDiskRadius);
    }
    else if (prop == this->PlaneActor)
    {
      if (this->LockNormalToCamera)
      { // Allow camera to work
        this->InteractionState = vtkDisplaySizedImplicitPlaneRepresentation::Outside;
        this->SetRepresentationState(vtkDisplaySizedImplicitPlaneRepresentation::Outside);
      }
      else
      {
        this->InteractionState = vtkDisplaySizedImplicitPlaneRepresentation::Pushing;
        this->SetRepresentationState(vtkDisplaySizedImplicitPlaneRepresentation::Pushing);
      }
    }
    else if (prop == this->SphereActor)
    {
      this->InteractionState = vtkDisplaySizedImplicitPlaneRepresentation::MovingOrigin;
      this->SetRepresentationState(vtkDisplaySizedImplicitPlaneRepresentation::MovingOrigin);
    }
    else
    {
      if (this->OutlineTranslation)
      {
        this->InteractionState = vtkDisplaySizedImplicitPlaneRepresentation::MovingOutline;
        this->SetRepresentationState(vtkDisplaySizedImplicitPlaneRepresentation::MovingOutline);
      }
      else
      {
        this->InteractionState = vtkDisplaySizedImplicitPlaneRepresentation::Outside;
        this->SetRepresentationState(vtkDisplaySizedImplicitPlaneRepresentation::Outside);
      }
    }
  }

  // We may add a condition to allow the camera to work IO scaling
  else if (this->InteractionState != vtkDisplaySizedImplicitPlaneRepresentation::Scaling)
  {
    this->InteractionState = vtkDisplaySizedImplicitPlaneRepresentation::Outside;
  }

  return this->InteractionState;
}

//------------------------------------------------------------------------------
int vtkDisplaySizedImplicitPlaneRepresentation::ComputeComplexInteractionState(
  vtkRenderWindowInteractor*, vtkAbstractWidget*, unsigned long, void* calldata, int)
{
  this->ComputeAdaptivePickerTolerance();
  vtkEventData* edata = static_cast<vtkEventData*>(calldata);
  vtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();
  if (edd)
  {
    double pos[3];
    edd->GetWorldPosition(pos);
    if (this->DrawOutline)
    {
      this->CellPicker->DeletePickList(this->OutlineActor);
    }
    vtkAssemblyPath* path = this->GetAssemblyPath3DPoint(pos, this->CellPicker);
    if (this->DrawOutline)
    {
      this->CellPicker->AddPickList(this->OutlineActor);
      if (path == nullptr)
      {
        path = this->GetAssemblyPath3DPoint(pos, this->CellPicker);
      }
    }

    if (path == nullptr) // Not picking this widget
    {
      this->SetRepresentationState(vtkDisplaySizedImplicitPlaneRepresentation::Outside);
      this->InteractionState = vtkDisplaySizedImplicitPlaneRepresentation::Outside;
      return this->InteractionState;
    }

    // Something picked, continue
    this->ValidPick = 1;

    // Depending on the interaction state (set by the widget) we modify
    // this state based on what is picked.
    if (this->InteractionState == vtkDisplaySizedImplicitPlaneRepresentation::Moving)
    {
      vtkProp* prop = path->GetFirstNode()->GetViewProp();
      if (prop == this->ConeActor || prop == this->LineActor || prop == this->ConeActor2)
      {
        this->InteractionState = vtkDisplaySizedImplicitPlaneRepresentation::Rotating;
        this->SetRepresentationState(vtkDisplaySizedImplicitPlaneRepresentation::Rotating);
      }
      else if (prop == this->EdgesActor)
      {
        this->InteractionState = vtkDisplaySizedImplicitPlaneRepresentation::ResizeDiskRadius;
        this->SetRepresentationState(vtkDisplaySizedImplicitPlaneRepresentation::ResizeDiskRadius);
      }
      else if (prop == this->PlaneActor)
      {
        if (this->LockNormalToCamera)
        { // Allow camera to work
          this->InteractionState = vtkDisplaySizedImplicitPlaneRepresentation::Outside;
          this->SetRepresentationState(vtkDisplaySizedImplicitPlaneRepresentation::Outside);
        }
        else
        {
          this->InteractionState = vtkDisplaySizedImplicitPlaneRepresentation::Pushing;
          this->SetRepresentationState(vtkDisplaySizedImplicitPlaneRepresentation::Pushing);
        }
      }
      else if (prop == this->SphereActor)
      {
        this->InteractionState = vtkDisplaySizedImplicitPlaneRepresentation::MovingOrigin;
        this->SetRepresentationState(vtkDisplaySizedImplicitPlaneRepresentation::MovingOrigin);
      }
      else
      {
        if (this->OutlineTranslation)
        {
          this->InteractionState = vtkDisplaySizedImplicitPlaneRepresentation::MovingOutline;
          this->SetRepresentationState(vtkDisplaySizedImplicitPlaneRepresentation::MovingOutline);
        }
        else
        {
          this->InteractionState = vtkDisplaySizedImplicitPlaneRepresentation::Outside;
          this->SetRepresentationState(vtkDisplaySizedImplicitPlaneRepresentation::Outside);
        }
      }
    }
    // We may add a condition to allow the camera to work IO scaling
    else if (this->InteractionState != vtkDisplaySizedImplicitPlaneRepresentation::Scaling)
    {
      this->InteractionState = vtkDisplaySizedImplicitPlaneRepresentation::Outside;
    }
  }

  return this->InteractionState;
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::SetRepresentationState(int state)
{
  if (this->RepresentationState == state)
  {
    return;
  }

  // Clamp the state
  state = (state < vtkDisplaySizedImplicitPlaneRepresentation::Outside
      ? vtkDisplaySizedImplicitPlaneRepresentation::Outside
      : (state > vtkDisplaySizedImplicitPlaneRepresentation::Scaling
            ? vtkDisplaySizedImplicitPlaneRepresentation::Scaling
            : state));

  this->RepresentationState = state;
  this->Modified();

  if (state == vtkDisplaySizedImplicitPlaneRepresentation::Rotating)
  {
    this->HighlightOutline(0);
    this->HighlightNormal(1);
    this->HighlightSphere(0);
    this->HighlightPlane(1);
    this->HighlightEdges(0);
  }
  else if (state == vtkDisplaySizedImplicitPlaneRepresentation::Pushing)
  {
    this->HighlightOutline(0);
    this->HighlightNormal(0);
    this->HighlightSphere(0);
    this->HighlightPlane(1);
    this->HighlightEdges(0);
  }
  else if (state == vtkDisplaySizedImplicitPlaneRepresentation::MovingOrigin)
  {
    this->HighlightOutline(0);
    this->HighlightNormal(0);
    this->HighlightSphere(1);
    this->HighlightPlane(1);
    this->HighlightEdges(0);
  }
  else if (state == vtkDisplaySizedImplicitPlaneRepresentation::MovingOutline)
  {
    this->HighlightOutline(1);
    this->HighlightNormal(0);
    this->HighlightSphere(0);
    this->HighlightPlane(0);
    this->HighlightEdges(0);
  }
  else if (state == vtkDisplaySizedImplicitPlaneRepresentation::ResizeDiskRadius)
  {
    this->HighlightOutline(0);
    this->HighlightNormal(0);
    this->HighlightSphere(0);
    this->HighlightPlane(1);
    this->HighlightEdges(1);
  }
  else if (state == vtkDisplaySizedImplicitPlaneRepresentation::Scaling && this->ScaleEnabled)
  {
    this->HighlightOutline(0);
    this->HighlightNormal(1);
    this->HighlightSphere(1);
    this->HighlightPlane(1);
    this->HighlightEdges(1);
  }
  else
  {
    this->HighlightOutline(0);
    this->HighlightNormal(0);
    this->HighlightSphere(0);
    this->HighlightPlane(0);
    this->HighlightEdges(0);
  }
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::StartWidgetInteraction(double e[2])
{
  this->StartEventPosition[0] = e[0];
  this->StartEventPosition[1] = e[1];
  this->StartEventPosition[2] = 0.0;

  this->LastEventPosition[0] = e[0];
  this->LastEventPosition[1] = e[1];
  this->LastEventPosition[2] = 0.0;
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::StartComplexInteraction(
  vtkRenderWindowInteractor*, vtkAbstractWidget*, unsigned long, void* calldata)
{
  vtkEventData* edata = static_cast<vtkEventData*>(calldata);
  vtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();
  if (edd)
  {
    edd->GetWorldPosition(this->StartEventPosition);
    this->LastEventPosition[0] = this->StartEventPosition[0];
    this->LastEventPosition[1] = this->StartEventPosition[1];
    this->LastEventPosition[2] = this->StartEventPosition[2];
    edd->GetWorldOrientation(this->StartEventOrientation);
    std::copy(
      this->StartEventOrientation, this->StartEventOrientation + 4, this->LastEventOrientation);
    if (this->SnappedOrientation)
    {
      std::copy(this->StartEventOrientation, this->StartEventOrientation + 4,
        this->SnappedEventOrientation);
    }
  }
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::WidgetInteraction(double e[2])
{
  // Do different things depending on state
  // Calculations everybody does
  double focalPoint[4], pickPoint[4], prevPickPoint[4];
  double z, vpn[3];

  vtkCamera* camera = this->Renderer->GetActiveCamera();
  if (!camera)
  {
    return;
  }

  // Compute the two points defining the motion vector
  double pos[3];
  this->CellPicker->GetPickPosition(pos);
  vtkInteractorObserver::ComputeWorldToDisplay(this->Renderer, pos[0], pos[1], pos[2], focalPoint);
  z = focalPoint[2];
  vtkInteractorObserver::ComputeDisplayToWorld(
    this->Renderer, this->LastEventPosition[0], this->LastEventPosition[1], z, prevPickPoint);
  vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer, e[0], e[1], z, pickPoint);

  // Process the motion
  if (this->InteractionState == vtkDisplaySizedImplicitPlaneRepresentation::MovingOutline)
  {
    this->TranslateOutline(prevPickPoint, pickPoint);
  }
  else if (this->InteractionState == vtkDisplaySizedImplicitPlaneRepresentation::MovingOrigin)
  {
    this->TranslateOrigin(prevPickPoint, pickPoint);
  }
  else if (this->InteractionState == vtkDisplaySizedImplicitPlaneRepresentation::ResizeDiskRadius)
  {
    camera->GetViewPlaneNormal(vpn);
    this->ResizeRadius(prevPickPoint, pickPoint, vpn);
  }
  else if (this->InteractionState == vtkDisplaySizedImplicitPlaneRepresentation::Pushing)
  {
    this->Push(prevPickPoint, pickPoint);
  }
  else if (this->InteractionState == vtkDisplaySizedImplicitPlaneRepresentation::Scaling &&
    this->ScaleEnabled)
  {
    this->Scale(prevPickPoint, pickPoint, e[0], e[1]);
  }
  else if (this->InteractionState == vtkDisplaySizedImplicitPlaneRepresentation::Rotating)
  {
    camera->GetViewPlaneNormal(vpn);
    this->Rotate(e[0], e[1], prevPickPoint, pickPoint, vpn);
  }
  else if (this->InteractionState == vtkDisplaySizedImplicitPlaneRepresentation::Outside &&
    this->LockNormalToCamera)
  {
    this->SetNormalToCamera();
  }

  this->LastEventPosition[0] = e[0];
  this->LastEventPosition[1] = e[1];
  this->LastEventPosition[2] = 0.0;
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::ComplexInteraction(
  vtkRenderWindowInteractor*, vtkAbstractWidget*, unsigned long, void* calldata)
{
  vtkEventData* edata = static_cast<vtkEventData*>(calldata);
  vtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();
  if (edd)
  {
    double eventPos[3];
    edd->GetWorldPosition(eventPos);
    double eventDir[4];
    edd->GetWorldOrientation(eventDir);

    // Process the motion
    if (this->InteractionState == vtkDisplaySizedImplicitPlaneRepresentation::MovingOutline)
    {
      this->UpdatePose(this->LastEventPosition, this->LastEventOrientation, eventPos, eventDir);
    }
    else if (this->InteractionState == vtkDisplaySizedImplicitPlaneRepresentation::MovingOrigin)
    {
      this->UpdatePose(this->LastEventPosition, this->LastEventOrientation, eventPos, eventDir);
    }
    else if (this->InteractionState == vtkDisplaySizedImplicitPlaneRepresentation::ResizeDiskRadius)
    {
      this->ResizeRadius3D(this->LastEventPosition, eventPos);
    }
    else if (this->InteractionState == vtkDisplaySizedImplicitPlaneRepresentation::Pushing)
    {
      this->UpdatePose(this->LastEventPosition, this->LastEventOrientation, eventPos, eventDir);
    }
    else if (this->InteractionState == vtkDisplaySizedImplicitPlaneRepresentation::Scaling &&
      this->ScaleEnabled)
    {
      this->Scale(this->LastEventPosition, eventPos, 0.0, 0.0);
    }
    else if (this->InteractionState == vtkDisplaySizedImplicitPlaneRepresentation::Rotating)
    {
      this->Rotate3D(this->LastEventPosition, eventPos);
    }
    else if (this->InteractionState == vtkDisplaySizedImplicitPlaneRepresentation::Outside &&
      this->LockNormalToCamera)
    {
      this->SetNormalToCamera();
    }

    // Book keeping
    this->LastEventPosition[0] = eventPos[0];
    this->LastEventPosition[1] = eventPos[1];
    this->LastEventPosition[2] = eventPos[2];
    std::copy(eventDir, eventDir + 4, this->LastEventOrientation);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::EndWidgetInteraction(double vtkNotUsed(e)[2])
{
  this->SetRepresentationState(vtkDisplaySizedImplicitPlaneRepresentation::Outside);
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::EndComplexInteraction(
  vtkRenderWindowInteractor*, vtkAbstractWidget*, unsigned long, void*)
{
  this->SetRepresentationState(vtkDisplaySizedImplicitPlaneRepresentation::Outside);
}

//------------------------------------------------------------------------------
double* vtkDisplaySizedImplicitPlaneRepresentation::GetBounds()
{
  this->BuildRepresentation();
  if (this->ConstrainMaximumSizeToWidgetBounds)
  {
    return this->WidgetBounds;
  }
  else
  {
    // bounds need to be reset because the size of the widget changes overtime
    this->BoundingBox->SetBounds(VTK_DOUBLE_MAX, VTK_DOUBLE_MIN, VTK_DOUBLE_MAX, VTK_DOUBLE_MIN,
      VTK_DOUBLE_MAX, VTK_DOUBLE_MIN);
    this->BoundingBox->AddBounds(this->OutlineActor->GetBounds());
    this->BoundingBox->AddBounds(this->PlaneActor->GetBounds());
    this->BoundingBox->AddBounds(this->EdgesActor->GetBounds());
    this->BoundingBox->AddBounds(this->IntersectionEdgesActor->GetBounds());
    this->BoundingBox->AddBounds(this->ConeActor->GetBounds());
    this->BoundingBox->AddBounds(this->LineActor->GetBounds());
    this->BoundingBox->AddBounds(this->ConeActor2->GetBounds());
    this->BoundingBox->AddBounds(this->SphereActor->GetBounds());
    return this->BoundingBox->GetBounds();
  }
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::GetActors(vtkPropCollection* pc)
{
  pc->AddItem(this->OutlineActor);
  pc->AddItem(this->PlaneActor);
  pc->AddItem(this->EdgesActor);
  pc->AddItem(this->IntersectionEdgesActor);
  pc->AddItem(this->ConeActor);
  pc->AddItem(this->LineActor);
  pc->AddItem(this->ConeActor2);
  pc->AddItem(this->SphereActor);
  this->Superclass::GetActors(pc);
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::ReleaseGraphicsResources(vtkWindow* w)
{
  this->OutlineActor->ReleaseGraphicsResources(w);
  this->PlaneActor->ReleaseGraphicsResources(w);
  this->EdgesActor->ReleaseGraphicsResources(w);
  this->IntersectionEdgesActor->ReleaseGraphicsResources(w);
  this->ConeActor->ReleaseGraphicsResources(w);
  this->LineActor->ReleaseGraphicsResources(w);
  this->ConeActor2->ReleaseGraphicsResources(w);
  this->SphereActor->ReleaseGraphicsResources(w);
}

//------------------------------------------------------------------------------
int vtkDisplaySizedImplicitPlaneRepresentation::RenderOpaqueGeometry(vtkViewport* v)
{
  int count = 0;
  this->BuildRepresentation();
  if (this->DrawOutline)
  {
    count += this->OutlineActor->RenderOpaqueGeometry(v);
    if (this->DrawIntersectionEdges)
    {
      count += this->IntersectionEdgesActor->RenderOpaqueGeometry(v);
    }
  }
  if (!this->LockNormalToCamera)
  {
    count += this->ConeActor->RenderOpaqueGeometry(v);
    count += this->LineActor->RenderOpaqueGeometry(v);
    count += this->ConeActor2->RenderOpaqueGeometry(v);
    count += this->SphereActor->RenderOpaqueGeometry(v);
  }
  count += this->EdgesActor->RenderOpaqueGeometry(v);
  if (this->DrawPlane)
  {
    count += this->PlaneActor->RenderOpaqueGeometry(v);
  }

  return count;
}

//------------------------------------------------------------------------------
int vtkDisplaySizedImplicitPlaneRepresentation::RenderTranslucentPolygonalGeometry(vtkViewport* v)
{
  int count = 0;
  this->BuildRepresentation();
  if (this->DrawOutline)
  {
    count += this->OutlineActor->RenderTranslucentPolygonalGeometry(v);
    if (this->DrawIntersectionEdges)
    {
      count += this->IntersectionEdgesActor->RenderTranslucentPolygonalGeometry(v);
    }
  }
  if (!this->LockNormalToCamera)
  {
    count += this->ConeActor->RenderTranslucentPolygonalGeometry(v);
    count += this->LineActor->RenderTranslucentPolygonalGeometry(v);
    count += this->ConeActor2->RenderTranslucentPolygonalGeometry(v);
    count += this->SphereActor->RenderTranslucentPolygonalGeometry(v);
  }
  count += this->EdgesActor->RenderTranslucentPolygonalGeometry(v);
  if (this->DrawPlane)
  {
    count += this->PlaneActor->RenderTranslucentPolygonalGeometry(v);
  }

  return count;
}

//------------------------------------------------------------------------------
vtkTypeBool vtkDisplaySizedImplicitPlaneRepresentation::HasTranslucentPolygonalGeometry()
{
  int result = 0;
  if (this->DrawOutline)
  {
    result |= this->OutlineActor->HasTranslucentPolygonalGeometry();
    if (this->DrawIntersectionEdges)
    {
      result |= this->IntersectionEdgesActor->HasTranslucentPolygonalGeometry();
    }
  }
  if (!this->LockNormalToCamera)
  {
    result |= this->ConeActor->HasTranslucentPolygonalGeometry();
    result |= this->LineActor->HasTranslucentPolygonalGeometry();
    result |= this->ConeActor2->HasTranslucentPolygonalGeometry();
    result |= this->SphereActor->HasTranslucentPolygonalGeometry();
  }
  result |= this->EdgesActor->HasTranslucentPolygonalGeometry();
  if (this->DrawPlane)
  {
    result |= this->PlaneActor->HasTranslucentPolygonalGeometry();
  }

  return result;
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Snap To Axes: " << (this->SnapToAxes ? "On\n" : "Off\n");

  os << indent << "Normal Property: " << this->NormalProperty << "\n";
  os << indent << "Selected Normal Property: " << this->SelectedNormalProperty << "\n";
  os << indent << "Sphere Property: " << this->SphereProperty << "\n";
  os << indent << "Selected Sphere Property: " << this->SelectedSphereProperty << "\n";
  os << indent << "Plane Property: " << this->PlaneProperty << "\n";
  os << indent << "Selected Plane Property: " << this->SelectedPlaneProperty << "\n";
  os << indent << "Edges Property: " << this->EdgesProperty << "\n";
  os << indent << "Selected Edges Property: " << this->SelectedEdgesProperty << "\n";
  os << indent << "Intersection Edges Property: " << this->IntersectionEdgesProperty << "\n";
  os << indent << "Outline Property: " << this->OutlineProperty << "\n";
  os << indent << "Selected Outline Property: " << this->SelectedOutlineProperty << "\n";

  os << indent << "Normal To X Axis: " << (this->NormalToXAxis ? "On" : "Off") << "\n";
  os << indent << "Normal To Y Axis: " << (this->NormalToYAxis ? "On" : "Off") << "\n";
  os << indent << "Normal To Z Axis: " << (this->NormalToZAxis ? "On" : "Off") << "\n";
  os << indent << "Lock Normal To Camera: " << (this->LockNormalToCamera ? "On" : "Off") << "\n";

  os << indent << "Widget Bounds: " << this->WidgetBounds[0] << ", " << this->WidgetBounds[1]
     << ", " << this->WidgetBounds[2] << ", " << this->WidgetBounds[3] << ", "
     << this->WidgetBounds[4] << ", " << this->WidgetBounds[5] << "\n";

  os << indent << "Outline Translation: " << (this->OutlineTranslation ? "On" : "Off") << "\n";
  os << indent << "Outside Bounds: " << (this->OutsideBounds ? "On" : "Off") << "\n";
  os << indent << "Constrain to Widget Bounds: " << (this->ConstrainToWidgetBounds ? "On" : "Off")
     << "\n";
  os << indent << "Constrain Maximum Size to Widget Bounds: "
     << (this->ConstrainMaximumSizeToWidgetBounds ? "On" : "Off") << "\n";
  os << indent << "Scale Enabled: " << (this->ScaleEnabled ? "On" : "Off") << "\n";
  os << indent << "Draw Plane: " << (this->DrawPlane ? "On" : "Off") << "\n";
  os << indent << "Draw Outline: " << (this->DrawOutline ? "On" : "Off") << "\n";
  os << indent << "Draw Intersection Edges: " << (this->DrawIntersectionEdges ? "On" : "Off")
     << "\n";
  os << indent << "Bump Distance: " << this->BumpDistance << "\n";

  os << indent << "Representation State: ";
  switch (this->RepresentationState)
  {
    case Outside:
      os << "Outside\n";
      break;
    case Moving:
      os << "Moving\n";
      break;
    case MovingOutline:
      os << "MovingOutline\n";
      break;
    case MovingOrigin:
      os << "MovingOrigin\n";
      break;
    case Rotating:
      os << "Rotating\n";
      break;
    case Pushing:
      os << "Pushing\n";
      break;
    case ResizeDiskRadius:
      os << "ResizeDiskRadius\n";
      break;
    case Scaling:
      os << "Scaling\n";
      break;
  }

  // this->InteractionState is printed in superclass
  // this is commented to avoid PrintSelf errors
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::HighlightNormal(int highlight)
{
  if (highlight)
  {
    this->LineActor->SetProperty(this->SelectedNormalProperty);
    this->ConeActor->SetProperty(this->SelectedNormalProperty);
    this->ConeActor2->SetProperty(this->SelectedNormalProperty);
  }
  else
  {
    this->LineActor->SetProperty(this->NormalProperty);
    this->ConeActor->SetProperty(this->NormalProperty);
    this->ConeActor2->SetProperty(this->NormalProperty);
  }
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::HighlightSphere(int highlight)
{
  if (highlight)
  {
    this->SphereActor->SetProperty(this->SelectedSphereProperty);
  }
  else
  {
    this->SphereActor->SetProperty(this->SphereProperty);
  }
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::HighlightPlane(int highlight)
{
  if (highlight)
  {
    this->PlaneActor->SetProperty(this->SelectedPlaneProperty);
  }
  else
  {
    this->PlaneActor->SetProperty(this->PlaneProperty);
  }
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::HighlightOutline(int highlight)
{
  if (highlight)
  {
    this->OutlineActor->SetProperty(this->SelectedOutlineProperty);
  }
  else
  {
    this->OutlineActor->SetProperty(this->OutlineProperty);
  }
}

void vtkDisplaySizedImplicitPlaneRepresentation::HighlightEdges(int highlight)
{
  if (highlight)
  {
    this->EdgesActor->SetProperty(this->SelectedEdgesProperty);
  }
  else
  {
    this->EdgesActor->SetProperty(this->EdgesProperty);
  }
  this->SetEdgesColor(this->EdgesActor->GetProperty()->GetColor());
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::Rotate(
  double X, double Y, double* p1, double* p2, double* vpn)
{
  double v[3];    // vector of motion
  double axis[3]; // axis of rotation
  double theta;   // rotation angle

  // mouse motion vector in world space
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  double* origin = this->Plane->GetOrigin();
  double* normal = this->Plane->GetNormal();

  // Create axis of rotation and angle of rotation
  vtkMath::Cross(vpn, v, axis);
  if (vtkMath::Normalize(axis) == 0.0)
  {
    return;
  }
  const int* size = this->Renderer->GetSize();
  double l2 = (X - this->LastEventPosition[0]) * (X - this->LastEventPosition[0]) +
    (Y - this->LastEventPosition[1]) * (Y - this->LastEventPosition[1]);
  theta = 360.0 * sqrt(l2 / (size[0] * size[0] + size[1] * size[1]));

  // Manipulate the transform to reflect the rotation
  this->Transform->Identity();
  this->Transform->Translate(origin[0], origin[1], origin[2]);
  this->Transform->RotateWXYZ(theta, axis);
  this->Transform->Translate(-origin[0], -origin[1], -origin[2]);

  // Set the new normal
  double nNew[3];
  this->Transform->TransformNormal(normal, nNew);
  this->SetNormal(nNew);
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::Rotate3D(double* p1, double* p2)
{
  if (p1[0] == p2[0] && p1[1] == p2[1] && p1[2] == p2[2])
  {
    return;
  }

  double* origin = this->Plane->GetOrigin();
  double* normal = this->Plane->GetNormal();

  double v1[3] = {
    p1[0] - origin[0],
    p1[1] - origin[1],
    p1[2] - origin[2],
  };
  double v2[3] = {
    p2[0] - origin[0],
    p2[1] - origin[1],
    p2[2] - origin[2],
  };

  vtkMath::Normalize(v1);
  vtkMath::Normalize(v2);

  // Create axis of rotation and angle of rotation
  double axis[3];
  vtkMath::Cross(v1, v2, axis);

  double theta = vtkMath::DegreesFromRadians(acos(vtkMath::Dot(v1, v2)));

  // Manipulate the transform to reflect the rotation
  this->Transform->Identity();
  this->Transform->Translate(origin[0], origin[1], origin[2]);
  this->Transform->RotateWXYZ(theta, axis);
  this->Transform->Translate(-origin[0], -origin[1], -origin[2]);

  // Set the new normal
  double nNew[3];
  this->Transform->TransformNormal(normal, nNew);
  this->SetNormal(nNew);
}

//------------------------------------------------------------------------------
// Loop through all points and translate them
void vtkDisplaySizedImplicitPlaneRepresentation::TranslateOutline(double* p1, double* p2)
{
  // Get the motion vector
  double v[3] = { 0, 0, 0 };

  if (!this->IsTranslationConstrained())
  {
    v[0] = p2[0] - p1[0];
    v[1] = p2[1] - p1[1];
    v[2] = p2[2] - p1[2];
  }
  else
  {
    assert(this->TranslationAxis > -1 && this->TranslationAxis < 3 &&
      "this->TranslationAxis out of bounds");
    v[this->TranslationAxis] = p2[this->TranslationAxis] - p1[this->TranslationAxis];
  }

  // Translate the bounding box
  double* origin = this->Box->GetOrigin();
  double oNew[3];
  oNew[0] = origin[0] + v[0];
  oNew[1] = origin[1] + v[1];
  oNew[2] = origin[2] + v[2];
  this->Box->SetOrigin(oNew);
  this->Box->GetBounds(this->WidgetBounds);

  // Translate the plane
  origin = this->Plane->GetOrigin();
  oNew[0] = origin[0] + v[0];
  oNew[1] = origin[1] + v[1];
  oNew[2] = origin[2] + v[2];
  this->Plane->SetOrigin(oNew);

  this->BuildRepresentation();
}

//------------------------------------------------------------------------------
// Loop through all points and translate them
void vtkDisplaySizedImplicitPlaneRepresentation::TranslateOrigin(double* p1, double* p2)
{
  // Get the motion vector
  double v[3] = { 0, 0, 0 };

  if (!this->IsTranslationConstrained())
  {
    v[0] = p2[0] - p1[0];
    v[1] = p2[1] - p1[1];
    v[2] = p2[2] - p1[2];
  }
  else
  {
    assert(this->TranslationAxis > -1 && this->TranslationAxis < 3 &&
      "this->TranslationAxis out of bounds");
    v[this->TranslationAxis] = p2[this->TranslationAxis] - p1[this->TranslationAxis];
  }

  // Add to the current point, project back down onto plane
  double* o = this->Plane->GetOrigin();
  double* n = this->Plane->GetNormal();
  double newOrigin[3];

  newOrigin[0] = o[0] + v[0];
  newOrigin[1] = o[1] + v[1];
  newOrigin[2] = o[2] + v[2];

  vtkPlane::ProjectPoint(newOrigin, o, n, newOrigin);
  this->SetOrigin(newOrigin[0], newOrigin[1], newOrigin[2]);
  this->BuildRepresentation();
}

namespace
{
bool snapToAxis(vtkVector3d& in, vtkVector3d& out, double snapAngle)
{
  int largest = 0;
  if (fabs(in[1]) > fabs(in[0]))
  {
    largest = 1;
  }
  if (fabs(in[2]) > fabs(in[largest]))
  {
    largest = 2;
  }
  vtkVector3d axis(0, 0, 0);
  axis[largest] = 1.0;
  // 3 degrees of sticky
  if (fabs(in.Dot(axis)) > cos(vtkMath::Pi() * snapAngle / 180.0))
  {
    if (in.Dot(axis) < 0)
    {
      axis[largest] = -1;
    }
    out = axis;
    return true;
  }
  return false;
}
}

//------------------------------------------------------------------------------
// Loop through all points and translate and rotate them
void vtkDisplaySizedImplicitPlaneRepresentation::UpdatePose(
  double* p1, double* d1, double* p2, double* d2)
{
  double* origin = this->Plane->GetOrigin();
  double* normal = this->Plane->GetNormal();

  double nNew[3];
  double temp1[4];
  double temp2[4];
  std::copy(d1, d1 + 4, temp1);
  temp1[0] = vtkMath::RadiansFromDegrees(-temp1[0]);
  std::copy(d2, d2 + 4, temp2);
  temp2[0] = vtkMath::RadiansFromDegrees(temp2[0]);

  vtkMath::RotateVectorByWXYZ(normal, temp1, nNew);
  vtkMath::RotateVectorByWXYZ(nNew, temp2, nNew);

  if (this->SnapToAxes)
  {
    vtkVector3d basis(nNew);
    double temp3[4];
    if (this->SnappedOrientation)
    {
      double nNew2[3];
      std::copy(this->SnappedEventOrientation, this->SnappedEventOrientation + 4, temp3);
      temp3[0] = vtkMath::RadiansFromDegrees(-temp3[0]);
      vtkMath::RotateVectorByWXYZ(normal, temp3, nNew2);
      vtkMath::RotateVectorByWXYZ(nNew2, temp2, basis.GetData());
    }
    // 14 degrees to snap in, 16 to snap out
    // avoids noise on the boundary
    bool newSnap = snapToAxis(basis, basis, (this->SnappedOrientation ? 16 : 14));
    if (newSnap && !this->SnappedOrientation)
    {
      std::copy(d2, d2 + 4, this->SnappedEventOrientation);
    }
    this->SnappedOrientation = newSnap;
    this->SetNormal(basis.GetData());
  }
  else
  {
    this->SetNormal(nNew);
  }

  // adjust center for rotation
  double v[3];
  v[0] = origin[0] - 0.5 * (p2[0] + p1[0]);
  v[1] = origin[1] - 0.5 * (p2[1] + p1[1]);
  v[2] = origin[2] - 0.5 * (p2[2] + p1[2]);

  vtkMath::RotateVectorByWXYZ(v, temp1, v);
  vtkMath::RotateVectorByWXYZ(v, temp2, v);

  double newOrigin[3];
  newOrigin[0] = v[0] + 0.5 * (p2[0] + p1[0]);
  newOrigin[1] = v[1] + 0.5 * (p2[1] + p1[1]);
  newOrigin[2] = v[2] + 0.5 * (p2[2] + p1[2]);

  // Get the motion vector
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  // Add to the current point, project back down onto plane

  newOrigin[0] += v[0];
  newOrigin[1] += v[1];
  newOrigin[2] += v[2];

  // vtkPlane::ProjectPoint(newOrigin,origin,normal,newOrigin);
  this->SetOrigin(newOrigin[0], newOrigin[1], newOrigin[2]);
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::Scale(
  double* p1, double* p2, double vtkNotUsed(X), double Y)
{
  // Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  double* o = this->Plane->GetOrigin();

  // Compute the scale factor
  double sf = vtkMath::Norm(v) / vtkWidgetRepresentation::SizeHandlesRelativeToViewport(0.1, o);
  if (Y > this->LastEventPosition[1])
  {
    sf = 1.0 + sf;
  }
  else
  {
    sf = 1.0 - sf;
  }

  this->Transform->Identity();
  this->Transform->Translate(o[0], o[1], o[2]);
  this->Transform->Scale(sf, sf, sf);
  this->Transform->Translate(-o[0], -o[1], -o[2]);

  double* origin = this->Box->GetOrigin();
  double* spacing = this->Box->GetSpacing();
  double oNew[3], p[3], pNew[3];
  p[0] = origin[0] + spacing[0];
  p[1] = origin[1] + spacing[1];
  p[2] = origin[2] + spacing[2];

  this->Transform->TransformPoint(origin, oNew);
  this->Transform->TransformPoint(p, pNew);

  this->Box->SetOrigin(oNew);
  this->Box->SetSpacing((pNew[0] - oNew[0]), (pNew[1] - oNew[1]), (pNew[2] - oNew[2]));
  this->Box->GetBounds(this->WidgetBounds);

  this->BuildRepresentation();
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::Push(double* p1, double* p2)
{
  // Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  this->Plane->Push(vtkMath::Dot(v, this->Plane->GetNormal()));
  this->SetOrigin(this->Plane->GetOrigin());
  this->BuildRepresentation();
}

void vtkDisplaySizedImplicitPlaneRepresentation::ResizeRadius(
  double* vtkNotUsed(p1), double* p2, double* vpn)
{
  double* o = this->Plane->GetOrigin();
  double p2Projected[3], p2Intersection[3];
  double t, newRadius;
  vtkPlane::ProjectPoint(p2, o, vpn, p2Projected);
  int planeIsNotParallelToLine = this->Plane->IntersectWithLine(p2, p2Projected, t, p2Intersection);
  if (planeIsNotParallelToLine != 0)
  {
    newRadius = std::sqrt(vtkMath::Distance2BetweenPoints(p2Intersection, o));
  }
  else
  {
    newRadius = std::sqrt(vtkMath::Distance2BetweenPoints(p2, o));
  }
  double oldRadiusAdaptiveFactor = vtkWidgetRepresentation::SizeHandlesRelativeToViewport(0.04, o);
  this->SetRadiusMultiplier(newRadius / oldRadiusAdaptiveFactor);
}

void vtkDisplaySizedImplicitPlaneRepresentation::ResizeRadius3D(double* vtkNotUsed(p1), double* p2)
{
  double* o = this->Plane->GetOrigin();
  double p2Projected[3];
  this->Plane->ProjectPoint(p2, p2Projected);

  double oldRadiusAdaptiveFactor = vtkWidgetRepresentation::SizeHandlesRelativeToViewport(0.04, o);
  double newRadius = std::sqrt(vtkMath::Distance2BetweenPoints(p2Projected, o));
  this->SetRadiusMultiplier(newRadius / oldRadiusAdaptiveFactor);
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::CreateDefaultProperties()
{
  static constexpr double neutral[3] = { 1.0, 1.0, 1.0 };         // white
  static constexpr double unselectedColor[3] = { 1.0, 0.0, 0.0 }; // red
  static constexpr double selectedColor[3] = { 0.0, 1.0, 0.0 };   // green

  // Normal properties
  this->NormalProperty->SetColor(unselectedColor[0], unselectedColor[1], unselectedColor[2]);
  this->NormalProperty->SetLineWidth(2);

  this->SelectedNormalProperty->SetColor(selectedColor[0], selectedColor[1], selectedColor[2]);
  this->SelectedNormalProperty->SetLineWidth(2);

  // Normal properties
  this->SphereProperty->SetColor(unselectedColor[0], unselectedColor[1], unselectedColor[2]);

  this->SelectedSphereProperty->SetColor(selectedColor[0], selectedColor[1], selectedColor[2]);

  // Plane properties
  this->PlaneProperty->SetAmbient(1.0);
  this->PlaneProperty->SetColor(neutral[0], neutral[1], neutral[2]);
  this->PlaneProperty->SetOpacity(0.5);

  this->SelectedPlaneProperty->SetAmbient(1.0);
  this->SelectedPlaneProperty->SetColor(selectedColor[0], selectedColor[1], selectedColor[2]);
  this->SelectedPlaneProperty->SetOpacity(0.25);

  // Outline properties
  this->OutlineProperty->SetAmbient(1.0);
  this->OutlineProperty->SetAmbientColor(neutral[0], neutral[1], neutral[2]);

  this->SelectedOutlineProperty->SetAmbient(1.0);
  this->SelectedOutlineProperty->SetAmbientColor(
    selectedColor[0], selectedColor[1], selectedColor[2]);

  // Edges property
  this->EdgesProperty->SetAmbient(1.0);
  this->EdgesProperty->SetColor(unselectedColor[0], unselectedColor[1], unselectedColor[2]);

  this->SelectedEdgesProperty->SetAmbient(1.0);
  this->SelectedEdgesProperty->SetColor(selectedColor[0], selectedColor[1], selectedColor[2]);

  // Intersection Edges property
  this->IntersectionEdgesProperty->SetAmbient(1.0);
  this->IntersectionEdgesProperty->SetOpacity(0.35);
  this->IntersectionEdgesProperty->SetColor(neutral[0], neutral[1], neutral[2]);
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::SetSelectedWidgetColor(
  double r, double g, double b)
{
  this->SelectedNormalProperty->SetColor(r, g, b);
  this->SelectedSphereProperty->SetColor(r, g, b);
  this->SelectedEdgesProperty->SetColor(r, g, b);
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::SetSelectedWidgetColor(double c[3])
{
  this->SetSelectedWidgetColor(c[0], c[1], c[2]);
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::SetUnselectedWidgetColor(
  double r, double g, double b)
{
  this->NormalProperty->SetColor(r, g, b);
  this->SphereProperty->SetColor(r, g, b);
  this->EdgesProperty->SetColor(r, g, b);
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::SetUnselectedWidgetColor(double c[3])
{
  this->SetUnselectedWidgetColor(c[0], c[1], c[2]);
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::SetEdgesColor(vtkLookupTable* lut)
{
  this->EdgesMapper->SetLookupTable(lut);
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::SetEdgesColor(double r, double g, double b)
{
  vtkNew<vtkLookupTable> lookupTable;

  lookupTable->SetTableRange(0.0, 1.0);
  lookupTable->SetNumberOfTableValues(1);
  lookupTable->SetTableValue(0, r, g, b);
  lookupTable->Build();

  this->SetEdgesColor(lookupTable);
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::SetEdgesColor(double c[3])
{
  this->SetEdgesColor(c[0], c[1], c[2]);
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::SetIntersectionEdgesColor(vtkLookupTable* lut)
{
  this->IntersectionEdgesMapper->SetLookupTable(lut);
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::SetIntersectionEdgesColor(
  double r, double g, double b)
{
  vtkNew<vtkLookupTable> lookupTable;

  lookupTable->SetTableRange(0.0, 1.0);
  lookupTable->SetNumberOfTableValues(1);
  lookupTable->SetTableValue(0, r, g, b);
  lookupTable->Build();

  this->SetIntersectionEdgesColor(lookupTable);
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::SetIntersectionEdgesColor(double c[3])
{
  this->SetIntersectionEdgesColor(c[0], c[1], c[2]);
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::PlaceWidget(double bds[6])
{
  int i;
  double bounds[6], origin[3];

  this->AdjustBounds(bds, bounds, origin);

  // Set up the bounding box
  this->Box->SetOrigin(bounds[0], bounds[2], bounds[4]);
  this->Box->SetSpacing((bounds[1] - bounds[0]), (bounds[3] - bounds[2]), (bounds[5] - bounds[4]));
  this->Outline->Update();

  this->InitialLength = std::sqrt((bounds[1] - bounds[0]) * (bounds[1] - bounds[0]) +
    (bounds[3] - bounds[2]) * (bounds[3] - bounds[2]) +
    (bounds[5] - bounds[4]) * (bounds[5] - bounds[4]));

  this->LineSource->SetPoint1(this->Plane->GetOrigin());
  if (this->NormalToYAxis)
  {
    this->Plane->SetNormal(0, 1, 0);
    this->LineSource->SetPoint2(0, 1, 0);
    this->DiskPlaneSource->SetCenter(this->InitialLength / 3.0, 0, this->InitialLength / 3.0);
  }
  else if (this->NormalToZAxis)
  {
    this->Plane->SetNormal(0, 0, 1);
    this->LineSource->SetPoint2(0, 0, 1);
    this->DiskPlaneSource->SetCenter(this->InitialLength / 3.0, this->InitialLength / 3.0, 0);
  }
  else // default or x-normal
  {
    this->Plane->SetNormal(1, 0, 0);
    this->LineSource->SetPoint2(1, 0, 0);
    this->DiskPlaneSource->SetCenter(0, this->InitialLength / 3.0, this->InitialLength / 3.0);
  }
  this->DiskPlaneSource->SetNormal(this->Plane->GetNormal());
  this->DiskPlaneSource->SetOuterRadius(this->InitialLength / 3.0);

  for (i = 0; i < 6; i++)
  {
    this->InitialBounds[i] = bounds[i];
    this->WidgetBounds[i] = bounds[i];
  }

  this->ValidPick = 1; // since we have positioned the widget successfully
  this->BuildRepresentation();
}

//------------------------------------------------------------------------------
// Description:
// Set the origin of the plane.
void vtkDisplaySizedImplicitPlaneRepresentation::SetOrigin(double x, double y, double z)
{
  double origin[3];
  origin[0] = x;
  origin[1] = y;
  origin[2] = z;
  this->SetOrigin(origin);
}

//------------------------------------------------------------------------------
// Description:
// Set the origin of the plane. Note that the origin is clamped slightly inside
// the bounding box or the plane tends to disappear as it hits the boundary (and
// when the plane is parallel to one of the faces of the bounding box).
void vtkDisplaySizedImplicitPlaneRepresentation::SetOrigin(double x[3])
{
  this->Plane->SetOrigin(x);
  this->BuildRepresentation();
}

//------------------------------------------------------------------------------
// Description:
// Get the origin of the plane.
double* vtkDisplaySizedImplicitPlaneRepresentation::GetOrigin()
{
  return this->Plane->GetOrigin();
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::GetOrigin(double xyz[3])
{
  this->Plane->GetOrigin(xyz);
}

//------------------------------------------------------------------------------
// Description:
// Set the normal to the plane.
void vtkDisplaySizedImplicitPlaneRepresentation::SetNormal(double x, double y, double z)
{
  if (this->AlwaysSnapToNearestAxis)
  {
    x = std::abs(x) >= std::abs(y) && std::abs(x) >= std::abs(z) ? 1.0 : 0.0;
    y = std::abs(y) >= std::abs(x) && std::abs(y) >= std::abs(z) ? 1.0 : 0.0;
    z = std::abs(z) >= std::abs(y) && std::abs(z) >= std::abs(x) ? 1.0 : 0.0;
    this->Plane->SetNormal(x, y, z);
    this->Modified();
    return;
  }

  double n[3], n2[3];
  n[0] = x;
  n[1] = y;
  n[2] = z;
  vtkMath::Normalize(n);

  this->Plane->GetNormal(n2);
  if (n[0] != n2[0] || n[1] != n2[1] || n[2] != n2[2])
  {
    this->Plane->SetNormal(n);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
// Description:
// Set the normal to the plane.
void vtkDisplaySizedImplicitPlaneRepresentation::SetNormal(double n[3])
{
  this->SetNormal(n[0], n[1], n[2]);
}

//------------------------------------------------------------------------------
// Description:
// Get the normal to the plane.
double* vtkDisplaySizedImplicitPlaneRepresentation::GetNormal()
{
  return this->Plane->GetNormal();
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::GetNormal(double xyz[3])
{
  this->Plane->GetNormal(xyz);
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::SetRadiusMultiplier(double radiusMultiplier)
{
  if (this->RadiusMultiplier !=
    (radiusMultiplier < 0.000001
        ? 0.000001
        : (radiusMultiplier > VTK_DOUBLE_MAX ? VTK_DOUBLE_MAX : radiusMultiplier)))
  {
    this->RadiusMultiplier = (radiusMultiplier < 0.000001
        ? 0.000001
        : (radiusMultiplier > VTK_DOUBLE_MAX ? VTK_DOUBLE_MAX : radiusMultiplier));
    this->Modified();
    this->BuildRepresentation();
  }
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::SetDrawPlane(vtkTypeBool drawPlane)
{
  if (drawPlane == this->DrawPlane)
  {
    return;
  }

  this->Modified();
  this->DrawPlane = drawPlane;
  this->BuildRepresentation();
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::SetDrawOutline(vtkTypeBool val)
{
  if (val == this->DrawOutline)
  {
    return;
  }

  if (val)
  {
    this->CellPicker->AddPickList(this->OutlineActor);
  }
  else
  {
    this->CellPicker->DeletePickList(this->OutlineActor);
  }
  this->Modified();
  this->DrawOutline = val;
  this->BuildRepresentation();
}

void vtkDisplaySizedImplicitPlaneRepresentation::SetDrawIntersectionEdges(vtkTypeBool val)
{
  if (val == this->DrawIntersectionEdges)
  {
    return;
  }
  this->Modified();
  this->DrawIntersectionEdges = val;
  this->BuildRepresentation();
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::SetNormalToXAxis(vtkTypeBool var)
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

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::SetNormalToYAxis(vtkTypeBool var)
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

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::SetNormalToZAxis(vtkTypeBool var)
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

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::GetPolyData(vtkPolyData* pd)
{
  pd->ShallowCopy(this->DiskPlaneSource->GetOutput());
}

//------------------------------------------------------------------------------
vtkPolyDataAlgorithm* vtkDisplaySizedImplicitPlaneRepresentation::GetPolyDataAlgorithm()
{
  return this->DiskPlaneSource;
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::GetPlane(vtkPlane* plane)
{
  if (plane == nullptr)
  {
    return;
  }

  plane->SetNormal(this->Plane->GetNormal());
  plane->SetOrigin(this->Plane->GetOrigin());
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::SetPlane(vtkPlane* plane)
{
  if (plane == nullptr)
  {
    return;
  }

  this->Plane->SetNormal(plane->GetNormal());
  this->Plane->SetOrigin(plane->GetOrigin());
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::UpdatePlacement()
{
  this->Outline->Update();
  this->BuildRepresentation();
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::BumpPlane(int dir, double factor)
{
  // Compute the distance
  double d = this->InitialLength * this->BumpDistance * factor;

  // Push the plane
  this->PushPlane((dir > 0 ? d : -d));
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::PushPlane(double d)
{
  this->Plane->Push(d);
  this->BuildRepresentation();
}

//------------------------------------------------------------------------------
bool vtkDisplaySizedImplicitPlaneRepresentation::PickOrigin(int X, int Y, bool snapToMeshPoint)
{
  this->HardwarePicker->SetSnapToMeshPoint(snapToMeshPoint);
  vtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->HardwarePicker);
  if (path == nullptr) // actors of renderer were not touched
  {
    if (this->PickCameraFocalInfo)
    {
      double pos[3];
      this->HardwarePicker->GetPickPosition(pos);
      this->SetOrigin(pos);
      this->BuildRepresentation();
    }
    return this->PickCameraFocalInfo;
  }
  else // actors of renderer were touched
  {
    double pos[3];
    this->HardwarePicker->GetPickPosition(pos);
    if (!std::isnan(pos[0]) || !std::isnan(pos[1]) || !std::isnan(pos[2]))
    {
      this->SetOrigin(pos);
      this->BuildRepresentation();
      return true;
    }
    else
    {
      return false;
    }
  }
}

//------------------------------------------------------------------------------
bool vtkDisplaySizedImplicitPlaneRepresentation::PickNormal(int X, int Y, bool snapToMeshPoint)
{
  this->HardwarePicker->SetSnapToMeshPoint(snapToMeshPoint);
  vtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->HardwarePicker);
  if (path == nullptr) // actors of renderer were not touched
  {
    if (this->PickCameraFocalInfo)
    {
      double normal[3];
      this->HardwarePicker->GetPickNormal(normal);
      this->SetNormal(normal);
      this->BuildRepresentation();
    }
    return this->PickCameraFocalInfo;
  }
  else // actors of renderer were touched
  {
    double normal[3];
    this->HardwarePicker->GetPickNormal(normal);
    if (!std::isnan(normal[0]) || !std::isnan(normal[1]) || !std::isnan(normal[2]))
    {
      this->SetNormal(normal);
      this->BuildRepresentation();
      return true;
    }
    else
    {
      return false;
    }
  }
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::BuildRepresentation()
{
  if (!this->Renderer || !this->Renderer->GetRenderWindow())
  {
    return;
  }

  vtkInformation* info = this->GetPropertyKeys();
  this->OutlineActor->SetPropertyKeys(info);
  this->PlaneActor->SetPropertyKeys(info);
  this->EdgesActor->SetPropertyKeys(info);
  this->IntersectionEdgesActor->SetPropertyKeys(info);
  this->ConeActor->SetPropertyKeys(info);
  this->LineActor->SetPropertyKeys(info);
  this->ConeActor2->SetPropertyKeys(info);
  this->SphereActor->SetPropertyKeys(info);

  if (this->GetMTime() > this->BuildTime || this->Plane->GetMTime() > this->BuildTime)
  {
    double* origin = this->Plane->GetOrigin();
    double* normal = this->Plane->GetNormal();

    double bounds[6];
    std::copy(this->WidgetBounds, this->WidgetBounds + 6, bounds);

    if (!this->OutsideBounds)
    {
      // restrict the origin inside InitialBounds
      double* ibounds = this->InitialBounds;
      for (int i = 0; i < 3; i++)
      {
        if (origin[i] < ibounds[2 * i])
        {
          origin[i] = ibounds[2 * i];
        }
        else if (origin[i] > ibounds[2 * i + 1])
        {
          origin[i] = ibounds[2 * i + 1];
        }
      }
    }

    if (this->ConstrainToWidgetBounds)
    {
      if (!this->OutsideBounds)
      {
        // origin cannot move outside InitialBounds. Therefore, restrict
        // movement of the Box.
        double v[3] = { 0.0, 0.0, 0.0 };
        for (int i = 0; i < 3; ++i)
        {
          if (origin[i] <= bounds[2 * i])
          {
            v[i] = origin[i] - bounds[2 * i] - FLT_EPSILON;
          }
          else if (origin[i] >= bounds[2 * i + 1])
          {
            v[i] = origin[i] - bounds[2 * i + 1] + FLT_EPSILON;
          }
          bounds[2 * i] += v[i];
          bounds[2 * i + 1] += v[i];
        }
      }

      // restrict origin inside bounds
      for (int i = 0; i < 3; ++i)
      {
        if (origin[i] <= bounds[2 * i])
        {
          origin[i] = bounds[2 * i] + FLT_EPSILON;
        }
        if (origin[i] >= bounds[2 * i + 1])
        {
          origin[i] = bounds[2 * i + 1] - FLT_EPSILON;
        }
      }
    }
    else // plane can move freely, adjust the bounds to change with it
    {
      double offset = this->Box->GetLength() * 0.02;
      for (int i = 0; i < 3; ++i)
      {
        bounds[2 * i] = vtkMath::Min(origin[i] - offset, this->WidgetBounds[2 * i]);
        bounds[2 * i + 1] = vtkMath::Max(origin[i] + offset, this->WidgetBounds[2 * i + 1]);
      }
    }

    this->Box->SetOrigin(bounds[0], bounds[2], bounds[4]);
    this->Box->SetSpacing(
      (bounds[1] - bounds[0]), (bounds[3] - bounds[2]), (bounds[5] - bounds[4]));
    this->Outline->Update();

    this->DiskPlaneSource->SetCenter(origin);
    this->DiskPlaneSource->SetNormal(normal);
    this->ConeSource->SetDirection(normal);
    this->ConeSource2->SetDirection(normal[0], normal[1], normal[2]);

    // Set up the position handle
    this->Sphere->SetCenter(origin[0], origin[1], origin[2]);
  }

  // we resize handles when we rebuilt, when the camera changes and when the radius changes
  if (this->GetMTime() > this->BuildTime || this->Plane->GetMTime() > this->BuildTime ||
    this->Renderer->GetRenderWindow()->GetMTime() > this->BuildTime ||
    (this->Renderer->GetActiveCamera() &&
      this->Renderer->GetActiveCamera()->GetMTime() > this->BuildTime))
  {
    this->SizeHandles();
    this->BuildTime.Modified();
  }
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::SizeHandles()
{
  double* o = this->Plane->GetOrigin();
  double* n = this->Plane->GetNormal();

  double radiusAdaptiveFactor = vtkWidgetRepresentation::SizeHandlesRelativeToViewport(0.04, o);

  double halfDiagonal = 0.0;
  if (this->ConstrainMaximumSizeToWidgetBounds)
  {
    // calculate half diagonal of the bounding box formed by the widget bounds
    // half diagonal is used as the maximum size of the radius and then normal plane arrow
    double diagonal = 0.0;
    for (int i = 0; i < 3; i++)
    {
      double diff = static_cast<double>(this->WidgetBounds[2 * i + 1]) -
        static_cast<double>(this->WidgetBounds[2 * i]);
      diagonal += diff * diff;
    }
    diagonal = std::sqrt(diagonal);
    halfDiagonal = diagonal / 2.0;

    if (radiusAdaptiveFactor * this->RadiusMultiplier > halfDiagonal)
    {
      radiusAdaptiveFactor = std::min(radiusAdaptiveFactor, halfDiagonal);
      this->RadiusMultiplier = halfDiagonal / radiusAdaptiveFactor;
    }
  }
  // set up plane disk radius
  this->DiskPlaneSource->SetOuterRadius(radiusAdaptiveFactor * this->RadiusMultiplier);

  // set up the plane normal
  double p2[3];
  const double d = this->ConstrainMaximumSizeToWidgetBounds
    ? std::min(radiusAdaptiveFactor, halfDiagonal)
    : radiusAdaptiveFactor;
  p2[0] = o[0] + d * n[0];
  p2[1] = o[1] + d * n[1];
  p2[2] = o[2] + d * n[2];

  this->LineSource->SetPoint1(p2);
  this->ConeSource->SetCenter(p2);

  p2[0] = o[0] - d * n[0];
  p2[1] = o[1] - d * n[1];
  p2[2] = o[2] - d * n[2];

  this->LineSource->SetPoint2(p2);
  this->ConeSource2->SetCenter(p2);

  // set up cones, sphere and edge tuber size
  const double radius = this->vtkWidgetRepresentation::SizeHandlesInPixels(3.0, o);

  this->ConeSource->SetHeight(2.0 * radius);
  this->ConeSource->SetRadius(radius);
  this->ConeSource2->SetHeight(2.0 * radius);
  this->ConeSource2->SetRadius(radius);

  this->Sphere->SetRadius(radius);

  this->EdgesTuber->SetRadius(0.35 * radius);
  this->IntersectionEdgesTuber->SetRadius(0.25 * radius);
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::SetNormalToCamera()
{
  if (!this->Renderer)
  {
    return;
  }

  double normal[3];
  this->Renderer->GetActiveCamera()->GetViewPlaneNormal(normal);
  this->SetNormal(normal);
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::RegisterPickers()
{
  vtkPickingManager* pm = this->GetPickingManager();
  if (!pm)
  {
    return;
  }
  pm->AddPicker(this->CellPicker, this);
  pm->AddPicker(this->HardwarePicker, this);
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneRepresentation::ComputeAdaptivePickerTolerance()
{
  double pickerCylinderRadius =
    vtkWidgetRepresentation::SizeHandlesRelativeToViewport(0.000001, this->Plane->GetOrigin());
  double tolerance = pickerCylinderRadius < DefaultPickTol ? pickerCylinderRadius : DefaultPickTol;
  this->CellPicker->SetTolerance(tolerance);
}
