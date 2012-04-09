/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParallelopipedWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkParallelopipedWidget.h"
#include "vtkParallelopipedRepresentation.h"
#include "vtkActor.h"
#include "vtkCallbackCommand.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkRenderWindow.h"
#include "vtkCommand.h"
#include "vtkWidgetEventTranslator.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkEvent.h"
#include "vtkWidgetEvent.h"
#include "vtkInteractorObserver.h"
#include "vtkHandleWidget.h"
#include "vtkWidgetSet.h"
#include "vtkGarbageCollector.h"

vtkStandardNewMacro(vtkParallelopipedWidget);

//----------------------------------------------------------------------
vtkParallelopipedWidget::vtkParallelopipedWidget()
{
  // Allow chairs to be created.
  this->EnableChairCreation = 1;

  // 8 handles for the 8 corners of the piped.
  this->HandleWidgets = new vtkHandleWidget* [8];
  for (int i=0; i<8; i++)
    {
    this->HandleWidgets[i] = vtkHandleWidget::New();

    // The widget gets a higher priority than the handles.
    this->HandleWidgets[i]->SetPriority(this->Priority-0.01);
    this->HandleWidgets[i]->SetParent(this);

    // The piped widget will decide what cursor to show.
    this->HandleWidgets[i]->ManagesCursorOff();
    }

  // Define widget events
  this->CallbackMapper->SetCallbackMethod(
            vtkCommand::LeftButtonPressEvent,
            vtkEvent::NoModifier, 0, 1, NULL,
            vtkParallelopipedWidget::RequestResizeEvent,
            this, vtkParallelopipedWidget::RequestResizeAlongAnAxisCallback);
// Commented out by Will because it is unstable code
//            this, vtkParallelopipedWidget::RequestResizeCallback);
  this->CallbackMapper->SetCallbackMethod(
            vtkCommand::LeftButtonPressEvent,
            vtkEvent::ShiftModifier, 0, 1, NULL,
            vtkParallelopipedWidget::RequestResizeAlongAnAxisEvent,
            this, vtkParallelopipedWidget::RequestResizeAlongAnAxisCallback);
  this->CallbackMapper->SetCallbackMethod(
            vtkCommand::LeftButtonPressEvent,
            vtkEvent::ControlModifier, 0, 1, NULL,
            vtkParallelopipedWidget::RequestChairModeEvent,
            this, vtkParallelopipedWidget::RequestChairModeCallback);
  this->CallbackMapper->SetCallbackMethod(
            vtkCommand::LeftButtonReleaseEvent,
            vtkWidgetEvent::EndSelect,
            this, vtkParallelopipedWidget::OnLeftButtonUpCallback);
  this->CallbackMapper->SetCallbackMethod(
            vtkCommand::MouseMoveEvent,
            vtkWidgetEvent::Move,
            this, vtkParallelopipedWidget::OnMouseMoveCallback);

  this->WidgetSet = NULL;
}

//----------------------------------------------------------------------
vtkParallelopipedWidget::~vtkParallelopipedWidget()
{
  for (int i=0; i<8; i++)
    {
    this->HandleWidgets[i]->Delete();
    }
  delete [] this->HandleWidgets;
}

//----------------------------------------------------------------------
void vtkParallelopipedWidget::CreateDefaultRepresentation()
{
  if ( !this->WidgetRep )
    {
    this->WidgetRep = vtkParallelopipedRepresentation::New();
    this->WidgetRep->SetRenderer(this->CurrentRenderer);
    }
}

