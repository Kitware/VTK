// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCoordinateFrameRepresentation.h"

#include "vtkActor.h"
#include "vtkAssemblyPath.h"
#include "vtkBox.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCellPicker.h"
#include "vtkConeSource.h"
#include "vtkGenericCell.h"
#include "vtkHardwarePicker.h"
#include "vtkImageData.h"
#include "vtkInteractorObserver.h"
#include "vtkLineSource.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
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
#include "vtkWindow.h"

#include <algorithm>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkCoordinateFrameRepresentation);

static constexpr double DefaultPickTol = 0.001;

//------------------------------------------------------------------------------
vtkCoordinateFrameRepresentation::vtkCoordinateFrameRepresentation()
{
  this->RepresentationState = vtkCoordinateFrameRepresentation::Outside;

  // Handle size is in pixels for this widget
  this->HandleSize = 5.0;

  this->PickCameraFocalInfo = false;

  this->XVectorIsLocked = false;
  this->YVectorIsLocked = false;
  this->ZVectorIsLocked = false;

  // Create the origin handle
  this->OriginSphereSource->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
  this->OriginSphereSource->SetThetaResolution(16);
  this->OriginSphereSource->SetPhiResolution(8);
  this->OriginSphereMapper->SetInputConnection(this->OriginSphereSource->GetOutputPort());
  this->OriginSphereActor->SetMapper(this->OriginSphereMapper);

  // Create the X vector
  this->XVectorLineSource->SetResolution(1);
  this->XVectorLineSource->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
  this->XVectorLineMapper->SetInputConnection(this->XVectorLineSource->GetOutputPort());
  this->XVectorLineActor->SetMapper(this->XVectorLineMapper);

  this->XVectorConeSource->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
  this->XVectorConeSource->SetResolution(12);
  this->XVectorConeSource->SetAngle(25.0);
  this->XVectorConeMapper->SetInputConnection(this->XVectorConeSource->GetOutputPort());
  this->XVectorConeActor->SetMapper(this->XVectorConeMapper);

  this->LockerXVectorConeSource->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
  this->LockerXVectorConeSource->SetResolution(12);
  this->LockerXVectorConeSource->SetAngle(25.0);
  this->LockerXVectorConeMapper->SetInputConnection(this->LockerXVectorConeSource->GetOutputPort());
  this->LockerXVectorConeActor->SetMapper(this->LockerXVectorConeMapper);

  // Create the Y vector
  this->YVectorLineSource->SetResolution(1);
  this->YVectorLineSource->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
  this->YVectorLineMapper->SetInputConnection(this->YVectorLineSource->GetOutputPort());
  this->YVectorLineActor->SetMapper(this->YVectorLineMapper);

  this->YVectorConeSource->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
  this->YVectorConeSource->SetResolution(12);
  this->YVectorConeSource->SetAngle(25.0);
  this->YVectorConeMapper->SetInputConnection(this->YVectorConeSource->GetOutputPort());
  this->YVectorConeActor->SetMapper(this->YVectorConeMapper);

  this->LockerYVectorConeSource->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
  this->LockerYVectorConeSource->SetResolution(12);
  this->LockerYVectorConeSource->SetAngle(25.0);
  this->LockerYVectorConeMapper->SetInputConnection(this->LockerYVectorConeSource->GetOutputPort());
  this->LockerYVectorConeActor->SetMapper(this->LockerYVectorConeMapper);

  // Create the Z vector
  this->ZVectorLineSource->SetResolution(1);
  this->ZVectorLineSource->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
  this->ZVectorLineMapper->SetInputConnection(this->ZVectorLineSource->GetOutputPort());
  this->ZVectorLineActor->SetMapper(this->ZVectorLineMapper);

  this->ZVectorConeSource->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
  this->ZVectorConeSource->SetResolution(12);
  this->ZVectorConeSource->SetAngle(25.0);
  this->ZVectorConeMapper->SetInputConnection(this->ZVectorConeSource->GetOutputPort());
  this->ZVectorConeActor->SetMapper(this->ZVectorConeMapper);

  this->LockerZVectorConeSource->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
  this->LockerZVectorConeSource->SetResolution(12);
  this->LockerZVectorConeSource->SetAngle(25.0);
  this->LockerZVectorConeMapper->SetInputConnection(this->LockerZVectorConeSource->GetOutputPort());
  this->LockerZVectorConeActor->SetMapper(this->LockerZVectorConeMapper);

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
  this->CellPicker->PickFromListOn();
  this->CellPicker->AddPickList(this->OriginSphereActor);

  this->CellPicker->AddPickList(this->XVectorLineActor);
  this->CellPicker->AddPickList(this->XVectorConeActor);
  this->CellPicker->AddPickList(this->LockerXVectorConeActor);

  this->CellPicker->AddPickList(this->YVectorLineActor);
  this->CellPicker->AddPickList(this->YVectorConeActor);
  this->CellPicker->AddPickList(this->LockerYVectorConeActor);

  this->CellPicker->AddPickList(this->ZVectorLineActor);
  this->CellPicker->AddPickList(this->ZVectorConeActor);
  this->CellPicker->AddPickList(this->LockerZVectorConeActor);

  this->HardwarePicker->PickFromListOff();

  // Set up the initial properties
  this->CreateDefaultProperties();

  this->OriginSphereActor->SetProperty(this->OriginProperty);

  this->XVectorLineActor->SetProperty(this->XVectorProperty);
  this->XVectorConeActor->SetProperty(this->XVectorProperty);
  this->LockerXVectorConeActor->SetProperty(this->UnlockedXVectorProperty);

  this->YVectorLineActor->SetProperty(this->YVectorProperty);
  this->YVectorConeActor->SetProperty(this->YVectorProperty);
  this->LockerYVectorConeActor->SetProperty(this->UnlockedYVectorProperty);

  this->ZVectorLineActor->SetProperty(this->ZVectorProperty);
  this->ZVectorConeActor->SetProperty(this->ZVectorProperty);
  this->LockerZVectorConeActor->SetProperty(this->UnlockedZVectorProperty);

  this->TranslationAxis = Axis::NONE;
}

//------------------------------------------------------------------------------
vtkCoordinateFrameRepresentation::~vtkCoordinateFrameRepresentation() = default;

