/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCoordinateFrameWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCoordinateFrameWidget.h"

#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkCoordinateFrameRepresentation.h"
#include "vtkEvent.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"

vtkStandardNewMacro(vtkCoordinateFrameWidget);

// The coordinate frame widget observes its representation. The representation
// may invoke an InteractionEvent when the camera moves when LockedNormalToCamera
// is enabled.
class vtkCoordinateFrameWidgetInteractionCallback : public vtkCommand
{
public:
  static vtkCoordinateFrameWidgetInteractionCallback* New()
  {
    return new vtkCoordinateFrameWidgetInteractionCallback;
  }
  void Execute(vtkObject*, unsigned long eventId, void*) override
  {
    switch (eventId)
    {
      case vtkCommand::ModifiedEvent:
        this->CoordinateFrameWidget->InvokeInteractionCallback();
        break;
    }
  }
  vtkCoordinateFrameWidget* CoordinateFrameWidget;
};

//------------------------------------------------------------------------------
vtkCoordinateFrameWidget::vtkCoordinateFrameWidget()
{
  this->WidgetState = vtkCoordinateFrameWidget::Start;

  // Define widget events
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent, vtkWidgetEvent::Select,
    this, vtkCoordinateFrameWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
    vtkWidgetEvent::EndSelect, this, vtkCoordinateFrameWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonPressEvent,
    vtkWidgetEvent::Translate, this, vtkCoordinateFrameWidget::TranslateAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonReleaseEvent,
    vtkWidgetEvent::EndTranslate, this, vtkCoordinateFrameWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(
    vtkCommand::MouseMoveEvent, vtkWidgetEvent::Move, this, vtkCoordinateFrameWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 'p', 1,
    "p", vtkWidgetEvent::PickPoint, this, vtkCoordinateFrameWidget::PickOriginAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 'P', 1,
    "P", vtkWidgetEvent::PickPoint, this, vtkCoordinateFrameWidget::PickOriginAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 'n', 1,
    "n", vtkWidgetEvent::PickNormal, this, vtkCoordinateFrameWidget::PickNormalAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 'N', 1,
    "N", vtkWidgetEvent::PickNormal, this, vtkCoordinateFrameWidget::PickNormalAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 'd', 1,
    "d", vtkWidgetEvent::PickDirectionPoint, this,
    vtkCoordinateFrameWidget::PickDirectionPointAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 'D', 1,
    "D", vtkWidgetEvent::PickDirectionPoint, this,
    vtkCoordinateFrameWidget::PickDirectionPointAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 'x', 1,
    "x", vtkWidgetEvent::ModifyEvent, this, vtkCoordinateFrameWidget::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 'X', 1,
    "X", vtkWidgetEvent::ModifyEvent, this, vtkCoordinateFrameWidget::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 'y', 1,
    "y", vtkWidgetEvent::ModifyEvent, this, vtkCoordinateFrameWidget::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 'Y', 1,
    "Y", vtkWidgetEvent::ModifyEvent, this, vtkCoordinateFrameWidget::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 'z', 1,
    "z", vtkWidgetEvent::ModifyEvent, this, vtkCoordinateFrameWidget::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 'Z', 1,
    "Z", vtkWidgetEvent::ModifyEvent, this, vtkCoordinateFrameWidget::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent, vtkEvent::AnyModifier, 'x',
    1, "x", vtkWidgetEvent::Reset, this, vtkCoordinateFrameWidget::TranslationAxisUnLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent, vtkEvent::AnyModifier, 'X',
    1, "X", vtkWidgetEvent::Reset, this, vtkCoordinateFrameWidget::TranslationAxisUnLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent, vtkEvent::AnyModifier, 'y',
    1, "y", vtkWidgetEvent::Reset, this, vtkCoordinateFrameWidget::TranslationAxisUnLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent, vtkEvent::AnyModifier, 'Y',
    1, "Y", vtkWidgetEvent::Reset, this, vtkCoordinateFrameWidget::TranslationAxisUnLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent, vtkEvent::AnyModifier, 'z',
    1, "z", vtkWidgetEvent::Reset, this, vtkCoordinateFrameWidget::TranslationAxisUnLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent, vtkEvent::AnyModifier, 'Z',
    1, "Z", vtkWidgetEvent::Reset, this, vtkCoordinateFrameWidget::TranslationAxisUnLock);

  this->InteractionCallback = vtkCoordinateFrameWidgetInteractionCallback::New();
  this->InteractionCallback->CoordinateFrameWidget = this;
}

//------------------------------------------------------------------------------
vtkCoordinateFrameWidget::~vtkCoordinateFrameWidget()
{
  this->InteractionCallback->Delete();
}

