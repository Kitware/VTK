/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkResliceCursorWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkResliceCursorWidget.h"
#include "vtkResliceCursorLineRepresentation.h"
#include "vtkCommand.h"
#include "vtkResliceCursor.h"
#include "vtkCallbackCommand.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkWidgetEventTranslator.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkEvent.h"
#include "vtkWidgetEvent.h"
#include "vtkImageData.h"
#include "vtkInteractorStyleImage.h"
#include "vtkImageMapToWindowLevelColors.h"

vtkStandardNewMacro(vtkResliceCursorWidget);

//----------------------------------------------------------------------------
vtkResliceCursorWidget::vtkResliceCursorWidget()
{
  // Set the initial state
  this->WidgetState = vtkResliceCursorWidget::Start;

  this->ModifierActive = 0;

  // Okay, define the events for this widget
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent,
                                          vtkEvent::NoModifier, 0, 0, NULL,
                                          vtkWidgetEvent::Select,
                                          this, vtkResliceCursorWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent,
                                          vtkEvent::ControlModifier, 0, 0, NULL,
                                          vtkWidgetEvent::Rotate,
                                          this, vtkResliceCursorWidget::RotateAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
                                          vtkWidgetEvent::EndSelect,
                                          this, vtkResliceCursorWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonPressEvent,
                                          vtkWidgetEvent::Resize,
                                          this, vtkResliceCursorWidget::ResizeThicknessAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonReleaseEvent,
                                          vtkWidgetEvent::EndResize,
                                          this, vtkResliceCursorWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MouseMoveEvent,
                                          vtkWidgetEvent::Move,
                                          this, vtkResliceCursorWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent,
                vtkEvent::NoModifier, 111, 1, "o",
                vtkWidgetEvent::Reset,
                this, vtkResliceCursorWidget::ResetResliceCursorAction);

  this->ManageWindowLevel = 1;
}

//----------------------------------------------------------------------------
vtkResliceCursorWidget::~vtkResliceCursorWidget()
{
}

//----------------------------------------------------------------------
void vtkResliceCursorWidget::SetEnabled(int enabling)
{
  this->Superclass::SetEnabled(enabling);
}

//----------------------------------------------------------------------
void vtkResliceCursorWidget::CreateDefaultRepresentation()
{
  if ( ! this->WidgetRep )
    {
    this->WidgetRep = vtkResliceCursorLineRepresentation::New();
    }
}

//-------------------------------------------------------------------------
void vtkResliceCursorWidget::SetCursor(int cState)
{
  switch (cState)
    {
    case vtkResliceCursorRepresentation::OnAxis1:
    case vtkResliceCursorRepresentation::OnAxis2:
      this->RequestCursorShape(VTK_CURSOR_HAND);
      break;
    case vtkResliceCursorRepresentation::OnCenter:
      if (vtkEvent::GetModifier(this->Interactor) != vtkEvent::ControlModifier)
        {
        this->RequestCursorShape(VTK_CURSOR_SIZEALL);
        }
      break;
    case vtkResliceCursorRepresentation::Outside:
    default:
      this->RequestCursorShape(VTK_CURSOR_DEFAULT);
    }
}

