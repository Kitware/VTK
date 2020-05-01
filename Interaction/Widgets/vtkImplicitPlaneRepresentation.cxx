/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitPlaneRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImplicitPlaneRepresentation.h"

#include "vtkActor.h"
#include "vtkAssemblyNode.h"
#include "vtkAssemblyPath.h"
#include "vtkBox.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCellPicker.h"
#include "vtkCommand.h"
#include "vtkConeSource.h"
#include "vtkCutter.h"
#include "vtkEventData.h"
#include "vtkFeatureEdges.h"
#include "vtkImageData.h"
#include "vtkInteractorObserver.h"
#include "vtkLineSource.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkOutlineFilter.h"
#include "vtkPickingManager.h"
#include "vtkPlane.h"
#include "vtkPlaneSource.h"
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

vtkStandardNewMacro(vtkImplicitPlaneRepresentation);

//----------------------------------------------------------------------------
vtkImplicitPlaneRepresentation::vtkImplicitPlaneRepresentation()
{
  this->NormalToXAxis = 0;
  this->NormalToYAxis = 0;
  this->NormalToZAxis = 0;

  this->SnappedOrientation = false;
  this->SnapToAxes = false;

  this->LockNormalToCamera = 0;

  this->CropPlaneToBoundingBox = true;

  // Handle size is in pixels for this widget
  this->HandleSize = 5.0;

  // Pushing operation
  this->BumpDistance = 0.01;

  // Build the representation of the widget
  //
  this->Plane = vtkPlane::New();
  this->Plane->SetNormal(0, 0, 1);
  this->Plane->SetOrigin(0, 0, 0);

  this->Box = vtkImageData::New();
  this->Box->SetDimensions(2, 2, 2);
  this->Outline = vtkOutlineFilter::New();
  this->Outline->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
  this->Outline->SetInputData(this->Box);
  this->OutlineMapper = vtkPolyDataMapper::New();
  this->OutlineMapper->SetInputConnection(this->Outline->GetOutputPort());
  this->OutlineActor = vtkActor::New();
  this->OutlineActor->SetMapper(this->OutlineMapper);
  this->OutlineTranslation = 1;
  this->ScaleEnabled = 1;
  this->OutsideBounds = 1;
  this->ConstrainToWidgetBounds = 1;

  this->Cutter = vtkCutter::New();
  this->Cutter->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
  this->Cutter->SetInputData(this->Box);
  this->Cutter->SetCutFunction(this->Plane);
  this->PlaneSource = vtkPlaneSource::New();
  this->PlaneSource->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
  this->CutMapper = vtkPolyDataMapper::New();
  this->CutMapper->SetInputConnection(this->Cutter->GetOutputPort());
  this->CutActor = vtkActor::New();
  this->CutActor->SetMapper(this->CutMapper);
  this->DrawPlane = 1;
  this->DrawOutline = 1;

  this->Edges = vtkFeatureEdges::New();
  this->Edges->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
  this->Edges->SetInputConnection(this->Cutter->GetOutputPort());
  this->EdgesTuber = vtkTubeFilter::New();
  this->EdgesTuber->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
  this->EdgesTuber->SetInputConnection(this->Edges->GetOutputPort());
  this->EdgesTuber->SetNumberOfSides(12);
  this->EdgesMapper = vtkPolyDataMapper::New();
  this->EdgesMapper->SetInputConnection(this->EdgesTuber->GetOutputPort());
  this->EdgesActor = vtkActor::New();
  this->EdgesActor->SetMapper(this->EdgesMapper);
  this->Tubing = 1; // control whether tubing is on

  // Create the + plane normal
  this->LineSource = vtkLineSource::New();
  this->LineSource->SetResolution(1);
  this->LineSource->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
  this->LineMapper = vtkPolyDataMapper::New();
  this->LineMapper->SetInputConnection(this->LineSource->GetOutputPort());
  this->LineActor = vtkActor::New();
  this->LineActor->SetMapper(this->LineMapper);

  this->ConeSource = vtkConeSource::New();
  this->ConeSource->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
  this->ConeSource->SetResolution(12);
  this->ConeSource->SetAngle(25.0);
  this->ConeMapper = vtkPolyDataMapper::New();
  this->ConeMapper->SetInputConnection(this->ConeSource->GetOutputPort());
  this->ConeActor = vtkActor::New();
  this->ConeActor->SetMapper(this->ConeMapper);

  // Create the - plane normal
  this->LineSource2 = vtkLineSource::New();
  this->LineSource2->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
  this->LineSource2->SetResolution(1);
  this->LineMapper2 = vtkPolyDataMapper::New();
  this->LineMapper2->SetInputConnection(this->LineSource2->GetOutputPort());
  this->LineActor2 = vtkActor::New();
  this->LineActor2->SetMapper(this->LineMapper2);

  this->ConeSource2 = vtkConeSource::New();
  this->ConeSource2->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
  this->ConeSource2->SetResolution(12);
  this->ConeSource2->SetAngle(25.0);
  this->ConeMapper2 = vtkPolyDataMapper::New();
  this->ConeMapper2->SetInputConnection(this->ConeSource2->GetOutputPort());
  this->ConeActor2 = vtkActor::New();
  this->ConeActor2->SetMapper(this->ConeMapper2);

  // Create the origin handle
  this->Sphere = vtkSphereSource::New();
  this->Sphere->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
  this->Sphere->SetThetaResolution(16);
  this->Sphere->SetPhiResolution(8);
  this->SphereMapper = vtkPolyDataMapper::New();
  this->SphereMapper->SetInputConnection(this->Sphere->GetOutputPort());
  this->SphereActor = vtkActor::New();
  this->SphereActor->SetMapper(this->SphereMapper);

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

  // Manage the picking stuff
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

  // Pass the initial properties to the actors.
  this->LineActor->SetProperty(this->NormalProperty);
  this->ConeActor->SetProperty(this->NormalProperty);
  this->LineActor2->SetProperty(this->NormalProperty);
  this->ConeActor2->SetProperty(this->NormalProperty);
  this->SphereActor->SetProperty(this->NormalProperty);
  this->CutActor->SetProperty(this->PlaneProperty);
  this->OutlineActor->SetProperty(this->OutlineProperty);

  // The bounding box
  this->BoundingBox = vtkBox::New();

  this->RepresentationState = vtkImplicitPlaneRepresentation::Outside;

  this->TranslationAxis = Axis::NONE;
  this->AlwaysSnapToNearestAxis = false;
}

