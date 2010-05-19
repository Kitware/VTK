/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitPlaneWidget2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImplicitPlaneWidget2.h"
#include "vtkImplicitPlaneRepresentation.h"
#include "vtkCommand.h"
#include "vtkCallbackCommand.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkObjectFactory.h"
#include "vtkWidgetEventTranslator.h"
#include "vtkWidgetCallbackMapper.h" 
#include "vtkEvent.h"
#include "vtkWidgetEvent.h"
#include "vtkRenderWindow.h"


vtkStandardNewMacro(vtkImplicitPlaneWidget2);

//----------------------------------------------------------------------------
vtkImplicitPlaneWidget2::vtkImplicitPlaneWidget2()
{
  this->WidgetState = vtkImplicitPlaneWidget2::Start;

  // Define widget events
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent,
                                          vtkWidgetEvent::Select,
                                          this, vtkImplicitPlaneWidget2::SelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
                                          vtkWidgetEvent::EndSelect,
                                          this, vtkImplicitPlaneWidget2::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonPressEvent,
                                          vtkWidgetEvent::Translate,
                                          this, vtkImplicitPlaneWidget2::TranslateAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonReleaseEvent,
                                          vtkWidgetEvent::EndTranslate,
                                          this, vtkImplicitPlaneWidget2::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonPressEvent,
                                          vtkWidgetEvent::Scale,
                                          this, vtkImplicitPlaneWidget2::ScaleAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonReleaseEvent,
                                          vtkWidgetEvent::EndScale,
                                          this, vtkImplicitPlaneWidget2::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MouseMoveEvent,
                                          vtkWidgetEvent::Move,
                                          this, vtkImplicitPlaneWidget2::MoveAction);
}

//----------------------------------------------------------------------------
vtkImplicitPlaneWidget2::~vtkImplicitPlaneWidget2()
{  
}

//----------------------------------------------------------------------
void vtkImplicitPlaneWidget2::SelectAction(vtkAbstractWidget *w)
{
  vtkImplicitPlaneWidget2 *self = reinterpret_cast<vtkImplicitPlaneWidget2*>(w);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  
  // We want to compute an orthogonal vector to the plane that has been selected
  reinterpret_cast<vtkImplicitPlaneRepresentation*>(self->WidgetRep)->
    SetInteractionState(vtkImplicitPlaneRepresentation::Moving);
  int interactionState = self->WidgetRep->ComputeInteractionState(X, Y);
  self->UpdateCursorShape(interactionState);
  
  if ( self->WidgetRep->GetInteractionState() == vtkImplicitPlaneRepresentation::Outside )
    {
    return;
    }
  
  // We are definitely selected
  self->GrabFocus(self->EventCallbackCommand);
  double eventPos[2];
  eventPos[0] = static_cast<double>(X);
  eventPos[1] = static_cast<double>(Y);
  self->WidgetState = vtkImplicitPlaneWidget2::Active;
  self->WidgetRep->StartWidgetInteraction(eventPos);

  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  self->Render();
}

//----------------------------------------------------------------------
void vtkImplicitPlaneWidget2::TranslateAction(vtkAbstractWidget *w)
{
  vtkImplicitPlaneWidget2 *self = reinterpret_cast<vtkImplicitPlaneWidget2*>(w);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  
  // We want to compute an orthogonal vector to the pane that has been selected
  reinterpret_cast<vtkImplicitPlaneRepresentation*>(self->WidgetRep)->
    SetInteractionState(vtkImplicitPlaneRepresentation::Moving);
  int interactionState = self->WidgetRep->ComputeInteractionState(X, Y);
  self->UpdateCursorShape(interactionState);
  
  if ( self->WidgetRep->GetInteractionState() == vtkImplicitPlaneRepresentation::Outside )
    {
    return;
    }
  
  // We are definitely selected
  self->GrabFocus(self->EventCallbackCommand);
  double eventPos[2];
  eventPos[0] = static_cast<double>(X);
  eventPos[1] = static_cast<double>(Y);
  self->WidgetState = vtkImplicitPlaneWidget2::Active;
  self->WidgetRep->StartWidgetInteraction(eventPos);

  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  self->Render();
}