//------------------------------------------------------------------------------
void vtkCoordinateFrameWidget::SelectAction(vtkAbstractWidget* w)
{
  vtkCoordinateFrameWidget* self = reinterpret_cast<vtkCoordinateFrameWidget*>(w);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  reinterpret_cast<vtkCoordinateFrameRepresentation*>(self->WidgetRep)
    ->SetInteractionState(vtkCoordinateFrameRepresentation::Moving);
  int interactionState = self->WidgetRep->ComputeInteractionState(X, Y);
  self->UpdateCursorShape(interactionState);

  if (self->WidgetRep->GetInteractionState() == vtkCoordinateFrameRepresentation::Outside)
  {
    return;
  }

  if (self->WidgetRep->GetInteractionState() ==
      vtkCoordinateFrameRepresentation::ModifyingLockerXVector ||
    self->WidgetRep->GetInteractionState() ==
      vtkCoordinateFrameRepresentation::ModifyingLockerYVector ||
    self->WidgetRep->GetInteractionState() ==
      vtkCoordinateFrameRepresentation::ModifyingLockerZVector)
  {
    double eventPos[2];
    eventPos[0] = static_cast<double>(X);
    eventPos[1] = static_cast<double>(Y);

    self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
    self->GetCoordinateFrameRepresentation()->WidgetInteraction(eventPos);
    self->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
    self->EventCallbackCommand->SetAbortFlag(1);
    self->InvokeEvent(vtkCommand::EndInteractionEvent, nullptr);
    self->Render();
  }
  else
  {
    // We are definitely selected
    self->GrabFocus(self->EventCallbackCommand);
    double eventPos[2];
    eventPos[0] = static_cast<double>(X);
    eventPos[1] = static_cast<double>(Y);
    self->WidgetState = vtkCoordinateFrameWidget::Active;
    self->WidgetRep->StartWidgetInteraction(eventPos);

    self->EventCallbackCommand->SetAbortFlag(1);
    self->StartInteraction();
    self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
    self->Render();
  }
}

//------------------------------------------------------------------------------
void vtkCoordinateFrameWidget::PickOriginAction(vtkAbstractWidget* w)
{
  vtkCoordinateFrameWidget* self = reinterpret_cast<vtkCoordinateFrameWidget*>(w);

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
  bool newOriginPicked = self->GetCoordinateFrameRepresentation()->PickOrigin(
    X, Y, self->Interactor->GetControlKey() == 1);
  self->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(vtkCommand::EndInteractionEvent, nullptr);
  if (newOriginPicked)
  {
    self->Render();
  }
}

//------------------------------------------------------------------------------
void vtkCoordinateFrameWidget::PickNormalAction(vtkAbstractWidget* w)
{
  vtkCoordinateFrameWidget* self = reinterpret_cast<vtkCoordinateFrameWidget*>(w);

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
  bool newNormalPicked = self->GetCoordinateFrameRepresentation()->PickNormal(
    X, Y, self->Interactor->GetControlKey() == 1);
  self->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(vtkCommand::EndInteractionEvent, nullptr);
  if (newNormalPicked)
  {
    self->Render();
  }
}

//------------------------------------------------------------------------------
void vtkCoordinateFrameWidget::PickDirectionPointAction(vtkAbstractWidget* w)
{
  vtkCoordinateFrameWidget* self = reinterpret_cast<vtkCoordinateFrameWidget*>(w);

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
  bool newDirectionPointPicked = self->GetCoordinateFrameRepresentation()->PickDirectionPoint(
    X, Y, self->Interactor->GetControlKey() == 1);
  self->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(vtkCommand::EndInteractionEvent, nullptr);
  if (newDirectionPointPicked)
  {
    self->Render();
  }
}

