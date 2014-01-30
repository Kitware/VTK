/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSphereWidget2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSphereWidget2.h"
#include "vtkSphereRepresentation.h"
#include "vtkCommand.h"
#include "vtkCallbackCommand.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkObjectFactory.h"
#include "vtkWidgetEventTranslator.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkEvent.h"
#include "vtkWidgetEvent.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"


vtkStandardNewMacro(vtkSphereWidget2);

//----------------------------------------------------------------------------
vtkSphereWidget2::vtkSphereWidget2()
{
  this->WidgetState = vtkSphereWidget2::Start;
  this->ManagesCursor = 1;

  this->TranslationEnabled = 1;
  this->ScalingEnabled = 1;

  // Define widget events
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent,
                                          vtkWidgetEvent::Select,
                                          this, vtkSphereWidget2::SelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
                                          vtkWidgetEvent::EndSelect,
                                          this, vtkSphereWidget2::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonPressEvent,
                                          vtkWidgetEvent::Translate,
                                          this, vtkSphereWidget2::TranslateAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonReleaseEvent,
                                          vtkWidgetEvent::EndTranslate,
                                          this, vtkSphereWidget2::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonPressEvent,
                                          vtkWidgetEvent::Scale,
                                          this, vtkSphereWidget2::ScaleAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonReleaseEvent,
                                          vtkWidgetEvent::EndScale,
                                          this, vtkSphereWidget2::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MouseMoveEvent,
                                          vtkWidgetEvent::Move,
                                          this, vtkSphereWidget2::MoveAction);
}

//----------------------------------------------------------------------------
vtkSphereWidget2::~vtkSphereWidget2()
{
}

//----------------------------------------------------------------------
void vtkSphereWidget2::SelectAction(vtkAbstractWidget *w)
{
  // We are in a static method, cast to ourself
  vtkSphereWidget2 *self = reinterpret_cast<vtkSphereWidget2*>(w);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if ( !self->CurrentRenderer ||
       !self->CurrentRenderer->IsInViewport(X,Y) )
    {
    self->WidgetState = vtkSphereWidget2::Start;
    return;
    }

  // Begin the widget interaction which has the side effect of setting the
  // interaction state.
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(e);
  int interactionState = self->WidgetRep->GetInteractionState();
  if ( interactionState == vtkSphereRepresentation::Outside )
    {
    return;
    }

  // We are definitely selected
  self->WidgetState = vtkSphereWidget2::Active;
  self->GrabFocus(self->EventCallbackCommand);

  // Modifier keys force us into translare mode
  // The SetInteractionState has the side effect of highlighting the widget
  if ( interactionState == vtkSphereRepresentation::OnSphere ||
       self->Interactor->GetShiftKey() || self->Interactor->GetControlKey() )
    {
    // If  translation is disabled, do it
    if ( self->TranslationEnabled )
      {
      reinterpret_cast<vtkSphereRepresentation*>(self->WidgetRep)->
        SetInteractionState(vtkSphereRepresentation::Translating);
      }
    }
  else
    {
    reinterpret_cast<vtkSphereRepresentation*>(self->WidgetRep)->
      SetInteractionState(interactionState);
    }

  // start the interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  self->Render();
}

//----------------------------------------------------------------------
void vtkSphereWidget2::TranslateAction(vtkAbstractWidget *w)
{
  // We are in a static method, cast to ourself
  vtkSphereWidget2 *self = reinterpret_cast<vtkSphereWidget2*>(w);

  // If  translation is disabled, get out of here
  if ( ! self->TranslationEnabled )
    {
    return;
    }

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if ( !self->CurrentRenderer ||
       !self->CurrentRenderer->IsInViewport(X,Y) )
    {
    self->WidgetState = vtkSphereWidget2::Start;
    return;
    }

  // Begin the widget interaction which has the side effect of setting the
  // interaction state.
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(e);
  int interactionState = self->WidgetRep->GetInteractionState();
  if ( interactionState == vtkSphereRepresentation::Outside )
    {
    return;
    }

  // We are definitely selected
  self->WidgetState = vtkSphereWidget2::Active;
  self->GrabFocus(self->EventCallbackCommand);
  reinterpret_cast<vtkSphereRepresentation*>(self->WidgetRep)->
    SetInteractionState(vtkSphereRepresentation::Translating);

  // start the interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  self->Render();
}

//----------------------------------------------------------------------
void vtkSphereWidget2::ScaleAction(vtkAbstractWidget *w)
{
  // We are in a static method, cast to ourself
  vtkSphereWidget2 *self = reinterpret_cast<vtkSphereWidget2*>(w);

  // If  scaling is disabled, get out of here
  if ( ! self->ScalingEnabled )
    {
    return;
    }

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if ( !self->CurrentRenderer ||
       !self->CurrentRenderer->IsInViewport(X,Y) )
    {
    self->WidgetState = vtkSphereWidget2::Start;
    return;
    }

  // Begin the widget interaction which has the side effect of setting the
  // interaction state.
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(e);
  int interactionState = self->WidgetRep->GetInteractionState();
  if ( interactionState == vtkSphereRepresentation::Outside )
    {
    return;
    }

  // We are definitely selected
  self->WidgetState = vtkSphereWidget2::Active;
  self->GrabFocus(self->EventCallbackCommand);
  reinterpret_cast<vtkSphereRepresentation*>(self->WidgetRep)->
    SetInteractionState(vtkSphereRepresentation::Scaling);

  // start the interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  self->Render();
}

//----------------------------------------------------------------------
void vtkSphereWidget2::MoveAction(vtkAbstractWidget *w)
{
  vtkSphereWidget2 *self = reinterpret_cast<vtkSphereWidget2*>(w);

  // See whether we're active
  if ( self->WidgetState == vtkSphereWidget2::Start )
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
void vtkSphereWidget2::EndSelectAction(vtkAbstractWidget *w)
{
  vtkSphereWidget2 *self = reinterpret_cast<vtkSphereWidget2*>(w);
  if ( self->WidgetState == vtkSphereWidget2::Start )
    {
    return;
    }

  // Return state to not active
  self->WidgetState = vtkSphereWidget2::Start;
  reinterpret_cast<vtkSphereRepresentation*>(self->WidgetRep)->
    SetInteractionState(vtkSphereRepresentation::Outside);
  self->ReleaseFocus();

  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  self->Render();
}

//----------------------------------------------------------------------
void vtkSphereWidget2::CreateDefaultRepresentation()
{
  if ( ! this->WidgetRep )
    {
    this->WidgetRep = vtkSphereRepresentation::New();
    }
}

//----------------------------------------------------------------------------
void vtkSphereWidget2::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Translation Enabled: " << (this->TranslationEnabled ? "On\n" : "Off\n");
  os << indent << "Scaling Enabled: " << (this->ScalingEnabled ? "On\n" : "Off\n");
}