//----------------------------------------------------------------------
void vtkParallelopipedWidget::SetEnabled(int enabling)
{
  if ( enabling ) //----------------
    {
    vtkDebugMacro(<<"Enabling widget");

    if ( this->Enabled ) //already enabled, just return
      {
      return;
      }

    if ( ! this->Interactor )
      {
      vtkErrorMacro(<<"The interactor must be set prior to enabling the widget");
      return;
      }

    int X=this->Interactor->GetEventPosition()[0];
    int Y=this->Interactor->GetEventPosition()[1];

    if ( ! this->CurrentRenderer )
      {
      this->SetCurrentRenderer(this->Interactor->FindPokedRenderer(X,Y));

      if (this->CurrentRenderer == NULL)
        {
        return;
        }
      }

    // We're ready to enable
    this->Enabled = 1;
    this->CreateDefaultRepresentation();
    this->WidgetRep->SetRenderer(this->CurrentRenderer);

    // listen for the events found in the EventTranslator
    if ( ! this->Parent )
      {
      this->EventTranslator->AddEventsToInteractor(this->Interactor,
        this->EventCallbackCommand,this->Priority);
      }
    else
      {
      this->EventTranslator->AddEventsToParent(this->Parent,
        this->EventCallbackCommand,this->Priority);
      }

    // Enable each of the handle widgets.
    for(int i=0; i<8; i++)
      {
      if(this->HandleWidgets[i])
        {
        this->HandleWidgets[i]->SetRepresentation(
          vtkParallelopipedRepresentation::SafeDownCast
          (this->WidgetRep)->GetHandleRepresentation(i));
        this->HandleWidgets[i]->SetInteractor(this->Interactor);
        this->HandleWidgets[i]->GetRepresentation()->SetRenderer(
          this->CurrentRenderer);

        this->HandleWidgets[i]->SetEnabled(enabling);

        }
      }

    if ( this->ManagesCursor )
      {
      this->WidgetRep->ComputeInteractionState(X, Y);
      this->SetCursor(this->WidgetRep->GetInteractionState());
      }

    this->WidgetRep->BuildRepresentation();
    this->CurrentRenderer->AddViewProp(this->WidgetRep);

    this->InvokeEvent(vtkCommand::EnableEvent,NULL);
    }

  else //disabling------------------
    {
    vtkDebugMacro(<<"Disabling widget");

    if ( ! this->Enabled ) //already disabled, just return
      {
      return;
      }

    this->Enabled = 0;

    // don't listen for events any more
    if ( ! this->Parent )
      {
      this->Interactor->RemoveObserver(this->EventCallbackCommand);
      }
    else
      {
      this->Parent->RemoveObserver(this->EventCallbackCommand);
      }

    // Disable each of the handle widgets.
    for(int i=0; i<8; i++)
      {
      if(this->HandleWidgets[i])
        {
        this->HandleWidgets[i]->SetEnabled(enabling);
        }
      }

    this->CurrentRenderer->RemoveViewProp(this->WidgetRep);

    this->InvokeEvent(vtkCommand::DisableEvent,NULL);
    this->SetCurrentRenderer(NULL);
    }

  // Should only render if there is no parent
  if ( this->Interactor && !this->Parent )
    {
    this->Interactor->Render();
    }
}

//----------------------------------------------------------------------
void vtkParallelopipedWidget::RequestResizeCallback(vtkAbstractWidget *w)
{
  vtkParallelopipedWidget *self = reinterpret_cast<vtkParallelopipedWidget*>(w);
  vtkParallelopipedRepresentation *rep =
    reinterpret_cast<vtkParallelopipedRepresentation*>(self->WidgetRep);

  const int modifier = self->Interactor->GetShiftKey()   |
                       self->Interactor->GetControlKey() |
                       self->Interactor->GetAltKey();

  // This interaction could potentially select a handle, if we are close to
  // one. Lets make a request on the representation and see what it says.
  rep->SetInteractionState(
      vtkParallelopipedRepresentation::RequestResizeParallelopiped );

  // Let the representation decide what the appropriate state is.
  int interactionState = rep->ComputeInteractionState(
      self->Interactor->GetEventPosition()[0],
      self->Interactor->GetEventPosition()[1], modifier );
  self->SetCursor(interactionState);

  if (interactionState != vtkParallelopipedRepresentation::Outside)
    {
    self->EventCallbackCommand->SetAbortFlag(1);
    self->StartInteraction();
    self->InvokeEvent(vtkCommand::StartInteractionEvent, NULL);
    self->Interactor->Render();
    }
}

