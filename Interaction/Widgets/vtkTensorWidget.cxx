// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkTensorWidget.h"
#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkEvent.h"
#include "vtkEventData.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTensorRepresentation.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"
#include "vtkWidgetEventTranslator.h"

#include <algorithm>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkTensorWidget);

//------------------------------------------------------------------------------
vtkTensorWidget::vtkTensorWidget()
{
  this->WidgetState = vtkTensorWidget::Start;
  this->ManagesCursor = 1;

  this->TranslationEnabled = true;
  this->ScalingEnabled = true;
  this->RotationEnabled = true;
  this->MoveFacesEnabled = true;

  // Define widget events
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent, vtkEvent::NoModifier, 0,
    0, nullptr, vtkWidgetEvent::Select, this, vtkTensorWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent, vtkEvent::NoModifier,
    0, 0, nullptr, vtkWidgetEvent::EndSelect, this, vtkTensorWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonPressEvent,
    vtkWidgetEvent::Translate, this, vtkTensorWidget::TranslateAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonReleaseEvent,
    vtkWidgetEvent::EndTranslate, this, vtkTensorWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent,
    vtkEvent::ControlModifier, 0, 0, nullptr, vtkWidgetEvent::Translate, this,
    vtkTensorWidget::TranslateAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
    vtkEvent::ControlModifier, 0, 0, nullptr, vtkWidgetEvent::EndTranslate, this,
    vtkTensorWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent, vtkEvent::ShiftModifier,
    0, 0, nullptr, vtkWidgetEvent::Translate, this, vtkTensorWidget::TranslateAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
    vtkEvent::ShiftModifier, 0, 0, nullptr, vtkWidgetEvent::EndTranslate, this,
    vtkTensorWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(
    vtkCommand::RightButtonPressEvent, vtkWidgetEvent::Scale, this, vtkTensorWidget::ScaleAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonReleaseEvent,
    vtkWidgetEvent::EndScale, this, vtkTensorWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(
    vtkCommand::MouseMoveEvent, vtkWidgetEvent::Move, this, vtkTensorWidget::MoveAction);

  {
    vtkNew<vtkEventDataDevice3D> ed;
    ed->SetDevice(vtkEventDataDevice::Any);
    ed->SetInput(vtkEventDataDeviceInput::Any);
    ed->SetAction(vtkEventDataAction::Press);
    this->CallbackMapper->SetCallbackMethod(vtkCommand::Select3DEvent, ed.Get(),
      vtkWidgetEvent::Select3D, this, vtkTensorWidget::SelectAction3D);
  }

  {
    vtkNew<vtkEventDataDevice3D> ed;
    ed->SetDevice(vtkEventDataDevice::Any);
    ed->SetInput(vtkEventDataDeviceInput::Any);
    ed->SetAction(vtkEventDataAction::Release);
    this->CallbackMapper->SetCallbackMethod(vtkCommand::Select3DEvent, ed.Get(),
      vtkWidgetEvent::EndSelect3D, this, vtkTensorWidget::EndSelectAction3D);
  }

  {
    vtkNew<vtkEventDataDevice3D> ed;
    ed->SetDevice(vtkEventDataDevice::Any);
    ed->SetInput(vtkEventDataDeviceInput::Any);
    this->CallbackMapper->SetCallbackMethod(vtkCommand::Move3DEvent, ed.Get(),
      vtkWidgetEvent::Move3D, this, vtkTensorWidget::MoveAction3D);
  }

  this->KeyEventCallbackCommand = vtkCallbackCommand::New();
  this->KeyEventCallbackCommand->SetClientData(this);
  this->KeyEventCallbackCommand->SetCallback(vtkTensorWidget::ProcessKeyEvents);
}

//------------------------------------------------------------------------------
vtkTensorWidget::~vtkTensorWidget()
{
  this->KeyEventCallbackCommand->Delete();
}

//------------------------------------------------------------------------------
void vtkTensorWidget::SetEnabled(int enabling)
{
  int enabled = this->Enabled;

  // We do this step first because it sets the CurrentRenderer
  this->Superclass::SetEnabled(enabling);

  // We defer enabling the handles until the selection process begins
  if (enabling && !enabled)
  {
    if (this->Parent)
    {
      this->Parent->AddObserver(
        vtkCommand::KeyPressEvent, this->KeyEventCallbackCommand, this->Priority);
      this->Parent->AddObserver(
        vtkCommand::KeyReleaseEvent, this->KeyEventCallbackCommand, this->Priority);
    }
    else
    {
      this->Interactor->AddObserver(
        vtkCommand::KeyPressEvent, this->KeyEventCallbackCommand, this->Priority);
      this->Interactor->AddObserver(
        vtkCommand::KeyReleaseEvent, this->KeyEventCallbackCommand, this->Priority);
    }
  }
  else if (!enabling && enabled)
  {
    if (this->Parent)
    {
      this->Parent->RemoveObserver(this->KeyEventCallbackCommand);
    }
    else
    {
      this->Interactor->RemoveObserver(this->KeyEventCallbackCommand);
    }
  }
}

//------------------------------------------------------------------------------
void vtkTensorWidget::SelectAction(vtkAbstractWidget* w)
{
  // We are in a static method, cast to ourself
  vtkTensorWidget* self = reinterpret_cast<vtkTensorWidget*>(w);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!self->CurrentRenderer || !self->CurrentRenderer->IsInViewport(X, Y))
  {
    self->WidgetState = vtkTensorWidget::Start;
    return;
  }

  // Begin the widget interaction which has the side effect of setting the
  // interaction state.
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(e);
  int interactionState = self->WidgetRep->GetInteractionState();
  if (interactionState == vtkTensorRepresentation::Outside)
  {
    return;
  }

  // Test for states that involve face or handle picking here so
  // selection highlighting doesn't happen if that interaction is disabled.
  // Non-handle-grabbing transformations are tested in the "Action" methods.

  // Rotation
  if (interactionState == vtkTensorRepresentation::Rotating && self->RotationEnabled == 0)
  {
    return;
  }
  // Face Movement
  if ((interactionState == vtkTensorRepresentation::MoveF0 ||
        interactionState == vtkTensorRepresentation::MoveF1 ||
        interactionState == vtkTensorRepresentation::MoveF2 ||
        interactionState == vtkTensorRepresentation::MoveF3 ||
        interactionState == vtkTensorRepresentation::MoveF4 ||
        interactionState == vtkTensorRepresentation::MoveF5) &&
    self->MoveFacesEnabled == 0)
  {
    return;
  }
  // Translation
  if (interactionState == vtkTensorRepresentation::Translating && self->TranslationEnabled == 0)
  {
    return;
  }

  // We are definitely selected
  self->WidgetState = vtkTensorWidget::Active;
  self->GrabFocus(self->EventCallbackCommand);

  // The SetInteractionState has the side effect of highlighting the widget
  reinterpret_cast<vtkTensorRepresentation*>(self->WidgetRep)
    ->SetInteractionState(interactionState);

  // start the interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//------------------------------------------------------------------------------
void vtkTensorWidget::SelectAction3D(vtkAbstractWidget* w)
{
  vtkTensorWidget* self = reinterpret_cast<vtkTensorWidget*>(w);

  // We want to compute an orthogonal vector to the plane that has been selected
  int interactionState = self->WidgetRep->ComputeComplexInteractionState(
    self->Interactor, self, vtkWidgetEvent::Select3D, self->CallData);

  if (interactionState == vtkTensorRepresentation::Outside)
  {
    return;
  }

  // Test for states that involve face or handle picking here so
  // selection highlighting doesn't happen if that interaction is disabled.
  // Non-handle-grabbing transformations are tested in the "Action" methods.

  // Rotation
  if (interactionState == vtkTensorRepresentation::Rotating && self->RotationEnabled == 0)
  {
    return;
  }
  // Face Movement
  if ((interactionState == vtkTensorRepresentation::MoveF0 ||
        interactionState == vtkTensorRepresentation::MoveF1 ||
        interactionState == vtkTensorRepresentation::MoveF2 ||
        interactionState == vtkTensorRepresentation::MoveF3 ||
        interactionState == vtkTensorRepresentation::MoveF4 ||
        interactionState == vtkTensorRepresentation::MoveF5) &&
    self->MoveFacesEnabled == 0)
  {
    return;
  }
  // Translation
  if (interactionState == vtkTensorRepresentation::Translating && self->TranslationEnabled == 0)
  {
    return;
  }

  // We are definitely selected
  if (!self->Parent)
  {
    self->GrabFocus(self->EventCallbackCommand);
  }

  // watch for motion events from this device
  vtkEventData* edata = static_cast<vtkEventData*>(self->CallData);
  vtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();
  if (!edd)
  {
    return;
  }
  self->LastDevice = static_cast<int>(edd->GetDevice());

  self->WidgetState = vtkTensorWidget::Active;
  self->WidgetRep->StartComplexInteraction(
    self->Interactor, self, vtkWidgetEvent::Select3D, self->CallData);

  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
}

//------------------------------------------------------------------------------
void vtkTensorWidget::TranslateAction(vtkAbstractWidget* w)
{
  // We are in a static method, cast to ourself
  vtkTensorWidget* self = reinterpret_cast<vtkTensorWidget*>(w);

  if (self->TranslationEnabled == 0)
  {
    return;
  }

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!self->CurrentRenderer || !self->CurrentRenderer->IsInViewport(X, Y))
  {
    self->WidgetState = vtkTensorWidget::Start;
    return;
  }

  // Begin the widget interaction which has the side effect of setting the
  // interaction state.
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(e);
  int interactionState = self->WidgetRep->GetInteractionState();
  if (interactionState == vtkTensorRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->WidgetState = vtkTensorWidget::Active;
  self->GrabFocus(self->EventCallbackCommand);
  reinterpret_cast<vtkTensorRepresentation*>(self->WidgetRep)
    ->SetInteractionState(vtkTensorRepresentation::Translating);

  // start the interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//------------------------------------------------------------------------------
void vtkTensorWidget::ScaleAction(vtkAbstractWidget* w)
{
  // We are in a static method, cast to ourself
  vtkTensorWidget* self = reinterpret_cast<vtkTensorWidget*>(w);

  if (self->ScalingEnabled == 0)
  {
    return;
  }

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!self->CurrentRenderer || !self->CurrentRenderer->IsInViewport(X, Y))
  {
    self->WidgetState = vtkTensorWidget::Start;
    return;
  }

  // Begin the widget interaction which has the side effect of setting the
  // interaction state.
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(e);
  int interactionState = self->WidgetRep->GetInteractionState();
  if (interactionState == vtkTensorRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->WidgetState = vtkTensorWidget::Active;
  self->GrabFocus(self->EventCallbackCommand);
  reinterpret_cast<vtkTensorRepresentation*>(self->WidgetRep)
    ->SetInteractionState(vtkTensorRepresentation::Scaling);

  // start the interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//------------------------------------------------------------------------------
void vtkTensorWidget::MoveAction(vtkAbstractWidget* w)
{
  vtkTensorWidget* self = reinterpret_cast<vtkTensorWidget*>(w);

  // See whether we're active
  if (self->WidgetState == vtkTensorWidget::Start)
  {
    return;
  }

  // compute some info we need for all cases
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Okay, adjust the representation
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  self->WidgetRep->WidgetInteraction(e);

  // moving something
  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
  self->Render();
}

//------------------------------------------------------------------------------
void vtkTensorWidget::MoveAction3D(vtkAbstractWidget* w)
{
  vtkTensorWidget* self = reinterpret_cast<vtkTensorWidget*>(w);

  // See whether we're active
  if (self->WidgetState == vtkTensorWidget::Start)
  {
    return;
  }

  // watch for motion events from this device
  vtkEventData* edata = static_cast<vtkEventData*>(self->CallData);
  vtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();
  if (!edd)
  {
    return;
  }

  if (!edd->DeviceMatches(static_cast<vtkEventDataDevice>(self->LastDevice)))
  {
    return;
  }

  // Okay, adjust the representation
  self->WidgetRep->ComplexInteraction(
    self->Interactor, self, vtkWidgetEvent::Move3D, self->CallData);

  // moving something
  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
}

//------------------------------------------------------------------------------
void vtkTensorWidget::EndSelectAction(vtkAbstractWidget* w)
{
  vtkTensorWidget* self = reinterpret_cast<vtkTensorWidget*>(w);
  if (self->WidgetState == vtkTensorWidget::Start)
  {
    return;
  }

  // Return state to not active
  self->WidgetState = vtkTensorWidget::Start;
  reinterpret_cast<vtkTensorRepresentation*>(self->WidgetRep)
    ->SetInteractionState(vtkTensorRepresentation::Outside);
  self->ReleaseFocus();

  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(vtkCommand::EndInteractionEvent, nullptr);
  self->Render();
}

//------------------------------------------------------------------------------
void vtkTensorWidget::EndSelectAction3D(vtkAbstractWidget* w)
{
  vtkTensorWidget* self = reinterpret_cast<vtkTensorWidget*>(w);

  if (self->WidgetState != vtkTensorWidget::Active ||
    self->WidgetRep->GetInteractionState() == vtkTensorRepresentation::Outside)
  {
    return;
  }

  // Return state to not selected
  self->WidgetRep->EndComplexInteraction(
    self->Interactor, self, vtkWidgetEvent::Select3D, self->CallData);

  self->WidgetState = vtkTensorWidget::Start;
  if (!self->Parent)
  {
    self->ReleaseFocus();
  }

  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(vtkCommand::EndInteractionEvent, nullptr);
}

//------------------------------------------------------------------------------
void vtkTensorWidget::StepAction3D(vtkAbstractWidget* w)
{
  vtkTensorWidget* self = reinterpret_cast<vtkTensorWidget*>(w);

  // We want to compute an orthogonal vector to the plane that has been selected
  int interactionState = self->WidgetRep->ComputeComplexInteractionState(
    self->Interactor, self, vtkWidgetEvent::Select3D, self->CallData);

  if (interactionState == vtkTensorRepresentation::Outside)
  {
    return;
  }

  // self->WidgetRep->SetInteractionState(vtkTensorRepresentation::Outside);

  // Okay, adjust the representation
  self->WidgetRep->ComplexInteraction(
    self->Interactor, self, vtkWidgetEvent::Move3D, self->CallData);

  // moving something
  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
}

//------------------------------------------------------------------------------
void vtkTensorWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = vtkTensorRepresentation::New();
  }
}

//------------------------------------------------------------------------------
void vtkTensorWidget::ProcessKeyEvents(vtkObject*, unsigned long event, void* clientdata, void*)
{
  vtkTensorWidget* self = static_cast<vtkTensorWidget*>(clientdata);
  vtkTensorRepresentation* rep = vtkTensorRepresentation::SafeDownCast(self->WidgetRep);
  char* cKeySym = self->Interactor->GetKeySym();
  std::string keySym = cKeySym != nullptr ? cKeySym : "";
  std::transform(keySym.begin(), keySym.end(), keySym.begin(), ::toupper);
  if (event == vtkCommand::KeyPressEvent)
  {
    if (keySym == "X")
    {
      rep->SetXTranslationAxisOn();
    }
    else if (keySym == "Y")
    {
      rep->SetYTranslationAxisOn();
    }
    else if (keySym == "Z")
    {
      rep->SetZTranslationAxisOn();
    }
  }
  else if (event == vtkCommand::KeyReleaseEvent)
  {
    if (keySym == "X" || keySym == "Y" || keySym == "Z")
    {
      rep->SetTranslationAxisOff();
    }
  }
}

//------------------------------------------------------------------------------
void vtkTensorWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Translation Enabled: " << (this->TranslationEnabled ? "On\n" : "Off\n");
  os << indent << "Scaling Enabled: " << (this->ScalingEnabled ? "On\n" : "Off\n");
  os << indent << "Rotation Enabled: " << (this->RotationEnabled ? "On\n" : "Off\n");
  os << indent << "Move Faces Enabled: " << (this->MoveFacesEnabled ? "On\n" : "Off\n");
}
VTK_ABI_NAMESPACE_END
