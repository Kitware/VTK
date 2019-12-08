/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBoxWidget2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBoxWidget2.h"
#include "vtkBoxRepresentation.h"
#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkEvent.h"
#include "vtkEventData.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"
#include "vtkWidgetEventTranslator.h"

vtkStandardNewMacro(vtkBoxWidget2);

//----------------------------------------------------------------------------
vtkBoxWidget2::vtkBoxWidget2()
{
  this->WidgetState = vtkBoxWidget2::Start;
  this->ManagesCursor = 1;

  this->TranslationEnabled = true;
  this->ScalingEnabled = true;
  this->RotationEnabled = true;
  this->MoveFacesEnabled = true;

  // Define widget events
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent, vtkEvent::NoModifier, 0,
    0, nullptr, vtkWidgetEvent::Select, this, vtkBoxWidget2::SelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent, vtkEvent::NoModifier,
    0, 0, nullptr, vtkWidgetEvent::EndSelect, this, vtkBoxWidget2::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonPressEvent,
    vtkWidgetEvent::Translate, this, vtkBoxWidget2::TranslateAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonReleaseEvent,
    vtkWidgetEvent::EndTranslate, this, vtkBoxWidget2::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent,
    vtkEvent::ControlModifier, 0, 0, nullptr, vtkWidgetEvent::Translate, this,
    vtkBoxWidget2::TranslateAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
    vtkEvent::ControlModifier, 0, 0, nullptr, vtkWidgetEvent::EndTranslate, this,
    vtkBoxWidget2::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent, vtkEvent::ShiftModifier,
    0, 0, nullptr, vtkWidgetEvent::Translate, this, vtkBoxWidget2::TranslateAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
    vtkEvent::ShiftModifier, 0, 0, nullptr, vtkWidgetEvent::EndTranslate, this,
    vtkBoxWidget2::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(
    vtkCommand::RightButtonPressEvent, vtkWidgetEvent::Scale, this, vtkBoxWidget2::ScaleAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonReleaseEvent,
    vtkWidgetEvent::EndScale, this, vtkBoxWidget2::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(
    vtkCommand::MouseMoveEvent, vtkWidgetEvent::Move, this, vtkBoxWidget2::MoveAction);

  {
    vtkNew<vtkEventDataButton3D> ed;
    ed->SetDevice(vtkEventDataDevice::RightController);
    ed->SetInput(vtkEventDataDeviceInput::Trigger);
    ed->SetAction(vtkEventDataAction::Press);
    this->CallbackMapper->SetCallbackMethod(vtkCommand::Button3DEvent, ed.Get(),
      vtkWidgetEvent::Select3D, this, vtkBoxWidget2::SelectAction3D);
  }

  {
    vtkNew<vtkEventDataButton3D> ed;
    ed->SetDevice(vtkEventDataDevice::RightController);
    ed->SetInput(vtkEventDataDeviceInput::Trigger);
    ed->SetAction(vtkEventDataAction::Release);
    this->CallbackMapper->SetCallbackMethod(vtkCommand::Button3DEvent, ed.Get(),
      vtkWidgetEvent::EndSelect3D, this, vtkBoxWidget2::EndSelectAction3D);
  }

  {
    vtkNew<vtkEventDataMove3D> ed;
    ed->SetDevice(vtkEventDataDevice::RightController);
    this->CallbackMapper->SetCallbackMethod(
      vtkCommand::Move3DEvent, ed.Get(), vtkWidgetEvent::Move3D, this, vtkBoxWidget2::MoveAction3D);
  }

  this->KeyEventCallbackCommand = vtkCallbackCommand::New();
  this->KeyEventCallbackCommand->SetClientData(this);
  this->KeyEventCallbackCommand->SetCallback(vtkBoxWidget2::ProcessKeyEvents);
}

//----------------------------------------------------------------------------
vtkBoxWidget2::~vtkBoxWidget2()
{
  this->KeyEventCallbackCommand->Delete();
}

//----------------------------------------------------------------------------
void vtkBoxWidget2::SetEnabled(int enabling)
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
void vtkBoxWidget2::SelectAction(vtkAbstractWidget* w)
{
  // We are in a static method, cast to ourself
  vtkBoxWidget2* self = reinterpret_cast<vtkBoxWidget2*>(w);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!self->CurrentRenderer || !self->CurrentRenderer->IsInViewport(X, Y))
  {
    self->WidgetState = vtkBoxWidget2::Start;
    return;
  }

  // Begin the widget interaction which has the side effect of setting the
  // interaction state.
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(e);
  int interactionState = self->WidgetRep->GetInteractionState();
  if (interactionState == vtkBoxRepresentation::Outside)
  {
    return;
  }

  // Test for states that involve face or handle picking here so
  // selection highlighting doesn't happen if that interaction is disabled.
  // Non-handle-grabbing transformations are tested in the "Action" methods.

  // Rotation
  if (interactionState == vtkBoxRepresentation::Rotating && self->RotationEnabled == 0)
  {
    return;
  }
  // Face Movement
  if ((interactionState == vtkBoxRepresentation::MoveF0 ||
        interactionState == vtkBoxRepresentation::MoveF1 ||
        interactionState == vtkBoxRepresentation::MoveF2 ||
        interactionState == vtkBoxRepresentation::MoveF3 ||
        interactionState == vtkBoxRepresentation::MoveF4 ||
        interactionState == vtkBoxRepresentation::MoveF5) &&
    self->MoveFacesEnabled == 0)
  {
    return;
  }
  // Translation
  if (interactionState == vtkBoxRepresentation::Translating && self->TranslationEnabled == 0)
  {
    return;
  }

  // We are definitely selected
  self->WidgetState = vtkBoxWidget2::Active;
  self->GrabFocus(self->EventCallbackCommand);

  // The SetInteractionState has the side effect of highlighting the widget
  reinterpret_cast<vtkBoxRepresentation*>(self->WidgetRep)->SetInteractionState(interactionState);

  // start the interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//-------------------------------------------------------------------------
void vtkBoxWidget2::SelectAction3D(vtkAbstractWidget* w)
{
  vtkBoxWidget2* self = reinterpret_cast<vtkBoxWidget2*>(w);

  // We want to compute an orthogonal vector to the plane that has been selected
  int interactionState = self->WidgetRep->ComputeComplexInteractionState(
    self->Interactor, self, vtkWidgetEvent::Select3D, self->CallData);

  if (interactionState == vtkBoxRepresentation::Outside)
  {
    return;
  }

  // Test for states that involve face or handle picking here so
  // selection highlighting doesn't happen if that interaction is disabled.
  // Non-handle-grabbing transformations are tested in the "Action" methods.

  // Rotation
  if (interactionState == vtkBoxRepresentation::Rotating && self->RotationEnabled == 0)
  {
    return;
  }
  // Face Movement
  if ((interactionState == vtkBoxRepresentation::MoveF0 ||
        interactionState == vtkBoxRepresentation::MoveF1 ||
        interactionState == vtkBoxRepresentation::MoveF2 ||
        interactionState == vtkBoxRepresentation::MoveF3 ||
        interactionState == vtkBoxRepresentation::MoveF4 ||
        interactionState == vtkBoxRepresentation::MoveF5) &&
    self->MoveFacesEnabled == 0)
  {
    return;
  }
  // Translation
  if (interactionState == vtkBoxRepresentation::Translating && self->TranslationEnabled == 0)
  {
    return;
  }

  // We are definitely selected
  if (!self->Parent)
  {
    self->GrabFocus(self->EventCallbackCommand);
  }

  self->WidgetState = vtkBoxWidget2::Active;
  self->WidgetRep->StartComplexInteraction(
    self->Interactor, self, vtkWidgetEvent::Select3D, self->CallData);

  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
}

//----------------------------------------------------------------------
void vtkBoxWidget2::TranslateAction(vtkAbstractWidget* w)
{
  // We are in a static method, cast to ourself
  vtkBoxWidget2* self = reinterpret_cast<vtkBoxWidget2*>(w);

  if (self->TranslationEnabled == 0)
  {
    return;
  }

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!self->CurrentRenderer || !self->CurrentRenderer->IsInViewport(X, Y))
  {
    self->WidgetState = vtkBoxWidget2::Start;
    return;
  }

  // Begin the widget interaction which has the side effect of setting the
  // interaction state.
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(e);
  int interactionState = self->WidgetRep->GetInteractionState();
  if (interactionState == vtkBoxRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->WidgetState = vtkBoxWidget2::Active;
  self->GrabFocus(self->EventCallbackCommand);
  reinterpret_cast<vtkBoxRepresentation*>(self->WidgetRep)
    ->SetInteractionState(vtkBoxRepresentation::Translating);

  // start the interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------
void vtkBoxWidget2::ScaleAction(vtkAbstractWidget* w)
{
  // We are in a static method, cast to ourself
  vtkBoxWidget2* self = reinterpret_cast<vtkBoxWidget2*>(w);

  if (self->ScalingEnabled == 0)
  {
    return;
  }

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!self->CurrentRenderer || !self->CurrentRenderer->IsInViewport(X, Y))
  {
    self->WidgetState = vtkBoxWidget2::Start;
    return;
  }

  // Begin the widget interaction which has the side effect of setting the
  // interaction state.
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(e);
  int interactionState = self->WidgetRep->GetInteractionState();
  if (interactionState == vtkBoxRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->WidgetState = vtkBoxWidget2::Active;
  self->GrabFocus(self->EventCallbackCommand);
  reinterpret_cast<vtkBoxRepresentation*>(self->WidgetRep)
    ->SetInteractionState(vtkBoxRepresentation::Scaling);

  // start the interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------
void vtkBoxWidget2::MoveAction(vtkAbstractWidget* w)
{
  vtkBoxWidget2* self = reinterpret_cast<vtkBoxWidget2*>(w);

  // See whether we're active
  if (self->WidgetState == vtkBoxWidget2::Start)
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
void vtkBoxWidget2::MoveAction3D(vtkAbstractWidget* w)
{
  vtkBoxWidget2* self = reinterpret_cast<vtkBoxWidget2*>(w);

  // See whether we're active
  if (self->WidgetState == vtkBoxWidget2::Start)
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
void vtkBoxWidget2::EndSelectAction(vtkAbstractWidget* w)
{
  vtkBoxWidget2* self = reinterpret_cast<vtkBoxWidget2*>(w);
  if (self->WidgetState == vtkBoxWidget2::Start)
  {
    return;
  }

  // Return state to not active
  self->WidgetState = vtkBoxWidget2::Start;
  reinterpret_cast<vtkBoxRepresentation*>(self->WidgetRep)
    ->SetInteractionState(vtkBoxRepresentation::Outside);
  self->ReleaseFocus();

  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(vtkCommand::EndInteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------
void vtkBoxWidget2::EndSelectAction3D(vtkAbstractWidget* w)
{
  vtkBoxWidget2* self = reinterpret_cast<vtkBoxWidget2*>(w);

  if (self->WidgetState != vtkBoxWidget2::Active ||
    self->WidgetRep->GetInteractionState() == vtkBoxRepresentation::Outside)
  {
    return;
  }

  // Return state to not selected
  self->WidgetRep->EndComplexInteraction(
    self->Interactor, self, vtkWidgetEvent::Select3D, self->CallData);

  self->WidgetState = vtkBoxWidget2::Start;
  if (!self->Parent)
  {
    self->ReleaseFocus();
  }

  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(vtkCommand::EndInteractionEvent, nullptr);
}

//----------------------------------------------------------------------
void vtkBoxWidget2::StepAction3D(vtkAbstractWidget* w)
{
  vtkBoxWidget2* self = reinterpret_cast<vtkBoxWidget2*>(w);

  // We want to compute an orthogonal vector to the plane that has been selected
  int interactionState = self->WidgetRep->ComputeComplexInteractionState(
    self->Interactor, self, vtkWidgetEvent::Select3D, self->CallData);

  if (interactionState == vtkBoxRepresentation::Outside)
  {
    return;
  }

  // self->WidgetRep->SetInteractionState(vtkBoxRepresentation::Outside);

  // Okay, adjust the representation
  self->WidgetRep->ComplexInteraction(
    self->Interactor, self, vtkWidgetEvent::Move3D, self->CallData);

  // moving something
  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
}

//----------------------------------------------------------------------
void vtkBoxWidget2::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = vtkBoxRepresentation::New();
  }
}

//----------------------------------------------------------------------------
void vtkBoxWidget2::ProcessKeyEvents(vtkObject*, unsigned long event, void* clientdata, void*)
{
  vtkBoxWidget2* self = static_cast<vtkBoxWidget2*>(clientdata);
  vtkRenderWindowInteractor* iren = self->GetInteractor();
  vtkBoxRepresentation* rep = vtkBoxRepresentation::SafeDownCast(self->WidgetRep);
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
void vtkBoxWidget2::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Translation Enabled: " << (this->TranslationEnabled ? "On\n" : "Off\n");
  os << indent << "Scaling Enabled: " << (this->ScalingEnabled ? "On\n" : "Off\n");
  os << indent << "Rotation Enabled: " << (this->RotationEnabled ? "On\n" : "Off\n");
  os << indent << "Move Faces Enabled: " << (this->MoveFacesEnabled ? "On\n" : "Off\n");
}