//------------------------------------------------------------------------------
void vtkCoordinateFrameRepresentation::CreateDefaultProperties()
{
  static constexpr double red[3] = { 1.0, 0.0, 0.0 };
  static constexpr double yellow[3] = { 1.0, 1.0, 0.0 };
  static constexpr double green[3] = { 0.0, 1.0, 0.0 };
  static constexpr double white[3] = { 0.9, 0.9, 0.9 };

  // origin properties
  this->OriginProperty->SetColor(white[0], white[1], white[2]);
  this->SelectedOriginProperty->SetAmbient(1.0);
  this->SelectedOriginProperty->SetColor(white[0], white[1], white[2]);

  // XVector properties
  this->XVectorProperty->SetColor(red[0], red[1], red[2]);
  this->XVectorProperty->SetLineWidth(5);
  this->SelectedXVectorProperty->SetAmbient(1.0);
  this->SelectedXVectorProperty->SetColor(red[0], red[1], red[2]);
  this->SelectedXVectorProperty->SetLineWidth(5);
  // LockedXVector properties
  this->LockedXVectorProperty->SetColor(red[0], red[1], red[2]);
  this->SelectedLockedXVectorProperty->SetAmbient(1.0);
  this->SelectedLockedXVectorProperty->SetColor(red[0], red[1], red[2]);
  // UnlockedXVector properties
  this->UnlockedXVectorProperty->SetColor(red[0], red[1], red[2]);
  this->UnlockedXVectorProperty->SetOpacity(0.3);
  this->SelectedUnlockedXVectorProperty->SetAmbient(1.0);
  this->SelectedUnlockedXVectorProperty->SetColor(red[0], red[1], red[2]);
  this->SelectedUnlockedXVectorProperty->SetOpacity(0.3);

  // YVector properties
  this->YVectorProperty->SetColor(yellow[0], yellow[1], yellow[2]);
  this->YVectorProperty->SetLineWidth(5);
  this->SelectedYVectorProperty->SetAmbient(1.0);
  this->SelectedYVectorProperty->SetColor(yellow[0], yellow[1], yellow[2]);
  this->SelectedYVectorProperty->SetLineWidth(5);
  // LockedYVector properties
  this->LockedYVectorProperty->SetColor(yellow[0], yellow[1], yellow[2]);
  this->SelectedLockedYVectorProperty->SetAmbient(1.0);
  this->SelectedLockedYVectorProperty->SetColor(yellow[0], yellow[1], yellow[2]);
  // UnlockedYVector properties
  this->UnlockedYVectorProperty->SetColor(yellow[0], yellow[1], yellow[2]);
  this->UnlockedYVectorProperty->SetOpacity(0.3);
  this->SelectedUnlockedYVectorProperty->SetAmbient(1.0);
  this->SelectedUnlockedYVectorProperty->SetColor(yellow[0], yellow[1], yellow[2]);
  this->SelectedUnlockedYVectorProperty->SetOpacity(0.3);

  // ZVector properties
  this->ZVectorProperty->SetColor(green[0], green[1], green[2]);
  this->ZVectorProperty->SetLineWidth(5);
  this->SelectedZVectorProperty->SetAmbient(1.0);
  this->SelectedZVectorProperty->SetColor(green[0], green[1], green[2]);
  this->SelectedZVectorProperty->SetLineWidth(5);
  // LockNormalZ properties
  this->LockedZVectorProperty->SetColor(green[0], green[1], green[2]);
  this->SelectedLockedZVectorProperty->SetAmbient(1.0);
  this->SelectedLockedZVectorProperty->SetColor(green[0], green[1], green[2]);
  // UnlockNormalZ properties
  this->UnlockedZVectorProperty->SetColor(green[0], green[1], green[2]);
  this->UnlockedZVectorProperty->SetOpacity(0.3);
  this->SelectedUnlockedZVectorProperty->SetAmbient(1.0);
  this->SelectedUnlockedZVectorProperty->SetColor(green[0], green[1], green[2]);
  this->SelectedUnlockedZVectorProperty->SetOpacity(0.3);
}

//------------------------------------------------------------------------------
void vtkCoordinateFrameRepresentation::SetLockNormalToCamera(vtkTypeBool lock)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting " << LockNormalToCamera
                << " to " << lock);
  if (lock == this->LockNormalToCamera)
  {
    return;
  }

  if (lock)
  {
    this->CellPicker->DeletePickList(this->OriginSphereActor);
    this->CellPicker->DeletePickList(this->XVectorLineActor);
    this->CellPicker->DeletePickList(this->XVectorConeActor);
    this->CellPicker->DeletePickList(this->YVectorLineActor);
    this->CellPicker->DeletePickList(this->YVectorConeActor);
    this->CellPicker->DeletePickList(this->ZVectorLineActor);
    this->CellPicker->DeletePickList(this->ZVectorConeActor);

    this->SetNormalToCamera();
  }
  else
  {
    this->CellPicker->AddPickList(this->OriginSphereActor);
    this->CellPicker->AddPickList(this->XVectorLineActor);
    this->CellPicker->AddPickList(this->XVectorConeActor);
    this->CellPicker->AddPickList(this->YVectorLineActor);
    this->CellPicker->AddPickList(this->YVectorConeActor);
    this->CellPicker->AddPickList(this->ZVectorLineActor);
    this->CellPicker->AddPickList(this->ZVectorConeActor);
  }

  this->LockNormalToCamera = lock;
  this->Modified();
}

//------------------------------------------------------------------------------
int vtkCoordinateFrameRepresentation::ComputeInteractionState(int X, int Y, int vtkNotUsed(modify))
{
  // See if anything has been selected
  this->ComputeAdaptivePickerTolerance();
  vtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->CellPicker);

  if (path == nullptr) // Not picking this widget
  {
    this->SetRepresentationState(vtkCoordinateFrameRepresentation::Outside);
    this->InteractionState = vtkCoordinateFrameRepresentation::Outside;
    return this->InteractionState;
  }

  // Something picked, continue
  this->ValidPick = 1;

  // Depending on the interaction state (set by the widget) we modify
  // this state based on what is picked.
  if (this->InteractionState == vtkCoordinateFrameRepresentation::Moving)
  {
    vtkProp* prop = path->GetFirstNode()->GetViewProp();
    if (prop == this->XVectorLineActor || prop == this->XVectorConeActor)
    {
      this->InteractionState = vtkCoordinateFrameRepresentation::RotatingXVector;
      this->SetRepresentationState(vtkCoordinateFrameRepresentation::RotatingXVector);
    }
    else if (prop == this->YVectorLineActor || prop == this->YVectorConeActor)
    {
      this->InteractionState = vtkCoordinateFrameRepresentation::RotatingYVector;
      this->SetRepresentationState(vtkCoordinateFrameRepresentation::RotatingYVector);
    }
    else if (prop == this->ZVectorLineActor || prop == this->ZVectorConeActor)
    {
      this->InteractionState = vtkCoordinateFrameRepresentation::RotatingZVector;
      this->SetRepresentationState(vtkCoordinateFrameRepresentation::RotatingZVector);
    }
    else if (prop == this->LockerXVectorConeActor)
    {
      this->InteractionState = vtkCoordinateFrameRepresentation::ModifyingLockerXVector;
      this->SetRepresentationState(vtkCoordinateFrameRepresentation::ModifyingLockerXVector);
    }
    else if (prop == this->LockerYVectorConeActor)
    {
      this->InteractionState = vtkCoordinateFrameRepresentation::ModifyingLockerYVector;
      this->SetRepresentationState(vtkCoordinateFrameRepresentation::ModifyingLockerYVector);
    }
    else if (prop == this->LockerZVectorConeActor)
    {
      this->InteractionState = vtkCoordinateFrameRepresentation::ModifyingLockerZVector;
      this->SetRepresentationState(vtkCoordinateFrameRepresentation::ModifyingLockerZVector);
    }
    else if (prop == this->OriginSphereActor)
    {
      this->InteractionState = vtkCoordinateFrameRepresentation::MovingOrigin;
      this->SetRepresentationState(vtkCoordinateFrameRepresentation::MovingOrigin);
    }
    else
    {
      this->InteractionState = vtkCoordinateFrameRepresentation::Outside;
      this->SetRepresentationState(vtkCoordinateFrameRepresentation::Outside);
    }
  }

  return this->InteractionState;
}