//----------------------------------------------------------------------
void vtkImplicitPlaneWidget2::ScaleAction(vtkAbstractWidget *w)
{
  vtkImplicitPlaneWidget2 *self = reinterpret_cast<vtkImplicitPlaneWidget2*>(w);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  
  // We want to compute an orthogonal vector to the pane that has been selected
  reinterpret_cast<vtkImplicitPlaneRepresentation*>(self->WidgetRep)->
    SetInteractionState(vtkImplicitPlaneRepresentation::Scaling);
  int interactionState = self->WidgetRep->ComputeInteractionState(X, Y);
  self->UpdateCursorShape(interactionState);
  
  if ( self->WidgetRep->GetInteractionState() == vtkImplicitPlaneRepresentation::Outside )
    {
    return;
    }
  
  // We are definitely selected
  self->GrabFocus(self->EventCallbackCommand);
  double eventPos[2];
  eventPos[0] = static_cast<double>(X);
  eventPos[1] = static_cast<double>(Y);
  self->WidgetState = vtkImplicitPlaneWidget2::Active;
  self->WidgetRep->StartWidgetInteraction(eventPos);

  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  self->Render();
}

//----------------------------------------------------------------------
void vtkImplicitPlaneWidget2::MoveAction(vtkAbstractWidget *w)
{
  vtkImplicitPlaneWidget2 *self = reinterpret_cast<vtkImplicitPlaneWidget2*>(w);

  // So as to change the cursor shape when the mouse is poised over
  // the widget. Unfortunately, this results in a few extra picks
  // due to the cell picker. However given that its picking planes
  // and the handles/arrows, this should be very quick
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  int changed = 0;

  if (self->ManagesCursor)
    {
    int oldInteractionState = reinterpret_cast<vtkImplicitPlaneRepresentation*>(
        self->WidgetRep)->GetInteractionState();

    reinterpret_cast<vtkImplicitPlaneRepresentation*>(self->WidgetRep)->
      SetInteractionState(vtkImplicitPlaneRepresentation::Moving);
    int state = self->WidgetRep->ComputeInteractionState( X, Y );
    changed = self->UpdateCursorShape(state);
    reinterpret_cast<vtkImplicitPlaneRepresentation*>(self->WidgetRep)->
      SetInteractionState(oldInteractionState);
    }

  // See whether we're active
  if ( self->WidgetState == vtkImplicitPlaneWidget2::Start )
    {
    if (changed && self->ManagesCursor)
      {
      self->Render();
      }
    return;
    }
  
  // Okay, adjust the representation
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  self->WidgetRep->WidgetInteraction(e);

  // moving something
  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(vtkCommand::InteractionEvent,NULL);
  self->Render();
}

//----------------------------------------------------------------------
void vtkImplicitPlaneWidget2::EndSelectAction(vtkAbstractWidget *w)
{
  vtkImplicitPlaneWidget2 *self = reinterpret_cast<vtkImplicitPlaneWidget2*>(w);

  if ( self->WidgetState != vtkImplicitPlaneWidget2::Active  ||
       self->WidgetRep->GetInteractionState() == vtkImplicitPlaneRepresentation::Outside )
    {
    return;
    }
  
  // Return state to not selected
  double e[2];
  self->WidgetRep->EndWidgetInteraction(e);
  self->WidgetState = vtkImplicitPlaneWidget2::Start;
  self->ReleaseFocus();

  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  self->Render();
}

//----------------------------------------------------------------------
void vtkImplicitPlaneWidget2::CreateDefaultRepresentation()
{
  if ( ! this->WidgetRep )
    {
    this->WidgetRep = vtkImplicitPlaneRepresentation::New();
    }
}

//----------------------------------------------------------------------
int vtkImplicitPlaneWidget2::UpdateCursorShape( int state )
{
  // So as to change the cursor shape when the mouse is poised over
  // the widget.
  if (this->ManagesCursor)
    {
    if (state == vtkImplicitPlaneRepresentation::Outside)
      {
      return this->RequestCursorShape(VTK_CURSOR_DEFAULT);
      }
    else if (state == vtkImplicitPlaneRepresentation::MovingOutline)
      {
      return this->RequestCursorShape(VTK_CURSOR_SIZEALL);
      }
    else
      {
      return this->RequestCursorShape(VTK_CURSOR_HAND);
      }  
    }

  return 0;
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneWidget2::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

}


