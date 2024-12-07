// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkImplicitAnnulusWidget.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkEvent.h"
#include "vtkImplicitAnnulusRepresentation.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"
#include "vtkWidgetEventTranslator.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkImplicitAnnulusWidget);

namespace
{
constexpr double SPEED_FACTOR_HALF = 0.5;
constexpr double SPEED_FACTOR_FULL = 1.0;
}

//------------------------------------------------------------------------------
vtkImplicitAnnulusWidget::vtkImplicitAnnulusWidget()
{
  // Define widget events
  // Mouse buttons: Selection
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent, vtkWidgetEvent::Select,
    this, vtkImplicitAnnulusWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
    vtkWidgetEvent::EndSelect, this, vtkImplicitAnnulusWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonPressEvent,
    vtkWidgetEvent::Translate, this, vtkImplicitAnnulusWidget::TranslateAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonReleaseEvent,
    vtkWidgetEvent::EndTranslate, this, vtkImplicitAnnulusWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonPressEvent, vtkWidgetEvent::Scale,
    this, vtkImplicitAnnulusWidget::ScaleAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonReleaseEvent,
    vtkWidgetEvent::EndScale, this, vtkImplicitAnnulusWidget::EndSelectAction);

  // Mouse motion: Move the annulus around
  this->CallbackMapper->SetCallbackMethod(
    vtkCommand::MouseMoveEvent, vtkWidgetEvent::Move, this, vtkImplicitAnnulusWidget::MoveAction);

  // Arrow keys: Move the annulus around
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 30, 1,
    "Up", vtkWidgetEvent::Up, this, vtkImplicitAnnulusWidget::MoveAnnulusAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 28, 1,
    "Right", vtkWidgetEvent::Up, this, vtkImplicitAnnulusWidget::MoveAnnulusAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 31, 1,
    "Down", vtkWidgetEvent::Down, this, vtkImplicitAnnulusWidget::MoveAnnulusAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 29, 1,
    "Left", vtkWidgetEvent::Down, this, vtkImplicitAnnulusWidget::MoveAnnulusAction);

  // X Key: Lock/Unlock X Axis
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 'x', 1,
    "x", vtkWidgetEvent::ModifyEvent, this, vtkImplicitAnnulusWidget::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 24, 1,
    "x", vtkWidgetEvent::ModifyEvent, this, vtkImplicitAnnulusWidget::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 'X', 1,
    "X", vtkWidgetEvent::ModifyEvent, this, vtkImplicitAnnulusWidget::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent, vtkEvent::AnyModifier, 'x',
    1, "x", vtkWidgetEvent::Reset, this, vtkImplicitAnnulusWidget::TranslationAxisUnLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent, vtkEvent::AnyModifier, 24, 1,
    "x", vtkWidgetEvent::Reset, this, vtkImplicitAnnulusWidget::TranslationAxisUnLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent, vtkEvent::AnyModifier, 'X',
    1, "X", vtkWidgetEvent::Reset, this, vtkImplicitAnnulusWidget::TranslationAxisUnLock);

  // Y Key: Lock/Unlock Y Axis
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 'y', 1,
    "y", vtkWidgetEvent::ModifyEvent, this, vtkImplicitAnnulusWidget::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 25, 1,
    "y", vtkWidgetEvent::ModifyEvent, this, vtkImplicitAnnulusWidget::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 'Y', 1,
    "Y", vtkWidgetEvent::ModifyEvent, this, vtkImplicitAnnulusWidget::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent, vtkEvent::AnyModifier, 'y',
    1, "y", vtkWidgetEvent::Reset, this, vtkImplicitAnnulusWidget::TranslationAxisUnLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent, vtkEvent::AnyModifier, 25, 1,
    "y", vtkWidgetEvent::Reset, this, vtkImplicitAnnulusWidget::TranslationAxisUnLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent, vtkEvent::AnyModifier, 'Y',
    1, "Y", vtkWidgetEvent::Reset, this, vtkImplicitAnnulusWidget::TranslationAxisUnLock);

  // Z Key: Lock/Unlock Z Axis
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 'z', 1,
    "z", vtkWidgetEvent::ModifyEvent, this, vtkImplicitAnnulusWidget::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 26, 1,
    "z", vtkWidgetEvent::ModifyEvent, this, vtkImplicitAnnulusWidget::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 'Z', 1,
    "Z", vtkWidgetEvent::ModifyEvent, this, vtkImplicitAnnulusWidget::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent, vtkEvent::AnyModifier, 'z',
    1, "z", vtkWidgetEvent::Reset, this, vtkImplicitAnnulusWidget::TranslationAxisUnLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent, vtkEvent::AnyModifier, 26, 1,
    "z", vtkWidgetEvent::Reset, this, vtkImplicitAnnulusWidget::TranslationAxisUnLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent, vtkEvent::AnyModifier, 'Z',
    1, "Z", vtkWidgetEvent::Reset, this, vtkImplicitAnnulusWidget::TranslationAxisUnLock);
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusWidget::SelectAction(vtkAbstractWidget* w)
{
  vtkImplicitAnnulusWidget* self = reinterpret_cast<vtkImplicitAnnulusWidget*>(w);
  vtkImplicitAnnulusRepresentation* repr = self->GetAnnulusRepresentation();

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // We want to update the angle, axis and origin as appropriate
  repr->SetInteractionState(vtkImplicitAnnulusRepresentation::InteractionStateType::Moving);
  int interactionState = repr->ComputeInteractionState(X, Y);

  self->UpdateCursorShape(interactionState);

  if (repr->GetInteractionState() == vtkImplicitAnnulusRepresentation::Outside)
  {
    return;
  }

  if (self->Interactor->GetControlKey() &&
    interactionState == vtkImplicitAnnulusRepresentation::MovingCenter)
  {
    repr->SetInteractionState(vtkImplicitAnnulusRepresentation::TranslatingCenter);
  }

  // We are definitely selected
  self->GrabFocus(self->EventCallbackCommand);
  double eventPos[2] = {
    static_cast<double>(X),
    static_cast<double>(Y),
  };
  self->WidgetState = vtkImplicitAnnulusWidget::Active;
  repr->StartWidgetInteraction(eventPos);

  self->EventCallbackCommand->SetAbortFlag(true);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusWidget::TranslateAction(vtkAbstractWidget* w)
{
  vtkImplicitAnnulusWidget* self = reinterpret_cast<vtkImplicitAnnulusWidget*>(w);
  vtkImplicitAnnulusRepresentation* repr = self->GetAnnulusRepresentation();

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // We want to compute an orthogonal vector to the plane that has been selected
  repr->SetInteractionState(vtkImplicitAnnulusRepresentation::Moving);
  int interactionState = repr->ComputeInteractionState(X, Y);
  self->UpdateCursorShape(interactionState);

  if (repr->GetInteractionState() == vtkImplicitAnnulusRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->GrabFocus(self->EventCallbackCommand);
  double eventPos[2] = {
    static_cast<double>(X),
    static_cast<double>(Y),
  };
  self->WidgetState = vtkImplicitAnnulusWidget::Active;
  repr->StartWidgetInteraction(eventPos);

  self->EventCallbackCommand->SetAbortFlag(true);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusWidget::ScaleAction(vtkAbstractWidget* w)
{
  vtkImplicitAnnulusWidget* self = reinterpret_cast<vtkImplicitAnnulusWidget*>(w);
  vtkImplicitAnnulusRepresentation* repr = self->GetAnnulusRepresentation();

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // We want to compute an orthogonal vector to the pane that has been selected
  repr->SetInteractionState(vtkImplicitAnnulusRepresentation::Scaling);
  int interactionState = repr->ComputeInteractionState(X, Y);
  self->UpdateCursorShape(interactionState);

  if (repr->GetInteractionState() == vtkImplicitAnnulusRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->GrabFocus(self->EventCallbackCommand);

  double eventPos[2] = {
    static_cast<double>(X),
    static_cast<double>(Y),
  };

  self->WidgetState = vtkImplicitAnnulusWidget::Active;
  repr->StartWidgetInteraction(eventPos);

  self->EventCallbackCommand->SetAbortFlag(true);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusWidget::MoveAction(vtkAbstractWidget* w)
{
  vtkImplicitAnnulusWidget* self = reinterpret_cast<vtkImplicitAnnulusWidget*>(w);
  vtkImplicitAnnulusRepresentation* representation = self->GetAnnulusRepresentation();

  // Change the cursor shape when the mouse is hovering
  // the widget. Unfortunately, this results in a few extra picks
  // due to the cell picker. However given that its picking simple geometry
  // like the handles/arrows, this should be very quick
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  bool changed = false;

  if (self->ManagesCursor && self->WidgetState != vtkImplicitAnnulusWidget::Active)
  {
    auto oldState = static_cast<vtkImplicitAnnulusRepresentation::InteractionStateType>(
      representation->GetInteractionState());

    representation->SetInteractionState(vtkImplicitAnnulusRepresentation::Moving);
    auto newState = static_cast<vtkImplicitAnnulusRepresentation::InteractionStateType>(
      representation->ComputeInteractionState(X, Y));

    changed = self->UpdateCursorShape(newState);

    representation->SetInteractionState(oldState);

    changed |= newState != oldState;
  }

  // See whether we're active
  if (self->WidgetState == vtkImplicitAnnulusWidget::Idle)
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
void vtkImplicitAnnulusWidget::EndSelectAction(vtkAbstractWidget* w)
{
  vtkImplicitAnnulusWidget* self = reinterpret_cast<vtkImplicitAnnulusWidget*>(w);
  vtkImplicitAnnulusRepresentation* repr = self->GetAnnulusRepresentation();

  if (self->WidgetState != vtkImplicitAnnulusWidget::Active ||
    repr->GetInteractionState() == vtkImplicitAnnulusRepresentation::Outside)
  {
    return;
  }

  // Return state to not selected
  double e[2];
  repr->EndWidgetInteraction(e);
  self->WidgetState = vtkImplicitAnnulusWidget::Idle;
  self->ReleaseFocus();

  // Update cursor if managed
  self->UpdateCursorShape(repr->GetRepresentationState());

  self->EventCallbackCommand->SetAbortFlag(true);
  self->EndInteraction();
  self->InvokeEvent(vtkCommand::EndInteractionEvent, nullptr);
  self->Render();
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusWidget::MoveAnnulusAction(vtkAbstractWidget* w)
{
  vtkImplicitAnnulusWidget* self = reinterpret_cast<vtkImplicitAnnulusWidget*>(w);
  vtkImplicitAnnulusRepresentation* repr = self->GetAnnulusRepresentation();

  repr->SetInteractionState(vtkImplicitAnnulusRepresentation::Moving);

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  repr->ComputeInteractionState(X, Y);

  // The cursor must be over part of the widget for these key presses to work
  if (repr->GetInteractionState() == vtkImplicitAnnulusRepresentation::Outside)
  {
    return;
  }

  // Invoke all of the events associated with moving the annulus
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);

  // Move the annulus
  double factor = (self->Interactor->GetControlKey() ? SPEED_FACTOR_HALF : SPEED_FACTOR_FULL);
  char* cKeySym = self->Interactor->GetKeySym();
  std::string keySym = cKeySym != nullptr ? cKeySym : "";

  if (keySym == "Down" || keySym == "Left")
  {
    self->GetAnnulusRepresentation()->BumpAnnulus(-1, factor);
  }
  else
  {
    self->GetAnnulusRepresentation()->BumpAnnulus(1, factor);
  }
  self->InvokeEvent(vtkCommand::InteractionEvent, nullptr);

  self->EventCallbackCommand->SetAbortFlag(true);
  self->InvokeEvent(vtkCommand::EndInteractionEvent, nullptr);
  self->Render();
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = vtkImplicitAnnulusRepresentation::New();
  }
}

//------------------------------------------------------------------------------
void vtkImplicitAnnulusWidget::SetRepresentation(vtkImplicitAnnulusRepresentation* rep)
{
  this->Superclass::SetWidgetRepresentation(reinterpret_cast<vtkWidgetRepresentation*>(rep));
}

//------------------------------------------------------------------------------
bool vtkImplicitAnnulusWidget::UpdateCursorShape(int state)
{
  // So as to change the cursor shape when the mouse is poised over
  // the widget.
  if (this->ManagesCursor)
  {
    if (state == vtkImplicitAnnulusRepresentation::Outside)
    {
      return this->RequestCursorShape(VTK_CURSOR_DEFAULT);
    }
    else if (state == vtkImplicitAnnulusRepresentation::MovingOutline)
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
void vtkImplicitAnnulusWidget::TranslationAxisLock(vtkAbstractWidget* widget)
{
  vtkImplicitAnnulusWidget* self = reinterpret_cast<vtkImplicitAnnulusWidget*>(widget);
  vtkImplicitAnnulusRepresentation* repr = self->GetAnnulusRepresentation();

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
void vtkImplicitAnnulusWidget::TranslationAxisUnLock(vtkAbstractWidget* widget)
{
  vtkImplicitAnnulusWidget* self = reinterpret_cast<vtkImplicitAnnulusWidget*>(widget);
  self->GetAnnulusRepresentation()->SetTranslationAxisOff();
}

VTK_ABI_NAMESPACE_END
