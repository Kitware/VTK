// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkInteractorStyleManipulator.h"

#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCameraManipulator.h"
#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkCommand.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

VTK_ABI_NAMESPACE_BEGIN

class vtkInteractorStyleManipulator::vtkInternals
{
  vtkInteractorStyleManipulator* Parent;
  vtkCameraManipulator* CurrentManipulator = nullptr;

public:
  explicit vtkInternals(vtkInteractorStyleManipulator* parent)
    : Parent(parent)
  {
  }

  vtkCameraManipulator* GetCurrentManipulator() const { return this->CurrentManipulator; }

  //-------------------------------------------------------------------------
  void OnButtonDown(
    vtkCameraManipulator::MouseButtonType button, bool alt, bool control, bool shift)
  {
    // Must not be processing an interaction to start another.
    if (this->CurrentManipulator)
    {
      return;
    }
    auto parent = vtk::MakeSmartPointer(this->Parent);
    if (!parent)
    {
      return;
    }

    // Get the renderer.
    parent->FindPokedRenderer(
      parent->Interactor->GetEventPosition()[0], parent->Interactor->GetEventPosition()[1]);
    if (parent->CurrentRenderer == nullptr)
    {
      return;
    }

    // Look for a matching camera interactor.
    this->CurrentManipulator = this->FindManipulator(button, alt, control, shift);
    if (this->CurrentManipulator)
    {
      this->CurrentManipulator->Register(parent);
      parent->InvokeEvent(vtkCommand::StartInteractionEvent);
      this->CurrentManipulator->SetCenterOfRotation(parent->GetCenterOfRotation());
      this->CurrentManipulator->SetMouseMotionFactor(parent->MouseMotionFactor);
      this->CurrentManipulator->StartInteraction();
      this->CurrentManipulator->OnButtonDown(parent->Interactor->GetEventPosition()[0],
        parent->Interactor->GetEventPosition()[1], parent->CurrentRenderer, parent->Interactor);
    }
  }

  //-------------------------------------------------------------------------
  vtkCameraManipulator* FindManipulator(
    vtkCameraManipulator::MouseButtonType button, bool alt, bool control, bool shift)
  {
    auto parent = vtk::MakeSmartPointer(this->Parent);
    if (!parent)
    {
      return nullptr;
    }
    // Look for a matching camera interactor.
    parent->CameraManipulators->InitTraversal();
    vtkCameraManipulator* manipulator = nullptr;
    while ((manipulator = vtkCameraManipulator::SafeDownCast(
              parent->CameraManipulators->GetNextItemAsObject())))
    {
      if (manipulator->GetMouseButton() == button && manipulator->GetAlt() == alt &&
        manipulator->GetControl() == control && manipulator->GetShift() == shift)
      {
        return manipulator;
      }
    }
    return nullptr;
  }

  //-------------------------------------------------------------------------
  void OnButtonUp(vtkCameraManipulator::MouseButtonType button)
  {
    if (this->CurrentManipulator == nullptr)
    {
      return;
    }
    auto parent = vtk::MakeSmartPointer(this->Parent);
    if (!parent)
    {
      return;
    }
    if (this->CurrentManipulator->GetMouseButton() == button)
    {
      this->CurrentManipulator->OnButtonUp(parent->Interactor->GetEventPosition()[0],
        parent->Interactor->GetEventPosition()[1], parent->CurrentRenderer, parent->Interactor);
      this->CurrentManipulator->EndInteraction();
      parent->InvokeEvent(vtkCommand::EndInteractionEvent);
      this->CurrentManipulator->UnRegister(parent);
      this->CurrentManipulator = nullptr;
    }
  }

