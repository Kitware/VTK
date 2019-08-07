/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLightWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLightWidget.h"

#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkLightRepresentation.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"

vtkStandardNewMacro(vtkLightWidget);

//----------------------------------------------------------------------------
vtkLightWidget::vtkLightWidget()
{
  // Define widget events
  this->CallbackMapper->SetCallbackMethod(
    vtkCommand::LeftButtonPressEvent, vtkWidgetEvent::Select, this, vtkLightWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
    vtkWidgetEvent::EndSelect, this, vtkLightWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(
    vtkCommand::MouseMoveEvent, vtkWidgetEvent::Move, this, vtkLightWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(
    vtkCommand::RightButtonPressEvent, vtkWidgetEvent::Scale, this, vtkLightWidget::ScaleAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonReleaseEvent,
    vtkWidgetEvent::EndScale, this, vtkLightWidget::EndSelectAction);
}

//----------------------------------------------------------------------
void vtkLightWidget::SelectAction(vtkAbstractWidget* w)
{
  vtkLightWidget* self = vtkLightWidget::SafeDownCast(w);
  if (self->WidgetRep->GetInteractionState() == vtkLightRepresentation::Outside)
  {
    return;
  }

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // We are definitely selected
  self->WidgetActive = true;
  self->GrabFocus(self->EventCallbackCommand);
  double eventPosition[2];
  eventPosition[0] = static_cast<double>(X);
  eventPosition[1] = static_cast<double>(Y);
  vtkLightRepresentation::SafeDownCast(self->WidgetRep)->StartWidgetInteraction(eventPosition);
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
  self->StartInteraction();
  self->EventCallbackCommand->SetAbortFlag(1);
}

//----------------------------------------------------------------------
void vtkLightWidget::MoveAction(vtkAbstractWidget* w)
{
  vtkLightWidget* self = vtkLightWidget::SafeDownCast(w);
  // compute some info we need for all cases
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // See whether we're active
  if (!self->WidgetActive)
  {
    self->Interactor->Disable(); // avoid extra renders

    int oldState = self->WidgetRep->GetInteractionState();
    int state = self->WidgetRep->ComputeInteractionState(X, Y);
    int changed;
    // Determine if we are near the end points or the line
    if (state == vtkLightRepresentation::Outside)
    {
      changed = self->RequestCursorShape(VTK_CURSOR_DEFAULT);
    }
    else // must be near something
    {
      changed = self->RequestCursorShape(VTK_CURSOR_HAND);
    }
    self->Interactor->Enable(); // avoid extra renders
    if (changed || oldState != state)
    {
      self->Render();
    }
  }
  else // Already Active
  {
    // moving something
    double eventPosition[2];
    eventPosition[0] = static_cast<double>(X);
    eventPosition[1] = static_cast<double>(Y);
    vtkLightRepresentation::SafeDownCast(self->WidgetRep)->WidgetInteraction(eventPosition);
    self->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
    self->EventCallbackCommand->SetAbortFlag(1);
    self->Render();
  }
}

//----------------------------------------------------------------------
void vtkLightWidget::EndSelectAction(vtkAbstractWidget* w)
{
  vtkLightWidget* self = vtkLightWidget::SafeDownCast(w);
  if (!self->WidgetActive)
  {
    return;
  }

  // Return state to not active
  self->WidgetActive = false;
  self->ReleaseFocus();
  self->EventCallbackCommand->SetAbortFlag(1);
  self->Superclass::EndInteraction();
  self->InvokeEvent(vtkCommand::EndInteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------
void vtkLightWidget::ScaleAction(vtkAbstractWidget* w)
{
  // We are in a static method, cast to ourself
  vtkLightWidget* self = vtkLightWidget::SafeDownCast(w);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!self->CurrentRenderer || !self->CurrentRenderer->IsInViewport(X, Y))
  {
    self->WidgetActive = false;
    return;
  }

  // Begin the widget interaction which has the side effect of setting the
  // interaction state.
  double eventPosition[2];
  eventPosition[0] = static_cast<double>(X);
  eventPosition[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(eventPosition);
  int interactionState = self->WidgetRep->GetInteractionState();
  if (interactionState != vtkLightRepresentation::MovingPositionalFocalPoint)
  {
    return;
  }

  // We are definitely scaling the cone angle
  self->WidgetActive = true;
  self->GrabFocus(self->EventCallbackCommand);
  vtkLightRepresentation::SafeDownCast(self->WidgetRep)
    ->SetInteractionState(vtkLightRepresentation::ScalingConeAngle);

  // start the interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//----------------------------------------------------------------------
void vtkLightWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = vtkLightRepresentation::New();
  }
}

//----------------------------------------------------------------------
void vtkLightWidget::SetRepresentation(vtkLightRepresentation* r)
{
  this->Superclass::SetWidgetRepresentation(r);
}

//----------------------------------------------------------------------------
vtkLightRepresentation* vtkLightWidget::GetLightRepresentation()
{
  return vtkLightRepresentation::SafeDownCast(this->WidgetRep);
}

//----------------------------------------------------------------------------
void vtkLightWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "WidgetActive: " << this->WidgetActive << endl;
  this->Superclass::PrintSelf(os, indent);
}
