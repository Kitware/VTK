// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkCompassWidget.h"
#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkCompassRepresentation.h"
#include "vtkEvent.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTimerLog.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"
#include "vtkWidgetEventTranslator.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkCompassWidget);

//------------------------------------------------------------
vtkCompassWidget::vtkCompassWidget()
{
  // Set the initial state
  this->WidgetState = vtkCompassWidget::Start;

  // Okay, define the events
  this->CallbackMapper->SetCallbackMethod(
    vtkCommand::LeftButtonPressEvent, vtkWidgetEvent::Select, this, vtkCompassWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(
    vtkCommand::MouseMoveEvent, vtkWidgetEvent::Move, this, vtkCompassWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
    vtkWidgetEvent::EndSelect, this, vtkCompassWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(
    vtkCommand::TimerEvent, vtkWidgetEvent::TimedOut, this, vtkCompassWidget::TimerAction);
}

//------------------------------------------------------------------------------
void vtkCompassWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = vtkCompassRepresentation::New();
  }
}

//-----------------------------------------------------------------
double vtkCompassWidget::GetHeading()
{
  this->CreateDefaultRepresentation();
  vtkCompassRepresentation* representation =
    vtkCompassRepresentation::SafeDownCast(this->WidgetRep);
  return representation->GetHeading();
}

//-----------------------------------------------------------------
void vtkCompassWidget::SetHeading(double value)
{
  this->CreateDefaultRepresentation();
  vtkCompassRepresentation* representation =
    vtkCompassRepresentation::SafeDownCast(this->WidgetRep);
  representation->SetHeading(value);
  this->InvokeEvent(vtkCommand::WidgetValueChangedEvent, nullptr);
}

//-----------------------------------------------------------------
double vtkCompassWidget::GetTilt()
{
  this->CreateDefaultRepresentation();
  vtkCompassRepresentation* representation =
    vtkCompassRepresentation::SafeDownCast(this->WidgetRep);
  return representation->GetTilt();
}

//-----------------------------------------------------------------
void vtkCompassWidget::SetTilt(double tilt)
{
  this->CreateDefaultRepresentation();
  vtkCompassRepresentation* representation =
    vtkCompassRepresentation::SafeDownCast(this->WidgetRep);
  representation->SetTilt(tilt);
  this->InvokeEvent(vtkCommand::WidgetValueChangedEvent, nullptr);
}

//-----------------------------------------------------------------
double vtkCompassWidget::GetDistance()
{
  this->CreateDefaultRepresentation();
  vtkCompassRepresentation* representation =
    vtkCompassRepresentation::SafeDownCast(this->WidgetRep);
  return representation->GetDistance();
}

//-----------------------------------------------------------------
void vtkCompassWidget::SetDistance(double distance)
{
  this->CreateDefaultRepresentation();
  vtkCompassRepresentation* representation =
    vtkCompassRepresentation::SafeDownCast(this->WidgetRep);
  representation->SetDistance(distance);
  this->InvokeEvent(vtkCommand::WidgetValueChangedEvent, nullptr);
}