//-------------------------------------------------------------------------
void vtkResliceCursorWidget::ResizeThicknessAction(vtkAbstractWidget *w)
{
  vtkResliceCursorWidget *self = reinterpret_cast<vtkResliceCursorWidget*>(w);
  vtkResliceCursorRepresentation *rep =
    reinterpret_cast<vtkResliceCursorRepresentation*>(self->WidgetRep);

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  rep->ComputeInteractionState(X, Y, self->ModifierActive);

  if ( self->WidgetRep->GetInteractionState()
      == vtkResliceCursorRepresentation::Outside ||
      rep->GetResliceCursor()->GetThickMode() == 0 )
    {
    return;
    }

  rep->SetManipulationMode(vtkResliceCursorRepresentation::ResizeThickness);

  self->GrabFocus(self->EventCallbackCommand);
  double eventPos[2];
  eventPos[0] = static_cast<double>(X);
  eventPos[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(eventPos);

  // We are definitely selected
  self->WidgetState = vtkResliceCursorWidget::Active;
  self->SetCursor(self->WidgetRep->GetInteractionState());

  // Highlight as necessary
  self->WidgetRep->Highlight(1);

  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  self->Render();

  self->InvokeAnEvent();

  // Show the thickness in "mm"
  rep->ActivateText(1);
}

//-------------------------------------------------------------------------
void vtkResliceCursorWidget::EndResizeThicknessAction(vtkAbstractWidget *)
{
}

//-------------------------------------------------------------------------
void vtkResliceCursorWidget::SelectAction(vtkAbstractWidget *w)
{
  vtkResliceCursorWidget *self = reinterpret_cast<vtkResliceCursorWidget*>(w);
  vtkResliceCursorRepresentation *rep =
    reinterpret_cast<vtkResliceCursorRepresentation*>(self->WidgetRep);

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  self->ModifierActive = vtkEvent::GetModifier(self->Interactor);
  rep->ComputeInteractionState(X, Y, self->ModifierActive);

  if ( self->WidgetRep->GetInteractionState()
      == vtkResliceCursorRepresentation::Outside )
    {
    if (self->GetManageWindowLevel() && rep->GetShowReslicedImage())
      {
      self->StartWindowLevel();
      }
    else
      {
      rep->SetManipulationMode(vtkResliceCursorRepresentation::None);
      return;
      }
    }
  else
    {
    rep->SetManipulationMode(vtkResliceCursorRepresentation::PanAndRotate);
    }

  if (rep->GetManipulationMode() == vtkResliceCursorRepresentation::None)
    {
    return;
    }

  self->GrabFocus(self->EventCallbackCommand);
  double eventPos[2];
  eventPos[0] = static_cast<double>(X);
  eventPos[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(eventPos);

  // We are definitely selected
  self->WidgetState = vtkResliceCursorWidget::Active;
  self->SetCursor(self->WidgetRep->GetInteractionState());

  // Highlight as necessary
  self->WidgetRep->Highlight(1);

  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  self->Render();

  self->InvokeAnEvent();
}

//-------------------------------------------------------------------------
void vtkResliceCursorWidget::RotateAction(vtkAbstractWidget *w)
{
  vtkResliceCursorWidget *self = reinterpret_cast<vtkResliceCursorWidget*>(w);
  vtkResliceCursorRepresentation *rep =
    reinterpret_cast<vtkResliceCursorRepresentation*>(self->WidgetRep);

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  self->ModifierActive = vtkEvent::GetModifier(self->Interactor);
  rep->ComputeInteractionState(X, Y, self->ModifierActive);

  if ( self->WidgetRep->GetInteractionState()
      == vtkResliceCursorRepresentation::Outside )
    {
    return;
    }

  rep->SetManipulationMode(vtkResliceCursorRepresentation::RotateBothAxes);

  self->GrabFocus(self->EventCallbackCommand);
  double eventPos[2];
  eventPos[0] = static_cast<double>(X);
  eventPos[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(eventPos);

  // We are definitely selected
  self->WidgetState = vtkResliceCursorWidget::Active;
  self->SetCursor(self->WidgetRep->GetInteractionState());

  // Highlight as necessary
  self->WidgetRep->Highlight(1);

  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  self->Render();

  self->InvokeAnEvent();
}

//-------------------------------------------------------------------------
void vtkResliceCursorWidget::MoveAction(vtkAbstractWidget *w)
{
  vtkResliceCursorWidget *self = reinterpret_cast<vtkResliceCursorWidget*>(w);
  vtkResliceCursorRepresentation *rep =
    reinterpret_cast<vtkResliceCursorRepresentation*>(self->WidgetRep);

  // compute some info we need for all cases
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Set the cursor appropriately
  if ( self->WidgetState == vtkResliceCursorWidget::Start )
    {
    self->ModifierActive = vtkEvent::GetModifier(self->Interactor);
    int state = self->WidgetRep->GetInteractionState();

    rep->ComputeInteractionState(X, Y, self->ModifierActive );

    self->SetCursor(self->WidgetRep->GetInteractionState());

    if ( state != self->WidgetRep->GetInteractionState() )
      {
      self->Render();
      }
    return;
    }

  // Okay, adjust the representation

  double eventPosition[2];
  eventPosition[0] = static_cast<double>(X);
  eventPosition[1] = static_cast<double>(Y);
  self->WidgetRep->WidgetInteraction(eventPosition);

  // Got this event, we are finished
  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(vtkCommand::InteractionEvent,NULL);
  self->Render();

  self->InvokeAnEvent();
}

//-------------------------------------------------------------------------
void vtkResliceCursorWidget::EndSelectAction(vtkAbstractWidget *w)
{
  vtkResliceCursorWidget *self = static_cast<vtkResliceCursorWidget*>(w);
  vtkResliceCursorRepresentation *rep =
    reinterpret_cast<vtkResliceCursorRepresentation*>(self->WidgetRep);

  if ( self->WidgetState != vtkResliceCursorWidget::Active )
    {
    return;
    }

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  double eventPos[2];
  eventPos[0] = static_cast<double>(X);
  eventPos[1] = static_cast<double>(Y);

  self->WidgetRep->EndWidgetInteraction(eventPos);

  // return to initial state
  self->WidgetState = vtkResliceCursorWidget::Start;
  self->ModifierActive = 0;

  // Highlight as necessary
  self->WidgetRep->Highlight(0);

  // Remove any text displays. We are no longer active.
  rep->ActivateText(0);

  // stop adjusting
  self->ReleaseFocus();
  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  self->WidgetState = vtkResliceCursorWidget::Start;
  rep->SetManipulationMode(vtkResliceCursorRepresentation::None);

  self->Render();

  self->InvokeAnEvent();
}

//-------------------------------------------------------------------------
void vtkResliceCursorWidget::ResetResliceCursorAction(vtkAbstractWidget *w)
{
  vtkResliceCursorWidget *self = reinterpret_cast<vtkResliceCursorWidget*>(w);
  self->ResetResliceCursor();

  // Render in response to changes
  self->Render();

  // Invoke a reslice cursor event
  self->InvokeEvent( vtkResliceCursorWidget::ResetCursorEvent, NULL );
}

//-------------------------------------------------------------------------
void vtkResliceCursorWidget::ResetResliceCursor()
{
  vtkResliceCursorRepresentation *rep =
    reinterpret_cast<vtkResliceCursorRepresentation*>(this->WidgetRep);

  if (!rep->GetResliceCursor())
    {
    return; // nothing to reset
    }

  // Reset the reslice cursor
  rep->GetResliceCursor()->Reset();
  rep->InitializeReslicePlane();
}

//----------------------------------------------------------------------------
void vtkResliceCursorWidget::StartWindowLevel()
{
  vtkResliceCursorRepresentation *rep =
    reinterpret_cast<vtkResliceCursorRepresentation*>(this->WidgetRep);

  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!this->CurrentRenderer || !this->CurrentRenderer->IsInViewport(X, Y))
    {
    rep->SetManipulationMode(vtkResliceCursorRepresentation::None);
    return;
    }

  rep->SetManipulationMode(vtkResliceCursorRepresentation::WindowLevelling);

  rep->ActivateText(1);
  rep->ManageTextDisplay();
}

//----------------------------------------------------------------------------
void vtkResliceCursorWidget::InvokeAnEvent()
{
  // We invoke the appropriate event. In cases where the cursor is moved
  // around, or rotated, also have the reslice cursor invoke an event.

  vtkResliceCursorRepresentation *rep =
    reinterpret_cast<vtkResliceCursorRepresentation*>(this->WidgetRep);
  if (rep)
    {
    int mode = rep->GetManipulationMode();
    if (mode == vtkResliceCursorRepresentation::WindowLevelling)
      {
      this->InvokeEvent(WindowLevelEvent,NULL);
      }
    else if (mode == vtkResliceCursorRepresentation::PanAndRotate)
      {
      this->InvokeEvent(ResliceAxesChangedEvent,NULL);
      rep->GetResliceCursor()->InvokeEvent(ResliceAxesChangedEvent,NULL);
      }
    else if (mode == vtkResliceCursorRepresentation::RotateBothAxes)
      {
      this->InvokeEvent(ResliceAxesChangedEvent,NULL);
      rep->GetResliceCursor()->InvokeEvent(ResliceAxesChangedEvent,NULL);
      }
    else if (mode == vtkResliceCursorRepresentation::ResizeThickness)
      {
      this->InvokeEvent(ResliceThicknessChangedEvent,NULL);
      rep->GetResliceCursor()->InvokeEvent(ResliceAxesChangedEvent,NULL);
      }
    }
}

//----------------------------------------------------------------------------
void vtkResliceCursorWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ManageWindowLevel: " << this->ManageWindowLevel << endl;
  // this->ModifierActive;
  // this->WidgetState;

}