//----------------------------------------------------------------------
void vtkParallelopipedWidget
::RequestResizeAlongAnAxisCallback(vtkAbstractWidget *w)
{
  vtkParallelopipedWidget *self = reinterpret_cast<vtkParallelopipedWidget*>(w);
  vtkParallelopipedRepresentation *rep =
    reinterpret_cast<vtkParallelopipedRepresentation*>(self->WidgetRep);

  const int modifier = self->Interactor->GetShiftKey()   |
                       self->Interactor->GetControlKey() |
                       self->Interactor->GetAltKey();

  // This interaction could potentially select a handle, if we are close to
  // one. Lets make a request on the representation and see what it says.
  rep->SetInteractionState(
    vtkParallelopipedRepresentation::RequestResizeParallelopipedAlongAnAxis );

  // Let the representation decide what the appropriate state is.
  int interactionState = rep->ComputeInteractionState(
      self->Interactor->GetEventPosition()[0],
      self->Interactor->GetEventPosition()[1], modifier );

  self->SetCursor(interactionState);

  if (interactionState == vtkParallelopipedRepresentation::Inside)
    {
    // We did not select any of the handles, nevertheless we are at least
    // inside the parallelopiped. We could do things like Translate etc. So
    // we will delegate responsibility to those callbacks
    self->TranslateCallback( self );
    }

  else if (interactionState != vtkParallelopipedRepresentation::Outside)
    {
    self->EventCallbackCommand->SetAbortFlag(1);
    self->StartInteraction();
    self->InvokeEvent(vtkCommand::StartInteractionEvent, NULL);
    self->Interactor->Render();
    }
}

//----------------------------------------------------------------------
void vtkParallelopipedWidget::RequestChairModeCallback(vtkAbstractWidget *w)
{
  vtkParallelopipedWidget *self = reinterpret_cast<vtkParallelopipedWidget*>(w);

  if ( ! self->EnableChairCreation )
    {
    return;
    }

  vtkParallelopipedRepresentation *rep =
    reinterpret_cast<vtkParallelopipedRepresentation*>(self->WidgetRep);

  const int modifier = self->Interactor->GetShiftKey()   |
                       self->Interactor->GetControlKey() |
                       self->Interactor->GetAltKey();

  // This interaction could potentially select a handle, if we are close to
  // one. Lets make a request on the representation and see what it says.
  rep->SetInteractionState(
    vtkParallelopipedRepresentation::RequestChairMode );

  // Let the representation decide what the appropriate state is.
  int interactionState = rep->ComputeInteractionState(
      self->Interactor->GetEventPosition()[0],
      self->Interactor->GetEventPosition()[1], modifier );
  self->SetCursor(interactionState);

  if (interactionState != vtkParallelopipedRepresentation::Outside)
    {
    // Ok, so we did select a handle.... Render..

    self->EventCallbackCommand->SetAbortFlag(1);
    self->StartInteraction();
    self->InvokeEvent(vtkCommand::StartInteractionEvent, NULL);
    self->Interactor->Render();
    }
}

//----------------------------------------------------------------------
void vtkParallelopipedWidget::TranslateCallback(vtkAbstractWidget *w)
{
  vtkParallelopipedWidget *self = reinterpret_cast<vtkParallelopipedWidget*>(w);
  vtkParallelopipedRepresentation *rep =
    reinterpret_cast<vtkParallelopipedRepresentation*>(self->WidgetRep);

  // We know we are inside the parallelopiped.
  // Change the cursor to the Translate thingie.
  self->SetCursor( vtkParallelopipedRepresentation::TranslatingParallelopiped );
  rep->SetInteractionState( vtkParallelopipedRepresentation::TranslatingParallelopiped );

  // Dispatch to all widgets in the set.
  if (self->WidgetSet)
    {
    self->WidgetSet->DispatchAction(
      self, &vtkParallelopipedWidget::BeginTranslateAction);
    }
  else
    {
    self->BeginTranslateAction(self);
    }
}

//----------------------------------------------------------------------
void vtkParallelopipedWidget
::BeginTranslateAction(vtkParallelopipedWidget *vtkNotUsed(dispatcher))
{
  vtkParallelopipedRepresentation *rep =
    reinterpret_cast<vtkParallelopipedRepresentation*>(this->WidgetRep);

  // We know we are inside the parallelopiped.
  // Change the cursor to the Translate thingie.
  rep->SetInteractionState( vtkParallelopipedRepresentation::
                                  TranslatingParallelopiped );
  this->SetCursor( rep->GetInteractionState() );

  this->EventCallbackCommand->SetAbortFlag(1);
  this->StartInteraction();
  this->InvokeEvent(vtkCommand::StartInteractionEvent, NULL);
  this->Interactor->Render();
}

