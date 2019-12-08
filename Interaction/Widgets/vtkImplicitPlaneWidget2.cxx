/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitPlaneWidget2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImplicitPlaneWidget2.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkEvent.h"
#include "vtkEventData.h"
#include "vtkImplicitPlaneRepresentation.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStdString.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"
#include "vtkWidgetEventTranslator.h"

vtkStandardNewMacro(vtkImplicitPlaneWidget2);

// The implicit plane widget observes its representation. The representation
// may invoke an InteractionEvent when the camera moves when LockedNormalToCamera
// is enabled.
class vtkInteractionCallback : public vtkCommand
{
public:
  static vtkInteractionCallback* New() { return new vtkInteractionCallback; }
  void Execute(vtkObject*, unsigned long eventId, void*) override
  {
    switch (eventId)
    {
      case vtkCommand::ModifiedEvent:
        this->ImplicitPlaneWidget->InvokeInteractionCallback();
        break;
    }
  }
  vtkImplicitPlaneWidget2* ImplicitPlaneWidget;
};

//----------------------------------------------------------------------------
vtkImplicitPlaneWidget2::vtkImplicitPlaneWidget2()
{
  this->WidgetState = vtkImplicitPlaneWidget2::Start;

  // Define widget events
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent, vtkWidgetEvent::Select,
    this, vtkImplicitPlaneWidget2::SelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
    vtkWidgetEvent::EndSelect, this, vtkImplicitPlaneWidget2::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonPressEvent,
    vtkWidgetEvent::Translate, this, vtkImplicitPlaneWidget2::TranslateAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonReleaseEvent,
    vtkWidgetEvent::EndTranslate, this, vtkImplicitPlaneWidget2::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonPressEvent, vtkWidgetEvent::Scale,
    this, vtkImplicitPlaneWidget2::ScaleAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonReleaseEvent,
    vtkWidgetEvent::EndScale, this, vtkImplicitPlaneWidget2::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(
    vtkCommand::MouseMoveEvent, vtkWidgetEvent::Move, this, vtkImplicitPlaneWidget2::MoveAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 30, 1,
    "Up", vtkWidgetEvent::Up, this, vtkImplicitPlaneWidget2::MovePlaneAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 28, 1,
    "Right", vtkWidgetEvent::Up, this, vtkImplicitPlaneWidget2::MovePlaneAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 31, 1,
    "Down", vtkWidgetEvent::Down, this, vtkImplicitPlaneWidget2::MovePlaneAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 29, 1,
    "Left", vtkWidgetEvent::Down, this, vtkImplicitPlaneWidget2::MovePlaneAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 'x', 1,
    "x", vtkWidgetEvent::ModifyEvent, this, vtkImplicitPlaneWidget2::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 'X', 1,
    "X", vtkWidgetEvent::ModifyEvent, this, vtkImplicitPlaneWidget2::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 'y', 1,
    "y", vtkWidgetEvent::ModifyEvent, this, vtkImplicitPlaneWidget2::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 'Y', 1,
    "Y", vtkWidgetEvent::ModifyEvent, this, vtkImplicitPlaneWidget2::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 'z', 1,
    "z", vtkWidgetEvent::ModifyEvent, this, vtkImplicitPlaneWidget2::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent, vtkEvent::AnyModifier, 'Z', 1,
    "Z", vtkWidgetEvent::ModifyEvent, this, vtkImplicitPlaneWidget2::TranslationAxisLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent, vtkEvent::AnyModifier, 'x',
    1, "x", vtkWidgetEvent::Reset, this, vtkImplicitPlaneWidget2::TranslationAxisUnLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent, vtkEvent::AnyModifier, 'X',
    1, "X", vtkWidgetEvent::Reset, this, vtkImplicitPlaneWidget2::TranslationAxisUnLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent, vtkEvent::AnyModifier, 'y',
    1, "y", vtkWidgetEvent::Reset, this, vtkImplicitPlaneWidget2::TranslationAxisUnLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent, vtkEvent::AnyModifier, 'Y',
    1, "Y", vtkWidgetEvent::Reset, this, vtkImplicitPlaneWidget2::TranslationAxisUnLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent, vtkEvent::AnyModifier, 'z',
    1, "z", vtkWidgetEvent::Reset, this, vtkImplicitPlaneWidget2::TranslationAxisUnLock);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent, vtkEvent::AnyModifier, 'Z',
    1, "Z", vtkWidgetEvent::Reset, this, vtkImplicitPlaneWidget2::TranslationAxisUnLock);

  {
    vtkNew<vtkEventDataButton3D> ed;
    ed->SetDevice(vtkEventDataDevice::RightController);
    ed->SetInput(vtkEventDataDeviceInput::Trigger);
    ed->SetAction(vtkEventDataAction::Press);
    this->CallbackMapper->SetCallbackMethod(vtkCommand::Button3DEvent, ed, vtkWidgetEvent::Select3D,
      this, vtkImplicitPlaneWidget2::SelectAction3D);
  }

  {
    vtkNew<vtkEventDataButton3D> ed;
    ed->SetDevice(vtkEventDataDevice::RightController);
    ed->SetInput(vtkEventDataDeviceInput::Trigger);
    ed->SetAction(vtkEventDataAction::Release);
    this->CallbackMapper->SetCallbackMethod(vtkCommand::Button3DEvent, ed,
      vtkWidgetEvent::EndSelect3D, this, vtkImplicitPlaneWidget2::EndSelectAction3D);
  }

  {
    vtkNew<vtkEventDataMove3D> ed;
    ed->SetDevice(vtkEventDataDevice::RightController);
    this->CallbackMapper->SetCallbackMethod(vtkCommand::Move3DEvent, ed, vtkWidgetEvent::Move3D,
      this, vtkImplicitPlaneWidget2::MoveAction3D);
  }

  this->InteractionCallback = vtkInteractionCallback::New();
  this->InteractionCallback->ImplicitPlaneWidget = this;
}

