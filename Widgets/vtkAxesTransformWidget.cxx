/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAxesTransformWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAxesTransformWidget.h"
#include "vtkAxesTransformRepresentation.h"
#include "vtkPointHandleRepresentation3D.h"
#include "vtkHandleWidget.h"
#include "vtkCommand.h"
#include "vtkCallbackCommand.h"
#include "vtkRendererCollection.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkObjectFactory.h"
#include "vtkWidgetEventTranslator.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkEvent.h"
#include "vtkWidgetEvent.h"
#include "vtkRenderWindow.h"


vtkStandardNewMacro(vtkAxesTransformWidget);

//----------------------------------------------------------------------------
vtkAxesTransformWidget::vtkAxesTransformWidget()
{
  this->WidgetState = vtkAxesTransformWidget::Start;
  this->ManagesCursor = 1;
  this->CurrentHandle = 0;

  // The widgets for moving the end points. They observe this widget (i.e.,
  // this widget is the parent to the handles).
  this->OriginWidget = vtkHandleWidget::New();
  this->OriginWidget->SetPriority(this->Priority-0.01);
  this->OriginWidget->SetParent(this);
  this->OriginWidget->ManagesCursorOff();

  this->SelectionWidget = vtkHandleWidget::New();
  this->SelectionWidget->SetPriority(this->Priority-0.01);
  this->SelectionWidget->SetParent(this);
  this->SelectionWidget->ManagesCursorOff();

  // Define widget events
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent,
                                          vtkWidgetEvent::Select,
                                          this, vtkAxesTransformWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
                                          vtkWidgetEvent::EndSelect,
                                          this, vtkAxesTransformWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MouseMoveEvent,
                                          vtkWidgetEvent::Move,
                                          this, vtkAxesTransformWidget::MoveAction);
}

//----------------------------------------------------------------------------
vtkAxesTransformWidget::~vtkAxesTransformWidget()
{
  this->OriginWidget->Delete();
  this->SelectionWidget->Delete();
}

//----------------------------------------------------------------------
void vtkAxesTransformWidget::SetEnabled(int enabling)
{
  // We defer enabling the handles until the selection process begins
  if ( enabling )
    {

    if ( ! this->CurrentRenderer )
      {
      int X=this->Interactor->GetEventPosition()[0];
      int Y=this->Interactor->GetEventPosition()[1];

      this->SetCurrentRenderer(this->Interactor->FindPokedRenderer(X,Y));

      if (this->CurrentRenderer == NULL)
        {
        return;
        }
      }

    // Don't actually turn these on until cursor is near the end points or the line.
    this->CreateDefaultRepresentation();
    vtkHandleRepresentation * originRep = reinterpret_cast<vtkAxesTransformRepresentation*>
                                          (this->WidgetRep)->GetOriginRepresentation();
    originRep->SetRenderer(this->CurrentRenderer);
    this->OriginWidget->SetRepresentation(originRep);
    this->OriginWidget->SetInteractor(this->Interactor);

    vtkHandleRepresentation * selectionRep = reinterpret_cast<vtkAxesTransformRepresentation*>
                                          (this->WidgetRep)->GetSelectionRepresentation();
    selectionRep->SetRenderer(this->CurrentRenderer);
    this->SelectionWidget->SetRepresentation(selectionRep);
    this->SelectionWidget->SetInteractor(this->Interactor);

    // We do this step first because it sets the CurrentRenderer
    this->Superclass::SetEnabled(enabling);
    }
  else
    {
    this->OriginWidget->SetEnabled(0);
    this->SelectionWidget->SetEnabled(0);
    }
}

//----------------------------------------------------------------------
void vtkAxesTransformWidget::SelectAction(vtkAbstractWidget *w)
{
  vtkAxesTransformWidget *self = reinterpret_cast<vtkAxesTransformWidget*>(w);
  if ( self->WidgetRep->GetInteractionState() == vtkAxesTransformRepresentation::Outside )
    {
    return;
    }

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // We are definitely selected
  self->WidgetState = vtkAxesTransformWidget::Active;
  self->GrabFocus(self->EventCallbackCommand);
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  reinterpret_cast<vtkAxesTransformRepresentation*>(self->WidgetRep)->
    StartWidgetInteraction(e);
  self->InvokeEvent(vtkCommand::LeftButtonPressEvent,NULL); //for the handles
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  self->EventCallbackCommand->SetAbortFlag(1);
}

//----------------------------------------------------------------------
void vtkAxesTransformWidget::MoveAction(vtkAbstractWidget *w)
{
  vtkAxesTransformWidget *self = reinterpret_cast<vtkAxesTransformWidget*>(w);
  // compute some info we need for all cases
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // See whether we're active
  if ( self->WidgetState == vtkAxesTransformWidget::Start )
    {
    self->Interactor->Disable(); //avoid extra renders
    self->OriginWidget->SetEnabled(0);
    self->SelectionWidget->SetEnabled(0);

    int oldState = self->WidgetRep->GetInteractionState();
    int state = self->WidgetRep->ComputeInteractionState(X,Y);
    int changed;
    // Determine if we are near the end points or the line
    if ( state == vtkAxesTransformRepresentation::Outside )
      {
      changed = self->RequestCursorShape(VTK_CURSOR_DEFAULT);
      }
    else //must be near something
      {
      changed = self->RequestCursorShape(VTK_CURSOR_HAND);
      if ( state == vtkAxesTransformRepresentation::OnOrigin )
        {
        self->OriginWidget->SetEnabled(1);
        }
      else //if ( state == vtkAxesTransformRepresentation::OnXXX )
        {
        self->SelectionWidget->SetEnabled(1);
        changed = 1; //movement along the line always needs render
        }
      }
    self->Interactor->Enable(); //avoid extra renders
    if ( changed || oldState != state )
      {
      self->Render();
      }
    }
  else //if ( self->WidgetState == vtkAxesTransformWidget::Active )
    {
    // moving something
    double e[2];
    e[0] = static_cast<double>(X);
    e[1] = static_cast<double>(Y);
    self->InvokeEvent(vtkCommand::MouseMoveEvent,NULL); //handles observe this
    reinterpret_cast<vtkAxesTransformRepresentation*>(self->WidgetRep)->
      WidgetInteraction(e);
    self->InvokeEvent(vtkCommand::InteractionEvent,NULL);
    self->EventCallbackCommand->SetAbortFlag(1);
    self->Render();
    }
}

//----------------------------------------------------------------------
void vtkAxesTransformWidget::EndSelectAction(vtkAbstractWidget *w)
{
  vtkAxesTransformWidget *self = reinterpret_cast<vtkAxesTransformWidget*>(w);
  if ( self->WidgetState == vtkAxesTransformWidget::Start )
    {
    return;
    }

  // Return state to not active
  self->WidgetState = vtkAxesTransformWidget::Start;
  self->ReleaseFocus();
  self->InvokeEvent(vtkCommand::LeftButtonReleaseEvent,NULL); //handles observe this
  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  self->Superclass::EndInteraction();
  self->Render();
}

//----------------------------------------------------------------------
void vtkAxesTransformWidget::CreateDefaultRepresentation()
{
  if ( ! this->WidgetRep )
    {
    this->WidgetRep = vtkAxesTransformRepresentation::New();
    }
}

//----------------------------------------------------------------------------
void vtkAxesTransformWidget::SetProcessEvents(int pe)
{
  this->Superclass::SetProcessEvents(pe);

  this->OriginWidget->SetProcessEvents(pe);
  this->SelectionWidget->SetProcessEvents(pe);
}

//----------------------------------------------------------------------------
void vtkAxesTransformWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
