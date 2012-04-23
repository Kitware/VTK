/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompassWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkCompassWidget.h"
#include "vtkCompassRepresentation.h"
#include "vtkEvent.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"
#include "vtkWidgetEventTranslator.h"
#include "vtkTimerLog.h"

vtkStandardNewMacro(vtkCompassWidget);

//------------------------------------------------------------
vtkCompassWidget::vtkCompassWidget()
{
  // Set the initial state
  this->WidgetState = vtkCompassWidget::Start;
  this->TimerDuration = 50;

  // Okay, define the events
  this->CallbackMapper->SetCallbackMethod
    (vtkCommand::LeftButtonPressEvent,
     vtkWidgetEvent::Select,
     this, vtkCompassWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod
    (vtkCommand::MouseMoveEvent,
     vtkWidgetEvent::Move,
     this, vtkCompassWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod
    (vtkCommand::LeftButtonReleaseEvent,
     vtkWidgetEvent::EndSelect,
     this, vtkCompassWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod
    (vtkCommand::TimerEvent,
     vtkWidgetEvent::TimedOut,
     this, vtkCompassWidget::TimerAction);
}

//----------------------------------------------------------------------
void vtkCompassWidget::CreateDefaultRepresentation()
{
  if ( ! this->WidgetRep )
    {
    this->WidgetRep = vtkCompassRepresentation::New();
    }
}

//-----------------------------------------------------------------
double vtkCompassWidget::GetHeading()
{
  this->CreateDefaultRepresentation();
  vtkCompassRepresentation *slider =
    vtkCompassRepresentation::SafeDownCast(this->WidgetRep);
  return slider->GetHeading();
}

//-----------------------------------------------------------------
void vtkCompassWidget::SetHeading(double value)
{
  this->CreateDefaultRepresentation();
  vtkCompassRepresentation *slider =
    vtkCompassRepresentation::SafeDownCast(this->WidgetRep);
  slider->SetHeading(value);
}

//-----------------------------------------------------------------
double vtkCompassWidget::GetTilt()
{
  this->CreateDefaultRepresentation();
  vtkCompassRepresentation *slider =
    vtkCompassRepresentation::SafeDownCast(this->WidgetRep);
  return slider->GetTilt();
}

//-----------------------------------------------------------------
void vtkCompassWidget::SetTilt(double value)
{
  this->CreateDefaultRepresentation();
  vtkCompassRepresentation *slider =
    vtkCompassRepresentation::SafeDownCast(this->WidgetRep);
  slider->SetTilt(value);
}

//-----------------------------------------------------------------
double vtkCompassWidget::GetDistance()
{
  this->CreateDefaultRepresentation();
  vtkCompassRepresentation *slider =
    vtkCompassRepresentation::SafeDownCast(this->WidgetRep);
  return slider->GetDistance();
}

//-----------------------------------------------------------------
void vtkCompassWidget::SetDistance(double value)
{
  this->CreateDefaultRepresentation();
  vtkCompassRepresentation *slider =
    vtkCompassRepresentation::SafeDownCast(this->WidgetRep);
  slider->SetDistance(value);
}

//-------------------------------------------------------------
void vtkCompassWidget::SelectAction(vtkAbstractWidget *w)
{
  vtkCompassWidget *self =
    reinterpret_cast<vtkCompassWidget*>(w);

  double eventPos[2];
  eventPos[0] = self->Interactor->GetEventPosition()[0];
  eventPos[1] = self->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!self->CurrentRenderer ||
      !self->CurrentRenderer->IsInViewport(static_cast<int>(eventPos[0]),
                                           static_cast<int>(eventPos[1])))
    {
    return;
    }

  // See if the widget has been selected. StartWidgetInteraction records the
  // starting point of the motion.
  self->CreateDefaultRepresentation();
  self->WidgetRep->StartWidgetInteraction(eventPos);
  int interactionState = self->WidgetRep->GetInteractionState();

  if ( interactionState == vtkCompassRepresentation::TiltDown)
    {
    self->SetTilt(self->GetTilt() - 15);
    self->InvokeEvent(vtkCommand::InteractionEvent,NULL);
    self->EventCallbackCommand->SetAbortFlag(1);
    return;
    }
  if (interactionState == vtkCompassRepresentation::TiltUp)
    {
    self->SetTilt(self->GetTilt() + 15);
    self->InvokeEvent(vtkCommand::InteractionEvent,NULL);
    self->EventCallbackCommand->SetAbortFlag(1);
    return;
    }

  if ( interactionState == vtkCompassRepresentation::TiltAdjusting)
    {
    self->GrabFocus(self->EventCallbackCommand);
    self->WidgetState = vtkCompassWidget::TiltAdjusting;
    // Start off the timer
    self->TimerId =
      self->Interactor->CreateRepeatingTimer(self->TimerDuration);
    self->StartTime = vtkTimerLog::GetUniversalTime();
    // Highlight as necessary
    self->WidgetRep->Highlight(1);
    // start the interaction
    self->StartInteraction();
    self->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
    self->EventCallbackCommand->SetAbortFlag(1);
    self->Render();
    return;
    }

  if ( interactionState == vtkCompassRepresentation::DistanceIn)
    {
    self->SetDistance(self->GetDistance()*0.8);
    self->InvokeEvent(vtkCommand::InteractionEvent,NULL);
    self->EventCallbackCommand->SetAbortFlag(1);
    return;
    }
  if (interactionState == vtkCompassRepresentation::DistanceOut)
    {
    self->SetDistance(self->GetDistance()*1.2);
    self->InvokeEvent(vtkCommand::InteractionEvent,NULL);
    self->EventCallbackCommand->SetAbortFlag(1);
    return;
    }

  if ( interactionState == vtkCompassRepresentation::DistanceAdjusting)
    {
    self->GrabFocus(self->EventCallbackCommand);
    self->WidgetState = vtkCompassWidget::DistanceAdjusting;
    // Start off the timer
    self->TimerId =
      self->Interactor->CreateRepeatingTimer(self->TimerDuration);
    self->StartTime = vtkTimerLog::GetUniversalTime();
    // Highlight as necessary
    self->WidgetRep->Highlight(1);
    // start the interaction
    self->StartInteraction();
    self->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
    self->EventCallbackCommand->SetAbortFlag(1);
    self->Render();
    return;
    }

  if (interactionState != vtkCompassRepresentation::Adjusting)
    {
    return;
    }

  // We are definitely selected
  self->GrabFocus(self->EventCallbackCommand);
  self->EventCallbackCommand->SetAbortFlag(1);
  if ( interactionState == vtkCompassRepresentation::Adjusting )
    {
    self->WidgetState = vtkCompassWidget::Adjusting;
    // Highlight as necessary
    self->WidgetRep->Highlight(1);
    // start the interaction
    self->StartInteraction();
    self->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
    self->EventCallbackCommand->SetAbortFlag(1);
    self->Render();
    return;
    }
}


//---------------------------------------------------------------
void vtkCompassWidget::MoveAction(vtkAbstractWidget *w)
{
  vtkCompassWidget *self =
    reinterpret_cast<vtkCompassWidget*>(w);

  // do we need to change highlight state?
  self->CreateDefaultRepresentation();
  int interactionState = self->WidgetRep->ComputeInteractionState
    (self->Interactor->GetEventPosition()[0],
     self->Interactor->GetEventPosition()[1]);

  // if we are outside and in the start state then return
  if (interactionState == vtkCompassRepresentation::Outside &&
      self->WidgetState == vtkCompassWidget::Start)
    {
    return;
    }

  // if we are not outside and in the highlighting state then return
  if (interactionState != vtkCompassRepresentation::Outside &&
      self->WidgetState == vtkCompassWidget::Highlighting)
    {
    return;
    }

  // if we are not outside and in the Start state highlight
  if ( interactionState != vtkCompassRepresentation::Outside &&
       self->WidgetState == vtkCompassWidget::Start)
    {
    self->WidgetRep->Highlight(1);
    self->WidgetState = vtkCompassWidget::Highlighting;
    self->Render();
    return;
    }

  // if we are outside but in the highlight state then stop highlighting
  if ( self->WidgetState == vtkCompassWidget::Highlighting &&
       interactionState == vtkCompassRepresentation::Outside)
    {
    self->WidgetRep->Highlight(0);
    self->WidgetState = vtkCompassWidget::Start;
    self->Render();
    return;
    }

  vtkCompassRepresentation *rep =
    vtkCompassRepresentation::SafeDownCast(self->WidgetRep);

  // Definitely moving the slider, get the updated position
  double eventPos[2];
  eventPos[0] = self->Interactor->GetEventPosition()[0];
  eventPos[1] = self->Interactor->GetEventPosition()[1];
  if (self->WidgetState == vtkCompassWidget::TiltAdjusting)
    {
    rep->TiltWidgetInteraction(eventPos);
    }
  if (self->WidgetState == vtkCompassWidget::DistanceAdjusting)
    {
    rep->DistanceWidgetInteraction(eventPos);
    }
  if (self->WidgetState == vtkCompassWidget::Adjusting)
    {
    self->WidgetRep->WidgetInteraction(eventPos);
    }
  self->InvokeEvent(vtkCommand::InteractionEvent,NULL);

  // Interact, if desired
  self->EventCallbackCommand->SetAbortFlag(1);
}


//-----------------------------------------------------------------
void vtkCompassWidget::EndSelectAction(vtkAbstractWidget *w)
{
  vtkCompassWidget *self =
    reinterpret_cast<vtkCompassWidget*>(w);

  if ( self->WidgetState != vtkCompassWidget::Adjusting &&
       self->WidgetState != vtkCompassWidget::TiltAdjusting &&
       self->WidgetState != vtkCompassWidget::DistanceAdjusting)
    {
    return;
    }

  if (self->WidgetState == vtkCompassWidget::TiltAdjusting)
    {
    // stop the timer
    self->Interactor->DestroyTimer(self->TimerId);
    vtkCompassRepresentation *rep =
      vtkCompassRepresentation::SafeDownCast(self->WidgetRep);
    rep->EndTilt();
    }

  if (self->WidgetState == vtkCompassWidget::DistanceAdjusting)
    {
    // stop the timer
    self->Interactor->DestroyTimer(self->TimerId);
    vtkCompassRepresentation *rep =
      vtkCompassRepresentation::SafeDownCast(self->WidgetRep);
    rep->EndDistance();
    }

  int interactionState = self->WidgetRep->ComputeInteractionState
    (self->Interactor->GetEventPosition()[0],
     self->Interactor->GetEventPosition()[1]);
  if ( interactionState == vtkCompassRepresentation::Outside)
    {
    self->WidgetRep->Highlight(0);
    self->WidgetState = vtkCompassWidget::Start;
    }
  else
    {
    self->WidgetState = vtkCompassWidget::Highlighting;
    }

  // The state returns to unselected
  self->ReleaseFocus();

  // Complete interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  self->Render();
}

void vtkCompassWidget::TimerAction(vtkAbstractWidget *w)
{
  vtkCompassWidget *self =
    reinterpret_cast<vtkCompassWidget*>(w);
  int timerId = *(reinterpret_cast<int*>(self->CallData));

  // If this is the timer event we are waiting for...
  if ( timerId == self->TimerId)
    {
    vtkCompassRepresentation *rep =
      vtkCompassRepresentation::SafeDownCast(self->WidgetRep);
    if (self->WidgetState == vtkCompassWidget::TiltAdjusting)
      {
      double tElapsed = vtkTimerLog::GetUniversalTime() - self->StartTime;
      rep->UpdateTilt(tElapsed);
      }
    if (self->WidgetState == vtkCompassWidget::DistanceAdjusting)
      {
      double tElapsed = vtkTimerLog::GetUniversalTime() - self->StartTime;
      rep->UpdateDistance(tElapsed);
      }
    self->StartTime = vtkTimerLog::GetUniversalTime();
    self->InvokeEvent(vtkCommand::InteractionEvent,NULL);
    self->EventCallbackCommand->SetAbortFlag(1); //no one else gets this timer
    }
}

//-----------------------------------------------------------------
void vtkCompassWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);
}