  //-------------------------------------------------------------------------
  void Dolly(double factor)
  {
    auto parent = vtk::MakeSmartPointer(this->Parent);
    if (!parent)
    {
      return;
    }
    if (parent->Interactor->GetControlKey() || parent->MouseWheelZoomsToCursor)
    {
      vtkInteractorStyle::DollyToPosition(
        factor, parent->Interactor->GetEventPosition(), parent->CurrentRenderer);
      if (parent->AutoAdjustCameraClippingRange)
      {
        parent->CurrentRenderer->ResetCameraClippingRange();
      }
    }
    else
    {
      if (parent->CurrentRenderer == nullptr)
      {
        return;
      }

      vtkCamera* camera = parent->CurrentRenderer->GetActiveCamera();
      if (camera->GetParallelProjection())
      {
        camera->SetParallelScale(camera->GetParallelScale() / factor);
      }
      else
      {
        camera->Dolly(factor);
        if (parent->AutoAdjustCameraClippingRange)
        {
          parent->CurrentRenderer->ResetCameraClippingRange();
        }
      }

      if (parent->Interactor->GetLightFollowCamera())
      {
        parent->CurrentRenderer->UpdateLightsGeometryToFollowCamera();
      }

      parent->Interactor->Render();
    }
  }

  //-------------------------------------------------------------------------
  void ResetLights(vtkRenderer* renderer)
  {
    vtkLight* light;

    vtkLightCollection* lights = renderer->GetLights();
    vtkCamera* camera = renderer->GetActiveCamera();

    lights->InitTraversal();
    light = lights->GetNextItem();
    if (!light)
    {
      return;
    }
    light->SetPosition(camera->GetPosition());
    light->SetFocalPoint(camera->GetFocalPoint());
  }
};

vtkStandardNewMacro(vtkInteractorStyleManipulator);

//-------------------------------------------------------------------------
vtkInteractorStyleManipulator::vtkInteractorStyleManipulator()
  : Internals(std::make_unique<vtkInternals>(this))
{
  this->UseTimers = 0;
  this->CameraManipulators = vtkCollection::New();
}

//-------------------------------------------------------------------------
vtkInteractorStyleManipulator::~vtkInteractorStyleManipulator()
{
  if (this->CameraManipulators)
  {
    this->CameraManipulators->Delete();
  }
  this->CameraManipulators = nullptr;
}

//-------------------------------------------------------------------------
void vtkInteractorStyleManipulator::RemoveAllManipulators()
{
  this->CameraManipulators->RemoveAllItems();
}

//-------------------------------------------------------------------------
void vtkInteractorStyleManipulator::AddManipulator(vtkCameraManipulator* m)
{
  this->CameraManipulators->AddItem(m);
}

//-------------------------------------------------------------------------
void vtkInteractorStyleManipulator::OnLeftButtonDown()
{
  this->Internals->OnButtonDown(vtkCameraManipulator::MouseButtonType::Left,
    this->Interactor->GetAltKey() == 1, this->Interactor->GetControlKey() == 1,
    this->Interactor->GetShiftKey() == 1);
}

//-------------------------------------------------------------------------
void vtkInteractorStyleManipulator::OnMiddleButtonDown()
{
  this->Internals->OnButtonDown(vtkCameraManipulator::MouseButtonType::Middle,
    this->Interactor->GetAltKey() == 1, this->Interactor->GetControlKey() == 1,
    this->Interactor->GetShiftKey() == 1);
}

//-------------------------------------------------------------------------
void vtkInteractorStyleManipulator::OnRightButtonDown()
{
  this->Internals->OnButtonDown(vtkCameraManipulator::MouseButtonType::Right,
    this->Interactor->GetAltKey() == 1, this->Interactor->GetControlKey() == 1,
    this->Interactor->GetShiftKey() == 1);
}

//-------------------------------------------------------------------------
void vtkInteractorStyleManipulator::OnLeftButtonUp()
{
  this->Internals->OnButtonUp(vtkCameraManipulator::MouseButtonType::Left);
}
//-------------------------------------------------------------------------
void vtkInteractorStyleManipulator::OnMiddleButtonUp()
{
  this->Internals->OnButtonUp(vtkCameraManipulator::MouseButtonType::Middle);
}
//-------------------------------------------------------------------------
void vtkInteractorStyleManipulator::OnRightButtonUp()
{
  this->Internals->OnButtonUp(vtkCameraManipulator::MouseButtonType::Right);
}

