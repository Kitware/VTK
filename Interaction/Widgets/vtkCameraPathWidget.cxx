// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCameraPathWidget.h"

#include "vtkCallbackCommand.h"
#include "vtkCameraPathRepresentation.h"
#include "vtkCommand.h"
#include "vtkEvent.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"
#include "vtkWidgetEventTranslator.h"

#include <algorithm>

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkCameraPathWidget);

//------------------------------------------------------------------------------
vtkCameraPathWidget::vtkCameraPathWidget()
{
  this->ManagesCursor = 1;

  // Define widget events
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent, vtkWidgetEvent::Select,
    this, vtkCameraPathWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
    vtkWidgetEvent::EndSelect, this, vtkCameraPathWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonPressEvent,
    vtkWidgetEvent::Translate, this, vtkCameraPathWidget::TranslateAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonReleaseEvent,
    vtkWidgetEvent::EndTranslate, this, vtkCameraPathWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonPressEvent, vtkWidgetEvent::Scale,
    this, vtkCameraPathWidget::ScaleAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonReleaseEvent,
    vtkWidgetEvent::EndScale, this, vtkCameraPathWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(
    vtkCommand::MouseMoveEvent, vtkWidgetEvent::Move, this, vtkCameraPathWidget::MoveAction);

  this->KeyEventCallbackCommand->SetClientData(this);
  this->KeyEventCallbackCommand->SetCallback(vtkCameraPathWidget::ProcessKeyEvents);
}

//------------------------------------------------------------------------------
vtkCameraPathWidget::~vtkCameraPathWidget() = default;

//------------------------------------------------------------------------------
void vtkCameraPathWidget::SetRepresentation(vtkCameraPathRepresentation* r)
{
  this->Superclass::SetWidgetRepresentation(vtkWidgetRepresentation::SafeDownCast(r));
}

//------------------------------------------------------------------------------
void vtkCameraPathWidget::SetEnabled(int enabling)
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

//------------------------------------------------------------------------------
void vtkCameraPathWidget::SelectAction(vtkAbstractWidget* w)
{
  // We are in a static method, cast to ourself
  vtkCameraPathWidget* self = vtkCameraPathWidget::SafeDownCast(w);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!self->CurrentRenderer || !self->CurrentRenderer->IsInViewport(X, Y))
  {
    self->WidgetState = vtkCameraPathWidget::Start;
    return;
  }

  // Begin the widget interaction which has the side effect of setting the
  // interaction state.
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(e);
  int interactionState = self->WidgetRep->GetInteractionState();
  if (interactionState == vtkCameraPathRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->WidgetState = vtkCameraPathWidget::Active;
  self->GrabFocus(self->EventCallbackCommand);

  if (interactionState == vtkCameraPathRepresentation::OnLine && self->Interactor->GetControlKey())
  {
    // Add point.
    reinterpret_cast<vtkCameraPathRepresentation*>(self->WidgetRep)
      ->SetInteractionState(vtkCameraPathRepresentation::Inserting);
  }
  else if (interactionState == vtkCameraPathRepresentation::OnHandle &&
    self->Interactor->GetShiftKey())
  {
    // remove point.
    reinterpret_cast<vtkCameraPathRepresentation*>(self->WidgetRep)
      ->SetInteractionState(vtkCameraPathRepresentation::Erasing);
  }
  else
  {
    reinterpret_cast<vtkCameraPathRepresentation*>(self->WidgetRep)
      ->SetInteractionState(vtkCameraPathRepresentation::Moving);
  }

  // start the interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//------------------------------------------------------------------------------
void vtkCameraPathWidget::TranslateAction(vtkAbstractWidget* w)
{
  vtkCameraPathWidget::SelectAction(w);
}

//------------------------------------------------------------------------------
void vtkCameraPathWidget::ScaleAction(vtkAbstractWidget* w)
{
  // We are in a static method, cast to ourself
  vtkCameraPathWidget* self = reinterpret_cast<vtkCameraPathWidget*>(w);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!self->CurrentRenderer || !self->CurrentRenderer->IsInViewport(X, Y))
  {
    self->WidgetState = vtkCameraPathWidget::Start;
    return;
  }

  // Begin the widget interaction which has the side effect of setting the
  // interaction state.
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(e);
  int interactionState = self->WidgetRep->GetInteractionState();
  if (interactionState == vtkCameraPathRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->WidgetState = vtkCameraPathWidget::Active;
  self->GrabFocus(self->EventCallbackCommand);
  // Scale
  reinterpret_cast<vtkCameraPathRepresentation*>(self->WidgetRep)
    ->SetInteractionState(vtkCameraPathRepresentation::Scaling);

  // start the interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
  self->Render();
}

//------------------------------------------------------------------------------
void vtkCameraPathWidget::MoveAction(vtkAbstractWidget* w)
{
  vtkCameraPathWidget* self = reinterpret_cast<vtkCameraPathWidget*>(w);

  // See whether we're active
  if (self->WidgetState == vtkCameraPathWidget::Start)
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

//------------------------------------------------------------------------------
void vtkCameraPathWidget::EndSelectAction(vtkAbstractWidget* w)
{
  vtkCameraPathWidget* self = reinterpret_cast<vtkCameraPathWidget*>(w);
  if (self->WidgetState == vtkCameraPathWidget::Start)
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
  self->WidgetState = vtkCameraPathWidget::Start;
  reinterpret_cast<vtkCameraPathRepresentation*>(self->WidgetRep)
    ->SetInteractionState(vtkCameraPathRepresentation::Outside);
  self->ReleaseFocus();

  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(vtkCommand::EndInteractionEvent, nullptr);
  self->Render();
}

//------------------------------------------------------------------------------
void vtkCameraPathWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = vtkCameraPathRepresentation::New();
  }
}

//------------------------------------------------------------------------------
void vtkCameraPathWidget::ProcessKeyEvents(vtkObject*, unsigned long event, void* clientdata, void*)
{
  vtkCameraPathWidget* self = static_cast<vtkCameraPathWidget*>(clientdata);
  vtkCameraPathRepresentation* rep = vtkCameraPathRepresentation::SafeDownCast(self->WidgetRep);
  char* cKeySym = self->Interactor->GetKeySym();
  std::string keySym = cKeySym != nullptr ? cKeySym : "";
  std::transform(keySym.begin(), keySym.end(), keySym.begin(), ::toupper);
  if (event == vtkCommand::KeyPressEvent)
  {
    if (keySym == "X")
    {
      rep->SetXTranslationAxisOn();
    }
    else if (keySym == "Y")
    {
      rep->SetYTranslationAxisOn();
    }
    else if (keySym == "Z")
    {
      rep->SetZTranslationAxisOn();
    }
  }
  else if (event == vtkCommand::KeyReleaseEvent)
  {
    if (keySym == "X" || keySym == "Y" || keySym == "Z")
    {
      rep->SetTranslationAxisOff();
    }
  }
}

//------------------------------------------------------------------------------
void vtkCameraPathWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "WidgetState: " << this->WidgetState << "\n";
}
VTK_ABI_NAMESPACE_END
