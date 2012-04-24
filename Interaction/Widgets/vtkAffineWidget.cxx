/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAffineWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAffineWidget.h"
#include "vtkAffineRepresentation2D.h"
#include "vtkCommand.h"
#include "vtkCallbackCommand.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkWidgetEventTranslator.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkEvent.h"
#include "vtkWidgetEvent.h"

vtkStandardNewMacro(vtkAffineWidget);

//----------------------------------------------------------------------------------
vtkAffineWidget::vtkAffineWidget()
{
  // Set the initial state
  this->WidgetState = vtkAffineWidget::Start;

  this->ModifierActive = 0;

  // Okay, define the events for this widget
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent,
                                          vtkWidgetEvent::Select,
                                          this, vtkAffineWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
                                          vtkWidgetEvent::EndSelect,
                                          this, vtkAffineWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MouseMoveEvent,
                                          vtkWidgetEvent::Move,
                                          this, vtkAffineWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent,
                                          vtkWidgetEvent::ModifyEvent,
                                          this, vtkAffineWidget::ModifyEventAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent,
                                          vtkWidgetEvent::ModifyEvent,
                                          this, vtkAffineWidget::ModifyEventAction);
}

//----------------------------------------------------------------------------------
vtkAffineWidget::~vtkAffineWidget()
{
}

//----------------------------------------------------------------------
void vtkAffineWidget::SetEnabled(int enabling)
{
  this->Superclass::SetEnabled(enabling);
}

//----------------------------------------------------------------------
void vtkAffineWidget::CreateDefaultRepresentation()
{
  if ( ! this->WidgetRep )
    {
    this->WidgetRep = vtkAffineRepresentation2D::New();
    }
}

//-------------------------------------------------------------------------
void vtkAffineWidget::SetCursor(int cState)
{
  switch (cState)
    {
    case vtkAffineRepresentation::ScaleNE: case vtkAffineRepresentation::ScaleSW:
      this->RequestCursorShape(VTK_CURSOR_SIZESW);
      break;
    case vtkAffineRepresentation::ScaleNW: case vtkAffineRepresentation::ScaleSE:
      this->RequestCursorShape(VTK_CURSOR_SIZENW);
      break;
    case vtkAffineRepresentation::ScaleNEdge: case vtkAffineRepresentation::ScaleSEdge:
    case vtkAffineRepresentation::ShearWEdge: case vtkAffineRepresentation::ShearEEdge:
      this->RequestCursorShape(VTK_CURSOR_SIZENS);
      break;
    case vtkAffineRepresentation::ScaleWEdge: case vtkAffineRepresentation::ScaleEEdge:
    case vtkAffineRepresentation::ShearNEdge: case vtkAffineRepresentation::ShearSEdge:
      this->RequestCursorShape(VTK_CURSOR_SIZEWE);
      break;
    case vtkAffineRepresentation::Rotate:
      this->RequestCursorShape(VTK_CURSOR_HAND);
      break;
    case vtkAffineRepresentation::TranslateX: case vtkAffineRepresentation::MoveOriginX:
      this->RequestCursorShape(VTK_CURSOR_SIZEWE);
      break;
    case vtkAffineRepresentation::TranslateY: case vtkAffineRepresentation::MoveOriginY:
      this->RequestCursorShape(VTK_CURSOR_SIZENS);
      break;
    case vtkAffineRepresentation::Translate: case vtkAffineRepresentation::MoveOrigin:
      this->RequestCursorShape(VTK_CURSOR_SIZEALL);
      break;
    case vtkAffineRepresentation::Outside:
    default:
      this->RequestCursorShape(VTK_CURSOR_DEFAULT);
    }
}

//-------------------------------------------------------------------------
void vtkAffineWidget::SelectAction(vtkAbstractWidget *w)
{
  vtkAffineWidget *self = reinterpret_cast<vtkAffineWidget*>(w);

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  self->ModifierActive = self->Interactor->GetShiftKey() |
                         self->Interactor->GetControlKey();
  reinterpret_cast<vtkAffineRepresentation*>(self->WidgetRep)->
    ComputeInteractionState(X, Y, self->ModifierActive);

  if ( self->WidgetRep->GetInteractionState() == vtkAffineRepresentation::Outside )
    {
    return;
    }

  self->GrabFocus(self->EventCallbackCommand);
  double eventPos[2];
  eventPos[0] = static_cast<double>(X);
  eventPos[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(eventPos);

  // We are definitely selected
  self->WidgetState = vtkAffineWidget::Active;
  self->SetCursor(self->WidgetRep->GetInteractionState());

  // Highlight as necessary
  self->WidgetRep->Highlight(1);

  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  self->Render();
}

//-------------------------------------------------------------------------
void vtkAffineWidget::MoveAction(vtkAbstractWidget *w)
{
  vtkAffineWidget *self = reinterpret_cast<vtkAffineWidget*>(w);

  // compute some info we need for all cases
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Set the cursor appropriately
  if ( self->WidgetState == vtkAffineWidget::Start )
    {
    self->ModifierActive = self->Interactor->GetShiftKey() |
                           self->Interactor->GetControlKey();
    int state = self->WidgetRep->GetInteractionState();
    reinterpret_cast<vtkAffineRepresentation*>(self->WidgetRep)->
      ComputeInteractionState(X, Y, self->ModifierActive );
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
}

//-------------------------------------------------------------------------
void vtkAffineWidget::ModifyEventAction(vtkAbstractWidget *w)
{
  vtkAffineWidget *self = reinterpret_cast<vtkAffineWidget*>(w);
  if ( self->WidgetState == vtkAffineWidget::Start )
    {
    int modifierActive = self->Interactor->GetShiftKey() |
                         self->Interactor->GetControlKey();
    if ( self->ModifierActive != modifierActive )
      {
      self->ModifierActive = modifierActive;
      int X = self->Interactor->GetEventPosition()[0];
      int Y = self->Interactor->GetEventPosition()[1];
      reinterpret_cast<vtkAffineRepresentation*>(self->WidgetRep)->
        ComputeInteractionState(X, Y, self->ModifierActive );
      self->SetCursor(self->WidgetRep->GetInteractionState());
      }
    }
}


//-------------------------------------------------------------------------
void vtkAffineWidget::EndSelectAction(vtkAbstractWidget *w)
{
  vtkAffineWidget *self = reinterpret_cast<vtkAffineWidget*>(w);

  if ( self->WidgetState != vtkAffineWidget::Active )
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
  self->WidgetState = vtkAffineWidget::Start;
  self->ModifierActive = 0;

  // Highlight as necessary
  self->WidgetRep->Highlight(0);

  // stop adjusting
  self->EventCallbackCommand->SetAbortFlag(1);
  self->ReleaseFocus();
  self->EndInteraction();
  self->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  self->WidgetState = vtkAffineWidget::Start;
  self->Render();
}

//----------------------------------------------------------------------------------
void vtkAffineWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);

}