//------------------------------------------------------------------------------
void vtkCoordinateFrameRepresentation::SetRepresentationState(int state)
{
  // Clamp the state
  state = std::min<int>(std::max<int>(state, vtkCoordinateFrameRepresentation::Outside),
    vtkCoordinateFrameRepresentation::ModifyingLockerZVector);

  if (this->RepresentationState == state)
  {
    return;
  }

  this->RepresentationState = state;
  this->Modified();

  if (state == vtkCoordinateFrameRepresentation::RotatingXVector)
  {
    this->HighlightOrigin(0);
    this->HighlightXVector(1);
    this->HighlightYVector(0);
    this->HighlightZVector(0);
    this->HighlightLockerXVector(0);
    this->HighlightLockerYVector(0);
    this->HighlightLockerZVector(0);
  }
  else if (state == vtkCoordinateFrameRepresentation::RotatingYVector)
  {
    this->HighlightOrigin(0);
    this->HighlightXVector(0);
    this->HighlightYVector(1);
    this->HighlightZVector(0);
    this->HighlightLockerXVector(0);
    this->HighlightLockerYVector(0);
    this->HighlightLockerZVector(0);
  }
  else if (state == vtkCoordinateFrameRepresentation::RotatingZVector)
  {
    this->HighlightOrigin(0);
    this->HighlightXVector(0);
    this->HighlightYVector(0);
    this->HighlightZVector(1);
    this->HighlightLockerXVector(0);
    this->HighlightLockerYVector(0);
    this->HighlightLockerZVector(0);
  }
  else if (state == vtkCoordinateFrameRepresentation::ModifyingLockerXVector)
  {
    this->HighlightOrigin(0);
    this->HighlightXVector(0);
    this->HighlightYVector(0);
    this->HighlightZVector(0);
    this->HighlightLockerXVector(1);
    this->HighlightLockerYVector(0);
    this->HighlightLockerZVector(0);
  }
  else if (state == vtkCoordinateFrameRepresentation::ModifyingLockerYVector)
  {
    this->HighlightOrigin(0);
    this->HighlightXVector(0);
    this->HighlightYVector(0);
    this->HighlightZVector(0);
    this->HighlightLockerXVector(0);
    this->HighlightLockerYVector(1);
    this->HighlightLockerZVector(0);
  }
  else if (state == vtkCoordinateFrameRepresentation::ModifyingLockerZVector)
  {
    this->HighlightOrigin(0);
    this->HighlightXVector(0);
    this->HighlightYVector(0);
    this->HighlightZVector(0);
    this->HighlightLockerXVector(0);
    this->HighlightLockerYVector(0);
    this->HighlightLockerZVector(1);
  }
  else if (state == vtkCoordinateFrameRepresentation::MovingOrigin)
  {
    this->HighlightOrigin(1);
    this->HighlightXVector(0);
    this->HighlightYVector(0);
    this->HighlightZVector(0);
    this->HighlightLockerXVector(0);
    this->HighlightLockerYVector(0);
    this->HighlightLockerZVector(0);
  }
  else
  {
    this->HighlightOrigin(0);
    this->HighlightXVector(0);
    this->HighlightYVector(0);
    this->HighlightZVector(0);
    this->HighlightLockerXVector(0);
    this->HighlightLockerYVector(0);
    this->HighlightLockerZVector(0);
  }
}

//------------------------------------------------------------------------------
void vtkCoordinateFrameRepresentation::StartWidgetInteraction(double e[2])
{
  this->StartEventPosition[0] = e[0];
  this->StartEventPosition[1] = e[1];
  this->StartEventPosition[2] = 0.0;

  this->LastEventPosition[0] = e[0];
  this->LastEventPosition[1] = e[1];
  this->LastEventPosition[2] = 0.0;
}

//------------------------------------------------------------------------------
void vtkCoordinateFrameRepresentation::WidgetInteraction(double e[2])
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
  if (this->InteractionState == vtkCoordinateFrameRepresentation::RotatingXVector &&
    !this->XVectorIsLocked)
  {
    camera->GetViewPlaneNormal(vpn);
    this->Rotate(e[0], e[1], prevPickPoint, pickPoint, vpn);
  }
  else if (this->InteractionState == vtkCoordinateFrameRepresentation::RotatingYVector &&
    !this->YVectorIsLocked)
  {
    camera->GetViewPlaneNormal(vpn);
    this->Rotate(e[0], e[1], prevPickPoint, pickPoint, vpn);
  }
  else if (this->InteractionState == vtkCoordinateFrameRepresentation::RotatingZVector &&
    !this->ZVectorIsLocked)
  {
    camera->GetViewPlaneNormal(vpn);
    this->Rotate(e[0], e[1], prevPickPoint, pickPoint, vpn);
  }
  else if (this->InteractionState == vtkCoordinateFrameRepresentation::ModifyingLockerXVector)
  {
    this->ModifyingLocker(Axis::XAxis);
  }
  else if (this->InteractionState == vtkCoordinateFrameRepresentation::ModifyingLockerYVector)
  {
    this->ModifyingLocker(Axis::YAxis);
  }
  else if (this->InteractionState == vtkCoordinateFrameRepresentation::ModifyingLockerZVector)
  {
    this->ModifyingLocker(Axis::ZAxis);
  }
  else if (this->InteractionState == vtkCoordinateFrameRepresentation::MovingOrigin)
  {
    this->TranslateOrigin(prevPickPoint, pickPoint);
  }
  else if (this->InteractionState == vtkCoordinateFrameRepresentation::Outside &&
    this->LockNormalToCamera)
  {
    this->SetNormalToCamera();
  }

  this->LastEventPosition[0] = e[0];
  this->LastEventPosition[1] = e[1];
  this->LastEventPosition[2] = 0.0;
}

//------------------------------------------------------------------------------
void vtkCoordinateFrameRepresentation::EndWidgetInteraction(double vtkNotUsed(e)[2])
{
  this->SetRepresentationState(vtkCoordinateFrameRepresentation::Outside);
}

//------------------------------------------------------------------------------
double* vtkCoordinateFrameRepresentation::GetBounds()
{
  this->BuildRepresentation();
  // bounds need to be reset because the size of the widget changes over time
  this->BoundingBox->SetBounds(
    VTK_DOUBLE_MAX, VTK_DOUBLE_MIN, VTK_DOUBLE_MAX, VTK_DOUBLE_MIN, VTK_DOUBLE_MAX, VTK_DOUBLE_MIN);
  this->BoundingBox->AddBounds(this->OriginSphereActor->GetBounds());

  this->BoundingBox->AddBounds(this->XVectorLineActor->GetBounds());
  this->BoundingBox->AddBounds(this->XVectorConeActor->GetBounds());
  this->BoundingBox->AddBounds(this->LockerXVectorConeActor->GetBounds());

  this->BoundingBox->AddBounds(this->YVectorLineActor->GetBounds());
  this->BoundingBox->AddBounds(this->YVectorConeActor->GetBounds());
  this->BoundingBox->AddBounds(this->LockerYVectorConeActor->GetBounds());

  this->BoundingBox->AddBounds(this->ZVectorLineActor->GetBounds());
  this->BoundingBox->AddBounds(this->ZVectorConeActor->GetBounds());
  this->BoundingBox->AddBounds(this->LockerZVectorConeActor->GetBounds());

  return this->BoundingBox->GetBounds();
}

