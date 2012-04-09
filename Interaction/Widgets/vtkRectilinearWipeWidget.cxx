/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRectilinearWipeWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRectilinearWipeWidget.h"
#include "vtkRectilinearWipeRepresentation.h"
#include "vtkCommand.h"
#include "vtkCallbackCommand.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkWidgetEventTranslator.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkEvent.h"
#include "vtkWidgetEvent.h"


vtkStandardNewMacro(vtkRectilinearWipeWidget);


//----------------------------------------------------------------------
vtkRectilinearWipeWidget::vtkRectilinearWipeWidget()
{
  // Establish the initial widget state
  this->WidgetState = vtkRectilinearWipeWidget::Start;

  // Define widget events
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent,
                                          vtkWidgetEvent::Select,
                                          this, vtkRectilinearWipeWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
                                          vtkWidgetEvent::EndSelect,
                                          this, vtkRectilinearWipeWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MouseMoveEvent,
                                          vtkWidgetEvent::Move,
                                          this, vtkRectilinearWipeWidget::MoveAction);
}

//----------------------------------------------------------------------
vtkRectilinearWipeWidget::~vtkRectilinearWipeWidget()
{
}

//-------------------------------------------------------------------------
void vtkRectilinearWipeWidget::SetCursor(int cState)
{
  switch (cState)
    {
    case vtkRectilinearWipeRepresentation::MovingHPane:
      this->RequestCursorShape(VTK_CURSOR_SIZENS);
      break;
    case vtkRectilinearWipeRepresentation::MovingVPane:
      this->RequestCursorShape(VTK_CURSOR_SIZEWE);
      break;
    case vtkRectilinearWipeRepresentation::MovingCenter:
      this->RequestCursorShape(VTK_CURSOR_SIZEALL);
      break;
    default:
      this->RequestCursorShape(VTK_CURSOR_DEFAULT);
    }
}

//----------------------------------------------------------------------
void vtkRectilinearWipeWidget::SelectAction(vtkAbstractWidget *w)
{
  vtkRectilinearWipeWidget *self = reinterpret_cast<vtkRectilinearWipeWidget*>(w);

  if ( self->WidgetRep->GetInteractionState() == vtkRectilinearWipeRepresentation::Outside )
    {
    return;
    }

  // We are definitely selected
  self->WidgetState = vtkRectilinearWipeWidget::Selected;
  self->GrabFocus(self->EventCallbackCommand);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // This is redundant but necessary on some systems (windows) because the
  // cursor is switched during OS event processing and reverts to the default
  // cursor.
  self->SetCursor(self->WidgetRep->GetInteractionState());

  // We want to compute an orthogonal vector to the pane that has been selected
  double eventPos[2];
  eventPos[0] = static_cast<double>(X);
  eventPos[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(eventPos);

  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
}

//----------------------------------------------------------------------
void vtkRectilinearWipeWidget::MoveAction(vtkAbstractWidget *w)
{
  vtkRectilinearWipeWidget *self = reinterpret_cast<vtkRectilinearWipeWidget*>(w);

  // compute some info we need for all cases
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Set the cursor appropriately
  if ( self->WidgetState != vtkRectilinearWipeWidget::Selected )
    {
    self->WidgetRep->ComputeInteractionState(X, Y);
    self->SetCursor(self->WidgetRep->GetInteractionState());
    return;
    }

  // Okay, adjust the representation
  double newEventPosition[2];
  newEventPosition[0] = static_cast<double>(X);
  newEventPosition[1] = static_cast<double>(Y);
  self->WidgetRep->WidgetInteraction(newEventPosition);

  // moving something
  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(vtkCommand::InteractionEvent,NULL);
  self->Render();
}

//----------------------------------------------------------------------
void vtkRectilinearWipeWidget::EndSelectAction(vtkAbstractWidget *w)
{
  vtkRectilinearWipeWidget *self = reinterpret_cast<vtkRectilinearWipeWidget*>(w);

  if ( self->WidgetState != vtkRectilinearWipeWidget::Selected  ||
       self->WidgetRep->GetInteractionState() == vtkRectilinearWipeRepresentation::Outside )
    {
    return;
    }

  // Return state to not selected
  self->WidgetState = vtkRectilinearWipeWidget::Start;
  self->ReleaseFocus();

  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  self->WidgetState = vtkRectilinearWipeWidget::Start;
}

//----------------------------------------------------------------------
void vtkRectilinearWipeWidget::CreateDefaultRepresentation()
{
  if ( ! this->WidgetRep )
    {
    this->WidgetRep = vtkRectilinearWipeRepresentation::New();
    }
}


//----------------------------------------------------------------------
void vtkRectilinearWipeWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);

}
