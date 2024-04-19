// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCamera3DWidget.h"

#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCamera3DRepresentation.h"
#include "vtkCommand.h"
#include "vtkEvent.h"
#include "vtkEventData.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"
#include "vtkWidgetEventTranslator.h"

#include <algorithm>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkCamera3DWidget);

//------------------------------------------------------------------------------
vtkCamera3DWidget::vtkCamera3DWidget()
{
  this->ManagesCursor = 1;

  // Setup a default representation in case we don't set one
  this->CreateDefaultRepresentation();
  double bounds[6] = { -0.5, 0.5, -0.5, 0.5, -0.5, 0.5 };
  this->WidgetRep->PlaceWidget(bounds);

  // Define widget events
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent, vtkWidgetEvent::Select,
    this, vtkCamera3DWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
    vtkWidgetEvent::EndSelect, this, vtkCamera3DWidget::EndSelectAction);

  this->CallbackMapper->SetCallbackMethod(
    vtkCommand::MouseMoveEvent, vtkWidgetEvent::Move, this, vtkCamera3DWidget::MoveAction);

  this->KeyEventCallbackCommand->SetClientData(this);
  this->KeyEventCallbackCommand->SetCallback(vtkCamera3DWidget::ProcessKeyEvents);
}

//------------------------------------------------------------------------------
vtkCamera3DWidget::~vtkCamera3DWidget() = default;

//------------------------------------------------------------------------------
void vtkCamera3DWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = vtkCamera3DRepresentation::New();
  }
}

//------------------------------------------------------------------------------
void vtkCamera3DWidget::SetRepresentation(vtkCamera3DRepresentation* r)
{
  this->Superclass::SetWidgetRepresentation(r);
}

//------------------------------------------------------------------------------
void vtkCamera3DWidget::SetEnabled(int enabling)
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
void vtkCamera3DWidget::SelectAction(vtkAbstractWidget* w)
{
  // We are in a static method, cast to ourself
  vtkCamera3DWidget* self = vtkCamera3DWidget::SafeDownCast(w);
  int interactionState = self->WidgetRep->GetInteractionState();
  if (interactionState == vtkCamera3DRepresentation::Outside)
  {
    return;
  }

  // Begin the widget interaction which has the side effect of setting the
  // interaction state.
  double eventPosition[2];
  eventPosition[0] = static_cast<double>(self->Interactor->GetEventPosition()[0]);
  eventPosition[1] = static_cast<double>(self->Interactor->GetEventPosition()[1]);
  vtkCamera3DRepresentation::SafeDownCast(self->WidgetRep)->StartWidgetInteraction(eventPosition);

  // We are definitely selected
  self->Active = true;
  self->GrabFocus(self->EventCallbackCommand);

  // The SetInteractionState has the side effect of highlighting the widget
  vtkCamera3DRepresentation::SafeDownCast(self->WidgetRep)->SetInteractionState(interactionState);

  // start the interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
}

//------------------------------------------------------------------------------
void vtkCamera3DWidget::MoveAction(vtkAbstractWidget* w)
{
  vtkCamera3DWidget* self = vtkCamera3DWidget::SafeDownCast(w);
  // compute some info we need for all cases
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // See whether we're active
  if (!self->Active)
  {
    self->Interactor->Disable(); // avoid extra renders

    int oldState = self->WidgetRep->GetInteractionState();
    int state = self->WidgetRep->ComputeInteractionState(X, Y);
    int changed;
    // Determine if we are near the end points or the line
    if (state == vtkCamera3DRepresentation::Outside)
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
    vtkCamera3DRepresentation::SafeDownCast(self->WidgetRep)->WidgetInteraction(eventPosition);
    self->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
    self->EventCallbackCommand->SetAbortFlag(1);
    self->Render();
  }
}

//------------------------------------------------------------------------------
void vtkCamera3DWidget::EndSelectAction(vtkAbstractWidget* w)
{
  vtkCamera3DWidget* self = vtkCamera3DWidget::SafeDownCast(w);
  if (!self->Active)
  {
    return;
  }

  // Return state to not active
  self->Active = false;
  vtkCamera3DRepresentation::SafeDownCast(self->WidgetRep)
    ->SetInteractionState(vtkCamera3DRepresentation::Outside);
  self->ReleaseFocus();

  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(vtkCommand::EndInteractionEvent, nullptr);
  self->Render();
}

//------------------------------------------------------------------------------
void vtkCamera3DWidget::ProcessKeyEvents(vtkObject*, unsigned long event, void* clientdata, void*)
{
  vtkCamera3DWidget* self = static_cast<vtkCamera3DWidget*>(clientdata);
  vtkCamera3DRepresentation* rep = vtkCamera3DRepresentation::SafeDownCast(self->WidgetRep);
  char* cKeySym = self->Interactor->GetKeySym();
  std::string keySym = cKeySym != nullptr ? cKeySym : "";
  std::transform(keySym.begin(), keySym.end(), keySym.begin(), ::toupper);
  if (event == vtkCommand::KeyPressEvent)
  {
    if (keySym == "X")
    {
      if (rep->GetTranslationAxis() == vtkWidgetRepresentation::XAxis)
      {
        rep->SetTranslationAxisToNone();
      }
      else
      {
        rep->SetTranslationAxisToXAxis();
      }
    }
    else if (keySym == "Y")
    {
      if (rep->GetTranslationAxis() == vtkWidgetRepresentation::YAxis)
      {
        rep->SetTranslationAxisToNone();
      }
      else
      {
        rep->SetTranslationAxisToYAxis();
      }
    }
    else if (keySym == "Z")
    {
      if (rep->GetTranslationAxis() == vtkWidgetRepresentation::ZAxis)
      {
        rep->SetTranslationAxisToNone();
      }
      else
      {
        rep->SetTranslationAxisToZAxis();
      }
    }
    else if (keySym == "O")
    {
      rep->SetTranslationAxisToNone();
    }
    else if (keySym == "A")
    {
      if (rep->GetTranslatingAll())
      {
        rep->TranslatingAllOff();
      }
      else
      {
        rep->TranslatingAllOn();
      }
    }
    else if (keySym == "C")
    {
      if (rep->GetFrustumVisibility())
      {
        rep->FrustumVisibilityOff();
      }
      else
      {
        rep->FrustumVisibilityOn();
      }
    }
  }
}
VTK_ABI_NAMESPACE_END