//------------------------------------------------------------------------------
void vtkCoordinateFrameRepresentation::GetActors(vtkPropCollection* pc)
{
  if (pc != nullptr && this->GetVisibility())
  {
    pc->AddItem(this->OriginSphereActor);

    pc->AddItem(this->XVectorLineActor);
    pc->AddItem(this->XVectorConeActor);
    pc->AddItem(this->LockerXVectorConeActor);

    pc->AddItem(this->YVectorLineActor);
    pc->AddItem(this->YVectorConeActor);
    pc->AddItem(this->LockerYVectorConeActor);

    pc->AddItem(this->ZVectorLineActor);
    pc->AddItem(this->ZVectorConeActor);
    pc->AddItem(this->LockerZVectorConeActor);
  }
  this->Superclass::GetActors(pc);
}

//------------------------------------------------------------------------------
void vtkCoordinateFrameRepresentation::ReleaseGraphicsResources(vtkWindow* w)
{
  this->OriginSphereActor->ReleaseGraphicsResources(w);

  this->XVectorLineActor->ReleaseGraphicsResources(w);
  this->XVectorConeActor->ReleaseGraphicsResources(w);
  this->LockerXVectorConeActor->ReleaseGraphicsResources(w);

  this->YVectorLineActor->ReleaseGraphicsResources(w);
  this->YVectorConeActor->ReleaseGraphicsResources(w);
  this->LockerYVectorConeActor->ReleaseGraphicsResources(w);

  this->ZVectorLineActor->ReleaseGraphicsResources(w);
  this->ZVectorConeActor->ReleaseGraphicsResources(w);
  this->LockerZVectorConeActor->ReleaseGraphicsResources(w);
}

//------------------------------------------------------------------------------
int vtkCoordinateFrameRepresentation::RenderOpaqueGeometry(vtkViewport* v)
{
  int count = 0;
  this->BuildRepresentation();
  if (!this->LockNormalToCamera)
  {
    count += this->OriginSphereActor->RenderOpaqueGeometry(v);

    count += this->XVectorLineActor->RenderOpaqueGeometry(v);
    count += this->XVectorConeActor->RenderOpaqueGeometry(v);
    count += this->LockerXVectorConeActor->RenderOpaqueGeometry(v);

    count += this->YVectorLineActor->RenderOpaqueGeometry(v);
    count += this->YVectorConeActor->RenderOpaqueGeometry(v);
    count += this->LockerYVectorConeActor->RenderOpaqueGeometry(v);

    count += this->ZVectorLineActor->RenderOpaqueGeometry(v);
    count += this->ZVectorConeActor->RenderOpaqueGeometry(v);
    count += this->LockerZVectorConeActor->RenderOpaqueGeometry(v);
  }

  return count;
}

//------------------------------------------------------------------------------
int vtkCoordinateFrameRepresentation::RenderTranslucentPolygonalGeometry(vtkViewport* v)
{
  int count = 0;
  this->BuildRepresentation();
  if (!this->LockNormalToCamera)
  {
    count += this->OriginSphereActor->RenderTranslucentPolygonalGeometry(v);

    count += this->XVectorLineActor->RenderTranslucentPolygonalGeometry(v);
    count += this->XVectorConeActor->RenderTranslucentPolygonalGeometry(v);
    count += this->LockerXVectorConeActor->RenderTranslucentPolygonalGeometry(v);

    count += this->YVectorLineActor->RenderTranslucentPolygonalGeometry(v);
    count += this->YVectorConeActor->RenderTranslucentPolygonalGeometry(v);
    count += this->LockerYVectorConeActor->RenderTranslucentPolygonalGeometry(v);

    count += this->ZVectorLineActor->RenderTranslucentPolygonalGeometry(v);
    count += this->ZVectorConeActor->RenderTranslucentPolygonalGeometry(v);
    count += this->LockerZVectorConeActor->RenderTranslucentPolygonalGeometry(v);
  }

  return count;
}

//------------------------------------------------------------------------------
vtkTypeBool vtkCoordinateFrameRepresentation::HasTranslucentPolygonalGeometry()
{
  int result = 0;
  if (!this->LockNormalToCamera)
  {
    result |= this->OriginSphereActor->HasTranslucentPolygonalGeometry();

    result |= this->XVectorLineActor->HasTranslucentPolygonalGeometry();
    result |= this->XVectorConeActor->HasTranslucentPolygonalGeometry();
    result |= this->LockerXVectorConeActor->HasTranslucentPolygonalGeometry();

    result |= this->YVectorLineActor->HasTranslucentPolygonalGeometry();
    result |= this->YVectorConeActor->HasTranslucentPolygonalGeometry();
    result |= this->LockerYVectorConeActor->HasTranslucentPolygonalGeometry();

    result |= this->ZVectorLineActor->HasTranslucentPolygonalGeometry();
    result |= this->ZVectorConeActor->HasTranslucentPolygonalGeometry();
    result |= this->LockerZVectorConeActor->HasTranslucentPolygonalGeometry();
  }

  return result;
}

//------------------------------------------------------------------------------
void vtkCoordinateFrameRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Origin Property: " << this->OriginProperty << "\n";
  os << indent << "Selected Origin Property: " << this->SelectedOriginProperty << "\n";

  os << indent << "X Vector Property: " << this->XVectorProperty << "\n";
  os << indent << "Selected X Vector Property: " << this->SelectedXVectorProperty << "\n";
  os << indent << "Locked X Vector Property: " << this->LockedXVectorProperty << "\n";
  os << indent << "Selected Locked X Vector Property: " << this->SelectedLockedXVectorProperty
     << "\n";
  os << indent << "Unlocked X Vector Property: " << this->UnlockedXVectorProperty << "\n";
  os << indent << "Selected Unlocked X Vector Property: " << this->SelectedUnlockedXVectorProperty
     << "\n";

  os << indent << "Y Vector Property: " << this->YVectorProperty << "\n";
  os << indent << "Selected Y Vector Property: " << this->SelectedYVectorProperty << "\n";
  os << indent << "Locked Y Vector Property: " << this->LockedYVectorProperty << "\n";
  os << indent << "Selected Locked Y Vector Property: " << this->SelectedLockedYVectorProperty
     << "\n";
  os << indent << "Unlocked Y Vector Property: " << this->UnlockedYVectorProperty << "\n";
  os << indent << "Selected Unlocked Y Vector Property: " << this->SelectedUnlockedYVectorProperty
     << "\n";

  os << indent << "Z Vector Property: " << this->ZVectorProperty << "\n";
  os << indent << "Selected Z Vector Property: " << this->SelectedZVectorProperty << "\n";
  os << indent << "Locked Z Vector Property: " << this->LockedZVectorProperty << "\n";
  os << indent << "Selected Locked Z Vector Property: " << this->SelectedLockedZVectorProperty
     << "\n";
  os << indent << "Unlocked Z Vector Property: " << this->UnlockedZVectorProperty << "\n";
  os << indent << "Selected Unlocked Z Vector Property: " << this->SelectedUnlockedZVectorProperty
     << "\n";

  os << indent << "Lock Normal To Camera: " << (this->LockNormalToCamera ? "On" : "Off") << "\n";

  os << indent << "X Vector Is Locked: " << (this->XVectorIsLocked ? "On" : "Off") << "\n";
  os << indent << "Y Vector Is Locked: " << (this->YVectorIsLocked ? "On" : "Off") << "\n";
  os << indent << "Z Vector Is Locked: " << (this->ZVectorIsLocked ? "On" : "Off") << "\n";

  os << indent << "Representation State: ";
  switch (this->RepresentationState)
  {
    case Outside:
      os << "Outside\n";
      break;
    case Moving:
      os << "Moving\n";
      break;
    case MovingOrigin:
      os << "MovingOrigin\n";
      break;
    case RotatingXVector:
      os << "RotatingXVector\n";
      break;
    case RotatingYVector:
      os << "RotatingYVector\n";
      break;
    case RotatingZVector:
      os << "RotatingZVector\n";
      break;
    case ModifyingLockerXVector:
      os << "ModifyingLockerXVector\n";
      break;
    case ModifyingLockerYVector:
      os << "ModifyingLockerXVector\n";
      break;
    case ModifyingLockerZVector:
      os << "ModifyingLockerXVector\n";
      break;
  }

  // this->InteractionState is printed in superclass
  // this is commented to avoid PrintSelf errors
}