//----------------------------------------------------------------------
void vtkParallelopipedWidget
::TranslateAction(vtkParallelopipedWidget *vtkNotUsed(dispatcher))
{
  vtkParallelopipedRepresentation *rep =
    reinterpret_cast<vtkParallelopipedRepresentation*>(this->WidgetRep);
  rep->Translate( this->Interactor->GetEventPosition()[0],
                  this->Interactor->GetEventPosition()[1] );
}

//----------------------------------------------------------------------
void vtkParallelopipedWidget::OnLeftButtonUpCallback(vtkAbstractWidget *w)
{
  vtkParallelopipedWidget *self = reinterpret_cast<vtkParallelopipedWidget*>(w);
  vtkParallelopipedRepresentation *rep =
    reinterpret_cast<vtkParallelopipedRepresentation*>(self->WidgetRep);

  int interactionState = rep->GetInteractionState();

  // Reset the state
  rep->SetInteractionState( vtkParallelopipedRepresentation::Outside );

  // Let the representation re-compute what the appropriate state is.
  const int modifier = self->Interactor->GetShiftKey()   |
                       self->Interactor->GetControlKey() |
                       self->Interactor->GetAltKey();
  int newInteractionState = rep->ComputeInteractionState(
        self->Interactor->GetEventPosition()[0],
        self->Interactor->GetEventPosition()[1], modifier );

  // If we computed a different interaction state than the one we were in,
  // render in response to any changes.
  if (newInteractionState != interactionState)
    {
    self->Interactor->Render();
    self->SetCursor(newInteractionState);
    self->InvokeEvent(vtkCommand::StartInteractionEvent, NULL);
    }
}

//----------------------------------------------------------------------
void vtkParallelopipedWidget::OnMouseMoveCallback(vtkAbstractWidget *w)
{
  vtkParallelopipedWidget *self = reinterpret_cast<vtkParallelopipedWidget*>(w);
  vtkParallelopipedRepresentation *rep =
    reinterpret_cast<vtkParallelopipedRepresentation*>(self->WidgetRep);

  int interactionState = rep->GetInteractionState();
  int newInteractionState = interactionState;

  if (interactionState == vtkParallelopipedRepresentation
                              ::TranslatingParallelopiped)
    {
    // Dispatch to all widgets in the set.
    if (self->WidgetSet)
      {
      self->WidgetSet->DispatchAction(
        self, &vtkParallelopipedWidget::TranslateAction);
      }
    else
      {
      self->TranslateAction(self);
      }
    }

  else
    {
    // Let the representation re-compute what the appropriate state is.
    const int modifier = self->Interactor->GetShiftKey()   |
                         self->Interactor->GetControlKey() |
                         self->Interactor->GetAltKey();
    newInteractionState = rep->ComputeInteractionState(
          self->Interactor->GetEventPosition()[0],
          self->Interactor->GetEventPosition()[1], modifier );
    }

  // If we computed a different interaction state than the one we were in,
  // render in response to any changes. Also take care of trivial cases that
  // require no rendering.
  if (newInteractionState != interactionState ||
      (newInteractionState != vtkParallelopipedRepresentation::Inside &&
       newInteractionState != vtkParallelopipedRepresentation::Outside ))
    {
    self->Interactor->Render();
    self->SetCursor(newInteractionState);
    self->InvokeEvent(vtkCommand::InteractionEvent, NULL);
    }
}

//-------------------------------------------------------------------------
void vtkParallelopipedWidget::SetCursor(int state)
{
  switch (state)
    {
    case vtkParallelopipedRepresentation::ResizingParallelopiped :
    case vtkParallelopipedRepresentation::ResizingParallelopipedAlongAnAxis :
      this->RequestCursorShape(VTK_CURSOR_HAND);
      break;
    default:
      this->RequestCursorShape(VTK_CURSOR_DEFAULT);
    }
}

//----------------------------------------------------------------------------
void vtkParallelopipedWidget::SetProcessEvents(int pe)
{
  this->Superclass::SetProcessEvents(pe);
  for (int i=0; i<8; i++)
    {
    this->HandleWidgets[i]->SetProcessEvents(pe);
    }
}

//----------------------------------------------------------------------------
void vtkParallelopipedWidget::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  vtkGarbageCollectorReport(collector, this->WidgetSet, "WidgetSet");
}

//----------------------------------------------------------------------
void vtkParallelopipedWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Chair Creation: "
     << (this->EnableChairCreation ? "On\n" : "Off\n");

}
