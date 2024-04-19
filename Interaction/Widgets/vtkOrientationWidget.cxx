// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOrientationWidget.h"

#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkEvent.h"
#include "vtkEventData.h"
#include "vtkObjectFactory.h"
#include "vtkOrientationRepresentation.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"
#include "vtkWidgetEventTranslator.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOrientationWidget);

//------------------------------------------------------------------------------
vtkOrientationWidget::vtkOrientationWidget()
{
  this->ManagesCursor = 1;

  // Setup a default representation in case we don't set one
  this->CreateDefaultRepresentation();
  double bounds[6] = { -0.5, 0.5, -0.5, 0.5, -0.5, 0.5 };
  this->WidgetRep->PlaceWidget(bounds);

  // Define widget events
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent, vtkWidgetEvent::Select,
    this, vtkOrientationWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
    vtkWidgetEvent::EndSelect, this, vtkOrientationWidget::EndSelectAction);

  this->CallbackMapper->SetCallbackMethod(
    vtkCommand::MouseMoveEvent, vtkWidgetEvent::Move, this, vtkOrientationWidget::MoveAction);
}

//------------------------------------------------------------------------------
vtkOrientationWidget::~vtkOrientationWidget() = default;

//------------------------------------------------------------------------------
void vtkOrientationWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = vtkOrientationRepresentation::New();
  }
}

//------------------------------------------------------------------------------
void vtkOrientationWidget::SetRepresentation(vtkOrientationRepresentation* r)
{
  this->Superclass::SetWidgetRepresentation(r);
}

//------------------------------------------------------------------------------
void vtkOrientationWidget::SelectAction(vtkAbstractWidget* w)
{
  // We are in a static method, cast to ourself
  vtkOrientationWidget* self = vtkOrientationWidget::SafeDownCast(w);
  int interactionState = self->WidgetRep->GetInteractionState();
  if (interactionState == vtkOrientationRepresentation::Outside)
  {
    return;
  }

  // Begin the widget interaction which has the side effect of setting the
  // interaction state.
  double eventPosition[2];
  eventPosition[0] = static_cast<double>(self->Interactor->GetEventPosition()[0]);
  eventPosition[1] = static_cast<double>(self->Interactor->GetEventPosition()[1]);
  vtkOrientationRepresentation::SafeDownCast(self->WidgetRep)
    ->StartWidgetInteraction(eventPosition);

  // We are definitely selected
  self->Active = true;
  self->GrabFocus(self->EventCallbackCommand);

  // The SetInteractionState has the side effect of highlighting the widget
  vtkOrientationRepresentation::SafeDownCast(self->WidgetRep)
    ->SetInteractionState(interactionState);

  // start the interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
}

//------------------------------------------------------------------------------
void vtkOrientationWidget::MoveAction(vtkAbstractWidget* w)
{
  vtkOrientationWidget* self = vtkOrientationWidget::SafeDownCast(w);
  vtkOrientationRepresentation* repr = vtkOrientationRepresentation::SafeDownCast(self->WidgetRep);
  // compute some info we need for all cases
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  if (self->Active)
  {
    // moving something
    double eventPosition[2];
    eventPosition[0] = static_cast<double>(X);
    eventPosition[1] = static_cast<double>(Y);
    repr->WidgetInteraction(eventPosition);
    self->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
    self->EventCallbackCommand->SetAbortFlag(1);
    self->Render();
  }
  else
  {
    self->Interactor->Disable(); // avoid extra renders

    int oldState = self->WidgetRep->GetInteractionState();
    int state = self->WidgetRep->ComputeInteractionState(X, Y);
    int changed;
    // Determine if we are near the end points or the line
    if (state == vtkOrientationRepresentation::Outside)
    {
      changed = self->RequestCursorShape(VTK_CURSOR_DEFAULT);
    }
    else // must be near something
    {
      changed = self->RequestCursorShape(VTK_CURSOR_HAND);
    }

    repr->SetInteractionState(state);
    self->Interactor->Enable(); // avoid extra renders

    if (changed || oldState != state)
    {
      self->Render();
    }
  }
}

//------------------------------------------------------------------------------
void vtkOrientationWidget::EndSelectAction(vtkAbstractWidget* w)
{
  vtkOrientationWidget* self = vtkOrientationWidget::SafeDownCast(w);
  if (!self->Active)
  {
    return;
  }

  // Return state to not active
  self->Active = false;
  vtkOrientationRepresentation::SafeDownCast(self->WidgetRep)
    ->SetInteractionState(vtkOrientationRepresentation::Outside);
  self->ReleaseFocus();

  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(vtkCommand::EndInteractionEvent, nullptr);
  self->Render();
}
VTK_ABI_NAMESPACE_END