//----------------------------------------------------------------------------
vtkImplicitPlaneWidget2::~vtkImplicitPlaneWidget2()
{
  this->InteractionCallback->Delete();
}

//----------------------------------------------------------------------
void vtkImplicitPlaneWidget2::SelectAction(vtkAbstractWidget* w)
{
  vtkImplicitPlaneWidget2* self = reinterpret_cast<vtkImplicitPlaneWidget2*>(w);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // We want to compute an orthogonal vector to the plane that has been selected
  reinterpret_cast<vtkImplicitPlaneRepresentation*>(self->WidgetRep)
    ->SetInteractionState(vtkImplicitPlaneRepresentation::Moving);
  int interactionState = self->WidgetRep->ComputeInteractionState(X, Y);
  self->UpdateCursorShape(interactionState);

  if (self->WidgetRep->GetInteractionState() == vtkImplicitPlaneRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->GrabFocus(self->EventCallbackCommand);
  double eventPos[2];
  eventPos[0] = static_cast<double>(X);
  eventPos[1] = static_cast<double>(Y);
  self->WidgetState = vtkImplicitPlaneWidget2::Active;
  self->WidgetRep->StartWidgetInteraction(eventPos);

  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//-------------------------------------------------------------------------
void vtkImplicitPlaneWidget2::SelectAction3D(vtkAbstractWidget* w)
{
  vtkImplicitPlaneWidget2* self = reinterpret_cast<vtkImplicitPlaneWidget2*>(w);

  // We want to compute an orthogonal vector to the plane that has been selected
  reinterpret_cast<vtkImplicitPlaneRepresentation*>(self->WidgetRep)
    ->SetInteractionState(vtkImplicitPlaneRepresentation::Moving);
  int interactionState = self->WidgetRep->ComputeComplexInteractionState(
    self->Interactor, self, vtkWidgetEvent::Select3D, self->CallData);
  self->UpdateCursorShape(interactionState);

  if (self->WidgetRep->GetInteractionState() == vtkImplicitPlaneRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  if (!self->Parent)
  {
    self->GrabFocus(self->EventCallbackCommand);
  }

  self->WidgetState = vtkImplicitPlaneWidget2::Active;
  self->WidgetRep->StartComplexInteraction(
    self->Interactor, self, vtkWidgetEvent::Select3D, self->CallData);

  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
}

//----------------------------------------------------------------------
void vtkImplicitPlaneWidget2::TranslateAction(vtkAbstractWidget* w)
{
  vtkImplicitPlaneWidget2* self = reinterpret_cast<vtkImplicitPlaneWidget2*>(w);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // We want to compute an orthogonal vector to the pane that has been selected
  reinterpret_cast<vtkImplicitPlaneRepresentation*>(self->WidgetRep)
    ->SetInteractionState(vtkImplicitPlaneRepresentation::Moving);
  int interactionState = self->WidgetRep->ComputeInteractionState(X, Y);
  self->UpdateCursorShape(interactionState);

  if (self->WidgetRep->GetInteractionState() == vtkImplicitPlaneRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->GrabFocus(self->EventCallbackCommand);
  double eventPos[2];
  eventPos[0] = static_cast<double>(X);
  eventPos[1] = static_cast<double>(Y);
  self->WidgetState = vtkImplicitPlaneWidget2::Active;
  self->WidgetRep->StartWidgetInteraction(eventPos);

  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------
void vtkImplicitPlaneWidget2::ScaleAction(vtkAbstractWidget* w)
{
  vtkImplicitPlaneWidget2* self = reinterpret_cast<vtkImplicitPlaneWidget2*>(w);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // We want to compute an orthogonal vector to the pane that has been selected
  reinterpret_cast<vtkImplicitPlaneRepresentation*>(self->WidgetRep)
    ->SetInteractionState(vtkImplicitPlaneRepresentation::Scaling);
  int interactionState = self->WidgetRep->ComputeInteractionState(X, Y);
  self->UpdateCursorShape(interactionState);

  if (self->WidgetRep->GetInteractionState() == vtkImplicitPlaneRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->GrabFocus(self->EventCallbackCommand);
  double eventPos[2];
  eventPos[0] = static_cast<double>(X);
  eventPos[1] = static_cast<double>(Y);
  self->WidgetState = vtkImplicitPlaneWidget2::Active;
  self->WidgetRep->StartWidgetInteraction(eventPos);

  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------
void vtkImplicitPlaneWidget2::MoveAction(vtkAbstractWidget* w)
{
  vtkImplicitPlaneWidget2* self = reinterpret_cast<vtkImplicitPlaneWidget2*>(w);

  // So as to change the cursor shape when the mouse is poised over
  // the widget. Unfortunately, this results in a few extra picks
  // due to the cell picker. However given that its picking planes
  // and the handles/arrows, this should be very quick
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  int changed = 0;

  if (self->ManagesCursor && self->WidgetState != vtkImplicitPlaneWidget2::Active)
  {
    int oldInteractionState =
      reinterpret_cast<vtkImplicitPlaneRepresentation*>(self->WidgetRep)->GetInteractionState();

    reinterpret_cast<vtkImplicitPlaneRepresentation*>(self->WidgetRep)
      ->SetInteractionState(vtkImplicitPlaneRepresentation::Moving);
    int state = self->WidgetRep->ComputeInteractionState(X, Y);
    changed = self->UpdateCursorShape(state);
    reinterpret_cast<vtkImplicitPlaneRepresentation*>(self->WidgetRep)
      ->SetInteractionState(oldInteractionState);
    changed = (changed || state != oldInteractionState) ? 1 : 0;
  }

  // See whether we're active
  if (self->WidgetState == vtkImplicitPlaneWidget2::Start)
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

//----------------------------------------------------------------------
void vtkImplicitPlaneWidget2::MoveAction3D(vtkAbstractWidget* w)
{
  vtkImplicitPlaneWidget2* self = reinterpret_cast<vtkImplicitPlaneWidget2*>(w);

  // See whether we're active
  if (self->WidgetState == vtkImplicitPlaneWidget2::Start)
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

//----------------------------------------------------------------------
void vtkImplicitPlaneWidget2::EndSelectAction(vtkAbstractWidget* w)
{
  vtkImplicitPlaneWidget2* self = reinterpret_cast<vtkImplicitPlaneWidget2*>(w);

  if (self->WidgetState != vtkImplicitPlaneWidget2::Active ||
    self->WidgetRep->GetInteractionState() == vtkImplicitPlaneRepresentation::Outside)
  {
    return;
  }

  // Return state to not selected
  double e[2];
  self->WidgetRep->EndWidgetInteraction(e);
  self->WidgetState = vtkImplicitPlaneWidget2::Start;
  self->ReleaseFocus();

  // Update cursor if managed
  self->UpdateCursorShape(
    reinterpret_cast<vtkImplicitPlaneRepresentation*>(self->WidgetRep)->GetRepresentationState());

  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(vtkCommand::EndInteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------
void vtkImplicitPlaneWidget2::EndSelectAction3D(vtkAbstractWidget* w)
{
  vtkImplicitPlaneWidget2* self = reinterpret_cast<vtkImplicitPlaneWidget2*>(w);

  if (self->WidgetState != vtkImplicitPlaneWidget2::Active ||
    self->WidgetRep->GetInteractionState() == vtkImplicitPlaneRepresentation::Outside)
  {
    return;
  }

  // Return state to not selected
  self->WidgetRep->EndComplexInteraction(
    self->Interactor, self, vtkWidgetEvent::Select3D, self->CallData);

  self->WidgetState = vtkImplicitPlaneWidget2::Start;
  if (!self->Parent)
  {
    self->ReleaseFocus();
  }

  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(vtkCommand::EndInteractionEvent, nullptr);
}

//----------------------------------------------------------------------
void vtkImplicitPlaneWidget2::MovePlaneAction(vtkAbstractWidget* w)
{
  vtkImplicitPlaneWidget2* self = reinterpret_cast<vtkImplicitPlaneWidget2*>(w);

  reinterpret_cast<vtkImplicitPlaneRepresentation*>(self->WidgetRep)
    ->SetInteractionState(vtkImplicitPlaneRepresentation::Moving);

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  self->WidgetRep->ComputeInteractionState(X, Y);

  if (self->WidgetRep->GetInteractionState() == vtkImplicitPlaneRepresentation::Outside)
  {
    return;
  }

  // Invoke all of the events associated with moving the plane
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);

  // Move the plane
  double factor = (self->Interactor->GetControlKey() ? 0.5 : 1.0);
  if (vtkStdString(self->Interactor->GetKeySym()) == vtkStdString("Down") ||
    vtkStdString(self->Interactor->GetKeySym()) == vtkStdString("Left"))
  {
    self->GetImplicitPlaneRepresentation()->BumpPlane(-1, factor);
  }
  else
  {
    self->GetImplicitPlaneRepresentation()->BumpPlane(1, factor);
  }
  self->InvokeEvent(vtkCommand::InteractionEvent, nullptr);

  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(vtkCommand::EndInteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------
void vtkImplicitPlaneWidget2::SetEnabled(int enabling)
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

//----------------------------------------------------------------------
void vtkImplicitPlaneWidget2::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = vtkImplicitPlaneRepresentation::New();
  }
}

//----------------------------------------------------------------------
void vtkImplicitPlaneWidget2::SetRepresentation(vtkImplicitPlaneRepresentation* rep)
{
  this->Superclass::SetWidgetRepresentation(reinterpret_cast<vtkWidgetRepresentation*>(rep));
}

//----------------------------------------------------------------------
int vtkImplicitPlaneWidget2::UpdateCursorShape(int state)
{
  // So as to change the cursor shape when the mouse is poised over
  // the widget.
  if (this->ManagesCursor)
  {
    if (state == vtkImplicitPlaneRepresentation::Outside)
    {
      return this->RequestCursorShape(VTK_CURSOR_DEFAULT);
    }
    else if (state == vtkImplicitPlaneRepresentation::MovingOutline)
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

//----------------------------------------------------------------------------
void vtkImplicitPlaneWidget2::SetLockNormalToCamera(int lock)
{
  if (!this->GetImplicitPlaneRepresentation() || !this->Enabled || !this->GetCurrentRenderer())
  {
    return;
  }

  this->GetImplicitPlaneRepresentation()->SetLockNormalToCamera(lock);

  // We assume that the renderer of the Widget cannot be changed without
  // previously being disabled.
  if (lock)
  {
    // We Observe the Camera && make the update
    this->GetCurrentRenderer()->GetActiveCamera()->AddObserver(
      vtkCommand::ModifiedEvent, this->InteractionCallback, this->Priority);

    this->GetImplicitPlaneRepresentation()->SetNormalToCamera();
    this->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
  }
  else
  {
    this->GetCurrentRenderer()->GetActiveCamera()->RemoveObserver(this->InteractionCallback);
  }
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneWidget2::InvokeInteractionCallback()
{
  vtkMTimeType previousMtime;
  vtkImplicitPlaneRepresentation* widgetRep =
    reinterpret_cast<vtkImplicitPlaneRepresentation*>(this->WidgetRep);

  if (widgetRep->GetLockNormalToCamera())
  {
    previousMtime = widgetRep->GetMTime();
    this->GetImplicitPlaneRepresentation()->SetNormalToCamera();

    if (widgetRep->GetMTime() > previousMtime)
    {
      this->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
    }
  }
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneWidget2::TranslationAxisLock(vtkAbstractWidget* widget)
{
  vtkImplicitPlaneWidget2* self = reinterpret_cast<vtkImplicitPlaneWidget2*>(widget);
  vtkImplicitPlaneRepresentation* rep =
    vtkImplicitPlaneRepresentation::SafeDownCast(self->WidgetRep);
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

//----------------------------------------------------------------------------
void vtkImplicitPlaneWidget2::TranslationAxisUnLock(vtkAbstractWidget* widget)
{
  vtkImplicitPlaneWidget2* self = reinterpret_cast<vtkImplicitPlaneWidget2*>(widget);
  vtkImplicitPlaneRepresentation::SafeDownCast(self->WidgetRep)->SetTranslationAxisOff();
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneWidget2::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
