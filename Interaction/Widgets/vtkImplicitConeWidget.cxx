// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkImplicitConeWidget.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkEvent.h"
#include "vtkImplicitConeRepresentation.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"
#include "vtkWidgetEventTranslator.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkImplicitConeWidget);

namespace
{
constexpr double SPEED_FACTOR_HALF = 0.5;
constexpr double SPEED_FACTOR_FULL = 1.0;
}

//------------------------------------------------------------------------------
vtkImplicitConeWidget::vtkImplicitConeWidget()
{
  // Define widget events
  // Mouse buttons: Selection
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent, vtkWidgetEvent::Select,
    this, vtkImplicitConeWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
    vtkWidgetEvent::EndSelect, this, vtkImplicitConeWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonPressEvent,
    vtkWidgetEvent::Translate, this, vtkImplicitConeWidget::TranslateAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonReleaseEvent,
    vtkWidgetEvent::EndTranslate, this, vtkImplicitConeWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonPressEvent, vtkWidgetEvent::Scale,
    this, vtkImplicitConeWidget::ScaleAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonReleaseEvent,
    vtkWidgetEvent::EndScale, this, vtkImplicitConeWidget::EndSelectAction);

  // Mouse motion: Move the cone around
  this->CallbackMapper->SetCallbackMethod(
    vtkCommand::MouseMoveEvent, vtkWidgetEvent::Move, this, vtkImplicitConeWidget::MoveAction);

  // Arrow keys: Move the cone around
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 30, 1,
    "Up", vtkWidgetEvent::Up, this, vtkImplicitConeWidget::MoveConeAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 28, 1,
    "Right", vtkWidgetEvent::Up, this, vtkImplicitConeWidget::MoveConeAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 31, 1,
    "Down", vtkWidgetEvent::Down, this, vtkImplicitConeWidget::MoveConeAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 29, 1,
    "Left", vtkWidgetEvent::Down, this, vtkImplicitConeWidget::MoveConeAction);

  // X Key: Lock/Unlock X Axis
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 'x', 1,
    "x", vtkWidgetEvent::ModifyEvent, this, vtkImplicitConeWidget::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 24, 1,
    "x", vtkWidgetEvent::ModifyEvent, this, vtkImplicitConeWidget::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 'X', 1,
    "X", vtkWidgetEvent::ModifyEvent, this, vtkImplicitConeWidget::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent, vtkEvent::AnyModifier, 'x',
    1, "x", vtkWidgetEvent::Reset, this, vtkImplicitConeWidget::TranslationAxisUnLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent, vtkEvent::AnyModifier, 24, 1,
    "x", vtkWidgetEvent::Reset, this, vtkImplicitConeWidget::TranslationAxisUnLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent, vtkEvent::AnyModifier, 'X',
    1, "X", vtkWidgetEvent::Reset, this, vtkImplicitConeWidget::TranslationAxisUnLock);

  // Y Key: Lock/Unlock Y Axis
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 'y', 1,
    "y", vtkWidgetEvent::ModifyEvent, this, vtkImplicitConeWidget::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 25, 1,
    "y", vtkWidgetEvent::ModifyEvent, this, vtkImplicitConeWidget::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 'Y', 1,
    "Y", vtkWidgetEvent::ModifyEvent, this, vtkImplicitConeWidget::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent, vtkEvent::AnyModifier, 'y',
    1, "y", vtkWidgetEvent::Reset, this, vtkImplicitConeWidget::TranslationAxisUnLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent, vtkEvent::AnyModifier, 25, 1,
    "y", vtkWidgetEvent::Reset, this, vtkImplicitConeWidget::TranslationAxisUnLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent, vtkEvent::AnyModifier, 'Y',
    1, "Y", vtkWidgetEvent::Reset, this, vtkImplicitConeWidget::TranslationAxisUnLock);

  // Z Key: Lock/Unlock Z Axis
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 'z', 1,
    "z", vtkWidgetEvent::ModifyEvent, this, vtkImplicitConeWidget::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 26, 1,
    "z", vtkWidgetEvent::ModifyEvent, this, vtkImplicitConeWidget::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 'Z', 1,
    "Z", vtkWidgetEvent::ModifyEvent, this, vtkImplicitConeWidget::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent, vtkEvent::AnyModifier, 'z',
    1, "z", vtkWidgetEvent::Reset, this, vtkImplicitConeWidget::TranslationAxisUnLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent, vtkEvent::AnyModifier, 26, 1,
    "z", vtkWidgetEvent::Reset, this, vtkImplicitConeWidget::TranslationAxisUnLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent, vtkEvent::AnyModifier, 'Z',
    1, "Z", vtkWidgetEvent::Reset, this, vtkImplicitConeWidget::TranslationAxisUnLock);
}