//------------------------------------------------------------------------------
void vtkCoordinateFrameRepresentation::HighlightOrigin(int highlight)
{
  if (highlight)
  {
    this->OriginSphereActor->SetProperty(this->SelectedOriginProperty);
  }
  else
  {
    this->OriginSphereActor->SetProperty(this->OriginProperty);
  }
}

//------------------------------------------------------------------------------
void vtkCoordinateFrameRepresentation::HighlightXVector(int highlight)
{
  if (highlight)
  {
    this->XVectorLineActor->SetProperty(this->SelectedXVectorProperty);
    this->XVectorConeActor->SetProperty(this->SelectedXVectorProperty);
  }
  else
  {
    this->XVectorLineActor->SetProperty(this->XVectorProperty);
    this->XVectorConeActor->SetProperty(this->XVectorProperty);
  }
}

//------------------------------------------------------------------------------
void vtkCoordinateFrameRepresentation::HighlightLockerXVector(int highlight)
{
  if (highlight)
  {
    if (this->XVectorIsLocked)
    {
      this->LockerXVectorConeActor->SetProperty(this->SelectedLockedXVectorProperty);
    }
    else
    {
      this->LockerXVectorConeActor->SetProperty(this->SelectedUnlockedXVectorProperty);
    }
  }
  else
  {
    if (this->XVectorIsLocked)
    {
      this->LockerXVectorConeActor->SetProperty(this->LockedXVectorProperty);
    }
    else
    {
      this->LockerXVectorConeActor->SetProperty(this->UnlockedXVectorProperty);
    }
  }
}

//------------------------------------------------------------------------------
void vtkCoordinateFrameRepresentation::HighlightYVector(int highlight)
{
  if (highlight)
  {
    this->YVectorLineActor->SetProperty(this->SelectedYVectorProperty);
    this->YVectorConeActor->SetProperty(this->SelectedYVectorProperty);
  }
  else
  {
    this->YVectorLineActor->SetProperty(this->YVectorProperty);
    this->YVectorConeActor->SetProperty(this->YVectorProperty);
  }
}

//------------------------------------------------------------------------------
void vtkCoordinateFrameRepresentation::HighlightLockerYVector(int highlight)
{
  if (highlight)
  {
    if (this->YVectorIsLocked)
    {
      this->LockerYVectorConeActor->SetProperty(this->SelectedLockedYVectorProperty);
    }
    else
    {
      this->LockerYVectorConeActor->SetProperty(this->SelectedUnlockedYVectorProperty);
    }
  }
  else
  {
    if (this->YVectorIsLocked)
    {
      this->LockerYVectorConeActor->SetProperty(this->LockedYVectorProperty);
    }
    else
    {
      this->LockerYVectorConeActor->SetProperty(this->UnlockedYVectorProperty);
    }
  }
}

//------------------------------------------------------------------------------
void vtkCoordinateFrameRepresentation::HighlightZVector(int highlight)
{
  if (highlight)
  {
    this->ZVectorLineActor->SetProperty(this->SelectedZVectorProperty);
    this->ZVectorConeActor->SetProperty(this->SelectedZVectorProperty);
  }
  else
  {
    this->ZVectorLineActor->SetProperty(this->ZVectorProperty);
    this->ZVectorConeActor->SetProperty(this->ZVectorProperty);
  }
}

//------------------------------------------------------------------------------
void vtkCoordinateFrameRepresentation::HighlightLockerZVector(int highlight)
{
  if (highlight)
  {
    if (this->ZVectorIsLocked)
    {
      this->LockerZVectorConeActor->SetProperty(this->SelectedLockedZVectorProperty);
    }
    else
    {
      this->LockerZVectorConeActor->SetProperty(this->SelectedUnlockedZVectorProperty);
    }
  }
  else
  {
    if (this->ZVectorIsLocked)
    {
      this->LockerZVectorConeActor->SetProperty(this->LockedZVectorProperty);
    }
    else
    {
      this->LockerZVectorConeActor->SetProperty(this->UnlockedZVectorProperty);
    }
  }
}

//------------------------------------------------------------------------------
static double GetRotationAngle(double* origin, double* axisNormal, double* p1, double* p2)
{
  double p1Project[3], p2Project[3];
  vtkPlane::ProjectPoint(p1, origin, axisNormal, p1Project);
  vtkPlane::ProjectPoint(p2, origin, axisNormal, p2Project);

  // mouse motion vector in world space
  double v1[3];
  v1[0] = p1Project[0] - origin[0];
  v1[1] = p1Project[1] - origin[1];
  v1[2] = p1Project[2] - origin[2];

  // mouse motion vector in world space
  double v2[3];
  v2[0] = p2Project[0] - origin[0];
  v2[1] = p2Project[1] - origin[1];
  v2[2] = p2Project[2] - origin[2];

  return vtkMath::DegreesFromRadians(vtkMath::SignedAngleBetweenVectors(v1, v2, axisNormal));
}

