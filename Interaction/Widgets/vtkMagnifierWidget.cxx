// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkMagnifierWidget.h"
#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkEvent.h"
#include "vtkMagnifierRepresentation.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"
#include "vtkWidgetEventTranslator.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkMagnifierWidget);

//------------------------------------------------------------------------------
vtkMagnifierWidget::vtkMagnifierWidget()
{
  // Change activation value inherited from superclass
  this->KeyPressActivationValue = 'm';

  // Define increase and decrease keystrokes
  this->KeyPressIncreaseValue = '+';
  this->KeyPressDecreaseValue = '-';

  this->WidgetState = vtkMagnifierWidget::Invisible;

  this->CallbackMapper->SetCallbackMethod(
    vtkCommand::MouseMoveEvent, vtkWidgetEvent::Move, this, vtkMagnifierWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(
    vtkCommand::CharEvent, vtkWidgetEvent::Up, this, vtkMagnifierWidget::CharAction);
  this->CallbackMapper->SetCallbackMethod(
    vtkCommand::CharEvent, vtkWidgetEvent::Down, this, vtkMagnifierWidget::CharAction);
}

//------------------------------------------------------------------------------
vtkMagnifierWidget::~vtkMagnifierWidget() = default;

//------------------------------------------------------------------------------
void vtkMagnifierWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = vtkMagnifierRepresentation::New();
  }
}

//------------------------------------------------------------------------------
void vtkMagnifierWidget::SetEnabled(int enabling)
{
  int enabled = this->Enabled;

  // We do this step first because it sets the CurrentRenderer
  this->Superclass::SetEnabled(enabling);

  // Activate the representation
  if (enabling && !enabled)
  {
    this->WidgetState = Visible;
    static_cast<vtkMagnifierRepresentation*>(this->WidgetRep)->SetInteractionState(Visible);
  }
  else if (!enabling && enabled)
  {
    this->WidgetState = Invisible;
    static_cast<vtkMagnifierRepresentation*>(this->WidgetRep)->SetInteractionState(Invisible);
  }

  // Bring everything up to date
  double eventPos[2];
  eventPos[0] = static_cast<double>(this->Interactor->GetEventPosition()[0]);
  eventPos[1] = static_cast<double>(this->Interactor->GetEventPosition()[1]);
  static_cast<vtkMagnifierRepresentation*>(this->WidgetRep)->WidgetInteraction(eventPos);

  this->Render();
}

//------------------------------------------------------------------------------
void vtkMagnifierWidget::MoveAction(vtkAbstractWidget* w)
{
  vtkMagnifierWidget* self = reinterpret_cast<vtkMagnifierWidget*>(w);

  // Return if not active
  if (self->WidgetState == vtkMagnifierWidget::Invisible)
  {
    return;
  }

  // Note current mouse location
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Forward the event to the representation
  double eventPos[2];
  eventPos[0] = static_cast<double>(X);
  eventPos[1] = static_cast<double>(Y);
  self->WidgetRep->WidgetInteraction(eventPos);
  self->Render();
}

//------------------------------------------------------------------------------
void vtkMagnifierWidget::CharAction(vtkAbstractWidget* w)
{
  vtkMagnifierWidget* self = reinterpret_cast<vtkMagnifierWidget*>(w);

  // Return if not active
  if (self->WidgetState == vtkMagnifierWidget::Invisible)
  {
    return;
  }

  if (self->Interactor->GetKeyCode() == self->KeyPressIncreaseValue ||
    self->Interactor->GetKeyCode() == self->KeyPressDecreaseValue)
  {
    int X = self->Interactor->GetEventPosition()[0];
    int Y = self->Interactor->GetEventPosition()[1];

    // Forward the event to the representation
    self->InvokeEvent(vtkCommand::WidgetValueChangedEvent, nullptr);
    double eventPos[2];
    eventPos[0] = static_cast<double>(X);
    eventPos[1] = static_cast<double>(Y);
    self->WidgetRep->WidgetInteraction(eventPos);
    self->Render();
  }
}

//------------------------------------------------------------------------------
void vtkMagnifierWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
