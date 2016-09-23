/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFinitePlaneWidget.cxx

  Copyright (c)
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkFinitePlaneWidget.h"
#include "vtkFinitePlaneRepresentation.h"
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

vtkStandardNewMacro(vtkFinitePlaneWidget);

//----------------------------------------------------------------------------
vtkFinitePlaneWidget::vtkFinitePlaneWidget()
{
  this->WidgetState = vtkFinitePlaneWidget::Start;
  this->ManagesCursor = 1;

  // Define widget events
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent,
                                          vtkWidgetEvent::Select,
                                          this, vtkFinitePlaneWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
                                          vtkWidgetEvent::EndSelect,
                                          this, vtkFinitePlaneWidget::EndSelectAction);

  this->CallbackMapper->SetCallbackMethod(vtkCommand::MouseMoveEvent,
                                         vtkWidgetEvent::Move,
                                          this, vtkFinitePlaneWidget::MoveAction);

}

//----------------------------------------------------------------------------
vtkFinitePlaneWidget::~vtkFinitePlaneWidget()
{
}

//----------------------------------------------------------------------------
void vtkFinitePlaneWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkFinitePlaneWidget::SetRepresentation(vtkFinitePlaneRepresentation *r)
{
  this->Superclass::SetWidgetRepresentation(r);
}

//----------------------------------------------------------------------
void vtkFinitePlaneWidget::SelectAction(vtkAbstractWidget *w)
{
   // We are in a static method, cast to ourself
  vtkFinitePlaneWidget *self = reinterpret_cast<vtkFinitePlaneWidget*>(w);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // We want to compute an orthogonal vector to the plane that has been selected
  reinterpret_cast<vtkFinitePlaneRepresentation*>(self->WidgetRep)->
    SetInteractionState(vtkFinitePlaneRepresentation::Moving);
  int interactionState = self->WidgetRep->ComputeInteractionState(X, Y);
  self->UpdateCursorShape(interactionState);

  if ( self->WidgetRep->GetInteractionState() == vtkFinitePlaneRepresentation::Outside )
  {
    return;
  }

  // We are definitely selected
  self->GrabFocus(self->EventCallbackCommand);
  double eventPos[2];
  eventPos[0] = static_cast<double>(X);
  eventPos[1] = static_cast<double>(Y);
  self->WidgetState = vtkFinitePlaneWidget::Active;
  self->WidgetRep->StartWidgetInteraction(eventPos);

  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  self->Render();
}

//----------------------------------------------------------------------
void vtkFinitePlaneWidget::MoveAction(vtkAbstractWidget *w)
{
  vtkFinitePlaneWidget *self = reinterpret_cast<vtkFinitePlaneWidget*>(w);

  // So as to change the cursor shape when the mouse is poised over
  // the widget. Unfortunately, this results in a few extra picks
  // due to the cell picker. However given that its picking planes
  // and the handles/arrows, this should be very quick
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  int changed = 0;

  vtkFinitePlaneRepresentation* repr =
    reinterpret_cast<vtkFinitePlaneRepresentation*>(self->WidgetRep);

  if (self->ManagesCursor && self->WidgetState != vtkFinitePlaneWidget::Active)
  {
    int oldInteractionState = repr->GetInteractionState();

    repr->SetInteractionState(vtkFinitePlaneRepresentation::Moving);
    int state = self->WidgetRep->ComputeInteractionState( X, Y );
    changed = self->UpdateCursorShape(state);
    repr->SetInteractionState(oldInteractionState);
    changed = (changed || state != oldInteractionState) ? 1 : 0;
  }

  // See whether we're active
  if (self->WidgetState == vtkFinitePlaneWidget::Start)
  {
    if (changed && self->ManagesCursor)
    {
      self->Render();
    }
    return;
  }

  // Adjust the representation
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  self->WidgetRep->WidgetInteraction(e);

  // Moving something
  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(vtkCommand::InteractionEvent,NULL);
  self->Render();
}

//----------------------------------------------------------------------
void vtkFinitePlaneWidget::EndSelectAction(vtkAbstractWidget *w)
{
  vtkFinitePlaneWidget *self = reinterpret_cast<vtkFinitePlaneWidget*>(w);

  if (self->WidgetState != vtkFinitePlaneWidget::Active ||
      self->WidgetRep->GetInteractionState() == vtkFinitePlaneRepresentation::Outside)
  {
    return;
  }

  // Return state to not selected
  double e[2];
  self->WidgetRep->EndWidgetInteraction(e);
  self->WidgetState = vtkFinitePlaneWidget::Start;
  self->ReleaseFocus();

  // Update cursor if managed
  self->UpdateCursorShape(reinterpret_cast<vtkFinitePlaneRepresentation*>
    (self->WidgetRep)->GetRepresentationState());

  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  self->Render();
}

//----------------------------------------------------------------------
void vtkFinitePlaneWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = vtkFinitePlaneRepresentation::New();
  }
}

//----------------------------------------------------------------------
int vtkFinitePlaneWidget::UpdateCursorShape(int state)
{
  // So as to change the cursor shape when the mouse is poised over
  // the widget.
  if (this->ManagesCursor)
  {
    if (state == vtkFinitePlaneRepresentation::Outside)
    {
      return this->RequestCursorShape(VTK_CURSOR_DEFAULT);
    }
    else
    {
      return this->RequestCursorShape(VTK_CURSOR_HAND);
    }
  }

  return 0;
}
