/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCenteredSliderWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCenteredSliderWidget.h"
#include "vtkSliderRepresentation2D.h"
#include "vtkCommand.h"
#include "vtkCallbackCommand.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkWidgetEventTranslator.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkEvent.h"
#include "vtkWidgetEvent.h"
#include "vtkTimerLog.h"

vtkStandardNewMacro(vtkCenteredSliderWidget);

//------------------------------------------------------------
vtkCenteredSliderWidget::vtkCenteredSliderWidget()
{
  // Set the initial state
  this->WidgetState = vtkCenteredSliderWidget::Start;
  this->TimerDuration = 50;
  this->Value = 0;

  // Okay, define the events
  this->CallbackMapper->SetCallbackMethod
    (vtkCommand::LeftButtonPressEvent,
     vtkWidgetEvent::Select,
     this, vtkCenteredSliderWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod
    (vtkCommand::MouseMoveEvent,
     vtkWidgetEvent::Move,
     this, vtkCenteredSliderWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod
    (vtkCommand::LeftButtonReleaseEvent,
     vtkWidgetEvent::EndSelect,
     this, vtkCenteredSliderWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod
    (vtkCommand::TimerEvent,
     vtkWidgetEvent::TimedOut,
     this, vtkCenteredSliderWidget::TimerAction);
}

//----------------------------------------------------------------------
void vtkCenteredSliderWidget::CreateDefaultRepresentation()
{
  if ( ! this->WidgetRep )
    {
    this->WidgetRep = vtkSliderRepresentation2D::New();
    }
}


//-------------------------------------------------------------
void vtkCenteredSliderWidget::SelectAction(vtkAbstractWidget *w)
{
  vtkCenteredSliderWidget *self = vtkCenteredSliderWidget::SafeDownCast(w);

  double eventPos[2];
  eventPos[0] = self->Interactor->GetEventPosition()[0];
  eventPos[1] = self->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!self->CurrentRenderer ||
      !self->CurrentRenderer->IsInViewport(static_cast<int>(eventPos[0]),
                                           static_cast<int>(eventPos[1])))
    {
    self->WidgetState = vtkCenteredSliderWidget::Start;
    return;
    }

  // See if the widget has been selected. StartWidgetInteraction records the
  // starting point of the motion.
  self->WidgetRep->StartWidgetInteraction(eventPos);
  int interactionState = self->WidgetRep->GetInteractionState();
  if ( interactionState == vtkSliderRepresentation::Outside )
    {
    return;
    }

  // We are definitely selected
  vtkSliderRepresentation *slider =
    vtkSliderRepresentation::SafeDownCast(self->WidgetRep);
  self->EventCallbackCommand->SetAbortFlag(1);
  if ( interactionState == vtkSliderRepresentation::Slider )
    {
    self->GrabFocus(self->EventCallbackCommand);
    self->WidgetState = vtkCenteredSliderWidget::Sliding;
    // Start off the timer
    self->TimerId =
      self->Interactor->CreateRepeatingTimer(self->TimerDuration);
    self->StartTime = vtkTimerLog::GetUniversalTime();
    // Highlight as necessary
    self->WidgetRep->Highlight(1);
    // start the interaction
    self->StartInteraction();
    self->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
    self->Render();
    return;
    }

  if ( interactionState == vtkSliderRepresentation::LeftCap )
    {
    self->Value = slider->GetMinimumValue();
    self->InvokeEvent(vtkCommand::InteractionEvent,NULL);
    self->Render();
    return;
    }
  if ( interactionState == vtkSliderRepresentation::RightCap )
    {
    self->Value = slider->GetMaximumValue();
    self->InvokeEvent(vtkCommand::InteractionEvent,NULL);
    self->Render();
    return;
    }
}


//---------------------------------------------------------------
void vtkCenteredSliderWidget::MoveAction(vtkAbstractWidget *w)
{
  vtkCenteredSliderWidget *self = vtkCenteredSliderWidget::SafeDownCast(w);

  // See whether we're active
  if ( self->WidgetState == vtkCenteredSliderWidget::Start )
    {
    return;
    }

  // Definitely moving the slider, get the updated position
  double eventPos[2];
  eventPos[0] = self->Interactor->GetEventPosition()[0];
  eventPos[1] = self->Interactor->GetEventPosition()[1];
  self->WidgetRep->WidgetInteraction(eventPos);

  // Interact, if desired
  self->EventCallbackCommand->SetAbortFlag(1);
}


//-----------------------------------------------------------------
void vtkCenteredSliderWidget::EndSelectAction(vtkAbstractWidget *w)
{
  vtkCenteredSliderWidget *self = vtkCenteredSliderWidget::SafeDownCast(w);

  if ( self->WidgetState == vtkCenteredSliderWidget::Start )
    {
    return;
    }

  // stop the timer
  self->Interactor->DestroyTimer(self->TimerId);

  // Highlight if necessary
  vtkSliderRepresentation *slider =
    vtkSliderRepresentation::SafeDownCast(self->WidgetRep);
  slider->SetValue((slider->GetMinimumValue() +
                    slider->GetMaximumValue())/2.0);
  self->WidgetRep->Highlight(0);

  // The state returns to unselected
  self->WidgetState = vtkCenteredSliderWidget::Start;
  self->ReleaseFocus();

  // Complete interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  self->Render();
}

void vtkCenteredSliderWidget::TimerAction(vtkAbstractWidget *w)
{
  vtkCenteredSliderWidget *self = vtkCenteredSliderWidget::SafeDownCast(w);
  int timerId = *(reinterpret_cast<int*>(self->CallData));

  // If this is the timer event we are waiting for...
  if ( timerId == self->TimerId &&
       self->WidgetState == vtkCenteredSliderWidget::Sliding )
    {
    self->Value = vtkTimerLog::GetUniversalTime() - self->StartTime;

    vtkSliderRepresentation *slider =
      vtkSliderRepresentation::SafeDownCast(self->WidgetRep);
    double avg =
      (slider->GetMinimumValue() + slider->GetMaximumValue())/2.0;
    self->Value = avg + (slider->GetValue() -  avg)*self->Value;
    self->StartTime = vtkTimerLog::GetUniversalTime();
    self->InvokeEvent(vtkCommand::InteractionEvent,NULL);
    self->EventCallbackCommand->SetAbortFlag(1); //no one else gets this timer
    self->Render();
    }
}


//-----------------------------------------------------------------
void vtkCenteredSliderWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);
}
