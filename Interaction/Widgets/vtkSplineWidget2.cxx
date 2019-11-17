/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSplineWidget2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSplineWidget2.h"

#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkEvent.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSplineRepresentation.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"
#include "vtkWidgetEventTranslator.h"

vtkStandardNewMacro(vtkSplineWidget2);
//----------------------------------------------------------------------------
vtkSplineWidget2::vtkSplineWidget2()
{
  this->WidgetState = vtkSplineWidget2::Start;
  this->ManagesCursor = 1;

  // Define widget events
  this->CallbackMapper->SetCallbackMethod(
    vtkCommand::LeftButtonPressEvent, vtkWidgetEvent::Select, this, vtkSplineWidget2::SelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
    vtkWidgetEvent::EndSelect, this, vtkSplineWidget2::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonPressEvent,
    vtkWidgetEvent::Translate, this, vtkSplineWidget2::TranslateAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonReleaseEvent,
    vtkWidgetEvent::EndTranslate, this, vtkSplineWidget2::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(
    vtkCommand::RightButtonPressEvent, vtkWidgetEvent::Scale, this, vtkSplineWidget2::ScaleAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonReleaseEvent,
    vtkWidgetEvent::EndScale, this, vtkSplineWidget2::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(
    vtkCommand::MouseMoveEvent, vtkWidgetEvent::Move, this, vtkSplineWidget2::MoveAction);
  this->CallbackMapper->SetCallbackMethod(
    vtkCommand::MouseMoveEvent, vtkWidgetEvent::Move, this, vtkSplineWidget2::MoveAction);

  this->KeyEventCallbackCommand = vtkCallbackCommand::New();
  this->KeyEventCallbackCommand->SetClientData(this);
  this->KeyEventCallbackCommand->SetCallback(vtkSplineWidget2::ProcessKeyEvents);
}

//----------------------------------------------------------------------------
vtkSplineWidget2::~vtkSplineWidget2()
{
  this->KeyEventCallbackCommand->Delete();
}

//----------------------------------------------------------------------------
void vtkSplineWidget2::SetEnabled(int enabling)
{
  int enabled = this->Enabled;

  // We do this step first because it sets the CurrentRenderer
  this->Superclass::SetEnabled(enabling);

  // We defer enabling the handles until the selection process begins
  if (enabling && !enabled)
  {
    if (this->Parent)
    {
      this->Parent->AddObserver(
        vtkCommand::KeyPressEvent, this->KeyEventCallbackCommand, this->Priority);
      this->Parent->AddObserver(
        vtkCommand::KeyReleaseEvent, this->KeyEventCallbackCommand, this->Priority);
    }
    else
    {
      this->Interactor->AddObserver(
        vtkCommand::KeyPressEvent, this->KeyEventCallbackCommand, this->Priority);
      this->Interactor->AddObserver(
        vtkCommand::KeyReleaseEvent, this->KeyEventCallbackCommand, this->Priority);
    }
  }
  else if (!enabling && enabled)
  {
    if (this->Parent)
    {
      this->Parent->RemoveObserver(this->KeyEventCallbackCommand);
    }
    else
    {
      this->Interactor->RemoveObserver(this->KeyEventCallbackCommand);
    }
  }
}

//----------------------------------------------------------------------
void vtkSplineWidget2::SelectAction(vtkAbstractWidget* w)
{
  // We are in a static method, cast to ourself
  vtkSplineWidget2* self = vtkSplineWidget2::SafeDownCast(w);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!self->CurrentRenderer || !self->CurrentRenderer->IsInViewport(X, Y))
  {
    self->WidgetState = vtkSplineWidget2::Start;
    return;
  }

  // Begin the widget interaction which has the side effect of setting the
  // interaction state.
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(e);
  int interactionState = self->WidgetRep->GetInteractionState();
  if (interactionState == vtkSplineRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->WidgetState = vtkSplineWidget2::Active;
  self->GrabFocus(self->EventCallbackCommand);

  if (interactionState == vtkSplineRepresentation::OnLine && self->Interactor->GetControlKey())
  {
    // Add point.
    reinterpret_cast<vtkSplineRepresentation*>(self->WidgetRep)
      ->SetInteractionState(vtkSplineRepresentation::Inserting);
  }
  else if (interactionState == vtkSplineRepresentation::OnHandle && self->Interactor->GetShiftKey())
  {
    // remove point.
    reinterpret_cast<vtkSplineRepresentation*>(self->WidgetRep)
      ->SetInteractionState(vtkSplineRepresentation::Erasing);
  }
  else
  {
    reinterpret_cast<vtkSplineRepresentation*>(self->WidgetRep)
      ->SetInteractionState(vtkSplineRepresentation::Moving);
  }

  // start the interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------
void vtkSplineWidget2::TranslateAction(vtkAbstractWidget* w)
{
  // Not sure this should be any different that SelectAction
  vtkSplineWidget2::SelectAction(w);
}

//----------------------------------------------------------------------
void vtkSplineWidget2::ScaleAction(vtkAbstractWidget* w)
{
  // We are in a static method, cast to ourself
  vtkSplineWidget2* self = reinterpret_cast<vtkSplineWidget2*>(w);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!self->CurrentRenderer || !self->CurrentRenderer->IsInViewport(X, Y))
  {
    self->WidgetState = vtkSplineWidget2::Start;
    return;
  }

  // Begin the widget interaction which has the side effect of setting the
  // interaction state.
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(e);
  int interactionState = self->WidgetRep->GetInteractionState();
  if (interactionState == vtkSplineRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->WidgetState = vtkSplineWidget2::Active;
  self->GrabFocus(self->EventCallbackCommand);
  // Scale
  reinterpret_cast<vtkSplineRepresentation*>(self->WidgetRep)
    ->SetInteractionState(vtkSplineRepresentation::Scaling);

  // start the interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------
void vtkSplineWidget2::MoveAction(vtkAbstractWidget* w)
{
  vtkSplineWidget2* self = reinterpret_cast<vtkSplineWidget2*>(w);

  // See whether we're active
  if (self->WidgetState == vtkSplineWidget2::Start)
  {
    return;
  }

  // compute some info we need for all cases
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

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
void vtkSplineWidget2::EndSelectAction(vtkAbstractWidget* w)
{
  vtkSplineWidget2* self = reinterpret_cast<vtkSplineWidget2*>(w);
  if (self->WidgetState == vtkSplineWidget2::Start)
  {
    return;
  }

  // compute some info we need for all cases
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Okay, adjust the representation
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);

  self->WidgetRep->EndWidgetInteraction(e);

  // Return state to not active
  self->WidgetState = vtkSplineWidget2::Start;
  reinterpret_cast<vtkSplineRepresentation*>(self->WidgetRep)
    ->SetInteractionState(vtkSplineRepresentation::Outside);
  self->ReleaseFocus();

  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(vtkCommand::EndInteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------
void vtkSplineWidget2::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = vtkSplineRepresentation::New();
  }
}

//----------------------------------------------------------------------------
void vtkSplineWidget2::ProcessKeyEvents(vtkObject*, unsigned long event, void* clientdata, void*)
{
  vtkSplineWidget2* self = static_cast<vtkSplineWidget2*>(clientdata);
  vtkRenderWindowInteractor* iren = self->GetInteractor();
  vtkSplineRepresentation* rep = vtkSplineRepresentation::SafeDownCast(self->WidgetRep);
  switch (event)
  {
    case vtkCommand::KeyPressEvent:
      switch (iren->GetKeyCode())
      {
        case 'x':
        case 'X':
          rep->SetXTranslationAxisOn();
          break;
        case 'y':
        case 'Y':
          rep->SetYTranslationAxisOn();
          break;
        case 'z':
        case 'Z':
          rep->SetZTranslationAxisOn();
          break;
        default:
          break;
      }
      break;
    case vtkCommand::KeyReleaseEvent:
      switch (iren->GetKeyCode())
      {
        case 'x':
        case 'X':
        case 'y':
        case 'Y':
        case 'z':
        case 'Z':
          rep->SetTranslationAxisOff();
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

//----------------------------------------------------------------------------
void vtkSplineWidget2::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