//------------------------------------------------------------------------------
void vtkCoordinateFrameRepresentation::Rotate(
  double X, double Y, double* p1, double* p2, double* vpn)
{
  double axis[3]; // axis of rotation
  double theta;   // rotation angle

  double* origin = this->GetOrigin();

  // Create axis of rotation and angle of rotation
  if (this->XVectorIsLocked)
  {
    axis[0] = this->XVectorNormal[0];
    axis[1] = this->XVectorNormal[1];
    axis[2] = this->XVectorNormal[2];
    theta = GetRotationAngle(origin, this->XVectorNormal, p1, p2);
  }
  else if (this->YVectorIsLocked)
  {
    axis[0] = this->YVectorNormal[0];
    axis[1] = this->YVectorNormal[1];
    axis[2] = this->YVectorNormal[2];
    theta = GetRotationAngle(origin, this->YVectorNormal, p1, p2);
  }
  else if (this->ZVectorIsLocked)
  {
    axis[0] = this->ZVectorNormal[0];
    axis[1] = this->ZVectorNormal[1];
    axis[2] = this->ZVectorNormal[2];
    theta = GetRotationAngle(origin, this->ZVectorNormal, p1, p2);
  }
  else
  {
    // mouse motion vector in world space
    double v[3];
    v[0] = p2[0] - p1[0];
    v[1] = p2[1] - p1[1];
    v[2] = p2[2] - p1[2];

    vtkMath::Cross(vpn, v, axis);
    if (vtkMath::Normalize(axis) == 0.0)
    {
      return;
    }

    const int* size = this->Renderer->GetSize();
    double l2 = (X - this->LastEventPosition[0]) * (X - this->LastEventPosition[0]) +
      (Y - this->LastEventPosition[1]) * (Y - this->LastEventPosition[1]);
    theta = 360.0 * sqrt(l2 / (size[0] * size[0] + size[1] * size[1]));
  }

  // Manipulate the transform to reflect the rotation
  this->Transform->Identity();
  this->Transform->Translate(origin[0], origin[1], origin[2]);
  this->Transform->RotateWXYZ(theta, axis);
  this->Transform->Translate(-origin[0], -origin[1], -origin[2]);

  double nNew[3];
  // Set the new X normal
  if (!this->XVectorIsLocked)
  {
    this->Transform->TransformNormal(this->XVectorNormal, nNew);
    this->SetXVectorNormal(nNew);
  }
  // Set the new Y normal
  if (!this->YVectorIsLocked)
  {
    this->Transform->TransformNormal(this->YVectorNormal, nNew);
    this->SetYVectorNormal(nNew);
  }
  // Set the new Z normal
  if (!this->ZVectorIsLocked)
  {
    this->Transform->TransformNormal(this->ZVectorNormal, nNew);
    this->SetZVectorNormal(nNew);
  }
}

//------------------------------------------------------------------------------
void vtkCoordinateFrameRepresentation::ModifyingLocker(int axis)
{
  switch (axis)
  {
    case Axis::XAxis:
    {
      this->XVectorIsLocked = !this->XVectorIsLocked;
      this->HighlightLockerXVector(1);
      // if we are locking the Χ vector
      if (this->XVectorIsLocked)
      {
        // unlock the other vectors if needed
        if (this->YVectorIsLocked)
        {
          this->YVectorIsLocked = false;
          this->HighlightLockerYVector(0);
        }
        if (this->ZVectorIsLocked)
        {
          this->ZVectorIsLocked = false;
          this->HighlightLockerZVector(0);
        }
      }
      break;
    }
    case Axis::YAxis:
    {
      this->YVectorIsLocked = !this->YVectorIsLocked;
      this->HighlightLockerYVector(1);
      // if we are locking the Υ vector
      if (this->YVectorIsLocked)
      {
        // unlock the other vectors if needed
        if (this->XVectorIsLocked)
        {
          this->XVectorIsLocked = false;
          this->HighlightLockerXVector(0);
        }
        if (this->ZVectorIsLocked)
        {
          this->ZVectorIsLocked = false;
          this->HighlightLockerZVector(0);
        }
      }
      break;
    }
    case Axis::ZAxis:
    {
      this->ZVectorIsLocked = !this->ZVectorIsLocked;
      this->HighlightLockerZVector(1);
      // if we are locking the Ζ vector
      if (this->ZVectorIsLocked)
      {
        // unlock the other vectors if needed
        if (this->XVectorIsLocked)
        {
          this->XVectorIsLocked = false;
          this->HighlightLockerXVector(0);
        }
        if (this->YVectorIsLocked)
        {
          this->YVectorIsLocked = false;
          this->HighlightLockerYVector(0);
        }
      }
      break;
    }
  }
  this->Modified();
  this->BuildRepresentation();
}

//------------------------------------------------------------------------------
// Loop through all points and translate them
void vtkCoordinateFrameRepresentation::TranslateOrigin(double* p1, double* p2)
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

  double* o = this->GetOrigin();
  double newOrigin[3];

  newOrigin[0] = o[0] + v[0];
  newOrigin[1] = o[1] + v[1];
  newOrigin[2] = o[2] + v[2];

  if (this->XVectorIsLocked)
  {
    vtkPlane::ProjectPoint(newOrigin, o, this->XVectorNormal, newOrigin);
  }
  else if (this->YVectorIsLocked)
  {
    vtkPlane::ProjectPoint(newOrigin, o, this->YVectorNormal, newOrigin);
  }
  else if (this->ZVectorIsLocked)
  {
    vtkPlane::ProjectPoint(newOrigin, o, this->ZVectorNormal, newOrigin);
  }
  this->SetOrigin(newOrigin[0], newOrigin[1], newOrigin[2]);
  this->BuildRepresentation();
}

//------------------------------------------------------------------------------
void vtkCoordinateFrameRepresentation::PlaceWidget(double bds[6])
{
  int i;
  double bounds[6], origin[3];

  this->AdjustBounds(bds, bounds, origin);

  // Set up initial vector normals
  this->SetXVectorNormal(1, 0, 0);
  this->SetYVectorNormal(0, 1, 0);
  this->SetZVectorNormal(0, 0, 1);

  this->SetOrigin(origin[0], origin[1], origin[2]);

  for (i = 0; i < 6; i++)
  {
    this->InitialBounds[i] = bounds[i];
  }

  this->ValidPick = 1; // since we have positioned the widget successfully
  this->BuildRepresentation();
}

//------------------------------------------------------------------------------
void vtkCoordinateFrameRepresentation::SetOrigin(double x, double y, double z)
{
  double origin[3] = { x, y, z };
  this->SetOrigin(origin);
}

//------------------------------------------------------------------------------
void vtkCoordinateFrameRepresentation::SetOrigin(double x[3])
{
  this->Origin[0] = x[0];
  this->Origin[1] = x[1];
  this->Origin[2] = x[2];
  this->Modified();
  this->BuildRepresentation();
}

//------------------------------------------------------------------------------
static void GramSchmidt(
  const double* v1, const double* v2, const double* v3, double* u1, double* u2, double* u3)
{
  double projectionV2U1[3], projectionV3U1[3], projectionV3U2[3];

  for (int i = 0; i < 3; ++i)
  {
    u1[i] = v1[i];
  }

  vtkMath::ProjectVector(v2, u1, projectionV2U1);
  for (int i = 0; i < 3; ++i)
  {
    u2[i] = v2[i] - projectionV2U1[i];
  }

  vtkMath::ProjectVector(v3, u1, projectionV3U1);
  vtkMath::ProjectVector(v3, u2, projectionV3U2);
  for (int i = 0; i < 3; ++i)
  {
    u3[i] = v3[i] - projectionV3U1[i] - projectionV3U2[i];
  }

  vtkMath::Normalize(u1);
  vtkMath::Normalize(u2);
  vtkMath::Normalize(u3);
}