//----------------------------------------------------------------------------
vtkImplicitPlaneRepresentation::~vtkImplicitPlaneRepresentation()
{
  this->Plane->Delete();
  this->Box->Delete();
  this->Outline->Delete();
  this->OutlineMapper->Delete();
  this->OutlineActor->Delete();

  this->Cutter->Delete();
  this->PlaneSource->Delete();
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
  this->BoundingBox->Delete();
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::SetLockNormalToCamera(vtkTypeBool lock)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting " << LockNormalToCamera
                << " to " << lock);
  if (lock == this->LockNormalToCamera)
  {
    return;
  }

  if (lock)
  {
    this->Picker->DeletePickList(this->LineActor);
    this->Picker->DeletePickList(this->ConeActor);
    this->Picker->DeletePickList(this->LineActor2);
    this->Picker->DeletePickList(this->ConeActor2);
    this->Picker->DeletePickList(this->SphereActor);

    this->SetNormalToCamera();
  }
  else
  {
    this->Picker->AddPickList(this->LineActor);
    this->Picker->AddPickList(this->ConeActor);
    this->Picker->AddPickList(this->LineActor2);
    this->Picker->AddPickList(this->ConeActor2);
    this->Picker->AddPickList(this->SphereActor);
  }

  this->LockNormalToCamera = lock;
  this->Modified();
}

void vtkImplicitPlaneRepresentation::SetCropPlaneToBoundingBox(bool val)
{
  if (this->CropPlaneToBoundingBox == val)
  {
    return;
  }

  this->CropPlaneToBoundingBox = val;
  if (val)
  {
    this->CutMapper->SetInputConnection(this->Cutter->GetOutputPort());
    this->Edges->SetInputConnection(this->Cutter->GetOutputPort());
  }
  else
  {
    this->CutMapper->SetInputConnection(this->PlaneSource->GetOutputPort());
    this->Edges->SetInputConnection(this->PlaneSource->GetOutputPort());
  }
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkImplicitPlaneRepresentation::ComputeInteractionState(int X, int Y, int vtkNotUsed(modify))
{
  // See if anything has been selected
  vtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->Picker);

  if (path == nullptr) // Not picking this widget
  {
    this->SetRepresentationState(vtkImplicitPlaneRepresentation::Outside);
    this->InteractionState = vtkImplicitPlaneRepresentation::Outside;
    return this->InteractionState;
  }

  // Something picked, continue
  this->ValidPick = 1;

  // Depending on the interaction state (set by the widget) we modify
  // this state based on what is picked.
  if (this->InteractionState == vtkImplicitPlaneRepresentation::Moving)
  {
    vtkProp* prop = path->GetFirstNode()->GetViewProp();
    if (prop == this->ConeActor || prop == this->LineActor || prop == this->ConeActor2 ||
      prop == this->LineActor2)
    {
      this->InteractionState = vtkImplicitPlaneRepresentation::Rotating;
      this->SetRepresentationState(vtkImplicitPlaneRepresentation::Rotating);
    }
    else if (prop == this->CutActor)
    {
      if (this->LockNormalToCamera)
      { // Allow camera to work
        this->InteractionState = vtkImplicitPlaneRepresentation::Outside;
        this->SetRepresentationState(vtkImplicitPlaneRepresentation::Outside);
      }
      else
      {
        this->InteractionState = vtkImplicitPlaneRepresentation::Pushing;
        this->SetRepresentationState(vtkImplicitPlaneRepresentation::Pushing);
      }
    }
    else if (prop == this->SphereActor)
    {
      this->InteractionState = vtkImplicitPlaneRepresentation::MovingOrigin;
      this->SetRepresentationState(vtkImplicitPlaneRepresentation::MovingOrigin);
    }
    else
    {
      if (this->OutlineTranslation)
      {
        this->InteractionState = vtkImplicitPlaneRepresentation::MovingOutline;
        this->SetRepresentationState(vtkImplicitPlaneRepresentation::MovingOutline);
      }
      else
      {
        this->InteractionState = vtkImplicitPlaneRepresentation::Outside;
        this->SetRepresentationState(vtkImplicitPlaneRepresentation::Outside);
      }
    }
  }

  // We may add a condition to allow the camera to work IO scaling
  else if (this->InteractionState != vtkImplicitPlaneRepresentation::Scaling)
  {
    this->InteractionState = vtkImplicitPlaneRepresentation::Outside;
  }

  return this->InteractionState;
}

