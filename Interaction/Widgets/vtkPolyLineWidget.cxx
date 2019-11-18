/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyLineWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPolyLineWidget.h"

#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkEvent.h"
#include "vtkObjectFactory.h"
#include "vtkPolyLineRepresentation.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"
#include "vtkWidgetEventTranslator.h"

vtkStandardNewMacro(vtkPolyLineWidget);
//----------------------------------------------------------------------------
vtkPolyLineWidget::vtkPolyLineWidget()
{
  this->WidgetState = vtkPolyLineWidget::Start;
  this->ManagesCursor = 1;

  // Define widget events
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent, vtkWidgetEvent::Select,
    this, vtkPolyLineWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
    vtkWidgetEvent::EndSelect, this, vtkPolyLineWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonPressEvent,
    vtkWidgetEvent::Translate, this, vtkPolyLineWidget::TranslateAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonReleaseEvent,
    vtkWidgetEvent::EndTranslate, this, vtkPolyLineWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(
    vtkCommand::RightButtonPressEvent, vtkWidgetEvent::Scale, this, vtkPolyLineWidget::ScaleAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonReleaseEvent,
    vtkWidgetEvent::EndScale, this, vtkPolyLineWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(
    vtkCommand::MouseMoveEvent, vtkWidgetEvent::Move, this, vtkPolyLineWidget::MoveAction);

  this->KeyEventCallbackCommand = vtkCallbackCommand::New();
  this->KeyEventCallbackCommand->SetClientData(this);
  this->KeyEventCallbackCommand->SetCallback(vtkPolyLineWidget::ProcessKeyEvents);
}

//----------------------------------------------------------------------------
vtkPolyLineWidget::~vtkPolyLineWidget()
{
  this->KeyEventCallbackCommand->Delete();
}

//----------------------------------------------------------------------------
void vtkPolyLineWidget::SetEnabled(int enabling)
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
void vtkPolyLineWidget::SelectAction(vtkAbstractWidget* w)
{
  // We are in a static method, cast to ourself
  vtkPolyLineWidget* self = vtkPolyLineWidget::SafeDownCast(w);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!self->CurrentRenderer || !self->CurrentRenderer->IsInViewport(X, Y))
  {
    self->WidgetState = vtkPolyLineWidget::Start;
    return;
  }

  // Begin the widget interaction which has the side effect of setting the
  // interaction state.
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(e);
  int interactionState = self->WidgetRep->GetInteractionState();
  if (interactionState == vtkPolyLineRepresentation::Outside && !self->Interactor->GetAltKey())
  {
    return;
  }
  // We are definitely selected
  self->WidgetState = vtkPolyLineWidget::Active;
  self->GrabFocus(self->EventCallbackCommand);

  if (self->Interactor->GetAltKey())
  {
    // push point.
    reinterpret_cast<vtkPolyLineRepresentation*>(self->WidgetRep)
      ->SetInteractionState(vtkPolyLineRepresentation::Pushing);
  }
  else if (interactionState == vtkPolyLineRepresentation::OnLine &&
    self->Interactor->GetControlKey())
  {
    // insert point.
    reinterpret_cast<vtkPolyLineRepresentation*>(self->WidgetRep)
      ->SetInteractionState(vtkPolyLineRepresentation::Inserting);
  }
  else if (interactionState == vtkPolyLineRepresentation::OnHandle &&
    self->Interactor->GetShiftKey())
  {
    // remove point.
    reinterpret_cast<vtkPolyLineRepresentation*>(self->WidgetRep)
      ->SetInteractionState(vtkPolyLineRepresentation::Erasing);
  }
  else
  {
    reinterpret_cast<vtkPolyLineRepresentation*>(self->WidgetRep)
      ->SetInteractionState(vtkPolyLineRepresentation::Moving);
  }

  // start the interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------
void vtkPolyLineWidget::TranslateAction(vtkAbstractWidget* w)
{
  // Not sure this should be any different than SelectAction
  vtkPolyLineWidget::SelectAction(w);
}

//----------------------------------------------------------------------
void vtkPolyLineWidget::ScaleAction(vtkAbstractWidget* w)
{
  // We are in a static method, cast to ourself
  vtkPolyLineWidget* self = reinterpret_cast<vtkPolyLineWidget*>(w);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!self->CurrentRenderer || !self->CurrentRenderer->IsInViewport(X, Y))
  {
    self->WidgetState = vtkPolyLineWidget::Start;
    return;
  }

  // Begin the widget interaction which has the side effect of setting the
  // interaction state.
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(e);
  int interactionState = self->WidgetRep->GetInteractionState();
  if (interactionState == vtkPolyLineRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->WidgetState = vtkPolyLineWidget::Active;
  self->GrabFocus(self->EventCallbackCommand);
  // Scale
  reinterpret_cast<vtkPolyLineRepresentation*>(self->WidgetRep)
    ->SetInteractionState(vtkPolyLineRepresentation::Scaling);

  // start the interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------
void vtkPolyLineWidget::MoveAction(vtkAbstractWidget* w)
{
  vtkPolyLineWidget* self = reinterpret_cast<vtkPolyLineWidget*>(w);

  // See whether we're active
  if (self->WidgetState == vtkPolyLineWidget::Start)
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
void vtkPolyLineWidget::EndSelectAction(vtkAbstractWidget* w)
{
  vtkPolyLineWidget* self = reinterpret_cast<vtkPolyLineWidget*>(w);
  if (self->WidgetState == vtkPolyLineWidget::Start)
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

  // EndWidgetInteraction for this widget can modify/add/remove points
  // Make sure the representation is updated
  self->InvokeEvent(vtkCommand::InteractionEvent, nullptr);

  // Return state to not active
  self->WidgetState = vtkPolyLineWidget::Start;
  reinterpret_cast<vtkPolyLineRepresentation*>(self->WidgetRep)
    ->SetInteractionState(vtkPolyLineRepresentation::Outside);
  self->ReleaseFocus();

  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(vtkCommand::EndInteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------------
void vtkPolyLineWidget::ProcessKeyEvents(vtkObject*, unsigned long event, void* clientdata, void*)
{
  vtkPolyLineWidget* self = static_cast<vtkPolyLineWidget*>(clientdata);
  vtkRenderWindowInteractor* iren = self->GetInteractor();
  vtkPolyLineRepresentation* rep = vtkPolyLineRepresentation::SafeDownCast(self->WidgetRep);
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

//----------------------------------------------------------------------
void vtkPolyLineWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = vtkPolyLineRepresentation::New();
  }
}

//----------------------------------------------------------------------------
void vtkPolyLineWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
