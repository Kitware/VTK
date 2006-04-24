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


vtkCxxRevisionMacro(vtkImplicitPlaneWidget2, "1.1");
vtkStandardNewMacro(vtkImplicitPlaneWidget2);

//----------------------------------------------------------------------------
vtkImplicitPlaneWidget2::vtkImplicitPlaneWidget2()
{
  this->WidgetState = vtkImplicitPlaneWidget2::Start;
  this->ManagesCursor = 0;

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
  
  // We want to compute an orthogonal vector to the pane that has been selected
  reinterpret_cast<vtkImplicitPlaneRepresentation*>(self->WidgetRep)->
    SetInteractionState(vtkImplicitPlaneRepresentation::Moving);
  self->WidgetRep->ComputeInteractionState(X, Y);
  
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
  self->WidgetRep->ComputeInteractionState(X, Y);
  
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
  self->WidgetRep->ComputeInteractionState(X, Y);
  
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
}

//----------------------------------------------------------------------
void vtkImplicitPlaneWidget2::MoveAction(vtkAbstractWidget *w)
{
  vtkImplicitPlaneWidget2 *self = reinterpret_cast<vtkImplicitPlaneWidget2*>(w);

  // See whether we're active
  if ( self->WidgetState == vtkImplicitPlaneWidget2::Start )
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


//----------------------------------------------------------------------------
void vtkImplicitPlaneWidget2::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

}


