/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkButtonWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkButtonWidget.h"
#include "vtkButtonRepresentation.h"
#include "vtkTexturedButtonRepresentation.h"
#include "vtkCommand.h"
#include "vtkCallbackCommand.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkWidgetEventTranslator.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkEvent.h"
#include "vtkWidgetEvent.h"
#include "vtkRenderWindow.h"

vtkStandardNewMacro(vtkButtonWidget);

//----------------------------------------------------------------------------------
vtkButtonWidget::vtkButtonWidget()
{
  // Set the initial state
  this->WidgetState = vtkButtonWidget::Start;

  // Okay, define the events
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent,
                                          vtkWidgetEvent::Select,
                                          this, vtkButtonWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MouseMoveEvent,
                                          vtkWidgetEvent::Move,
                                          this, vtkButtonWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
                                          vtkWidgetEvent::EndSelect,
                                          this, vtkButtonWidget::EndSelectAction);
}

//----------------------------------------------------------------------
void vtkButtonWidget::CreateDefaultRepresentation()
{
  if ( ! this->WidgetRep )
  {
    this->WidgetRep = vtkTexturedButtonRepresentation::New();
  }
}


//----------------------------------------------------------------------------------
void vtkButtonWidget::MoveAction(vtkAbstractWidget *w)
{
  vtkButtonWidget *self = reinterpret_cast<vtkButtonWidget*>(w);

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Motion while selecting is ignored
  if ( self->WidgetState == vtkButtonWidget::Selecting )
  {
    self->EventCallbackCommand->SetAbortFlag(1);
    return;
  }

  // Get the new state and compare it to the old
  int renderRequired = 0;
  int state = self->WidgetRep->ComputeInteractionState(X, Y);
  if ( self->WidgetState == vtkButtonWidget::Hovering )
  {
    if ( state == vtkButtonRepresentation::Outside )
    {
      renderRequired = 1;
      if ( self->ManagesCursor )
      {
        self->RequestCursorShape(VTK_CURSOR_DEFAULT);
      }
      self->WidgetRep->Highlight(vtkButtonRepresentation::HighlightNormal);
      self->WidgetState = vtkButtonWidget::Start;
    }
  }
  else //state is Start
  {
    if ( state == vtkButtonRepresentation::Inside )
    {
      renderRequired = 1;
      if ( self->ManagesCursor )
      {
        self->RequestCursorShape(VTK_CURSOR_HAND);
      }
      self->WidgetRep->Highlight(vtkButtonRepresentation::HighlightHovering);
      self->WidgetState = vtkButtonWidget::Hovering;
      self->EventCallbackCommand->SetAbortFlag(1);
    }
  }

  if ( renderRequired )
  {
    self->Render();
  }
}

//----------------------------------------------------------------------------------
void vtkButtonWidget::SelectAction(vtkAbstractWidget *w)
{
  vtkButtonWidget *self = reinterpret_cast<vtkButtonWidget*>(w);

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // The state must be hovering for anything to happen. MoveAction sets the
  // state.
  if ( self->WidgetState != vtkButtonWidget::Hovering )
  {
    return;
  }

  // Okay, make sure that the selection is in the current renderer
  if (!self->CurrentRenderer || !self->CurrentRenderer->IsInViewport(X,Y))
  {
    self->WidgetState = vtkButtonWidget::Start;
    return;
  }

  // We are definitely selected, Highlight as necessary.
  self->WidgetState = vtkButtonWidget::Selecting;
  self->WidgetRep->Highlight(vtkButtonRepresentation::HighlightSelecting);
  self->EventCallbackCommand->SetAbortFlag(1);
  self->Render();
}


//----------------------------------------------------------------------------------
void vtkButtonWidget::EndSelectAction(vtkAbstractWidget *w)
{
  vtkButtonWidget *self = reinterpret_cast<vtkButtonWidget*>(w);

  if ( self->WidgetState != vtkButtonWidget::Selecting )
  {
    return;
  }

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  int state = self->WidgetRep->ComputeInteractionState(X, Y);
  if ( state == vtkButtonRepresentation::Outside )
  {
    if ( self->ManagesCursor )
    {
      self->RequestCursorShape(VTK_CURSOR_DEFAULT);
    }
    self->WidgetRep->Highlight(vtkButtonRepresentation::HighlightNormal);
    self->WidgetState = vtkButtonWidget::Start;
  }
  else //state == vtkButtonRepresentation::Inside
  {
    if ( self->ManagesCursor )
    {
      self->RequestCursorShape(VTK_CURSOR_HAND);
    }
    self->WidgetRep->Highlight(vtkButtonRepresentation::HighlightHovering);
    self->WidgetState = vtkButtonWidget::Hovering;
  }

  // Complete interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  reinterpret_cast<vtkButtonRepresentation*>(self->WidgetRep)->NextState();
  self->InvokeEvent(vtkCommand::StateChangedEvent,NULL);
  self->Render();
}


//----------------------------------------------------------------------------------
void vtkButtonWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);

}