//------------------------------------------------------------------------------
void vtkCoordinateFrameRepresentation::SetNormal(double x, double y, double z)
{
  double n[3] = { x, y, z };
  vtkMath::Normalize(n);

  const double* xNormal = this->GetXVectorNormal();
  const double* yNormal = this->GetYVectorNormal();
  const double* zNormal = this->GetZVectorNormal();
  double newXNormal[3], newYNormal[3], newZNormal[3];

  // if none of the vectors is locked
  if (!this->XVectorIsLocked && !this->YVectorIsLocked && !this->ZVectorIsLocked)
  {
    // find the vector that is closest to the picked normal using the max dot product
    const double xDot = vtkMath::Dot(n, xNormal);
    const double yDot = vtkMath::Dot(n, yNormal);
    const double zDot = vtkMath::Dot(n, zNormal);
    const double maxDot = std::max(xDot, std::max(yDot, zDot));
    if (xDot == maxDot)
    {
      GramSchmidt(n, yNormal, zNormal, newXNormal, newYNormal, newZNormal);
    }
    else if (yDot == maxDot)
    {
      GramSchmidt(n, zNormal, xNormal, newYNormal, newZNormal, newXNormal);
    }
    else
    {
      GramSchmidt(n, xNormal, yNormal, newZNormal, newXNormal, newYNormal);
    }
  }
  // if one of the vectors is locked
  else
  {
    if (this->XVectorIsLocked)
    {
      GramSchmidt(n, yNormal, zNormal, newXNormal, newYNormal, newZNormal);
    }
    else if (this->YVectorIsLocked)
    {
      GramSchmidt(n, zNormal, xNormal, newYNormal, newZNormal, newXNormal);
    }
    else // ZVectorIsLocked
    {
      GramSchmidt(n, xNormal, yNormal, newZNormal, newXNormal, newYNormal);
    }
  }
  this->SetXVectorNormal(newXNormal);
  this->SetYVectorNormal(newYNormal);
  this->SetZVectorNormal(newZNormal);
  this->BuildRepresentation();
}

//------------------------------------------------------------------------------
void vtkCoordinateFrameRepresentation::SetNormal(double n[3])
{
  this->SetNormal(n[0], n[1], n[2]);
}

//------------------------------------------------------------------------------
void vtkCoordinateFrameRepresentation::SetNormalToCamera()
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
void vtkCoordinateFrameRepresentation::SetDirection(double x, double y, double z)
{
  double* o = this->GetOrigin();
  double newNormal[3];
  newNormal[0] = x - o[0];
  newNormal[1] = y - o[1];
  newNormal[2] = z - o[2];
  vtkMath::Normalize(newNormal);
  this->SetNormal(newNormal);
  this->BuildRepresentation();
}

//------------------------------------------------------------------------------
void vtkCoordinateFrameRepresentation::SetDirection(double point[3])
{
  this->SetDirection(point[0], point[1], point[2]);
}

//------------------------------------------------------------------------------
void vtkCoordinateFrameRepresentation::SetXAxisVector(const double v[3])
{
  if (v[0] == 0. && v[1] == 0. && v[2] == 0.)
  {
    return;
  }
  const double* yNormal = this->GetYVectorNormal();
  const double* zNormal = this->GetZVectorNormal();
  double newXNormal[3], newYNormal[3], newZNormal[3];

  GramSchmidt(v, yNormal, zNormal, newXNormal, newYNormal, newZNormal);
  this->SetXVectorNormal(newXNormal);
  this->SetYVectorNormal(newYNormal);
  this->SetZVectorNormal(newZNormal);
}

void vtkCoordinateFrameRepresentation::SetXAxisVector(double x, double y, double z)
{
  double vec[3] = { x, y, z };
  this->SetXAxisVector(vec);
}

//------------------------------------------------------------------------------
void vtkCoordinateFrameRepresentation::SetYAxisVector(const double v[3])
{
  if (v[0] == 0. && v[1] == 0. && v[2] == 0.)
  {
    return;
  }
  const double* xNormal = this->GetXVectorNormal();
  const double* zNormal = this->GetZVectorNormal();
  double newXNormal[3], newYNormal[3], newZNormal[3];

  GramSchmidt(v, zNormal, xNormal, newYNormal, newZNormal, newXNormal);
  this->SetXVectorNormal(newXNormal);
  this->SetYVectorNormal(newYNormal);
  this->SetZVectorNormal(newZNormal);
}

void vtkCoordinateFrameRepresentation::SetYAxisVector(double x, double y, double z)
{
  double vec[3] = { x, y, z };
  this->SetYAxisVector(vec);
}

//------------------------------------------------------------------------------
void vtkCoordinateFrameRepresentation::SetZAxisVector(const double v[3])
{
  if (v[0] == 0. && v[1] == 0. && v[2] == 0.)
  {
    return;
  }
  const double* xNormal = this->GetXVectorNormal();
  const double* yNormal = this->GetYVectorNormal();
  double newXNormal[3], newYNormal[3], newZNormal[3];

  GramSchmidt(v, xNormal, yNormal, newZNormal, newXNormal, newYNormal);
  this->SetXVectorNormal(newXNormal);
  this->SetYVectorNormal(newYNormal);
  this->SetZVectorNormal(newZNormal);
}

void vtkCoordinateFrameRepresentation::SetZAxisVector(double x, double y, double z)
{
  double vec[3] = { x, y, z };
  this->SetZAxisVector(vec);
}

//------------------------------------------------------------------------------
void vtkCoordinateFrameRepresentation::UpdatePlacement()
{
  this->BuildRepresentation();
}

//------------------------------------------------------------------------------
void vtkCoordinateFrameRepresentation::Reset()
{
  this->SetOrigin(0., 0., 0.);
  this->ResetAxes(); // Calls BuildRepresentation().
}

//------------------------------------------------------------------------------
void vtkCoordinateFrameRepresentation::ResetAxes()
{
  this->SetXVectorNormal(1.0, 0.0, 0.0);
  this->SetYVectorNormal(0.0, 1.0, 0.0);
  this->SetZVectorNormal(0.0, 0.0, 1.0);
  this->Modified();
  this->BuildRepresentation();
}

//------------------------------------------------------------------------------
bool vtkCoordinateFrameRepresentation::PickOrigin(int X, int Y, bool snapToMeshPoint)
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
bool vtkCoordinateFrameRepresentation::PickNormal(int X, int Y, bool snapToMeshPoint)
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
bool vtkCoordinateFrameRepresentation::PickDirectionPoint(int X, int Y, bool snapToMeshPoint)
{
  this->HardwarePicker->SetSnapToMeshPoint(snapToMeshPoint);
  vtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->HardwarePicker);
  if (path == nullptr) // actors of renderer were not touched
  {
    if (this->PickCameraFocalInfo)
    {
      double pickPoint[3];
      this->HardwarePicker->GetPickPosition(pickPoint);
      this->SetDirection(pickPoint);
    }
    return this->PickCameraFocalInfo;
  }
  else // actors of renderer were touched
  {
    double pickPoint[3];
    this->HardwarePicker->GetPickPosition(pickPoint);
    if (!std::isnan(pickPoint[0]) || !std::isnan(pickPoint[1]) || !std::isnan(pickPoint[2]))
    {
      this->SetDirection(pickPoint);
      return true;
    }
    else
    {
      return false;
    }
  }
}

