/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkVRPanelWidget.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVRPanelWidget.h"

#include "vtkCallbackCommand.h"
#include "vtkEventData.h"
#include "vtkObjectFactory.h"
#include "vtkVRPanelRepresentation.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"

vtkStandardNewMacro(vtkVRPanelWidget);

//------------------------------------------------------------------------------
vtkVRPanelWidget::vtkVRPanelWidget()
{
  this->WidgetState = vtkVRPanelWidget::Start;

  {
    vtkNew<vtkEventDataDevice3D> ed;
    ed->SetDevice(vtkEventDataDevice::Any);
    ed->SetInput(vtkEventDataDeviceInput::Any);
    ed->SetAction(vtkEventDataAction::Press);
    this->CallbackMapper->SetCallbackMethod(vtkCommand::Select3DEvent, ed, vtkWidgetEvent::Select3D,
      this, vtkVRPanelWidget::SelectAction3D);
  }

  {
    vtkNew<vtkEventDataDevice3D> ed;
    ed->SetDevice(vtkEventDataDevice::Any);
    ed->SetInput(vtkEventDataDeviceInput::Any);
    ed->SetAction(vtkEventDataAction::Release);
    this->CallbackMapper->SetCallbackMethod(vtkCommand::Select3DEvent, ed,
      vtkWidgetEvent::EndSelect3D, this, vtkVRPanelWidget::EndSelectAction3D);
  }

  {
    vtkNew<vtkEventDataDevice3D> ed;
    ed->SetDevice(vtkEventDataDevice::Any);
    ed->SetInput(vtkEventDataDeviceInput::Any);
    this->CallbackMapper->SetCallbackMethod(
      vtkCommand::Move3DEvent, ed, vtkWidgetEvent::Move3D, this, vtkVRPanelWidget::MoveAction3D);
  }
}

//------------------------------------------------------------------------------
void vtkVRPanelWidget::SetRepresentation(vtkVRPanelRepresentation* rep)
{
  this->Superclass::SetWidgetRepresentation(reinterpret_cast<vtkWidgetRepresentation*>(rep));
}

//------------------------------------------------------------------------------
void vtkVRPanelWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = vtkVRPanelRepresentation::New();
  }
}

//------------------------------------------------------------------------------
void vtkVRPanelWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkVRPanelWidget::SelectAction3D(vtkAbstractWidget* w)
{
  vtkVRPanelWidget* self = reinterpret_cast<vtkVRPanelWidget*>(w);

  // We want to compute an orthogonal vector to the plane that has been selected
  int interactionState = self->WidgetRep->ComputeComplexInteractionState(
    self->Interactor, self, vtkWidgetEvent::Select3D, self->CallData);

  if (interactionState == vtkVRPanelRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  if (!self->Parent)
  {
    self->GrabFocus(self->EventCallbackCommand);
  }

  self->WidgetState = vtkVRPanelWidget::Active;
  self->WidgetRep->StartComplexInteraction(
    self->Interactor, self, vtkWidgetEvent::Select3D, self->CallData);

  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
}

//------------------------------------------------------------------------------
void vtkVRPanelWidget::MoveAction3D(vtkAbstractWidget* w)
{
  vtkVRPanelWidget* self = reinterpret_cast<vtkVRPanelWidget*>(w);

  // See whether we're active
  if (self->WidgetState == vtkVRPanelWidget::Start)
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

//------------------------------------------------------------------------------
void vtkVRPanelWidget::EndSelectAction3D(vtkAbstractWidget* w)
{
  vtkVRPanelWidget* self = reinterpret_cast<vtkVRPanelWidget*>(w);

  // See whether we're active
  if (self->WidgetState != vtkVRPanelWidget::Active ||
    self->WidgetRep->GetInteractionState() == vtkVRPanelRepresentation::Outside)
  {
    return;
  }

  // Return state to not selected
  self->WidgetRep->EndComplexInteraction(
    self->Interactor, self, vtkWidgetEvent::EndSelect3D, self->CallData);

  self->WidgetState = vtkVRPanelWidget::Start;
  if (!self->Parent)
  {
    self->ReleaseFocus();
  }

  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(vtkCommand::EndInteractionEvent, nullptr);
}