//-------------------------------------------------------------
void vtkCompassWidget::SelectAction(vtkAbstractWidget* widget)
{
  vtkCompassWidget* self = reinterpret_cast<vtkCompassWidget*>(widget);

  double eventPos[2];
  eventPos[0] = self->Interactor->GetEventPosition()[0];
  eventPos[1] = self->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!self->CurrentRenderer ||
    !self->CurrentRenderer->IsInViewport(
      static_cast<int>(eventPos[0]), static_cast<int>(eventPos[1])))
  {
    return;
  }

  // See if the widget has been selected. StartWidgetInteraction records the
  // starting point of the motion.
  self->CreateDefaultRepresentation();
  self->WidgetRep->StartWidgetInteraction(eventPos);
  int interactionState = self->WidgetRep->GetInteractionState();

  self->WidgetState = Start;
  self->TimerId = -1;
  switch (interactionState)
  {
    case vtkCompassRepresentation::TiltDown:
      self->WidgetState = vtkCompassWidget::TiltTimerAdjustingDown;
      break;
    case vtkCompassRepresentation::TiltUp:
      self->WidgetState = vtkCompassWidget::TiltTimerAdjustingUp;
      break;
    case vtkCompassRepresentation::TiltAdjusting:
      self->WidgetState = vtkCompassWidget::TiltAdjusting;
      break;
    case vtkCompassRepresentation::DistanceIn:
      self->WidgetState = vtkCompassWidget::DistanceTimerAdjustingIn;
      break;
    case vtkCompassRepresentation::DistanceOut:
      self->WidgetState = vtkCompassWidget::DistanceTimerAdjustingOut;
      break;
    case vtkCompassRepresentation::DistanceAdjusting:
      self->WidgetState = vtkCompassWidget::DistanceAdjusting;
      break;
    case vtkCompassRepresentation::Adjusting:
      self->WidgetState = vtkCompassWidget::Adjusting;
      break;
    default:
      return;
  }

  // create the update timer
  self->TimerId = self->Interactor->CreateRepeatingTimer(self->TimerDuration);
  self->StartTime = vtkTimerLog::GetUniversalTime();

  self->GrabFocus(self->EventCallbackCommand);
  self->WidgetRep->Highlight(1);
  self->StartInteraction();

  self->EventCallbackCommand->SetAbortFlag(1);

  self->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
  self->Render();
}

//---------------------------------------------------------------
void vtkCompassWidget::MoveAction(vtkAbstractWidget* widget)
{
  vtkCompassWidget* self = reinterpret_cast<vtkCompassWidget*>(widget);
  vtkCompassRepresentation* representation =
    vtkCompassRepresentation::SafeDownCast(self->WidgetRep);

  // do we need to change highlight state?
  self->CreateDefaultRepresentation();
  int interactionState = self->WidgetRep->ComputeInteractionState(
    self->Interactor->GetEventPosition()[0], self->Interactor->GetEventPosition()[1]);

  // are we inside or outside of the widget?
  if (interactionState == vtkCompassRepresentation::Outside)
  {
    // we're outside of the widget
    switch (self->WidgetState)
    {
      case vtkCompassWidget::Start:
        // if we are in the start state then return
        self->Render();
        return;
      case vtkCompassWidget::Highlighting:
        // if in the highlight state then stop highlighting
        self->WidgetRep->Highlight(0);
        self->WidgetState = vtkCompassWidget::Start;
        self->Render();
        return;
      default:
        break;
    }
  }
  else
  {
    // we're doing something inside the widget
    switch (self->WidgetState)
    {
      case vtkCompassWidget::Start:
        // if we are in the start state then start highlighting
        self->WidgetRep->Highlight(1);
        self->WidgetState = vtkCompassWidget::Highlighting;
        self->Render();
        return;
      case vtkCompassWidget::Highlighting:
        // if we are in the highlighting state then return
        self->Render();
        return;
      default:
        break;
    }
  }

  // Definitely moving a slider or the compass wheel, get the updated position
  double eventPos[2];
  eventPos[0] = self->Interactor->GetEventPosition()[0];
  eventPos[1] = self->Interactor->GetEventPosition()[1];

  switch (self->WidgetState)
  {
    case vtkCompassWidget::TiltAdjusting:
      representation->TiltWidgetInteraction(eventPos);
      break;
    case vtkCompassWidget::DistanceAdjusting:
      representation->DistanceWidgetInteraction(eventPos);
      break;
    case vtkCompassWidget::Adjusting:
      self->WidgetRep->WidgetInteraction(eventPos);
      break;
    default:
      break;
  }

  self->EventCallbackCommand->SetAbortFlag(1);

  self->InvokeEvent(vtkCommand::WidgetValueChangedEvent, nullptr);
  self->InvokeEvent(vtkCommand::InteractionEvent, nullptr);

  self->Render();
}