//------------------------------------------------------------------------------
void vtkInteractorStyleManipulator::OnMouseWheelForward()
{
  this->FindPokedRenderer(
    this->Interactor->GetEventPosition()[0], this->Interactor->GetEventPosition()[1]);
  if (this->CurrentRenderer == nullptr)
  {
    return;
  }

  this->GrabFocus(this->EventCallbackCommand);
  this->StartDolly();
  double factor = this->MouseMotionFactor * 0.2 * this->MouseWheelMotionFactor;
  this->Internals->Dolly(pow(1.1, factor));
  this->EndDolly();
  this->ReleaseFocus();
}

//------------------------------------------------------------------------------
void vtkInteractorStyleManipulator::OnMouseWheelBackward()
{
  this->FindPokedRenderer(
    this->Interactor->GetEventPosition()[0], this->Interactor->GetEventPosition()[1]);
  if (this->CurrentRenderer == nullptr)
  {
    return;
  }

  this->GrabFocus(this->EventCallbackCommand);
  this->StartDolly();
  double factor = this->MouseMotionFactor * -0.2 * this->MouseWheelMotionFactor;
  this->Internals->Dolly(pow(1.1, factor));
  this->EndDolly();
  this->ReleaseFocus();
}

//-------------------------------------------------------------------------
void vtkInteractorStyleManipulator::OnMouseMove()
{
  auto* currentManipulator = this->Internals->GetCurrentManipulator();
  if (this->CurrentRenderer && currentManipulator)
  {
    // When an interaction is active, we should not change the renderer being
    // interacted with.
  }
  else
  {
    this->FindPokedRenderer(
      this->Interactor->GetEventPosition()[0], this->Interactor->GetEventPosition()[1]);
  }

  if (currentManipulator)
  {
    currentManipulator->OnMouseMove(this->Interactor->GetEventPosition()[0],
      this->Interactor->GetEventPosition()[1], this->CurrentRenderer, this->Interactor);
    this->InvokeEvent(vtkCommand::InteractionEvent);
  }
}

//-------------------------------------------------------------------------
void vtkInteractorStyleManipulator::OnKeyDown()
{
  // Look for a matching camera interactor.
  this->CameraManipulators->InitTraversal();
  vtkCameraManipulator* manipulator = nullptr;
  while ((manipulator = (vtkCameraManipulator*)this->CameraManipulators->GetNextItemAsObject()))
  {
    manipulator->OnKeyDown(this->Interactor);
  }
}

//-------------------------------------------------------------------------
void vtkInteractorStyleManipulator::OnKeyUp()
{
  // Look for a matching camera interactor.
  this->CameraManipulators->InitTraversal();
  vtkCameraManipulator* manipulator = nullptr;
  while ((manipulator = (vtkCameraManipulator*)this->CameraManipulators->GetNextItemAsObject()))
  {
    manipulator->OnKeyUp(this->Interactor);
  }
}

//-------------------------------------------------------------------------
void vtkInteractorStyleManipulator::Dolly()
{
  if (this->CurrentRenderer == nullptr)
  {
    return;
  }

  vtkRenderWindowInteractor* rwi = this->Interactor;
  const double* viewportCenter = this->CurrentRenderer->GetCenter();
  const int dy = rwi->GetEventPosition()[1] - rwi->GetLastEventPosition()[1];
  const double dyf = this->MouseMotionFactor * dy / viewportCenter[1];
  const double factor = pow(1.1, dyf);
  this->Internals->Dolly(factor);
}

//-------------------------------------------------------------------------
vtkCameraManipulator* vtkInteractorStyleManipulator::FindManipulator(
  int button, bool alt, bool control, bool shift)
{
  if (button >= 1 && button <= 3)
  {
    return this->Internals->FindManipulator(
      static_cast<vtkCameraManipulator::MouseButtonType>(button), alt, control, shift);
  }
  else
  {
    vtkErrorMacro(<< "Button " << button
                  << " is not a valid mouse button. Please provide 1 (left), 2 (middle), or 3 "
                     "(right) for the button argument.");
    return nullptr;
  }
}

//-------------------------------------------------------------------------
void vtkInteractorStyleManipulator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "MouseMotionFactor: " << this->MouseMotionFactor << endl;
  os << indent << "CameraManipulators: " << this->CameraManipulators << endl;
}
VTK_ABI_NAMESPACE_END
