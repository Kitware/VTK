/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContinuousValueWidget.cxx

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
#include "vtkContinuousValueWidget.h"
#include "vtkContinuousValueWidgetRepresentation.h"
#include "vtkEvent.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"
#include "vtkWidgetEventTranslator.h"


//------------------------------------------------------------
vtkContinuousValueWidget::vtkContinuousValueWidget()
{
  // Set the initial state
  this->WidgetState = vtkContinuousValueWidget::Start;
  this->Value = 0;

  // Okay, define the events
  this->CallbackMapper->SetCallbackMethod
    (vtkCommand::LeftButtonPressEvent,
     vtkWidgetEvent::Select,
     this, vtkContinuousValueWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod
    (vtkCommand::MouseMoveEvent,
     vtkWidgetEvent::Move,
     this, vtkContinuousValueWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod
    (vtkCommand::LeftButtonReleaseEvent,
     vtkWidgetEvent::EndSelect,
     this, vtkContinuousValueWidget::EndSelectAction);
}


//-----------------------------------------------------------------
double vtkContinuousValueWidget::GetValue()
{
  vtkContinuousValueWidgetRepresentation *slider =
    vtkContinuousValueWidgetRepresentation::SafeDownCast(this->WidgetRep);
  return slider->GetValue();
}

//-----------------------------------------------------------------
void vtkContinuousValueWidget::SetValue(double value)
{
  vtkContinuousValueWidgetRepresentation *slider =
    vtkContinuousValueWidgetRepresentation::SafeDownCast(this->WidgetRep);
  slider->SetValue(value);
}


//-------------------------------------------------------------
void vtkContinuousValueWidget::SelectAction(vtkAbstractWidget *w)
{
  vtkContinuousValueWidget *self =
    reinterpret_cast<vtkContinuousValueWidget*>(w);

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
  self->WidgetRep->StartWidgetInteraction(eventPos);
  int interactionState = self->WidgetRep->GetInteractionState();
  if (interactionState != vtkContinuousValueWidgetRepresentation::Adjusting)
    {
    return;
    }

  // We are definitely selected
  self->GrabFocus(self->EventCallbackCommand);
  self->EventCallbackCommand->SetAbortFlag(1);
  if ( interactionState == vtkContinuousValueWidgetRepresentation::Adjusting )
    {
    self->WidgetState = vtkContinuousValueWidget::Adjusting;
    // Highlight as necessary
    self->WidgetRep->Highlight(1);
    // start the interaction
    self->StartInteraction();
    self->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
    self->Render();
    return;
    }
}


//---------------------------------------------------------------
void vtkContinuousValueWidget::MoveAction(vtkAbstractWidget *w)
{
  vtkContinuousValueWidget *self =
    reinterpret_cast<vtkContinuousValueWidget*>(w);

  // do we need to change highlight state?
  int interactionState = self->WidgetRep->ComputeInteractionState
    (self->Interactor->GetEventPosition()[0],
     self->Interactor->GetEventPosition()[1]);

  // if we are outside and in the start state then return
  if (interactionState == vtkContinuousValueWidgetRepresentation::Outside &&
      self->WidgetState == vtkContinuousValueWidget::Start)
    {
    return;
    }

  // if we are not outside and in the highlighting state then return
  if (interactionState != vtkContinuousValueWidgetRepresentation::Outside &&
      self->WidgetState == vtkContinuousValueWidget::Highlighting)
    {
    return;
    }

  // if we are not outside and in the Start state highlight
  if ( interactionState != vtkContinuousValueWidgetRepresentation::Outside &&
       self->WidgetState == vtkContinuousValueWidget::Start)
    {
    self->WidgetRep->Highlight(1);
    self->WidgetState = vtkContinuousValueWidget::Highlighting;
    self->Render();
    return;
    }

  // if we are outside but in the highlight state then stop highlighting
  if ( self->WidgetState == vtkContinuousValueWidget::Highlighting &&
       interactionState == vtkContinuousValueWidgetRepresentation::Outside)
    {
    self->WidgetRep->Highlight(0);
    self->WidgetState = vtkContinuousValueWidget::Start;
    self->Render();
    return;
    }

  // Definitely moving the slider, get the updated position
  double eventPos[2];
  eventPos[0] = self->Interactor->GetEventPosition()[0];
  eventPos[1] = self->Interactor->GetEventPosition()[1];
  self->WidgetRep->WidgetInteraction(eventPos);
  self->InvokeEvent(vtkCommand::InteractionEvent,NULL);
  self->Render();

  // Interact, if desired
  self->EventCallbackCommand->SetAbortFlag(1);
}


//-----------------------------------------------------------------
void vtkContinuousValueWidget::EndSelectAction(vtkAbstractWidget *w)
{
  vtkContinuousValueWidget *self =
    reinterpret_cast<vtkContinuousValueWidget*>(w);

  if ( self->WidgetState != vtkContinuousValueWidget::Adjusting )
    {
    return;
    }

  int interactionState = self->WidgetRep->ComputeInteractionState
    (self->Interactor->GetEventPosition()[0],
     self->Interactor->GetEventPosition()[1]);
  if ( interactionState == vtkContinuousValueWidgetRepresentation::Outside)
    {
    self->WidgetRep->Highlight(0);
    self->WidgetState = vtkContinuousValueWidget::Start;
    }
  else
    {
    self->WidgetState = vtkContinuousValueWidget::Highlighting;
    }

  // The state returns to unselected
  self->ReleaseFocus();

  // Complete interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  self->Render();
}

//-----------------------------------------------------------------
void vtkContinuousValueWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);
}
