/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkOpenVRPanelWidget.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenVRPanelWidget.h"
#include "vtkOpenVRPanelRepresentation.h"

#include "vtkCallbackCommand.h"
#include "vtkEventData.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"

vtkStandardNewMacro(vtkOpenVRPanelWidget);

//----------------------------------------------------------------------
vtkOpenVRPanelWidget::vtkOpenVRPanelWidget()
{
  this->WidgetState = vtkOpenVRPanelWidget::Start;

  {
    vtkNew<vtkEventDataButton3D> ed;
    ed->SetDevice(vtkEventDataDevice::RightController);
    ed->SetInput(vtkEventDataDeviceInput::Trigger);
    ed->SetAction(vtkEventDataAction::Press);
    this->CallbackMapper->SetCallbackMethod(vtkCommand::Button3DEvent, ed, vtkWidgetEvent::Select3D,
      this, vtkOpenVRPanelWidget::SelectAction3D);
  }

  {
    vtkNew<vtkEventDataButton3D> ed;
    ed->SetDevice(vtkEventDataDevice::RightController);
    ed->SetInput(vtkEventDataDeviceInput::Trigger);
    ed->SetAction(vtkEventDataAction::Release);
    this->CallbackMapper->SetCallbackMethod(vtkCommand::Button3DEvent, ed,
      vtkWidgetEvent::EndSelect3D, this, vtkOpenVRPanelWidget::EndSelectAction3D);
  }

  {
    vtkNew<vtkEventDataMove3D> ed;
    ed->SetDevice(vtkEventDataDevice::RightController);
    this->CallbackMapper->SetCallbackMethod(vtkCommand::Move3DEvent, ed, vtkWidgetEvent::Move3D,
      this, vtkOpenVRPanelWidget::MoveAction3D);
  }
}

//----------------------------------------------------------------------
vtkOpenVRPanelWidget::~vtkOpenVRPanelWidget() {}

//----------------------------------------------------------------------
void vtkOpenVRPanelWidget::SetRepresentation(vtkOpenVRPanelRepresentation* rep)
{
  this->Superclass::SetWidgetRepresentation(reinterpret_cast<vtkWidgetRepresentation*>(rep));
}

//----------------------------------------------------------------------
void vtkOpenVRPanelWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = vtkOpenVRPanelRepresentation::New();
  }
}

//----------------------------------------------------------------------
void vtkOpenVRPanelWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-------------------------------------------------------------------------
void vtkOpenVRPanelWidget::SelectAction3D(vtkAbstractWidget* w)
{
  vtkOpenVRPanelWidget* self = reinterpret_cast<vtkOpenVRPanelWidget*>(w);

  // We want to compute an orthogonal vector to the plane that has been selected
  int interactionState = self->WidgetRep->ComputeComplexInteractionState(
    self->Interactor, self, vtkWidgetEvent::Select3D, self->CallData);

  if (interactionState == vtkOpenVRPanelRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  if (!self->Parent)
  {
    self->GrabFocus(self->EventCallbackCommand);
  }

  self->WidgetState = vtkOpenVRPanelWidget::Active;
  self->WidgetRep->StartComplexInteraction(
    self->Interactor, self, vtkWidgetEvent::Select3D, self->CallData);

  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
}

//----------------------------------------------------------------------
void vtkOpenVRPanelWidget::MoveAction3D(vtkAbstractWidget* w)
{
  vtkOpenVRPanelWidget* self = reinterpret_cast<vtkOpenVRPanelWidget*>(w);

  // See whether we're active
  if (self->WidgetState == vtkOpenVRPanelWidget::Start)
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
void vtkOpenVRPanelWidget::EndSelectAction3D(vtkAbstractWidget* w)
{
  vtkOpenVRPanelWidget* self = reinterpret_cast<vtkOpenVRPanelWidget*>(w);

  // See whether we're active
  if (self->WidgetState != vtkOpenVRPanelWidget::Active ||
    self->WidgetRep->GetInteractionState() == vtkOpenVRPanelRepresentation::Outside)
  {
    return;
  }

  // Return state to not selected
  self->WidgetRep->EndComplexInteraction(
    self->Interactor, self, vtkWidgetEvent::EndSelect3D, self->CallData);

  self->WidgetState = vtkOpenVRPanelWidget::Start;
  if (!self->Parent)
  {
    self->ReleaseFocus();
  }

  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(vtkCommand::EndInteractionEvent, nullptr);
}