//------------------------------------------------------------------------------
void vtkCoordinateFrameWidget::TranslateAction(vtkAbstractWidget* w)
{
  vtkCoordinateFrameWidget* self = reinterpret_cast<vtkCoordinateFrameWidget*>(w);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // We want to compute an orthogonal vector to the pane that has been selected
  reinterpret_cast<vtkCoordinateFrameRepresentation*>(self->WidgetRep)
    ->SetInteractionState(vtkCoordinateFrameRepresentation::Moving);
  int interactionState = self->WidgetRep->ComputeInteractionState(X, Y);
  self->UpdateCursorShape(interactionState);

  if (self->WidgetRep->GetInteractionState() == vtkCoordinateFrameRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->GrabFocus(self->EventCallbackCommand);
  double eventPos[2];
  eventPos[0] = static_cast<double>(X);
  eventPos[1] = static_cast<double>(Y);
  self->WidgetState = vtkCoordinateFrameWidget::Active;
  self->WidgetRep->StartWidgetInteraction(eventPos);

  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//------------------------------------------------------------------------------
void vtkCoordinateFrameWidget::MoveAction(vtkAbstractWidget* w)
{
  vtkCoordinateFrameWidget* self = reinterpret_cast<vtkCoordinateFrameWidget*>(w);

  // To change the cursor shape when the mouse is poised over
  // the widget. Unfortunately, this results in a few extra picks
  // due to the cell picker. However, given that it's picking planes
  // and the handles/arrows, this should be very quick
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  int changed = 0;

  if (self->ManagesCursor && self->WidgetState != vtkCoordinateFrameWidget::Active)
  {
    auto rep = reinterpret_cast<vtkCoordinateFrameRepresentation*>(self->WidgetRep);

    int oldState = rep->GetRepresentationState();

    rep->SetInteractionState(vtkCoordinateFrameRepresentation::Moving);
    int state = self->WidgetRep->ComputeInteractionState(X, Y);

    changed = self->UpdateCursorShape(state);
    rep->SetInteractionState(oldState);

    changed = (changed || state != oldState) ? 1 : 0;
  }

  // See whether we're active
  if (self->WidgetState == vtkCoordinateFrameWidget::Start)
  {
    if (changed && self->ManagesCursor)
    {
      self->Render();
    }
    return;
  }

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
void vtkCoordinateFrameWidget::EndSelectAction(vtkAbstractWidget* w)
{
  vtkCoordinateFrameWidget* self = reinterpret_cast<vtkCoordinateFrameWidget*>(w);

  if (self->WidgetState != vtkCoordinateFrameWidget::Active ||
    self->WidgetRep->GetInteractionState() == vtkCoordinateFrameRepresentation::Outside)
  {
    return;
  }

  // Return state to not selected
  double e[2];
  self->WidgetRep->EndWidgetInteraction(e);
  self->WidgetState = vtkCoordinateFrameWidget::Start;
  self->ReleaseFocus();

  // Update cursor if managed
  self->UpdateCursorShape(
    reinterpret_cast<vtkCoordinateFrameRepresentation*>(self->WidgetRep)->GetRepresentationState());

  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(vtkCommand::EndInteractionEvent, nullptr);
  self->Render();
}

//------------------------------------------------------------------------------
void vtkCoordinateFrameWidget::SetEnabled(int enabling)
{
  if (this->Enabled == enabling)
  {
    return;
  }

  if (this->GetCurrentRenderer() && !enabling)
  {
    this->GetCurrentRenderer()->GetActiveCamera()->RemoveObserver(this->InteractionCallback);
  }

  this->Superclass::SetEnabled(enabling);
}

//------------------------------------------------------------------------------
void vtkCoordinateFrameWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = vtkCoordinateFrameRepresentation::New();
  }
}

//------------------------------------------------------------------------------
void vtkCoordinateFrameWidget::SetRepresentation(vtkCoordinateFrameRepresentation* rep)
{
  this->Superclass::SetWidgetRepresentation(reinterpret_cast<vtkWidgetRepresentation*>(rep));
}

//------------------------------------------------------------------------------
int vtkCoordinateFrameWidget::UpdateCursorShape(int state)
{
  // So as to change the cursor shape when the mouse is poised over
  // the widget.
  if (this->ManagesCursor)
  {
    if (state == vtkCoordinateFrameRepresentation::Outside)
    {
      return this->RequestCursorShape(VTK_CURSOR_DEFAULT);
    }
    else
    {
      return this->RequestCursorShape(VTK_CURSOR_HAND);
    }
  }

  return 0;
}

//------------------------------------------------------------------------------
void vtkCoordinateFrameWidget::InvokeInteractionCallback()
{
  vtkMTimeType previousMtime;
  vtkCoordinateFrameRepresentation* widgetRep =
    reinterpret_cast<vtkCoordinateFrameRepresentation*>(this->WidgetRep);

  if (widgetRep->GetLockNormalToCamera())
  {
    previousMtime = widgetRep->GetMTime();
    this->GetCoordinateFrameRepresentation()->SetNormalToCamera();

    if (widgetRep->GetMTime() > previousMtime)
    {
      this->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
    }
  }
}

//------------------------------------------------------------------------------
void vtkCoordinateFrameWidget::TranslationAxisLock(vtkAbstractWidget* widget)
{
  vtkCoordinateFrameWidget* self = reinterpret_cast<vtkCoordinateFrameWidget*>(widget);
  vtkCoordinateFrameRepresentation* rep =
    vtkCoordinateFrameRepresentation::SafeDownCast(self->WidgetRep);
  if (self->Interactor->GetKeyCode() == 'x' || self->Interactor->GetKeyCode() == 'X')
  {
    rep->SetXTranslationAxisOn();
  }
  if (self->Interactor->GetKeyCode() == 'y' || self->Interactor->GetKeyCode() == 'Y')
  {
    rep->SetYTranslationAxisOn();
  }
  if (self->Interactor->GetKeyCode() == 'z' || self->Interactor->GetKeyCode() == 'Z')
  {
    rep->SetZTranslationAxisOn();
  }
}

//------------------------------------------------------------------------------
void vtkCoordinateFrameWidget::TranslationAxisUnLock(vtkAbstractWidget* widget)
{
  vtkCoordinateFrameWidget* self = reinterpret_cast<vtkCoordinateFrameWidget*>(widget);
  vtkCoordinateFrameRepresentation::SafeDownCast(self->WidgetRep)->SetTranslationAxisOff();
}

//------------------------------------------------------------------------------
void vtkCoordinateFrameWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