int vtkImplicitPlaneRepresentation::ComputeComplexInteractionState(
  vtkRenderWindowInteractor*, vtkAbstractWidget*, unsigned long, void* calldata, int)
{
  vtkEventData* edata = static_cast<vtkEventData*>(calldata);
  vtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();
  if (edd)
  {
    double pos[3];
    edd->GetWorldPosition(pos);
    if (this->DrawOutline)
    {
      this->Picker->DeletePickList(this->OutlineActor);
    }
    vtkAssemblyPath* path = this->GetAssemblyPath3DPoint(pos, this->Picker);
    if (this->DrawOutline)
    {
      this->Picker->AddPickList(this->OutlineActor);
      if (path == nullptr)
      {
        path = this->GetAssemblyPath3DPoint(pos, this->Picker);
      }
    }

    if (path == nullptr) // Not picking this widget
    {
      this->SetRepresentationState(vtkImplicitPlaneRepresentation::Outside);
      this->InteractionState = vtkImplicitPlaneRepresentation::Outside;
      return this->InteractionState;
    }

    // Something picked, continue
    this->ValidPick = 1;

    // Depending on the interaction state (set by the widget) we modify
    // this state based on what is picked.
    if (this->InteractionState == vtkImplicitPlaneRepresentation::Moving)
    {
      vtkProp* prop = path->GetFirstNode()->GetViewProp();
      if (prop == this->ConeActor || prop == this->LineActor || prop == this->ConeActor2 ||
        prop == this->LineActor2)
      {
        this->InteractionState = vtkImplicitPlaneRepresentation::Rotating;
        this->SetRepresentationState(vtkImplicitPlaneRepresentation::Rotating);
      }
      else if (prop == this->CutActor)
      {
        if (this->LockNormalToCamera)
        { // Allow camera to work
          this->InteractionState = vtkImplicitPlaneRepresentation::Outside;
          this->SetRepresentationState(vtkImplicitPlaneRepresentation::Outside);
        }
        else
        {
          this->InteractionState = vtkImplicitPlaneRepresentation::Pushing;
          this->SetRepresentationState(vtkImplicitPlaneRepresentation::Pushing);
        }
      }
      else if (prop == this->SphereActor)
      {
        this->InteractionState = vtkImplicitPlaneRepresentation::MovingOrigin;
        this->SetRepresentationState(vtkImplicitPlaneRepresentation::MovingOrigin);
      }
      else
      {
        if (this->OutlineTranslation)
        {
          this->InteractionState = vtkImplicitPlaneRepresentation::MovingOutline;
          this->SetRepresentationState(vtkImplicitPlaneRepresentation::MovingOutline);
        }
        else
        {
          this->InteractionState = vtkImplicitPlaneRepresentation::Outside;
          this->SetRepresentationState(vtkImplicitPlaneRepresentation::Outside);
        }
      }
    }

    // We may add a condition to allow the camera to work IO scaling
    else if (this->InteractionState != vtkImplicitPlaneRepresentation::Scaling)
    {
      this->InteractionState = vtkImplicitPlaneRepresentation::Outside;
    }
  }

  return this->InteractionState;
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::SetRepresentationState(int state)
{
  if (this->RepresentationState == state)
  {
    return;
  }

  // Clamp the state
  state = (state < vtkImplicitPlaneRepresentation::Outside
      ? vtkImplicitPlaneRepresentation::Outside
      : (state > vtkImplicitPlaneRepresentation::Scaling ? vtkImplicitPlaneRepresentation::Scaling
                                                         : state));

  this->RepresentationState = state;
  this->Modified();

  if (state == vtkImplicitPlaneRepresentation::Rotating)
  {
    this->HighlightNormal(1);
    this->HighlightPlane(1);
  }
  else if (state == vtkImplicitPlaneRepresentation::Pushing)
  {
    this->HighlightNormal(1);
    this->HighlightPlane(1);
  }
  else if (state == vtkImplicitPlaneRepresentation::MovingOrigin)
  {
    this->HighlightNormal(1);
  }
  else if (state == vtkImplicitPlaneRepresentation::MovingOutline)
  {
    this->HighlightOutline(1);
  }
  else if (state == vtkImplicitPlaneRepresentation::Scaling && this->ScaleEnabled)
  {
    this->HighlightNormal(1);
    this->HighlightPlane(1);
    this->HighlightOutline(1);
  }
  else
  {
    this->HighlightNormal(0);
    this->HighlightPlane(0);
    this->HighlightOutline(0);
  }
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::StartWidgetInteraction(double e[2])
{
  this->StartEventPosition[0] = e[0];
  this->StartEventPosition[1] = e[1];
  this->StartEventPosition[2] = 0.0;

  this->LastEventPosition[0] = e[0];
  this->LastEventPosition[1] = e[1];
  this->LastEventPosition[2] = 0.0;
}

void vtkImplicitPlaneRepresentation::StartComplexInteraction(
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

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::WidgetInteraction(double e[2])
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
  this->Picker->GetPickPosition(pos);
  vtkInteractorObserver::ComputeWorldToDisplay(this->Renderer, pos[0], pos[1], pos[2], focalPoint);
  z = focalPoint[2];
  vtkInteractorObserver::ComputeDisplayToWorld(
    this->Renderer, this->LastEventPosition[0], this->LastEventPosition[1], z, prevPickPoint);
  vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer, e[0], e[1], z, pickPoint);

  // Process the motion
  if (this->InteractionState == vtkImplicitPlaneRepresentation::MovingOutline)
  {
    this->TranslateOutline(prevPickPoint, pickPoint);
  }
  else if (this->InteractionState == vtkImplicitPlaneRepresentation::MovingOrigin)
  {
    this->TranslateOrigin(prevPickPoint, pickPoint);
  }
  else if (this->InteractionState == vtkImplicitPlaneRepresentation::Pushing)
  {
    this->Push(prevPickPoint, pickPoint);
  }
  else if (this->InteractionState == vtkImplicitPlaneRepresentation::Scaling && this->ScaleEnabled)
  {
    this->Scale(prevPickPoint, pickPoint, e[0], e[1]);
  }
  else if (this->InteractionState == vtkImplicitPlaneRepresentation::Rotating)
  {
    camera->GetViewPlaneNormal(vpn);
    this->Rotate(e[0], e[1], prevPickPoint, pickPoint, vpn);
  }
  else if (this->InteractionState == vtkImplicitPlaneRepresentation::Outside &&
    this->LockNormalToCamera)
  {
    this->SetNormalToCamera();
  }

  this->LastEventPosition[0] = e[0];
  this->LastEventPosition[1] = e[1];
  this->LastEventPosition[2] = 0.0;
}

void vtkImplicitPlaneRepresentation::ComplexInteraction(
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
    if (this->InteractionState == vtkImplicitPlaneRepresentation::MovingOutline)
    {
      // this->TranslateOutline(this->LastEventPosition, eventPos);
      this->UpdatePose(this->LastEventPosition, this->LastEventOrientation, eventPos, eventDir);
    }
    else if (this->InteractionState == vtkImplicitPlaneRepresentation::MovingOrigin)
    {
      this->UpdatePose(this->LastEventPosition, this->LastEventOrientation, eventPos, eventDir);
    }
    else if (this->InteractionState == vtkImplicitPlaneRepresentation::Pushing)
    {
      // this->Push(this->LastEventPosition, eventPos);
      this->UpdatePose(this->LastEventPosition, this->LastEventOrientation, eventPos, eventDir);
    }
    else if (this->InteractionState == vtkImplicitPlaneRepresentation::Scaling &&
      this->ScaleEnabled)
    {
      this->Scale(this->LastEventPosition, eventPos, 0.0, 0.0); // todo handle this
    }
    else if (this->InteractionState == vtkImplicitPlaneRepresentation::Rotating)
    {
      this->Rotate3D(this->LastEventPosition, eventPos);
    }
    else if (this->InteractionState == vtkImplicitPlaneRepresentation::Outside &&
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

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::EndWidgetInteraction(double vtkNotUsed(e)[2])
{
  this->SetRepresentationState(vtkImplicitPlaneRepresentation::Outside);
}

void vtkImplicitPlaneRepresentation::EndComplexInteraction(
  vtkRenderWindowInteractor*, vtkAbstractWidget*, unsigned long, void*)
{
  this->SetRepresentationState(vtkImplicitPlaneRepresentation::Outside);
}

//----------------------------------------------------------------------
double* vtkImplicitPlaneRepresentation::GetBounds()
{
  this->BuildRepresentation();
  this->BoundingBox->SetBounds(this->OutlineActor->GetBounds());
  this->BoundingBox->AddBounds(this->CutActor->GetBounds());
  this->BoundingBox->AddBounds(this->EdgesActor->GetBounds());
  this->BoundingBox->AddBounds(this->ConeActor->GetBounds());
  this->BoundingBox->AddBounds(this->LineActor->GetBounds());
  this->BoundingBox->AddBounds(this->ConeActor2->GetBounds());
  this->BoundingBox->AddBounds(this->LineActor2->GetBounds());
  this->BoundingBox->AddBounds(this->SphereActor->GetBounds());

  return this->BoundingBox->GetBounds();
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::GetActors(vtkPropCollection* pc)
{
  this->OutlineActor->GetActors(pc);
  this->CutActor->GetActors(pc);
  this->EdgesActor->GetActors(pc);
  this->ConeActor->GetActors(pc);
  this->LineActor->GetActors(pc);
  this->ConeActor2->GetActors(pc);
  this->LineActor2->GetActors(pc);
  this->SphereActor->GetActors(pc);
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::ReleaseGraphicsResources(vtkWindow* w)
{
  this->OutlineActor->ReleaseGraphicsResources(w);
  this->CutActor->ReleaseGraphicsResources(w);
  this->EdgesActor->ReleaseGraphicsResources(w);
  this->ConeActor->ReleaseGraphicsResources(w);
  this->LineActor->ReleaseGraphicsResources(w);
  this->ConeActor2->ReleaseGraphicsResources(w);
  this->LineActor2->ReleaseGraphicsResources(w);
  this->SphereActor->ReleaseGraphicsResources(w);
}

//----------------------------------------------------------------------------
int vtkImplicitPlaneRepresentation::RenderOpaqueGeometry(vtkViewport* v)
{
  int count = 0;
  this->BuildRepresentation();
  if (this->DrawOutline)
  {
    count += this->OutlineActor->RenderOpaqueGeometry(v);
  }
  count += this->EdgesActor->RenderOpaqueGeometry(v);
  if (!this->LockNormalToCamera)
  {
    count += this->ConeActor->RenderOpaqueGeometry(v);
    count += this->LineActor->RenderOpaqueGeometry(v);
    count += this->ConeActor2->RenderOpaqueGeometry(v);
    count += this->LineActor2->RenderOpaqueGeometry(v);
    count += this->SphereActor->RenderOpaqueGeometry(v);
  }
  if (this->DrawPlane)
  {
    count += this->CutActor->RenderOpaqueGeometry(v);
  }

  return count;
}

//-----------------------------------------------------------------------------
int vtkImplicitPlaneRepresentation::RenderTranslucentPolygonalGeometry(vtkViewport* v)
{
  int count = 0;
  this->BuildRepresentation();
  if (this->DrawOutline)
  {
    count += this->OutlineActor->RenderTranslucentPolygonalGeometry(v);
  }
  count += this->EdgesActor->RenderTranslucentPolygonalGeometry(v);
  if (!this->LockNormalToCamera)
  {
    count += this->ConeActor->RenderTranslucentPolygonalGeometry(v);
    count += this->LineActor->RenderTranslucentPolygonalGeometry(v);
    count += this->ConeActor2->RenderTranslucentPolygonalGeometry(v);
    count += this->LineActor2->RenderTranslucentPolygonalGeometry(v);
    count += this->SphereActor->RenderTranslucentPolygonalGeometry(v);
  }
  if (this->DrawPlane)
  {
    count += this->CutActor->RenderTranslucentPolygonalGeometry(v);
  }

  return count;
}

//-----------------------------------------------------------------------------
vtkTypeBool vtkImplicitPlaneRepresentation::HasTranslucentPolygonalGeometry()
{
  int result = 0;
  if (this->DrawOutline)
  {
    result |= this->OutlineActor->HasTranslucentPolygonalGeometry();
  }
  result |= this->EdgesActor->HasTranslucentPolygonalGeometry();
  if (!this->LockNormalToCamera)
  {
    result |= this->ConeActor->HasTranslucentPolygonalGeometry();
    result |= this->LineActor->HasTranslucentPolygonalGeometry();
    result |= this->ConeActor2->HasTranslucentPolygonalGeometry();
    result |= this->LineActor2->HasTranslucentPolygonalGeometry();
    result |= this->SphereActor->HasTranslucentPolygonalGeometry();
  }
  if (this->DrawPlane)
  {
    result |= this->CutActor->HasTranslucentPolygonalGeometry();
  }

  return result;
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Snap To Axes: " << (this->SnapToAxes ? "On\n" : "Off\n");

  if (this->NormalProperty)
  {
    os << indent << "Normal Property: " << this->NormalProperty << "\n";
  }
  else
  {
    os << indent << "Normal Property: (none)\n";
  }
  if (this->SelectedNormalProperty)
  {
    os << indent << "Selected Normal Property: " << this->SelectedNormalProperty << "\n";
  }
  else
  {
    os << indent << "Selected Normal Property: (none)\n";
  }

  if (this->PlaneProperty)
  {
    os << indent << "Plane Property: " << this->PlaneProperty << "\n";
  }
  else
  {
    os << indent << "Plane Property: (none)\n";
  }
  if (this->SelectedPlaneProperty)
  {
    os << indent << "Selected Plane Property: " << this->SelectedPlaneProperty << "\n";
  }
  else
  {
    os << indent << "Selected Plane Property: (none)\n";
  }

  if (this->OutlineProperty)
  {
    os << indent << "Outline Property: " << this->OutlineProperty << "\n";
  }
  else
  {
    os << indent << "Outline Property: (none)\n";
  }
  if (this->SelectedOutlineProperty)
  {
    os << indent << "Selected Outline Property: " << this->SelectedOutlineProperty << "\n";
  }
  else
  {
    os << indent << "Selected Outline Property: (none)\n";
  }

  if (this->EdgesProperty)
  {
    os << indent << "Edges Property: " << this->EdgesProperty << "\n";
  }
  else
  {
    os << indent << "Edges Property: (none)\n";
  }

  os << indent << "Crop plane to bounding box: " << (this->CropPlaneToBoundingBox ? "On" : "Off")
     << "\n";

  os << indent << "Normal To X Axis: " << (this->NormalToXAxis ? "On" : "Off") << "\n";
  os << indent << "Normal To Y Axis: " << (this->NormalToYAxis ? "On" : "Off") << "\n";
  os << indent << "Normal To Z Axis: " << (this->NormalToZAxis ? "On" : "Off") << "\n";
  os << indent << "Lock Normal To Camera: " << (this->LockNormalToCamera ? "On" : "Off") << "\n";

  os << indent << "Widget Bounds: " << this->WidgetBounds[0] << ", " << this->WidgetBounds[1]
     << ", " << this->WidgetBounds[2] << ", " << this->WidgetBounds[3] << ", "
     << this->WidgetBounds[4] << ", " << this->WidgetBounds[5] << "\n";

  os << indent << "Tubing: " << (this->Tubing ? "On" : "Off") << "\n";
  os << indent << "Outline Translation: " << (this->OutlineTranslation ? "On" : "Off") << "\n";
  os << indent << "Outside Bounds: " << (this->OutsideBounds ? "On" : "Off") << "\n";
  os << indent << "Constrain to Widget Bounds: " << (this->ConstrainToWidgetBounds ? "On" : "Off")
     << "\n";
  os << indent << "Scale Enabled: " << (this->ScaleEnabled ? "On" : "Off") << "\n";
  os << indent << "Draw Outline: " << (this->DrawOutline ? "On" : "Off") << "\n";
  os << indent << "Draw Plane: " << (this->DrawPlane ? "On" : "Off") << "\n";
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
    case Scaling:
      os << "Scaling\n";
      break;
  }

  // this->InteractionState is printed in superclass
  // this is commented to avoid PrintSelf errors
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::HighlightNormal(int highlight)
{
  if (highlight)
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
void vtkImplicitPlaneRepresentation::HighlightPlane(int highlight)
{
  if (highlight)
  {
    this->CutActor->SetProperty(this->SelectedPlaneProperty);
  }
  else
  {
    this->CutActor->SetProperty(this->PlaneProperty);
  }
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::HighlightOutline(int highlight)
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

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::Rotate(double X, double Y, double* p1, double* p2, double* vpn)
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

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::Rotate3D(double* p1, double* p2)
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

//----------------------------------------------------------------------------
// Loop through all points and translate them
void vtkImplicitPlaneRepresentation::TranslateOutline(double* p1, double* p2)
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

//----------------------------------------------------------------------------
// Loop through all points and translate them
void vtkImplicitPlaneRepresentation::TranslateOrigin(double* p1, double* p2)
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

//----------------------------------------------------------------------------
// Loop through all points and translate and rotate them
void vtkImplicitPlaneRepresentation::UpdatePose(double* p1, double* d1, double* p2, double* d2)
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

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::Scale(double* p1, double* p2, double vtkNotUsed(X), double Y)
{
  // Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  double* o = this->Plane->GetOrigin();

  // Compute the scale factor
  double sf = vtkMath::Norm(v) / this->Outline->GetOutput()->GetLength();
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

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::Push(double* p1, double* p2)
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

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::CreateDefaultProperties()
{
  // Normal properties
  this->NormalProperty = vtkProperty::New();
  this->NormalProperty->SetColor(1, 1, 1);
  this->NormalProperty->SetLineWidth(2);

  this->SelectedNormalProperty = vtkProperty::New();
  this->SelectedNormalProperty->SetColor(1, 0, 0);
  this->NormalProperty->SetLineWidth(2);

  // Plane properties
  this->PlaneProperty = vtkProperty::New();
  this->PlaneProperty->SetAmbient(1.0);
  this->PlaneProperty->SetAmbientColor(1.0, 1.0, 1.0);
  this->PlaneProperty->SetOpacity(0.5);
  this->CutActor->SetProperty(this->PlaneProperty);

  this->SelectedPlaneProperty = vtkProperty::New();
  this->SelectedPlaneProperty->SetAmbient(1.0);
  this->SelectedPlaneProperty->SetAmbientColor(0.0, 1.0, 0.0);
  this->SelectedPlaneProperty->SetOpacity(0.25);

  // Outline properties
  this->OutlineProperty = vtkProperty::New();
  this->OutlineProperty->SetAmbient(1.0);
  this->OutlineProperty->SetAmbientColor(1.0, 1.0, 1.0);

  this->SelectedOutlineProperty = vtkProperty::New();
  this->SelectedOutlineProperty->SetAmbient(1.0);
  this->SelectedOutlineProperty->SetAmbientColor(0.0, 1.0, 0.0);

  // Edge property
  this->EdgesProperty = vtkProperty::New();
  this->EdgesProperty->SetAmbient(1.0);
  this->EdgesProperty->SetAmbientColor(1.0, 1.0, 1.0);
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::SetEdgeColor(vtkLookupTable* lut)
{
  this->EdgesMapper->SetLookupTable(lut);
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::SetEdgeColor(double r, double g, double b)
{
  vtkSmartPointer<vtkLookupTable> lookupTable = vtkSmartPointer<vtkLookupTable>::New();

  lookupTable->SetTableRange(0.0, 1.0);
  lookupTable->SetNumberOfTableValues(1);
  lookupTable->SetTableValue(0, r, g, b);
  lookupTable->Build();

  this->SetEdgeColor(lookupTable);
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::SetEdgeColor(double c[3])
{
  this->SetEdgeColor(c[0], c[1], c[2]);
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::PlaceWidget(double bds[6])
{
  int i;
  double bounds[6], origin[3];

  this->AdjustBounds(bds, bounds, origin);

  // Set up the bounding box
  this->Box->SetOrigin(bounds[0], bounds[2], bounds[4]);
  this->Box->SetSpacing((bounds[1] - bounds[0]), (bounds[3] - bounds[2]), (bounds[5] - bounds[4]));
  this->Outline->Update();

  this->InitialLength = sqrt((bounds[1] - bounds[0]) * (bounds[1] - bounds[0]) +
    (bounds[3] - bounds[2]) * (bounds[3] - bounds[2]) +
    (bounds[5] - bounds[4]) * (bounds[5] - bounds[4]));

  this->LineSource->SetPoint1(this->Plane->GetOrigin());
  this->PlaneSource->SetOrigin(0, 0, 0);
  if (this->NormalToYAxis)
  {
    this->Plane->SetNormal(0, 1, 0);
    this->LineSource->SetPoint2(0, 1, 0);
    this->PlaneSource->SetPoint1(this->InitialLength, 0, 0);
    this->PlaneSource->SetPoint2(0, 0, this->InitialLength);
  }
  else if (this->NormalToZAxis)
  {
    this->Plane->SetNormal(0, 0, 1);
    this->LineSource->SetPoint2(0, 0, 1);
    this->PlaneSource->SetPoint1(this->InitialLength, 0, 0);
    this->PlaneSource->SetPoint2(0, this->InitialLength, 0);
  }
  else // default or x-normal
  {
    this->Plane->SetNormal(1, 0, 0);
    this->LineSource->SetPoint2(1, 0, 0);
    this->PlaneSource->SetPoint1(0, this->InitialLength, 0);
    this->PlaneSource->SetPoint2(0, 0, this->InitialLength);
  }

  for (i = 0; i < 6; i++)
  {
    this->InitialBounds[i] = bounds[i];
    this->WidgetBounds[i] = bounds[i];
  }

  this->ValidPick = 1; // since we have positioned the widget successfully
  this->BuildRepresentation();
}

//----------------------------------------------------------------------------
// Description:
// Set the origin of the plane.
void vtkImplicitPlaneRepresentation::SetOrigin(double x, double y, double z)
{
  double origin[3];
  origin[0] = x;
  origin[1] = y;
  origin[2] = z;
  this->SetOrigin(origin);
}

//----------------------------------------------------------------------------
// Description:
// Set the origin of the plane. Note that the origin is clamped slightly inside
// the bounding box or the plane tends to disappear as it hits the boundary (and
// when the plane is parallel to one of the faces of the bounding box).
void vtkImplicitPlaneRepresentation::SetOrigin(double x[3])
{
  this->Plane->SetOrigin(x);
  this->BuildRepresentation();
}

//----------------------------------------------------------------------------
// Description:
// Get the origin of the plane.
double* vtkImplicitPlaneRepresentation::GetOrigin()
{
  return this->Plane->GetOrigin();
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::GetOrigin(double xyz[3])
{
  this->Plane->GetOrigin(xyz);
}

//----------------------------------------------------------------------------
// Description:
// Set the normal to the plane.
void vtkImplicitPlaneRepresentation::SetNormal(double x, double y, double z)
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

//----------------------------------------------------------------------------
// Description:
// Set the normal to the plane.
void vtkImplicitPlaneRepresentation::SetNormal(double n[3])
{
  this->SetNormal(n[0], n[1], n[2]);
}

//----------------------------------------------------------------------------
// Description:
// Get the normal to the plane.
double* vtkImplicitPlaneRepresentation::GetNormal()
{
  return this->Plane->GetNormal();
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::GetNormal(double xyz[3])
{
  this->Plane->GetNormal(xyz);
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::SetDrawPlane(vtkTypeBool drawPlane)
{
  if (drawPlane == this->DrawPlane)
  {
    return;
  }

  this->Modified();
  this->DrawPlane = drawPlane;
  this->BuildRepresentation();
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::SetDrawOutline(vtkTypeBool val)
{
  if (val == this->DrawOutline)
  {
    return;
  }

  if (val)
  {
    this->Picker->AddPickList(this->OutlineActor);
  }
  else
  {
    this->Picker->DeletePickList(this->OutlineActor);
  }
  this->Modified();
  this->DrawOutline = val;
  this->BuildRepresentation();
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::SetNormalToXAxis(vtkTypeBool var)
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
void vtkImplicitPlaneRepresentation::SetNormalToYAxis(vtkTypeBool var)
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
void vtkImplicitPlaneRepresentation::SetNormalToZAxis(vtkTypeBool var)
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
void vtkImplicitPlaneRepresentation::GetPolyData(vtkPolyData* pd)
{
  this->Cutter->Update();
  pd->ShallowCopy(this->Cutter->GetOutput());
}

//----------------------------------------------------------------------------
vtkPolyDataAlgorithm* vtkImplicitPlaneRepresentation::GetPolyDataAlgorithm()
{
  return this->Cutter;
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::GetPlane(vtkPlane* plane)
{
  if (plane == nullptr)
  {
    return;
  }

  plane->SetNormal(this->Plane->GetNormal());
  plane->SetOrigin(this->Plane->GetOrigin());
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::SetPlane(vtkPlane* plane)
{
  if (plane == nullptr)
  {
    return;
  }

  this->Plane->SetNormal(plane->GetNormal());
  this->Plane->SetOrigin(plane->GetOrigin());
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::UpdatePlacement()
{
  this->Outline->Update();
  this->Cutter->Update();
  this->Edges->Update();
  this->BuildRepresentation();
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::BumpPlane(int dir, double factor)
{
  // Compute the distance
  double d = this->InitialLength * this->BumpDistance * factor;

  // Push the plane
  this->PushPlane((dir > 0 ? d : -d));
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::PushPlane(double d)
{
  this->Plane->Push(d);
  this->BuildRepresentation();
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::BuildRepresentation()
{
  if (!this->Renderer || !this->Renderer->GetRenderWindow())
  {
    return;
  }

  vtkInformation* info = this->GetPropertyKeys();
  this->OutlineActor->SetPropertyKeys(info);
  this->CutActor->SetPropertyKeys(info);
  this->EdgesActor->SetPropertyKeys(info);
  this->ConeActor->SetPropertyKeys(info);
  this->LineActor->SetPropertyKeys(info);
  this->ConeActor2->SetPropertyKeys(info);
  this->LineActor2->SetPropertyKeys(info);
  this->SphereActor->SetPropertyKeys(info);

  if (this->GetMTime() > this->BuildTime || this->Plane->GetMTime() > this->BuildTime ||
    this->Renderer->GetRenderWindow()->GetMTime() > this->BuildTime)
  {
    double* origin = this->Plane->GetOrigin();
    double* normal = this->Plane->GetNormal();

    double bounds[6];
    std::copy(this->WidgetBounds, this->WidgetBounds + 6, bounds);

    double p2[3];
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

    this->PlaneSource->SetCenter(origin);
    this->PlaneSource->SetNormal(normal);

    // Setup the plane normal
    double d = this->Outline->GetOutput()->GetLength();

    p2[0] = origin[0] + 0.30 * d * normal[0];
    p2[1] = origin[1] + 0.30 * d * normal[1];
    p2[2] = origin[2] + 0.30 * d * normal[2];

    this->LineSource->SetPoint1(origin);
    this->LineSource->SetPoint2(p2);
    this->ConeSource->SetCenter(p2);
    this->ConeSource->SetDirection(normal);

    p2[0] = origin[0] - 0.30 * d * normal[0];
    p2[1] = origin[1] - 0.30 * d * normal[1];
    p2[2] = origin[2] - 0.30 * d * normal[2];

    this->LineSource2->SetPoint1(origin[0], origin[1], origin[2]);
    this->LineSource2->SetPoint2(p2);
    this->ConeSource2->SetCenter(p2);
    this->ConeSource2->SetDirection(normal[0], normal[1], normal[2]);

    // Set up the position handle
    this->Sphere->SetCenter(origin[0], origin[1], origin[2]);

    // Control the look of the edges
    if (this->Tubing)
    {
      this->EdgesMapper->SetInputConnection(this->EdgesTuber->GetOutputPort());
    }
    else
    {
      this->EdgesMapper->SetInputConnection(this->Edges->GetOutputPort());
    }

    this->SizeHandles();
    this->BuildTime.Modified();
  }
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::SizeHandles()
{
  double radius =
    this->vtkWidgetRepresentation::SizeHandlesInPixels(1.5, this->Sphere->GetCenter());

  this->ConeSource->SetHeight(2.0 * radius);
  this->ConeSource->SetRadius(radius);
  this->ConeSource2->SetHeight(2.0 * radius);
  this->ConeSource2->SetRadius(radius);

  this->Sphere->SetRadius(radius);

  this->EdgesTuber->SetRadius(0.25 * radius);
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::SetNormalToCamera()
{
  if (!this->Renderer)
  {
    return;
  }

  double normal[3];
  this->Renderer->GetActiveCamera()->GetViewPlaneNormal(normal);
  this->SetNormal(normal);
}

//----------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::RegisterPickers()
{
  vtkPickingManager* pm = this->GetPickingManager();
  if (!pm)
  {
    return;
  }
  pm->AddPicker(this->Picker, this);
}