//------------------------------------------------------------------------------
void vtkImplicitConeWidget::SelectAction(vtkAbstractWidget* w)
{
  vtkImplicitConeWidget* self = reinterpret_cast<vtkImplicitConeWidget*>(w);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // We want to update the angle, axis and origin as appropriate
  reinterpret_cast<vtkImplicitConeRepresentation*>(self->WidgetRep)
    ->SetInteractionState(vtkImplicitConeRepresentation::InteractionStateType::Moving);
  int interactionState = self->WidgetRep->ComputeInteractionState(X, Y);

  self->UpdateCursorShape(interactionState);

  if (self->WidgetRep->GetInteractionState() == vtkImplicitConeRepresentation::Outside)
  {
    return;
  }

  if (self->Interactor->GetControlKey() &&
    interactionState == vtkImplicitConeRepresentation::MovingOrigin)
  {
    reinterpret_cast<vtkImplicitConeRepresentation*>(self->WidgetRep)
      ->SetInteractionState(vtkImplicitConeRepresentation::TranslatingOrigin);
  }

  // We are definitely selected
  self->GrabFocus(self->EventCallbackCommand);
  double eventPos[2] = {
    static_cast<double>(X),
    static_cast<double>(Y),
  };
  self->WidgetState = vtkImplicitConeWidget::Active;
  self->WidgetRep->StartWidgetInteraction(eventPos);

  self->EventCallbackCommand->SetAbortFlag(true);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//------------------------------------------------------------------------------
void vtkImplicitConeWidget::TranslateAction(vtkAbstractWidget* w)
{
  vtkImplicitConeWidget* self = reinterpret_cast<vtkImplicitConeWidget*>(w);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // We want to compute an orthogonal vector to the plane that has been selected
  reinterpret_cast<vtkImplicitConeRepresentation*>(self->WidgetRep)
    ->SetInteractionState(vtkImplicitConeRepresentation::Moving);
  int interactionState = self->WidgetRep->ComputeInteractionState(X, Y);
  self->UpdateCursorShape(interactionState);

  if (self->WidgetRep->GetInteractionState() == vtkImplicitConeRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->GrabFocus(self->EventCallbackCommand);
  double eventPos[2] = {
    static_cast<double>(X),
    static_cast<double>(Y),
  };
  self->WidgetState = vtkImplicitConeWidget::Active;
  self->WidgetRep->StartWidgetInteraction(eventPos);

  self->EventCallbackCommand->SetAbortFlag(true);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//------------------------------------------------------------------------------
void vtkImplicitConeWidget::ScaleAction(vtkAbstractWidget* w)
{
  vtkImplicitConeWidget* self = reinterpret_cast<vtkImplicitConeWidget*>(w);
  vtkImplicitConeRepresentation* repr =
    reinterpret_cast<vtkImplicitConeRepresentation*>(self->WidgetRep);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // We want to compute an orthogonal vector to the pane that has been selected
  repr->SetInteractionState(vtkImplicitConeRepresentation::Scaling);
  int interactionState = repr->ComputeInteractionState(X, Y);
  self->UpdateCursorShape(interactionState);

  if (repr->GetInteractionState() == vtkImplicitConeRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->GrabFocus(self->EventCallbackCommand);

  double eventPos[2] = {
    static_cast<double>(X),
    static_cast<double>(Y),
  };

  self->WidgetState = vtkImplicitConeWidget::Active;
  repr->StartWidgetInteraction(eventPos);

  self->EventCallbackCommand->SetAbortFlag(true);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//------------------------------------------------------------------------------
void vtkImplicitConeWidget::MoveAction(vtkAbstractWidget* w)
{
  vtkImplicitConeWidget* self = reinterpret_cast<vtkImplicitConeWidget*>(w);
  vtkImplicitConeRepresentation* representation =
    reinterpret_cast<vtkImplicitConeRepresentation*>(self->WidgetRep);

  // Change the cursor shape when the mouse is hovering
  // the widget. Unfortunately, this results in a few extra picks
  // due to the cell picker. However given that its picking simple geometry
  // like the handles/arrows, this should be very quick
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  bool changed = false;

  if (self->ManagesCursor && self->WidgetState != vtkImplicitConeWidget::Active)
  {
    auto oldState = static_cast<vtkImplicitConeRepresentation::InteractionStateType>(
      representation->GetInteractionState());

    representation->SetInteractionState(vtkImplicitConeRepresentation::Moving);
    auto newState = static_cast<vtkImplicitConeRepresentation::InteractionStateType>(
      representation->ComputeInteractionState(X, Y));

    changed = self->UpdateCursorShape(newState);

    representation->SetInteractionState(oldState);

    changed |= newState != oldState;
  }

  // See whether we're active
  if (self->WidgetState == vtkImplicitConeWidget::Idle)
  {
    if (changed && self->ManagesCursor)
    {
      self->Render();
    }
    return;
  }

  // Okay, adjust the representation
  double e[2] = { static_cast<double>(X), static_cast<double>(Y) };
  representation->WidgetInteraction(e);

  // moving something
  self->EventCallbackCommand->SetAbortFlag(true);
  self->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
  self->Render();
}

//------------------------------------------------------------------------------
void vtkImplicitConeWidget::EndSelectAction(vtkAbstractWidget* w)
{
  vtkImplicitConeWidget* self = reinterpret_cast<vtkImplicitConeWidget*>(w);

  if (self->WidgetState != vtkImplicitConeWidget::Active ||
    self->WidgetRep->GetInteractionState() == vtkImplicitConeRepresentation::Outside)
  {
    return;
  }

  // Return state to not selected
  double e[2];
  self->WidgetRep->EndWidgetInteraction(e);
  self->WidgetState = vtkImplicitConeWidget::Idle;
  self->ReleaseFocus();

  // Update cursor if managed
  self->UpdateCursorShape(
    reinterpret_cast<vtkImplicitConeRepresentation*>(self->WidgetRep)->GetRepresentationState());

  self->EventCallbackCommand->SetAbortFlag(true);
  self->EndInteraction();
  self->InvokeEvent(vtkCommand::EndInteractionEvent, nullptr);
  self->Render();
}

//------------------------------------------------------------------------------
void vtkImplicitConeWidget::MoveConeAction(vtkAbstractWidget* w)
{
  vtkImplicitConeWidget* self = reinterpret_cast<vtkImplicitConeWidget*>(w);

  reinterpret_cast<vtkImplicitConeRepresentation*>(self->WidgetRep)
    ->SetInteractionState(vtkImplicitConeRepresentation::Moving);

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  self->WidgetRep->ComputeInteractionState(X, Y);

  // The cursor must be over part of the widget for these key presses to work
  if (self->WidgetRep->GetInteractionState() == vtkImplicitConeRepresentation::Outside)
  {
    return;
  }

  // Invoke all of the events associated with moving the cone
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);

  // Move the cone
  double factor = (self->Interactor->GetControlKey() ? SPEED_FACTOR_HALF : SPEED_FACTOR_FULL);
  char* cKeySym = self->Interactor->GetKeySym();
  std::string keySym = cKeySym != nullptr ? cKeySym : "";

  if (keySym == "Down" || keySym == "Left")
  {
    self->GetConeRepresentation()->BumpCone(-1, factor);
  }
  else
  {
    self->GetConeRepresentation()->BumpCone(1, factor);
  }
  self->InvokeEvent(vtkCommand::InteractionEvent, nullptr);

  self->EventCallbackCommand->SetAbortFlag(true);
  self->InvokeEvent(vtkCommand::EndInteractionEvent, nullptr);
  self->Render();
}

//------------------------------------------------------------------------------
void vtkImplicitConeWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = vtkImplicitConeRepresentation::New();
  }
}

//------------------------------------------------------------------------------
void vtkImplicitConeWidget::SetRepresentation(vtkImplicitConeRepresentation* rep)
{
  this->Superclass::SetWidgetRepresentation(reinterpret_cast<vtkWidgetRepresentation*>(rep));
}

//------------------------------------------------------------------------------
bool vtkImplicitConeWidget::UpdateCursorShape(int state)
{
  // So as to change the cursor shape when the mouse is poised over
  // the widget.
  if (this->ManagesCursor)
  {
    if (state == vtkImplicitConeRepresentation::Outside)
    {
      return this->RequestCursorShape(VTK_CURSOR_DEFAULT);
    }
    else if (state == vtkImplicitConeRepresentation::MovingOutline)
    {
      return this->RequestCursorShape(VTK_CURSOR_SIZEALL);
    }
    else
    {
      return this->RequestCursorShape(VTK_CURSOR_HAND);
    }
  }

  return false;
}

//------------------------------------------------------------------------------
void vtkImplicitConeWidget::TranslationAxisLock(vtkAbstractWidget* widget)
{
  vtkImplicitConeWidget* self = reinterpret_cast<vtkImplicitConeWidget*>(widget);
  vtkImplicitConeRepresentation* rep = vtkImplicitConeRepresentation::SafeDownCast(self->WidgetRep);
  char* cKeySym = self->Interactor->GetKeySym();
  std::string keySym = cKeySym != nullptr ? cKeySym : "";
  std::transform(keySym.begin(), keySym.end(), keySym.begin(), ::toupper);
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

//------------------------------------------------------------------------------
void vtkImplicitConeWidget::TranslationAxisUnLock(vtkAbstractWidget* widget)
{
  vtkImplicitConeWidget* self = reinterpret_cast<vtkImplicitConeWidget*>(widget);
  vtkImplicitConeRepresentation::SafeDownCast(self->WidgetRep)->SetTranslationAxisOff();
}

VTK_ABI_NAMESPACE_END
