/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDisplaySizedImplicitPlaneWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDisplaySizedImplicitPlaneWidget.h"

#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkDisplaySizedImplicitPlaneRepresentation.h"
#include "vtkEvent.h"
#include "vtkEventData.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStdString.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"

vtkStandardNewMacro(vtkDisplaySizedImplicitPlaneWidget);

// The display sized implicit plane widget observes its representation. The representation
// may invoke an InteractionEvent when the camera moves when LockedNormalToCamera
// is enabled.
class vtkDisplaySizedImplicitPlaneInteractionCallback : public vtkCommand
{
public:
  static vtkDisplaySizedImplicitPlaneInteractionCallback* New()
  {
    return new vtkDisplaySizedImplicitPlaneInteractionCallback;
  }
  void Execute(vtkObject*, unsigned long eventId, void*) override
  {
    switch (eventId)
    {
      case vtkCommand::ModifiedEvent:
        this->DisplaySizedImplicitPlaneWidget->InvokeInteractionCallback();
        break;
    }
  }
  vtkDisplaySizedImplicitPlaneWidget* DisplaySizedImplicitPlaneWidget;
};

///------------------------------------------------------------------------------
vtkDisplaySizedImplicitPlaneWidget::vtkDisplaySizedImplicitPlaneWidget()
{
  this->WidgetState = vtkDisplaySizedImplicitPlaneWidget::Start;

  // Define widget events
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent, vtkWidgetEvent::Select,
    this, vtkDisplaySizedImplicitPlaneWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
    vtkWidgetEvent::EndSelect, this, vtkDisplaySizedImplicitPlaneWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonPressEvent,
    vtkWidgetEvent::Translate, this, vtkDisplaySizedImplicitPlaneWidget::TranslateAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonReleaseEvent,
    vtkWidgetEvent::EndTranslate, this, vtkDisplaySizedImplicitPlaneWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonPressEvent, vtkWidgetEvent::Scale,
    this, vtkDisplaySizedImplicitPlaneWidget::ScaleAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonReleaseEvent,
    vtkWidgetEvent::EndScale, this, vtkDisplaySizedImplicitPlaneWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MouseMoveEvent, vtkWidgetEvent::Move, this,
    vtkDisplaySizedImplicitPlaneWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 'p', 1,
    "p", vtkWidgetEvent::PickPoint, this, vtkDisplaySizedImplicitPlaneWidget::PickOriginAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 'P', 1,
    "P", vtkWidgetEvent::PickPoint, this, vtkDisplaySizedImplicitPlaneWidget::PickOriginAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 'n', 1,
    "n", vtkWidgetEvent::PickNormal, this, vtkDisplaySizedImplicitPlaneWidget::PickNormalAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 'N', 1,
    "N", vtkWidgetEvent::PickNormal, this, vtkDisplaySizedImplicitPlaneWidget::PickNormalAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 30, 1,
    "Up", vtkWidgetEvent::Up, this, vtkDisplaySizedImplicitPlaneWidget::MovePlaneAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 28, 1,
    "Right", vtkWidgetEvent::Up, this, vtkDisplaySizedImplicitPlaneWidget::MovePlaneAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 31, 1,
    "Down", vtkWidgetEvent::Down, this, vtkDisplaySizedImplicitPlaneWidget::MovePlaneAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 29, 1,
    "Left", vtkWidgetEvent::Down, this, vtkDisplaySizedImplicitPlaneWidget::MovePlaneAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 'x', 1,
    "x", vtkWidgetEvent::ModifyEvent, this,
    vtkDisplaySizedImplicitPlaneWidget::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 'X', 1,
    "X", vtkWidgetEvent::ModifyEvent, this,
    vtkDisplaySizedImplicitPlaneWidget::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 'y', 1,
    "y", vtkWidgetEvent::ModifyEvent, this,
    vtkDisplaySizedImplicitPlaneWidget::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 'Y', 1,
    "Y", vtkWidgetEvent::ModifyEvent, this,
    vtkDisplaySizedImplicitPlaneWidget::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 'z', 1,
    "z", vtkWidgetEvent::ModifyEvent, this,
    vtkDisplaySizedImplicitPlaneWidget::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 'Z', 1,
    "Z", vtkWidgetEvent::ModifyEvent, this,
    vtkDisplaySizedImplicitPlaneWidget::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent, vtkEvent::AnyModifier, 'x',
    1, "x", vtkWidgetEvent::Reset, this, vtkDisplaySizedImplicitPlaneWidget::TranslationAxisUnLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent, vtkEvent::AnyModifier, 'X',
    1, "X", vtkWidgetEvent::Reset, this, vtkDisplaySizedImplicitPlaneWidget::TranslationAxisUnLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent, vtkEvent::AnyModifier, 'y',
    1, "y", vtkWidgetEvent::Reset, this, vtkDisplaySizedImplicitPlaneWidget::TranslationAxisUnLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent, vtkEvent::AnyModifier, 'Y',
    1, "Y", vtkWidgetEvent::Reset, this, vtkDisplaySizedImplicitPlaneWidget::TranslationAxisUnLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent, vtkEvent::AnyModifier, 'z',
    1, "z", vtkWidgetEvent::Reset, this, vtkDisplaySizedImplicitPlaneWidget::TranslationAxisUnLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent, vtkEvent::AnyModifier, 'Z',
    1, "Z", vtkWidgetEvent::Reset, this, vtkDisplaySizedImplicitPlaneWidget::TranslationAxisUnLock);

  {
    vtkNew<vtkEventDataDevice3D> ed;
    ed->SetDevice(vtkEventDataDevice::Any);
    ed->SetInput(vtkEventDataDeviceInput::Any);
    ed->SetAction(vtkEventDataAction::Press);
    this->CallbackMapper->SetCallbackMethod(vtkCommand::Select3DEvent, ed, vtkWidgetEvent::Select3D,
      this, vtkDisplaySizedImplicitPlaneWidget::SelectAction3D);
  }

  {
    vtkNew<vtkEventDataDevice3D> ed;
    ed->SetDevice(vtkEventDataDevice::Any);
    ed->SetInput(vtkEventDataDeviceInput::Any);
    ed->SetAction(vtkEventDataAction::Release);
    this->CallbackMapper->SetCallbackMethod(vtkCommand::Select3DEvent, ed,
      vtkWidgetEvent::EndSelect3D, this, vtkDisplaySizedImplicitPlaneWidget::EndSelectAction3D);
  }

  {
    vtkNew<vtkEventDataDevice3D> ed;
    ed->SetDevice(vtkEventDataDevice::Any);
    ed->SetInput(vtkEventDataDeviceInput::Any);
    this->CallbackMapper->SetCallbackMethod(vtkCommand::Move3DEvent, ed, vtkWidgetEvent::Move3D,
      this, vtkDisplaySizedImplicitPlaneWidget::MoveAction3D);
  }

  this->InteractionCallback = vtkDisplaySizedImplicitPlaneInteractionCallback::New();
  this->InteractionCallback->DisplaySizedImplicitPlaneWidget = this;
}

