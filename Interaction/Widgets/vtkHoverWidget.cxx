/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHoverWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHoverWidget.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"
#include "vtkWidgetEventTranslator.h"
#include "vtkObjectFactory.h"
#include "vtkCallbackCommand.h"
#include "vtkEvent.h"

vtkStandardNewMacro(vtkHoverWidget);

//-------------------------------------------------------------------------
vtkHoverWidget::vtkHoverWidget()
{
  this->WidgetState = Start;
  this->TimerDuration = 250;

  // Okay, define the events for this widget. Note that we look for extra events
  // (like button press) because without it the hover widget thinks nothing has changed
  // and doesn't begin retiming.
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent,
                                          vtkWidgetEvent::Move,
                                          this, vtkHoverWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonPressEvent,
                                          vtkWidgetEvent::Move,
                                          this, vtkHoverWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonPressEvent,
                                          vtkWidgetEvent::Move,
                                          this, vtkHoverWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MouseWheelForwardEvent,
                                          vtkWidgetEvent::Move,
                                          this, vtkHoverWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MouseWheelBackwardEvent,
                                          vtkWidgetEvent::Move,
                                          this, vtkHoverWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MouseMoveEvent,
                                          vtkWidgetEvent::Move,
                                          this, vtkHoverWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::TimerEvent,
                                          vtkWidgetEvent::TimedOut,
                                          this, vtkHoverWidget::HoverAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent,
                                          vtkEvent::AnyModifier, 13, 1, "Return",
                                          vtkWidgetEvent::Select,
                                          this, vtkHoverWidget::SelectAction);
}

//-------------------------------------------------------------------------
vtkHoverWidget::~vtkHoverWidget()
{
}

//----------------------------------------------------------------------
void vtkHoverWidget::SetEnabled(int enabling)
{
  if ( enabling ) //----------------
  {
    vtkDebugMacro(<<"Enabling widget");

    if ( this->Enabled ) //already enabled, just return
    {
      return;
    }

    if ( ! this->Interactor )
    {
      vtkErrorMacro(<<"The interactor must be set prior to enabling the widget");
      return;
    }

    // We're ready to enable
    this->Enabled = 1;

    // listen for the events found in the EventTranslator
    this->EventTranslator->AddEventsToInteractor(this->Interactor,
                                                 this->EventCallbackCommand,this->Priority);

    // Start off the timer
    this->TimerId = this->Interactor->CreateRepeatingTimer(this->TimerDuration);
    this->WidgetState = vtkHoverWidget::Timing;

    this->InvokeEvent(vtkCommand::EnableEvent,NULL);
  }

  else //disabling------------------
  {
    vtkDebugMacro(<<"Disabling widget");

    if ( ! this->Enabled ) //already disabled, just return
    {
      return;
    }

    this->Enabled = 0;
    this->Interactor->RemoveObserver(this->EventCallbackCommand);
    this->InvokeEvent(vtkCommand::DisableEvent,NULL);
  }
}

//-------------------------------------------------------------------------
void vtkHoverWidget::MoveAction(vtkAbstractWidget *w)
{
  vtkHoverWidget *self = reinterpret_cast<vtkHoverWidget*>(w);
  if ( self->WidgetState == vtkHoverWidget::Timing )
  {
    self->Interactor->DestroyTimer(self->TimerId);
  }
  else //we have already timed out, on this move we begin retiming
  {
    self->WidgetState = vtkHoverWidget::Timing;
    self->SubclassEndHoverAction();
    self->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  }
  self->TimerId = self->Interactor->CreateRepeatingTimer(self->TimerDuration);
}

//-------------------------------------------------------------------------
void vtkHoverWidget::HoverAction(vtkAbstractWidget *w)
{
  vtkHoverWidget *self = reinterpret_cast<vtkHoverWidget*>(w);
  int timerId = *(reinterpret_cast<int*>(self->CallData));

  // If this is the timer event we are waiting for...
  if ( timerId == self->TimerId && self->WidgetState == vtkHoverWidget::Timing )
  {
    self->Interactor->DestroyTimer(self->TimerId);
    self->WidgetState = vtkHoverWidget::TimedOut;
    self->SubclassHoverAction();
    self->InvokeEvent(vtkCommand::TimerEvent,NULL);
    self->EventCallbackCommand->SetAbortFlag(1); //no one else gets this timer
  }
}

//-------------------------------------------------------------------------
void vtkHoverWidget::SelectAction(vtkAbstractWidget *w)
{
  vtkHoverWidget *self = reinterpret_cast<vtkHoverWidget*>(w);

  // If widget is hovering we grab the selection event
  if ( self->WidgetState == vtkHoverWidget::TimedOut )
  {
    self->SubclassSelectAction();
    self->InvokeEvent(vtkCommand::WidgetActivateEvent,NULL);
    self->EventCallbackCommand->SetAbortFlag(1); //no one else gets this event
  }
}

//-------------------------------------------------------------------------
void vtkHoverWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Timer Duration: " << this->TimerDuration << "\n";
}