//------------------------------------------------------------------------------
int vtkCoordinateFrameRepresentation::GetLockedAxis() const
{
  if (this->XVectorIsLocked)
  {
    return Axis::XAxis;
  }
  if (this->YVectorIsLocked)
  {
    return Axis::YAxis;
  }
  if (this->ZVectorIsLocked)
  {
    return Axis::ZAxis;
  }
  return Axis::NONE;
}

void vtkCoordinateFrameRepresentation::SetLockedAxis(int axis)
{
  if (axis < -1 || axis > 3)
  {
    return;
  }

  auto aa = static_cast<Axis>(axis);
  auto current = static_cast<Axis>(this->GetLockedAxis());
  if (aa != current)
  {
    if (aa == Axis::NONE)
    {
      // Unlock the currently-locked axis.
      this->ModifyingLocker(current);
    }
    else
    {
      // Lock a different axis.
      this->ModifyingLocker(axis);
    }
  }
}

//------------------------------------------------------------------------------
void vtkCoordinateFrameRepresentation::BuildRepresentation()
{
  if (!this->Renderer || !this->Renderer->GetRenderWindow())
  {
    return;
  }

  vtkInformation* info = this->GetPropertyKeys();
  this->OriginSphereActor->SetPropertyKeys(info);
  this->XVectorLineActor->SetPropertyKeys(info);
  this->XVectorConeActor->SetPropertyKeys(info);
  this->LockerXVectorConeActor->SetPropertyKeys(info);
  this->YVectorLineActor->SetPropertyKeys(info);
  this->YVectorConeActor->SetPropertyKeys(info);
  this->LockerYVectorConeActor->SetPropertyKeys(info);
  this->ZVectorLineActor->SetPropertyKeys(info);
  this->ZVectorConeActor->SetPropertyKeys(info);
  this->LockerZVectorConeActor->SetPropertyKeys(info);

  if (this->GetMTime() > this->BuildTime)
  {
    double* origin = this->GetOrigin();

    // Set up the position handle
    this->OriginSphereSource->SetCenter(origin);
    this->XVectorLineSource->SetPoint1(origin);
    this->XVectorConeSource->SetDirection(this->GetXVectorNormal());
    this->LockerXVectorConeSource->SetDirection(this->GetXVectorNormal());
    this->YVectorLineSource->SetPoint1(origin);
    this->YVectorConeSource->SetDirection(this->GetYVectorNormal());
    this->LockerYVectorConeSource->SetDirection(this->GetYVectorNormal());
    this->ZVectorLineSource->SetPoint1(origin);
    this->ZVectorConeSource->SetDirection(this->GetZVectorNormal());
    this->LockerZVectorConeSource->SetDirection(this->GetZVectorNormal());
  }

  if (this->GetMTime() > this->BuildTime ||
    this->Renderer->GetRenderWindow()->GetMTime() > this->BuildTime ||
    (this->Renderer->GetActiveCamera() &&
      this->Renderer->GetActiveCamera()->GetMTime() > this->BuildTime))

  {
    this->SizeHandles();
    this->BuildTime.Modified();
  }
}

//------------------------------------------------------------------------------
void vtkCoordinateFrameRepresentation::SizeHandles()
{
  double* origin = this->GetOrigin();
  double* xNormal = this->GetXVectorNormal();
  double* yNormal = this->GetYVectorNormal();
  double* zNormal = this->GetZVectorNormal();

  const double d =
    vtkWidgetRepresentation::SizeHandlesRelativeToViewport(this->LengthFactor, origin);
  const double radius = this->vtkWidgetRepresentation::SizeHandlesInPixels(3.0, origin);

  // set up origin
  this->OriginSphereSource->SetRadius(radius);

  // set up the x vector
  double xP2[3], xLocker[3];
  xP2[0] = origin[0] + d * xNormal[0];
  xP2[1] = origin[1] + d * xNormal[1];
  xP2[2] = origin[2] + d * xNormal[2];

  xLocker[0] = xP2[0] + 2 * radius * xNormal[0];
  xLocker[1] = xP2[1] + 2 * radius * xNormal[1];
  xLocker[2] = xP2[2] + 2 * radius * xNormal[2];

  this->XVectorLineSource->SetPoint2(xP2);
  this->XVectorConeSource->SetCenter(xP2);
  this->LockerXVectorConeSource->SetCenter(xLocker);

  this->XVectorConeSource->SetHeight(2.0 * radius);
  this->XVectorConeSource->SetRadius(radius);
  this->LockerXVectorConeSource->SetHeight(2.0 * radius);
  this->LockerXVectorConeSource->SetRadius(radius);

  // set up the y vector
  double yP2[3], yLocker[3];
  yP2[0] = origin[0] + d * yNormal[0];
  yP2[1] = origin[1] + d * yNormal[1];
  yP2[2] = origin[2] + d * yNormal[2];

  yLocker[0] = yP2[0] + 2 * radius * yNormal[0];
  yLocker[1] = yP2[1] + 2 * radius * yNormal[1];
  yLocker[2] = yP2[2] + 2 * radius * yNormal[2];

  this->YVectorLineSource->SetPoint2(yP2);
  this->YVectorConeSource->SetCenter(yP2);
  this->LockerYVectorConeSource->SetCenter(yLocker);

  this->YVectorConeSource->SetHeight(2.0 * radius);
  this->YVectorConeSource->SetRadius(radius);
  this->LockerYVectorConeSource->SetHeight(2.0 * radius);
  this->LockerYVectorConeSource->SetRadius(radius);

  // set up the z vector
  double zP2[3], zLocker[3];
  zP2[0] = origin[0] + d * zNormal[0];
  zP2[1] = origin[1] + d * zNormal[1];
  zP2[2] = origin[2] + d * zNormal[2];

  zLocker[0] = zP2[0] + 2 * radius * zNormal[0];
  zLocker[1] = zP2[1] + 2 * radius * zNormal[1];
  zLocker[2] = zP2[2] + 2 * radius * zNormal[2];

  this->ZVectorLineSource->SetPoint2(zP2);
  this->ZVectorConeSource->SetCenter(zP2);
  this->LockerZVectorConeSource->SetCenter(zLocker);

  this->ZVectorConeSource->SetHeight(2.0 * radius);
  this->ZVectorConeSource->SetRadius(radius);
  this->LockerZVectorConeSource->SetHeight(2.0 * radius);
  this->LockerZVectorConeSource->SetRadius(radius);
}

//------------------------------------------------------------------------------
void vtkCoordinateFrameRepresentation::RegisterPickers()
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
void vtkCoordinateFrameRepresentation::ComputeAdaptivePickerTolerance()
{
  double pickerCylinderRadius =
    vtkWidgetRepresentation::SizeHandlesRelativeToViewport(0.000001, this->GetOrigin());
  double tolerance = pickerCylinderRadius < DefaultPickTol ? pickerCylinderRadius : DefaultPickTol;
  this->CellPicker->SetTolerance(tolerance);
}
VTK_ABI_NAMESPACE_END