//-----------------------------------------------------------------
void vtkCompassWidget::EndSelectAction(vtkAbstractWidget* widget)
{
  vtkCompassWidget* self = reinterpret_cast<vtkCompassWidget*>(widget);
  vtkCompassRepresentation* representation =
    vtkCompassRepresentation::SafeDownCast(self->WidgetRep);

  // stop the timer
  if (self->TimerId >= 0)
  {
    self->Interactor->DestroyTimer(self->TimerId);
    self->TimerId = -1;
  }

  switch (self->WidgetState)
  {
    case vtkCompassWidget::TiltAdjusting:
    case vtkCompassWidget::TiltTimerAdjustingDown:
    case vtkCompassWidget::TiltTimerAdjustingUp:
      representation->EndTilt();
      break;
    case vtkCompassWidget::DistanceAdjusting:
    case vtkCompassWidget::DistanceTimerAdjustingIn:
    case vtkCompassWidget::DistanceTimerAdjustingOut:
      representation->EndDistance();
      break;
    case vtkCompassWidget::Adjusting:
      break;
    default:
      return;
  }

  int interactionState = self->WidgetRep->ComputeInteractionState(
    self->Interactor->GetEventPosition()[0], self->Interactor->GetEventPosition()[1]);

  if (interactionState == vtkCompassRepresentation::Outside)
  {
    self->WidgetRep->Highlight(0);
    self->WidgetState = vtkCompassWidget::Start;
  }
  else
  {
    self->WidgetState = vtkCompassWidget::Highlighting;
  }

  // the state returns to unselected
  self->ReleaseFocus();

  // complete interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();

  self->InvokeEvent(vtkCommand::WidgetValueChangedEvent, nullptr);
  self->InvokeEvent(vtkCommand::EndInteractionEvent, nullptr);

  self->Render();
}

void vtkCompassWidget::TimerAction(vtkAbstractWidget* widget)
{
  vtkCompassWidget* self = reinterpret_cast<vtkCompassWidget*>(widget);
  vtkCompassRepresentation* representation =
    vtkCompassRepresentation::SafeDownCast(self->WidgetRep);

  // if no call data is available assume that the timer event belongs this widget
  // under normal operations this won't happen but can be the case when playing back
  // recorded events during tests
  int timerId = self->TimerId;
  if (self->CallData)
  {
    timerId = *(reinterpret_cast<int*>(self->CallData));
  }

  // only continue if the timer belongs to this widget
  if (timerId != self->TimerId)
  {
    return;
  }

  double tNow = vtkTimerLog::GetUniversalTime();
  double tElapsed = tNow - self->StartTime;

  switch (self->WidgetState)
  {
    case vtkCompassWidget::TiltAdjusting:
      representation->UpdateTilt(0);
      break;
    case vtkCompassWidget::TiltTimerAdjustingUp:
      representation->UpdateTilt(tElapsed * self->TiltSpeed);
      break;
    case vtkCompassWidget::TiltTimerAdjustingDown:
      representation->UpdateTilt(-tElapsed * self->TiltSpeed);
      break;
    case vtkCompassWidget::DistanceAdjusting:
      representation->UpdateDistance(0);
      break;
    case vtkCompassWidget::DistanceTimerAdjustingOut:
      representation->UpdateDistance(tElapsed * self->DistanceSpeed);
      break;
    case vtkCompassWidget::DistanceTimerAdjustingIn:
      representation->UpdateDistance(-tElapsed * self->DistanceSpeed);
      break;
    default:
      break;
  }

  // reset the start time; we're only interested in elapsed time since last timer event
  self->StartTime = tNow;

  self->EventCallbackCommand->SetAbortFlag(1); // no one else gets this timer

  self->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
  self->InvokeEvent(vtkCommand::WidgetValueChangedEvent, nullptr);

  self->Render();
}

//-----------------------------------------------------------------
void vtkCompassWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  // Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