//------------------------------------------------------------------------------
vtkDisplaySizedImplicitPlaneWidget::~vtkDisplaySizedImplicitPlaneWidget()
{
  this->InteractionCallback->Delete();
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneWidget::SelectAction(vtkAbstractWidget* w)
{
  vtkDisplaySizedImplicitPlaneWidget* self =
    reinterpret_cast<vtkDisplaySizedImplicitPlaneWidget*>(w);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // We want to compute an orthogonal vector to the plane that has been selected
  reinterpret_cast<vtkDisplaySizedImplicitPlaneRepresentation*>(self->WidgetRep)
    ->SetInteractionState(vtkDisplaySizedImplicitPlaneRepresentation::Moving);
  int interactionState = self->WidgetRep->ComputeInteractionState(X, Y);
  self->UpdateCursorShape(interactionState);

  if (self->WidgetRep->GetInteractionState() == vtkDisplaySizedImplicitPlaneRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->GrabFocus(self->EventCallbackCommand);
  double eventPos[2];
  eventPos[0] = static_cast<double>(X);
  eventPos[1] = static_cast<double>(Y);
  self->WidgetState = vtkDisplaySizedImplicitPlaneWidget::Active;
  self->WidgetRep->StartWidgetInteraction(eventPos);

  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneWidget::PickOriginAction(vtkAbstractWidget* w)
{
  vtkDisplaySizedImplicitPlaneWidget* self =
    reinterpret_cast<vtkDisplaySizedImplicitPlaneWidget*>(w);

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Invoke all the events associated with moving the plane
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
  bool newOriginPicked = self->GetDisplaySizedImplicitPlaneRepresentation()->PickOrigin(
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
void vtkDisplaySizedImplicitPlaneWidget::PickNormalAction(vtkAbstractWidget* w)
{
  vtkDisplaySizedImplicitPlaneWidget* self =
    reinterpret_cast<vtkDisplaySizedImplicitPlaneWidget*>(w);

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Invoke all the events associated with moving the plane
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
  bool newNormalPicked = self->GetDisplaySizedImplicitPlaneRepresentation()->PickNormal(
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
void vtkDisplaySizedImplicitPlaneWidget::SelectAction3D(vtkAbstractWidget* w)
{
  vtkDisplaySizedImplicitPlaneWidget* self =
    reinterpret_cast<vtkDisplaySizedImplicitPlaneWidget*>(w);

  vtkEventData* edata = static_cast<vtkEventData*>(self->CallData);
  vtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();
  if (!edd)
  {
    return;
  }

  // We want to compute an orthogonal vector to the plane that has been selected
  reinterpret_cast<vtkDisplaySizedImplicitPlaneRepresentation*>(self->WidgetRep)
    ->SetInteractionState(vtkDisplaySizedImplicitPlaneRepresentation::Moving);
  int interactionState = self->WidgetRep->ComputeComplexInteractionState(
    self->Interactor, self, vtkWidgetEvent::Select3D, self->CallData);
  self->UpdateCursorShape(interactionState);

  if (self->WidgetRep->GetInteractionState() == vtkDisplaySizedImplicitPlaneRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  if (!self->Parent)
  {
    self->GrabFocus(self->EventCallbackCommand);
  }

  self->WidgetState = vtkDisplaySizedImplicitPlaneWidget::Active;
  self->WidgetRep->StartComplexInteraction(
    self->Interactor, self, vtkWidgetEvent::Select3D, self->CallData);

  self->LastDevice = static_cast<int>(edd->GetDevice());

  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneWidget::TranslateAction(vtkAbstractWidget* w)
{
  vtkDisplaySizedImplicitPlaneWidget* self =
    reinterpret_cast<vtkDisplaySizedImplicitPlaneWidget*>(w);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // We want to compute an orthogonal vector to the pane that has been selected
  reinterpret_cast<vtkDisplaySizedImplicitPlaneRepresentation*>(self->WidgetRep)
    ->SetInteractionState(vtkDisplaySizedImplicitPlaneRepresentation::Moving);
  int interactionState = self->WidgetRep->ComputeInteractionState(X, Y);
  self->UpdateCursorShape(interactionState);

  if (self->WidgetRep->GetInteractionState() == vtkDisplaySizedImplicitPlaneRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->GrabFocus(self->EventCallbackCommand);
  double eventPos[2];
  eventPos[0] = static_cast<double>(X);
  eventPos[1] = static_cast<double>(Y);
  self->WidgetState = vtkDisplaySizedImplicitPlaneWidget::Active;
  self->WidgetRep->StartWidgetInteraction(eventPos);

  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneWidget::ScaleAction(vtkAbstractWidget* w)
{
  vtkDisplaySizedImplicitPlaneWidget* self =
    reinterpret_cast<vtkDisplaySizedImplicitPlaneWidget*>(w);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // We want to compute an orthogonal vector to the pane that has been selected
  reinterpret_cast<vtkDisplaySizedImplicitPlaneRepresentation*>(self->WidgetRep)
    ->SetInteractionState(vtkDisplaySizedImplicitPlaneRepresentation::Scaling);
  int interactionState = self->WidgetRep->ComputeInteractionState(X, Y);
  self->UpdateCursorShape(interactionState);

  if (self->WidgetRep->GetInteractionState() == vtkDisplaySizedImplicitPlaneRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->GrabFocus(self->EventCallbackCommand);
  double eventPos[2];
  eventPos[0] = static_cast<double>(X);
  eventPos[1] = static_cast<double>(Y);
  self->WidgetState = vtkDisplaySizedImplicitPlaneWidget::Active;
  self->WidgetRep->StartWidgetInteraction(eventPos);

  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneWidget::MoveAction(vtkAbstractWidget* w)
{
  vtkDisplaySizedImplicitPlaneWidget* self =
    reinterpret_cast<vtkDisplaySizedImplicitPlaneWidget*>(w);

  // To change the cursor shape when the mouse is poised over
  // the widget. Unfortunately, this results in a few extra picks
  // due to the cell picker. However, given that it's picking planes
  // and the handles/arrows, this should be very quick
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  int changed = 0;

  if (self->ManagesCursor && self->WidgetState != vtkDisplaySizedImplicitPlaneWidget::Active)
  {
    auto rep = reinterpret_cast<vtkDisplaySizedImplicitPlaneRepresentation*>(self->WidgetRep);

    int oldState = rep->GetRepresentationState();

    rep->SetInteractionState(vtkDisplaySizedImplicitPlaneRepresentation::Moving);
    int state = self->WidgetRep->ComputeInteractionState(X, Y);

    changed = self->UpdateCursorShape(state);
    rep->SetInteractionState(oldState);

    changed = (changed || state != oldState) ? 1 : 0;
  }

  // See whether we're active
  if (self->WidgetState == vtkDisplaySizedImplicitPlaneWidget::Start)
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
void vtkDisplaySizedImplicitPlaneWidget::MoveAction3D(vtkAbstractWidget* w)
{
  vtkDisplaySizedImplicitPlaneWidget* self =
    reinterpret_cast<vtkDisplaySizedImplicitPlaneWidget*>(w);

  // See whether we're active
  if (self->WidgetState == vtkDisplaySizedImplicitPlaneWidget::Start)
  {
    return;
  }

  vtkEventData* edata = static_cast<vtkEventData*>(self->CallData);
  vtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();
  if (!edd || static_cast<int>(edd->GetDevice()) != self->LastDevice)
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
void vtkDisplaySizedImplicitPlaneWidget::EndSelectAction(vtkAbstractWidget* w)
{
  vtkDisplaySizedImplicitPlaneWidget* self =
    reinterpret_cast<vtkDisplaySizedImplicitPlaneWidget*>(w);

  if (self->WidgetState != vtkDisplaySizedImplicitPlaneWidget::Active ||
    self->WidgetRep->GetInteractionState() == vtkDisplaySizedImplicitPlaneRepresentation::Outside)
  {
    return;
  }

  // Return state to not selected
  double e[2];
  self->WidgetRep->EndWidgetInteraction(e);
  self->WidgetState = vtkDisplaySizedImplicitPlaneWidget::Start;
  self->ReleaseFocus();

  // Update cursor if managed
  self->UpdateCursorShape(
    reinterpret_cast<vtkDisplaySizedImplicitPlaneRepresentation*>(self->WidgetRep)
      ->GetRepresentationState());

  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(vtkCommand::EndInteractionEvent, nullptr);
  self->Render();
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneWidget::EndSelectAction3D(vtkAbstractWidget* w)
{
  vtkDisplaySizedImplicitPlaneWidget* self =
    reinterpret_cast<vtkDisplaySizedImplicitPlaneWidget*>(w);

  if (self->WidgetState != vtkDisplaySizedImplicitPlaneWidget::Active ||
    self->WidgetRep->GetInteractionState() == vtkDisplaySizedImplicitPlaneRepresentation::Outside)
  {
    return;
  }

  // Return state to not selected
  self->WidgetRep->EndComplexInteraction(
    self->Interactor, self, vtkWidgetEvent::Select3D, self->CallData);

  self->WidgetState = vtkDisplaySizedImplicitPlaneWidget::Start;
  if (!self->Parent)
  {
    self->ReleaseFocus();
  }

  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(vtkCommand::EndInteractionEvent, nullptr);
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneWidget::MovePlaneAction(vtkAbstractWidget* w)
{
  vtkDisplaySizedImplicitPlaneWidget* self =
    reinterpret_cast<vtkDisplaySizedImplicitPlaneWidget*>(w);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // We want to compute an orthogonal vector to the plane that has been selected
  reinterpret_cast<vtkDisplaySizedImplicitPlaneRepresentation*>(self->WidgetRep)
    ->SetInteractionState(vtkDisplaySizedImplicitPlaneRepresentation::Moving);
  self->WidgetRep->ComputeInteractionState(X, Y);

  if (self->WidgetRep->GetInteractionState() == vtkDisplaySizedImplicitPlaneRepresentation::Outside)
  {
    return;
  }

  // Invoke all the events associated with moving the plane
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);

  // Move the plane
  double factor = (self->Interactor->GetControlKey() ? 0.5 : 1.0);
  if (!strcmp(self->Interactor->GetKeySym(), "Down") ||
    !strcmp(self->Interactor->GetKeySym(), "Left"))
  {
    self->GetDisplaySizedImplicitPlaneRepresentation()->BumpPlane(-1, factor);
  }
  else
  {
    self->GetDisplaySizedImplicitPlaneRepresentation()->BumpPlane(1, factor);
  }
  self->InvokeEvent(vtkCommand::InteractionEvent, nullptr);

  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(vtkCommand::EndInteractionEvent, nullptr);
  self->Render();
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneWidget::SetEnabled(int enabling)
{
  if (this->Enabled == enabling)
  {
    return;
  }

  if (this->GetCurrentRenderer() && !enabling)
  {
    this->GetCurrentRenderer()->GetActiveCamera()->RemoveObserver(this->InteractionCallback);
  }

  Superclass::SetEnabled(enabling);
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = vtkDisplaySizedImplicitPlaneRepresentation::New();
  }
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneWidget::SetRepresentation(
  vtkDisplaySizedImplicitPlaneRepresentation* rep)
{
  this->Superclass::SetWidgetRepresentation(reinterpret_cast<vtkWidgetRepresentation*>(rep));
}

//------------------------------------------------------------------------------
int vtkDisplaySizedImplicitPlaneWidget::UpdateCursorShape(int state)
{
  // So as to change the cursor shape when the mouse is poised over
  // the widget.
  if (this->ManagesCursor)
  {
    if (state == vtkDisplaySizedImplicitPlaneRepresentation::Outside)
    {
      return this->RequestCursorShape(VTK_CURSOR_DEFAULT);
    }
    else if (state == vtkDisplaySizedImplicitPlaneRepresentation::MovingOutline)
    {
      return this->RequestCursorShape(VTK_CURSOR_SIZEALL);
    }
    else
    {
      return this->RequestCursorShape(VTK_CURSOR_HAND);
    }
  }

  return 0;
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneWidget::SetLockNormalToCamera(int lock)
{
  if (!this->GetDisplaySizedImplicitPlaneRepresentation() || !this->Enabled ||
    !this->GetCurrentRenderer())
  {
    return;
  }

  this->GetDisplaySizedImplicitPlaneRepresentation()->SetLockNormalToCamera(lock);

  // We assume that the renderer of the Widget cannot be changed without
  // previously being disabled.
  if (lock)
  {
    // We Observe the Camera && make the update
    this->GetCurrentRenderer()->GetActiveCamera()->AddObserver(
      vtkCommand::ModifiedEvent, this->InteractionCallback, this->Priority);

    this->GetDisplaySizedImplicitPlaneRepresentation()->SetNormalToCamera();
    this->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
  }
  else
  {
    this->GetCurrentRenderer()->GetActiveCamera()->RemoveObserver(this->InteractionCallback);
  }
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneWidget::InvokeInteractionCallback()
{
  vtkMTimeType previousMtime;
  vtkDisplaySizedImplicitPlaneRepresentation* widgetRep =
    reinterpret_cast<vtkDisplaySizedImplicitPlaneRepresentation*>(this->WidgetRep);

  if (widgetRep->GetLockNormalToCamera())
  {
    previousMtime = widgetRep->GetMTime();
    this->GetDisplaySizedImplicitPlaneRepresentation()->SetNormalToCamera();

    if (widgetRep->GetMTime() > previousMtime)
    {
      this->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
    }
  }
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneWidget::TranslationAxisLock(vtkAbstractWidget* widget)
{
  vtkDisplaySizedImplicitPlaneWidget* self =
    reinterpret_cast<vtkDisplaySizedImplicitPlaneWidget*>(widget);
  vtkDisplaySizedImplicitPlaneRepresentation* rep =
    vtkDisplaySizedImplicitPlaneRepresentation::SafeDownCast(self->WidgetRep);
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
void vtkDisplaySizedImplicitPlaneWidget::TranslationAxisUnLock(vtkAbstractWidget* widget)
{
  vtkDisplaySizedImplicitPlaneWidget* self =
    reinterpret_cast<vtkDisplaySizedImplicitPlaneWidget*>(widget);
  vtkDisplaySizedImplicitPlaneRepresentation::SafeDownCast(self->WidgetRep)
    ->SetTranslationAxisOff();
}

//------------------------------------------------------------------------------
void vtkDisplaySizedImplicitPlaneWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
