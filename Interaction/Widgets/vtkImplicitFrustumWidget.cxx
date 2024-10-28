// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkImplicitFrustumWidget.h"

#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkEvent.h"
#include "vtkImplicitFrustumRepresentation.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"
#include "vtkWidgetEventTranslator.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkImplicitFrustumWidget);

//------------------------------------------------------------------------------
vtkImplicitFrustumWidget::vtkImplicitFrustumWidget()
{
  // Define widget events
  // Left mouse button: widget handles selection
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent, vtkWidgetEvent::Select,
    this, vtkImplicitFrustumWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
    vtkWidgetEvent::EndSelect, this, vtkImplicitFrustumWidget::EndSelectAction);

  // Middle mouse button: Translate
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonPressEvent,
    vtkWidgetEvent::Translate, this, vtkImplicitFrustumWidget::TranslateAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonReleaseEvent,
    vtkWidgetEvent::EndTranslate, this, vtkImplicitFrustumWidget::EndSelectAction);

  // Mouse motion
  this->CallbackMapper->SetCallbackMethod(
    vtkCommand::MouseMoveEvent, vtkWidgetEvent::Move, this, vtkImplicitFrustumWidget::MoveAction);

  // X Key: Lock/Unlock X Axis
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 'x', 1,
    "x", vtkWidgetEvent::ModifyEvent, this, vtkImplicitFrustumWidget::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 24, 1,
    "x", vtkWidgetEvent::ModifyEvent, this, vtkImplicitFrustumWidget::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 'X', 1,
    "X", vtkWidgetEvent::ModifyEvent, this, vtkImplicitFrustumWidget::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent, vtkEvent::AnyModifier, 'x',
    1, "x", vtkWidgetEvent::Reset, this, vtkImplicitFrustumWidget::TranslationAxisUnLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent, vtkEvent::AnyModifier, 24, 1,
    "x", vtkWidgetEvent::Reset, this, vtkImplicitFrustumWidget::TranslationAxisUnLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent, vtkEvent::AnyModifier, 'X',
    1, "X", vtkWidgetEvent::Reset, this, vtkImplicitFrustumWidget::TranslationAxisUnLock);

  // Y Key: Lock/Unlock Y Axis
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 'y', 1,
    "y", vtkWidgetEvent::ModifyEvent, this, vtkImplicitFrustumWidget::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 25, 1,
    "y", vtkWidgetEvent::ModifyEvent, this, vtkImplicitFrustumWidget::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 'Y', 1,
    "Y", vtkWidgetEvent::ModifyEvent, this, vtkImplicitFrustumWidget::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent, vtkEvent::AnyModifier, 'y',
    1, "y", vtkWidgetEvent::Reset, this, vtkImplicitFrustumWidget::TranslationAxisUnLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent, vtkEvent::AnyModifier, 25, 1,
    "y", vtkWidgetEvent::Reset, this, vtkImplicitFrustumWidget::TranslationAxisUnLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent, vtkEvent::AnyModifier, 'Y',
    1, "Y", vtkWidgetEvent::Reset, this, vtkImplicitFrustumWidget::TranslationAxisUnLock);

  // Z Key: Lock/Unlock Z Axis
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 'z', 1,
    "z", vtkWidgetEvent::ModifyEvent, this, vtkImplicitFrustumWidget::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 26, 1,
    "z", vtkWidgetEvent::ModifyEvent, this, vtkImplicitFrustumWidget::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 'Z', 1,
    "Z", vtkWidgetEvent::ModifyEvent, this, vtkImplicitFrustumWidget::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent, vtkEvent::AnyModifier, 'z',
    1, "z", vtkWidgetEvent::Reset, this, vtkImplicitFrustumWidget::TranslationAxisUnLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent, vtkEvent::AnyModifier, 26, 1,
    "z", vtkWidgetEvent::Reset, this, vtkImplicitFrustumWidget::TranslationAxisUnLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent, vtkEvent::AnyModifier, 'Z',
    1, "Z", vtkWidgetEvent::Reset, this, vtkImplicitFrustumWidget::TranslationAxisUnLock);
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumWidget::SelectAction(vtkAbstractWidget* w)
{
  vtkImplicitFrustumWidget* self = reinterpret_cast<vtkImplicitFrustumWidget*>(w);
  vtkImplicitFrustumRepresentation* repr = self->GetFrustumRepresentation();

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // We want to update the angle, axis and origin as appropriate
  repr->SetInteractionState(vtkImplicitFrustumRepresentation::InteractionStateType::Moving);
  int interactionState = repr->ComputeInteractionState(X, Y);

  self->UpdateCursorShape(interactionState);

  if (repr->GetInteractionState() ==
    vtkImplicitFrustumRepresentation::InteractionStateType::Outside)
  {
    return;
  }

  if (self->Interactor->GetControlKey() &&
    interactionState == vtkImplicitFrustumRepresentation::InteractionStateType::MovingOrigin)
  {
    repr->SetInteractionState(
      vtkImplicitFrustumRepresentation::InteractionStateType::TranslatingOriginOnAxis);
  }

  // We are definitely selected
  self->GrabFocus(self->EventCallbackCommand);
  double eventPos[2] = { static_cast<double>(X), static_cast<double>(Y) };
  self->State = WidgetStateType::Active;
  repr->StartWidgetInteraction(eventPos);

  self->EventCallbackCommand->SetAbortFlag(true);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumWidget::TranslateAction(vtkAbstractWidget* w)
{
  vtkImplicitFrustumWidget* self = reinterpret_cast<vtkImplicitFrustumWidget*>(w);
  vtkImplicitFrustumRepresentation* repr = self->GetFrustumRepresentation();

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // We want to compute an orthogonal vector to the plane that has been selected
  repr->SetInteractionState(vtkImplicitFrustumRepresentation::InteractionStateType::MovingOrigin);
  int interactionState = repr->ComputeInteractionState(X, Y);
  self->UpdateCursorShape(interactionState);

  if (repr->GetInteractionState() ==
    vtkImplicitFrustumRepresentation::InteractionStateType::Outside)
  {
    return;
  }

  // We are definitely selected
  self->GrabFocus(self->EventCallbackCommand);
  self->State = WidgetStateType::Active;
  double eventPos[2] = { static_cast<double>(X), static_cast<double>(Y) };
  repr->StartWidgetInteraction(eventPos);

  self->EventCallbackCommand->SetAbortFlag(true);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumWidget::MoveAction(vtkAbstractWidget* w)
{
  vtkImplicitFrustumWidget* self = reinterpret_cast<vtkImplicitFrustumWidget*>(w);
  vtkImplicitFrustumRepresentation* representation = self->GetFrustumRepresentation();

  // Change the cursor shape when the mouse is hovering
  // the widget. Unfortunately, this results in a few extra picks
  // due to the cell picker. However given that its picking simple geometry
  // like the handles/arrows, this should be very quick
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  bool changed = false;

  if (self->ManagesCursor && self->State != WidgetStateType::Active)
  {
    auto oldState = static_cast<vtkImplicitFrustumRepresentation::InteractionStateType>(
      representation->GetInteractionState());

    representation->SetInteractionState(
      vtkImplicitFrustumRepresentation::InteractionStateType::Moving);
    auto newState = static_cast<vtkImplicitFrustumRepresentation::InteractionStateType>(
      representation->ComputeInteractionState(X, Y));

    changed = self->UpdateCursorShape(newState);

    representation->SetInteractionState(oldState);

    changed |= newState != oldState;
  }

  // See whether we're active
  if (self->State == WidgetStateType::Idle)
  {
    if (changed && self->ManagesCursor)
    {
      self->Render();
    }
    return;
  }

  // Okay, adjust the representation
  double eventPos[2] = { static_cast<double>(X), static_cast<double>(Y) };
  representation->WidgetInteraction(eventPos);

  // moving something
  self->EventCallbackCommand->SetAbortFlag(true);
  self->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
  self->Render();
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumWidget::EndSelectAction(vtkAbstractWidget* w)
{
  vtkImplicitFrustumWidget* self = reinterpret_cast<vtkImplicitFrustumWidget*>(w);
  vtkImplicitFrustumRepresentation* repr = self->GetFrustumRepresentation();

  if (self->State != WidgetStateType::Active ||
    repr->GetInteractionState() == vtkImplicitFrustumRepresentation::InteractionStateType::Outside)
  {
    return;
  }

  // Return state to not selected
  double eventPos[2];
  repr->EndWidgetInteraction(eventPos);
  self->State = WidgetStateType::Idle;
  self->ReleaseFocus();

  // Update cursor if managed
  self->UpdateCursorShape(repr->GetRepresentationState());

  self->EventCallbackCommand->SetAbortFlag(true);
  self->EndInteraction();
  self->InvokeEvent(vtkCommand::EndInteractionEvent, nullptr);
  self->Render();
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = vtkImplicitFrustumRepresentation::New();
  }
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumWidget::SetRepresentation(vtkImplicitFrustumRepresentation* rep)
{
  this->Superclass::SetWidgetRepresentation(reinterpret_cast<vtkWidgetRepresentation*>(rep));
}

//------------------------------------------------------------------------------
bool vtkImplicitFrustumWidget::UpdateCursorShape(int state)
{
  // So as to change the cursor shape when the mouse is poised over
  // the widget.
  if (this->ManagesCursor)
  {
    if (state == vtkImplicitFrustumRepresentation::InteractionStateType::Outside)
    {
      return this->RequestCursorShape(VTK_CURSOR_DEFAULT);
    }
    else
    {
      return this->RequestCursorShape(VTK_CURSOR_HAND);
    }
  }

  return false;
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumWidget::TranslationAxisLock(vtkAbstractWidget* widget)
{
  vtkImplicitFrustumWidget* self = reinterpret_cast<vtkImplicitFrustumWidget*>(widget);
  vtkImplicitFrustumRepresentation* repr = self->GetFrustumRepresentation();

  char* cKeySym = self->Interactor->GetKeySym();
  std::string keySym = cKeySym != nullptr ? cKeySym : "";
  std::transform(keySym.begin(), keySym.end(), keySym.begin(), ::toupper);
  if (keySym == "X")
  {
    repr->SetXTranslationAxisOn();
  }
  else if (keySym == "Y")
  {
    repr->SetYTranslationAxisOn();
  }
  else if (keySym == "Z")
  {
    repr->SetZTranslationAxisOn();
  }
}

//------------------------------------------------------------------------------
void vtkImplicitFrustumWidget::TranslationAxisUnLock(vtkAbstractWidget* widget)
{
  vtkImplicitFrustumWidget* self = reinterpret_cast<vtkImplicitFrustumWidget*>(widget);
  self->GetFrustumRepresentation()->SetTranslationAxisOff();
}

VTK_ABI_NAMESPACE_END
