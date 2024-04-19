// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPointCloudWidget.h"
#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkEvent.h"
#include "vtkEventData.h"
#include "vtkObjectFactory.h"
#include "vtkPointCloudRepresentation.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"
#include "vtkWidgetEventTranslator.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPointCloudWidget);

//------------------------------------------------------------------------------
vtkPointCloudWidget::vtkPointCloudWidget()
{
  this->WidgetState = vtkPointCloudWidget::Start;
  this->ManagesCursor = 1;

  // Define widget events: translate mouse events to widget events
  this->CallbackMapper->SetCallbackMethod(
    vtkCommand::MouseMoveEvent, vtkWidgetEvent::Move, this, vtkPointCloudWidget::MoveAction);

  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent, vtkEvent::NoModifier, 0,
    0, nullptr, vtkWidgetEvent::Select, this, vtkPointCloudWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent, vtkEvent::AnyModifier,
    0, 0, nullptr, vtkWidgetEvent::EndSelect, this, vtkPointCloudWidget::EndSelectAction);
}

//------------------------------------------------------------------------------
vtkPointCloudWidget::~vtkPointCloudWidget() = default;

//------------------------------------------------------------------------------
void vtkPointCloudWidget::SetEnabled(int enabling)
{
  // We do this step first because it sets the CurrentRenderer
  this->Superclass::SetEnabled(enabling);
}

//------------------------------------------------------------------------------
void vtkPointCloudWidget::MoveAction(vtkAbstractWidget* w)
{
  vtkPointCloudWidget* self = reinterpret_cast<vtkPointCloudWidget*>(w);

  // See whether we're active, return if we are in the middle of something
  // (i.e., a selection process).
  if (self->WidgetState == vtkPointCloudWidget::Active)
  {
    return;
  }

  // compute some info we need for all cases
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // If nothing changes, just return
  int oldState = self->WidgetRep->GetInteractionState();
  vtkIdType oldPtId = reinterpret_cast<vtkPointCloudRepresentation*>(self->WidgetRep)->GetPointId();
  int state = self->WidgetRep->ComputeInteractionState(X, Y);
  vtkIdType ptId = reinterpret_cast<vtkPointCloudRepresentation*>(self->WidgetRep)->GetPointId();
  if (oldState == state && oldPtId == ptId)
  {
    return;
  }

  // A new point has been picked
  if (state == vtkPointCloudRepresentation::Over)
  {
    self->EventCallbackCommand->SetAbortFlag(1);
    self->InvokeEvent(vtkCommand::PickEvent, nullptr);
  }

  // Refresh the renderer
  self->Render();
}

//------------------------------------------------------------------------------
void vtkPointCloudWidget::SelectAction(vtkAbstractWidget* w)
{
  // We are in a static method, cast to ourself
  vtkPointCloudWidget* self = reinterpret_cast<vtkPointCloudWidget*>(w);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!self->CurrentRenderer || !self->CurrentRenderer->IsInViewport(X, Y))
  {
    self->WidgetState = vtkPointCloudWidget::Start;
    return;
  }

  // Only can select if we are over a point
  int state = self->WidgetRep->GetInteractionState();
  if (state != vtkPointCloudRepresentation::Over)
  {
    return;
  }

  // We are definitely selected
  self->WidgetState = vtkPointCloudWidget::Active;
  if (!self->Parent)
  {
    self->GrabFocus(self->EventCallbackCommand);
  }

  // Something has changed, so render to see the changes
  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(vtkCommand::WidgetActivateEvent, nullptr);
  self->Render();
}

//------------------------------------------------------------------------------
void vtkPointCloudWidget::EndSelectAction(vtkAbstractWidget* w)
{
  vtkPointCloudWidget* self = reinterpret_cast<vtkPointCloudWidget*>(w);
  if (self->WidgetState == vtkPointCloudWidget::Start)
  {
    return;
  }

  // Return state to not active
  self->WidgetState = vtkPointCloudWidget::Start;
  reinterpret_cast<vtkPointCloudRepresentation*>(self->WidgetRep)
    ->SetInteractionState(vtkPointCloudRepresentation::Outside);
  self->ReleaseFocus();

  self->Render();
}

//------------------------------------------------------------------------------
void vtkPointCloudWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = vtkPointCloudRepresentation::New();
  }
}

//------------------------------------------------------------------------------
void vtkPointCloudWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
