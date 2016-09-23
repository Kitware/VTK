/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSliderWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSliderWidget.h"
#include "vtkSliderRepresentation3D.h"
#include "vtkCommand.h"
#include "vtkCallbackCommand.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkWidgetEventTranslator.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkEvent.h"
#include "vtkWidgetEvent.h"

vtkStandardNewMacro(vtkSliderWidget);

//----------------------------------------------------------------------------------
vtkSliderWidget::vtkSliderWidget()
{
  // Set the initial state
  this->WidgetState = vtkSliderWidget::Start;

  // Assign initial values
  this->AnimationMode = vtkSliderWidget::Jump;
  this->NumberOfAnimationSteps = 24;

  // Okay, define the events
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent,
                                          vtkWidgetEvent::Select,
                                          this, vtkSliderWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MouseMoveEvent,
                                          vtkWidgetEvent::Move,
                                          this, vtkSliderWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
                                          vtkWidgetEvent::EndSelect,
                                          this, vtkSliderWidget::EndSelectAction);
}

//----------------------------------------------------------------------
void vtkSliderWidget::CreateDefaultRepresentation()
{
  if ( ! this->WidgetRep )
  {
    this->WidgetRep = vtkSliderRepresentation3D::New();
  }
}


//----------------------------------------------------------------------------------
void vtkSliderWidget::SelectAction(vtkAbstractWidget *w)
{
  vtkSliderWidget *self = reinterpret_cast<vtkSliderWidget*>(w);

  double eventPos[2];
  eventPos[0] = self->Interactor->GetEventPosition()[0];
  eventPos[1] = self->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!self->CurrentRenderer ||
      !self->CurrentRenderer->IsInViewport(static_cast<int>(eventPos[0]), static_cast<int>(eventPos[1])))
  {
    self->WidgetState = vtkSliderWidget::Start;
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
  self->GrabFocus(self->EventCallbackCommand);
  if ( interactionState == vtkSliderRepresentation::Slider )
  {
    self->WidgetState = vtkSliderWidget::Sliding;
  }
  else
  {
    self->WidgetState = vtkSliderWidget::Animating;
  }

  // Highlight as necessary
  self->WidgetRep->Highlight(1);

  // start the interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  self->Render();
}


//----------------------------------------------------------------------------------
void vtkSliderWidget::MoveAction(vtkAbstractWidget *w)
{
  vtkSliderWidget *self = reinterpret_cast<vtkSliderWidget*>(w);

  // See whether we're active
  if ( self->WidgetState == vtkSliderWidget::Start ||
       self->WidgetState == vtkSliderWidget::Animating )
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
  self->InvokeEvent(vtkCommand::InteractionEvent,NULL);
  self->Render();
}


//----------------------------------------------------------------------------------
void vtkSliderWidget::EndSelectAction(vtkAbstractWidget *w)
{
  vtkSliderWidget *self = reinterpret_cast<vtkSliderWidget*>(w);

  if ( self->WidgetState == vtkSliderWidget::Start )
  {
    return;
  }

  // If animating we do it and return
  if ( self->WidgetState == vtkSliderWidget::Animating )
  {
    self->AnimateSlider(self->WidgetRep->GetInteractionState());
  }

  // Highlight if necessary
  self->WidgetRep->Highlight(0);

  // The state returns to unselected
  self->WidgetState = vtkSliderWidget::Start;
  self->ReleaseFocus();

  // Complete interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  self->Render();
}


//----------------------------------------------------------------------------------
void vtkSliderWidget::AnimateSlider(int selectionState)
{
  // Get the representation and grab some information
  vtkSliderRepresentation *sliderRep =
    reinterpret_cast<vtkSliderRepresentation*>(this->WidgetRep);

  // If the slider bead has been selected, then nothing happens
  if ( selectionState == vtkSliderRepresentation::Outside ||
       selectionState == vtkSliderRepresentation::Slider )
  {
    return;
  }

  // Depending on animation mode, we'll jump to the pick point or
  // animate towards it.
  double minValue=sliderRep->GetMinimumValue();
  double maxValue=sliderRep->GetMaximumValue();
  double pickedT=sliderRep->GetPickedT();

  if ( this->AnimationMode == vtkSliderWidget::Jump )
  {
    if ( selectionState == vtkSliderRepresentation::Tube )
    {
      sliderRep->SetValue(minValue + pickedT*(maxValue-minValue));
    }

    else if ( selectionState == vtkSliderRepresentation::LeftCap )
    {
      sliderRep->SetValue(minValue);
    }

    else if ( selectionState == vtkSliderRepresentation::RightCap )
    {
      sliderRep->SetValue(maxValue);
    }
    sliderRep->BuildRepresentation();
    this->InvokeEvent(vtkCommand::InteractionEvent,NULL);
  }

  else if ( this->AnimationMode == vtkSliderWidget::Animate )
  {
    double targetValue=minValue, originalValue=sliderRep->GetValue();
    if ( selectionState == vtkSliderRepresentation::Tube )
    {
      targetValue = minValue + pickedT*(maxValue-minValue);
    }

    else if ( selectionState == vtkSliderRepresentation::LeftCap )
    {
      targetValue = minValue;
    }

    else if ( selectionState == vtkSliderRepresentation::RightCap )
    {
      targetValue = maxValue;
    }

    // Okay, animate the slider
    double value;
    for (int i=0; i < this->NumberOfAnimationSteps; i++)
    {
      value = originalValue +
        (static_cast<double>(i+1)/this->NumberOfAnimationSteps)*(targetValue-originalValue);
      sliderRep->SetValue(value);
      sliderRep->BuildRepresentation();
      this->InvokeEvent(vtkCommand::InteractionEvent,NULL);
      this->Render();
    }
  }

  this->WidgetState = vtkSliderWidget::Start;
}


//----------------------------------------------------------------------------------
void vtkSliderWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Animation Mode: ";
  switch ( this->AnimationMode )
  {
    case vtkSliderWidget::Jump:
      os << "Jump\n";
      break;
    case vtkSliderWidget::Animate:
      os << "Animate\n";
      break;
    default:
      os << "AnimateOff\n";
  }

  os << indent << "Number of Animation Steps: "
     << this->NumberOfAnimationSteps << "\n";
}
