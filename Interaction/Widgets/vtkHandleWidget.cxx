/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHandleWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHandleWidget.h"
#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkEvent.h"
#include "vtkEventData.h"
#include "vtkObjectFactory.h"
#include "vtkPointHandleRepresentation3D.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"
#include "vtkWidgetEventTranslator.h"

vtkStandardNewMacro(vtkHandleWidget);

//----------------------------------------------------------------------------------
vtkHandleWidget::vtkHandleWidget()
{
  // Set the initial state
  this->WidgetState = vtkHandleWidget::Inactive;

  // Okay, define the events for this widget
  this->CallbackMapper->SetCallbackMethod(
    vtkCommand::LeftButtonPressEvent, vtkWidgetEvent::Select, this, vtkHandleWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
    vtkWidgetEvent::EndSelect, this, vtkHandleWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonPressEvent,
    vtkWidgetEvent::Translate, this, vtkHandleWidget::TranslateAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonReleaseEvent,
    vtkWidgetEvent::EndTranslate, this, vtkHandleWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(
    vtkCommand::RightButtonPressEvent, vtkWidgetEvent::Scale, this, vtkHandleWidget::ScaleAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonReleaseEvent,
    vtkWidgetEvent::EndScale, this, vtkHandleWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(
    vtkCommand::MouseMoveEvent, vtkWidgetEvent::Move, this, vtkHandleWidget::MoveAction);

  {
    vtkNew<vtkEventDataButton3D> ed;
    ed->SetDevice(vtkEventDataDevice::RightController);
    ed->SetInput(vtkEventDataDeviceInput::Trigger);
    ed->SetAction(vtkEventDataAction::Press);
    this->CallbackMapper->SetCallbackMethod(vtkCommand::Button3DEvent, ed, vtkWidgetEvent::Select3D,
      this, vtkHandleWidget::SelectAction3D);
  }

  {
    vtkNew<vtkEventDataButton3D> ed;
    ed->SetDevice(vtkEventDataDevice::RightController);
    ed->SetInput(vtkEventDataDeviceInput::Trigger);
    ed->SetAction(vtkEventDataAction::Release);
    this->CallbackMapper->SetCallbackMethod(vtkCommand::Button3DEvent, ed,
      vtkWidgetEvent::EndSelect3D, this, vtkHandleWidget::EndSelectAction);
  }

  {
    vtkNew<vtkEventDataMove3D> ed;
    ed->SetDevice(vtkEventDataDevice::RightController);
    this->CallbackMapper->SetCallbackMethod(
      vtkCommand::Move3DEvent, ed, vtkWidgetEvent::Move3D, this, vtkHandleWidget::MoveAction3D);
  }

  this->ShowInactive = false;
  this->EnableAxisConstraint = 1;
  this->EnableTranslation = 1;
  this->AllowHandleResize = 1;

  this->KeyEventCallbackCommand = vtkCallbackCommand::New();
  this->KeyEventCallbackCommand->SetClientData(this);
  this->KeyEventCallbackCommand->SetCallback(vtkHandleWidget::ProcessKeyEvents);
}

//----------------------------------------------------------------------------------
vtkHandleWidget::~vtkHandleWidget()
{
  this->KeyEventCallbackCommand->Delete();
}

//----------------------------------------------------------------------
void vtkHandleWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = vtkPointHandleRepresentation3D::New();
  }
}

//-------------------------------------------------------------------------
void vtkHandleWidget::SetCursor(int cState)
{
  if (this->ManagesCursor)
  {
    switch (cState)
    {
      case vtkHandleRepresentation::Outside:
        this->RequestCursorShape(VTK_CURSOR_DEFAULT);
        break;
      default:
        this->RequestCursorShape(VTK_CURSOR_HAND);
    }
  }
}

