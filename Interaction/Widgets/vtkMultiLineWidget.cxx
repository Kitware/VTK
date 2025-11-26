// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkMultiLineWidget.h"

#include "vtkCallbackCommand.h"
#include "vtkLineWidget2.h"
#include "vtkMultiLineRepresentation.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkMultiLineWidget);

//------------------------------------------------------------------------------
vtkMultiLineWidget::vtkMultiLineWidget()
{
  this->SetLineCount(4);

  // Define widget events
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent, vtkWidgetEvent::Select,
    this, vtkMultiLineWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
    vtkWidgetEvent::EndSelect, this, vtkMultiLineWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(
    vtkCommand::MouseMoveEvent, vtkWidgetEvent::Move, this, vtkMultiLineWidget::MoveAction);
}

//------------------------------------------------------------------------------
vtkMultiLineWidget::~vtkMultiLineWidget() = default;

//------------------------------------------------------------------------------
void vtkMultiLineWidget::SetLineCount(int newLineCount)
{
  if (newLineCount < 0)
  {
    vtkWarningMacro(<< newLineCount << " is an invalid line count.");
    return;
  }

  if (newLineCount == this->LineCount)
  {
    return;
  }

  vtkMultiLineRepresentation* multiLineRepresentation = this->GetMultiLineRepresentation();
  if (multiLineRepresentation != nullptr)
  {
    multiLineRepresentation->SetLineCount(newLineCount);
  }

  this->LineWidgetVector.resize(newLineCount);

  // If we have more lines than before, we set the new representation for these lines
  for (int i = this->LineCount; i < newLineCount; i++)
  {
    auto lineWidget = vtkSmartPointer<vtkLineWidget2>::New();
    lineWidget->SetInteractor(this->Interactor);
    if (multiLineRepresentation != nullptr)
    {
      lineWidget->SetRepresentation(multiLineRepresentation->GetLineRepresentation(i));
      lineWidget->GetRepresentation()->SetRenderer(this->CurrentRenderer);
    }
    lineWidget->SetEnabled(this->Enabled);
    lineWidget->SetParent(this); // the line now watches events of this widget

    this->LineWidgetVector[i] = lineWidget;
  }
  this->LineCount = newLineCount;
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkMultiLineWidget::SetInteractor(vtkRenderWindowInteractor* interactor)
{
  this->Superclass::SetInteractor(interactor);
  for (vtkLineWidget2* lineWidget : this->LineWidgetVector)
  {
    lineWidget->SetInteractor(interactor);
  }
}

//------------------------------------------------------------------------------
void vtkMultiLineWidget::SetEnabled(int enabling)
{
  this->Superclass::SetEnabled(enabling);
  for (vtkLineWidget2* lineWidget : this->LineWidgetVector)
  {
    lineWidget->SetEnabled(enabling);
  }
}

//------------------------------------------------------------------------------
void vtkMultiLineWidget::SelectAction(vtkAbstractWidget* widget)
{
  vtkMultiLineWidget* self = vtkMultiLineWidget::SafeDownCast(widget);
  if (self->WidgetRep->GetInteractionState() == vtkMultiLineRepresentation::MOUSE_OUTSIDE_LINES)
  {
    return;
  }

  self->WidgetState = vtkMultiLineWidget::ACTIVE_SELECTION;
  self->GrabFocus(self->EventCallbackCommand);
  self->InvokeEvent(
    vtkCommand::LeftButtonPressEvent, nullptr); // all the vtkLineWidget2 observe this
  self->Superclass::StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
  self->EventCallbackCommand->SetAbortFlag(1);
}

//------------------------------------------------------------------------------
void vtkMultiLineWidget::EndSelectAction(vtkAbstractWidget* widget)
{
  vtkMultiLineWidget* self = vtkMultiLineWidget::SafeDownCast(widget);
  if (self->WidgetState == vtkMultiLineWidget::NOT_SELECTED)
  {
    return;
  }

  self->WidgetState = vtkMultiLineWidget::NOT_SELECTED;
  self->ReleaseFocus();
  self->InvokeEvent(
    vtkCommand::LeftButtonReleaseEvent, nullptr); // all the vtkLineWidget2 observe this
  self->Superclass::EndInteraction();
  self->InvokeEvent(vtkCommand::EndInteractionEvent, nullptr);
  self->EventCallbackCommand->SetAbortFlag(1);
  self->Render();
}

//------------------------------------------------------------------------------
void vtkMultiLineWidget::MoveAction(vtkAbstractWidget* widget)
{
  vtkMultiLineWidget* self = vtkMultiLineWidget::SafeDownCast(widget);
  int xPos = self->Interactor->GetEventPosition()[0];
  int yPos = self->Interactor->GetEventPosition()[1];

  if (self->WidgetState == vtkMultiLineWidget::NOT_SELECTED)
  {
    int oldState = self->WidgetRep->GetInteractionState();
    int state = self->WidgetRep->ComputeInteractionState(xPos, yPos);

    if (state != oldState || state != vtkMultiLineRepresentation::MOUSE_OUTSIDE_LINES)
    {
      self->InvokeEvent(vtkCommand::MouseMoveEvent, nullptr); // all the vtkLineWidget2 observe this
      self->Render();
    }
  }
  else // if (self->WidgetState == vtkMultiLineWidget::ACTIVE_SELECTION)
  {
    self->InvokeEvent(vtkCommand::MouseMoveEvent, nullptr); // all the vtkLineWidget2 observe this
    self->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
    self->Render();
  }
}

//------------------------------------------------------------------------------
vtkMultiLineRepresentation* vtkMultiLineWidget::GetMultiLineRepresentation()
{
  return vtkMultiLineRepresentation::SafeDownCast(this->WidgetRep);
}

//------------------------------------------------------------------------------
void vtkMultiLineWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->SetRepresentation(vtkNew<vtkMultiLineRepresentation>());
  }
}

//------------------------------------------------------------------------------
void vtkMultiLineWidget::SetRepresentation(vtkMultiLineRepresentation* repr)
{
  this->Superclass::SetWidgetRepresentation(repr);
  repr->SetLineCount(this->LineCount);

  for (int i = 0; i < this->LineCount; i++)
  {
    this->LineWidgetVector[i]->SetRepresentation(repr->GetLineRepresentation(i));
  }
}

//------------------------------------------------------------------------------
vtkMTimeType vtkMultiLineWidget::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  for (vtkLineWidget2* lineWidget : LineWidgetVector)
  {
    mTime = std::max(mTime, lineWidget->GetMTime());
  }

  return mTime;
}

//------------------------------------------------------------------------------
void vtkMultiLineWidget::SetProcessEvents(vtkTypeBool enabled)
{
  this->Superclass::SetProcessEvents(enabled);
  for (vtkLineWidget2* lineWidget : this->LineWidgetVector)
  {
    lineWidget->SetProcessEvents(enabled);
  }
}

//------------------------------------------------------------------------------
void vtkMultiLineWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << "Line Count : " << this->LineCount << '\n';

  for (int i = 0; i < this->LineCount; i++)
  {
    os << "Line " << i << " :" << '\n';
    this->LineWidgetVector[i]->PrintSelf(os, indent);
  }
}
VTK_ABI_NAMESPACE_END