//-------------------------------------------------------------------------
void vtkHandleWidget::SelectAction(vtkAbstractWidget* w)
{
  vtkHandleWidget* self = reinterpret_cast<vtkHandleWidget*>(w);

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  self->WidgetRep->ComputeInteractionState(X, Y);
  if (self->WidgetRep->GetInteractionState() == vtkHandleRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  if (!self->Parent)
  {
    self->GrabFocus(self->EventCallbackCommand);
  }
  double eventPos[2];
  eventPos[0] = static_cast<double>(X);
  eventPos[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(eventPos);

  self->WidgetState = vtkHandleWidget::Active;
  reinterpret_cast<vtkHandleRepresentation*>(self->WidgetRep)
    ->SetInteractionState(vtkHandleRepresentation::Selecting);

  self->GenericAction(self);
}

//-------------------------------------------------------------------------
void vtkHandleWidget::SelectAction3D(vtkAbstractWidget* w)
{
  vtkHandleWidget* self = reinterpret_cast<vtkHandleWidget*>(w);

  self->WidgetRep->ComputeComplexInteractionState(
    self->Interactor, self, vtkWidgetEvent::Select3D, self->CallData);

  if (self->WidgetRep->GetInteractionState() == vtkHandleRepresentation::Outside)
  {
    return;
  }

  self->EventCallbackCommand->SetAbortFlag(1);
  self->WidgetRep->StartComplexInteraction(
    self->Interactor, self, vtkWidgetEvent::Select3D, self->CallData);

  self->WidgetState = vtkHandleWidget::Active;
  reinterpret_cast<vtkHandleRepresentation*>(self->WidgetRep)
    ->SetInteractionState(vtkHandleRepresentation::Selecting);

  self->GenericAction(self);
}

//-------------------------------------------------------------------------
void vtkHandleWidget::TranslateAction(vtkAbstractWidget* w)
{
  vtkHandleWidget* self = reinterpret_cast<vtkHandleWidget*>(w);

  double eventPos[2];
  eventPos[0] = static_cast<double>(self->Interactor->GetEventPosition()[0]);
  eventPos[1] = static_cast<double>(self->Interactor->GetEventPosition()[1]);
  self->WidgetRep->StartWidgetInteraction(eventPos);

  if (self->WidgetRep->GetInteractionState() == vtkHandleRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->WidgetState = vtkHandleWidget::Active;
  reinterpret_cast<vtkHandleRepresentation*>(self->WidgetRep)
    ->SetInteractionState(vtkHandleRepresentation::Translating);

  self->GenericAction(self);
}

//-------------------------------------------------------------------------
void vtkHandleWidget::ScaleAction(vtkAbstractWidget* w)
{
  vtkHandleWidget* self = reinterpret_cast<vtkHandleWidget*>(w);

  if (self->AllowHandleResize)
  {

    double eventPos[2];
    eventPos[0] = static_cast<double>(self->Interactor->GetEventPosition()[0]);
    eventPos[1] = static_cast<double>(self->Interactor->GetEventPosition()[1]);

    self->WidgetRep->StartWidgetInteraction(eventPos);
    if (self->WidgetRep->GetInteractionState() == vtkHandleRepresentation::Outside)
    {
      return;
    }

    // We are definitely selected
    self->WidgetState = vtkHandleWidget::Active;
    reinterpret_cast<vtkHandleRepresentation*>(self->WidgetRep)
      ->SetInteractionState(vtkHandleRepresentation::Scaling);

    self->GenericAction(self);
  }
}

//-------------------------------------------------------------------------
void vtkHandleWidget::GenericAction(vtkHandleWidget* self)
{
  // This is redundant but necessary on some systems (windows) because the
  // cursor is switched during OS event processing and reverts to the default
  // cursor.
  self->SetCursor(self->WidgetRep->GetInteractionState());

  // Check to see whether motion is constrained
  if (self->Interactor->GetShiftKey() && self->EnableAxisConstraint)
  {
    reinterpret_cast<vtkHandleRepresentation*>(self->WidgetRep)->ConstrainedOn();
  }
  else
  {
    reinterpret_cast<vtkHandleRepresentation*>(self->WidgetRep)->ConstrainedOff();
  }

  // Highlight as necessary
  self->WidgetRep->Highlight(1);

  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//-------------------------------------------------------------------------
void vtkHandleWidget::EndSelectAction(vtkAbstractWidget* w)
{
  vtkHandleWidget* self = reinterpret_cast<vtkHandleWidget*>(w);

  if (self->WidgetState != vtkHandleWidget::Active)
  {
    return;
  }

  // Return state to not selected
  self->WidgetState = vtkHandleWidget::Start;

  // Highlight as necessary
  self->WidgetRep->Highlight(0);

  // stop adjusting
  if (!self->Parent)
  {
    self->ReleaseFocus();
  }
  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(vtkCommand::EndInteractionEvent, nullptr);
  self->WidgetState = vtkHandleWidget::Start;
  self->Render();
}

//-------------------------------------------------------------------------
void vtkHandleWidget::MoveAction(vtkAbstractWidget* w)
{
  vtkHandleWidget* self = reinterpret_cast<vtkHandleWidget*>(w);

  // compute some info we need for all cases
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Set the cursor appropriately
  if (self->WidgetState == vtkHandleWidget::Start)
  {
    int state = self->WidgetRep->GetInteractionState();
    self->WidgetRep->ComputeInteractionState(X, Y);
    self->SetCursor(self->WidgetRep->GetInteractionState());
    // Must rerender if we change appearance
    if (reinterpret_cast<vtkHandleRepresentation*>(self->WidgetRep)->GetActiveRepresentation() &&
      state != self->WidgetRep->GetInteractionState())
    {
      self->Render();
    }
    return;
  }

  if (!self->EnableTranslation)
  {
    return;
  }

  // Okay, adjust the representation
  double eventPosition[2];
  eventPosition[0] = static_cast<double>(X);
  eventPosition[1] = static_cast<double>(Y);
  self->WidgetRep->WidgetInteraction(eventPosition);

  // Got this event, we are finished
  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
  self->Render();
}

//-------------------------------------------------------------------------
void vtkHandleWidget::MoveAction3D(vtkAbstractWidget* w)
{
  vtkHandleWidget* self = reinterpret_cast<vtkHandleWidget*>(w);

  // Set the cursor appropriately
  if (self->WidgetState == vtkHandleWidget::Start)
  {
    int state = self->WidgetRep->GetInteractionState();
    self->WidgetRep->ComputeComplexInteractionState(
      self->Interactor, self, vtkWidgetEvent::Move3D, self->CallData);

    self->SetCursor(self->WidgetRep->GetInteractionState());

    // Must rerender if we change appearance
    if (reinterpret_cast<vtkHandleRepresentation*>(self->WidgetRep)->GetActiveRepresentation() &&
      state != self->WidgetRep->GetInteractionState())
    {
      self->Render();
    }
    return;
  }

  // Okay, adjust the representation
  self->WidgetRep->ComplexInteraction(
    self->Interactor, self, vtkWidgetEvent::Move3D, self->CallData);

  // Got this event, we are finished
  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------------------
void vtkHandleWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  // Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Allow Handle Resize: " << (this->AllowHandleResize ? "On\n" : "Off\n");

  os << indent << "Enable Axis Constraint: " << (this->EnableAxisConstraint ? "On\n" : "Off\n");

  os << indent << "Show Inactive: " << (this->ShowInactive ? "On\n" : "Off\n");
  os << indent << "WidgetState: " << this->WidgetState << endl;
}

//-------------------------------------------------------------------------
void vtkHandleWidget::SetEnabled(int enabling)
{
  int enabled = this->Enabled;
  if (this->Enabled == enabling)
  {
    return;
  }
  if (!this->ShowInactive)
  {
    // Forward to superclass
    this->Superclass::SetEnabled(enabling);
    if (enabling)
    {
      this->WidgetState = vtkHandleWidget::Start;
    }
    else
    {
      this->WidgetState = vtkHandleWidget::Inactive;
    }
  }
  else
  {
    if (enabling) //----------------
    {
      this->Superclass::SetEnabled(enabling);
      this->WidgetState = vtkHandleWidget::Start;
    }

    else // disabling------------------
    {
      vtkDebugMacro(<< "Disabling widget");

      this->Enabled = 0;

      // don't listen for events any more
      if (!this->Parent)
      {
        this->Interactor->RemoveObserver(this->EventCallbackCommand);
      }
      else
      {
        this->Parent->RemoveObserver(this->EventCallbackCommand);
      }

      this->WidgetState = vtkHandleWidget::Inactive;
      this->InvokeEvent(vtkCommand::DisableEvent, nullptr);
    }
  }

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

//----------------------------------------------------------------------------
void vtkHandleWidget::ProcessKeyEvents(vtkObject*, unsigned long event, void* clientdata, void*)
{
  vtkHandleWidget* self = static_cast<vtkHandleWidget*>(clientdata);
  vtkRenderWindowInteractor* iren = self->GetInteractor();
  vtkHandleRepresentation* rep = vtkHandleRepresentation::SafeDownCast(self->WidgetRep);
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
