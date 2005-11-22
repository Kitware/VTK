/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDistanceWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDistanceWidget.h"
#include "vtkDistanceRepresentation2D.h"
#include "vtkCommand.h"
#include "vtkCallbackCommand.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkHandleWidget.h"
#include "vtkCoordinate.h"
#include "vtkHandleRepresentation.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkAxisActor2D.h"
#include "vtkWidgetEvent.h"

vtkCxxRevisionMacro(vtkDistanceWidget, "1.3");
vtkStandardNewMacro(vtkDistanceWidget);


// The checkerboard simply observes the behavior of four vtkSliderWidgets.
// Here we create the command/observer classes to respond to the 
// slider widgets.
class vtkDistanceWidgetCallback : public vtkCommand
{
public:
  static vtkDistanceWidgetCallback *New() 
    { return new vtkDistanceWidgetCallback; }
  virtual void Execute(vtkObject*, unsigned long eventId, void*)
    {
      switch (eventId)
        {
        case vtkCommand::StartInteractionEvent:
          this->MeasureWidget->StartMeasureInteraction(this->HandleNumber);
          break;
        case vtkCommand::InteractionEvent:
          this->MeasureWidget->MeasureInteraction(this->HandleNumber);
          break;
        case vtkCommand::EndInteractionEvent:
          this->MeasureWidget->EndMeasureInteraction(this->HandleNumber);
          break;
        }
    }
  int HandleNumber;
  vtkDistanceWidget *MeasureWidget;
};


//----------------------------------------------------------------------
vtkDistanceWidget::vtkDistanceWidget()
{
  this->ManagesCursor = 0;

  this->WidgetState = vtkDistanceWidget::Start;
  this->CurrentHandle = 0;

  // The widgets for moving the end points. They observe this widget (i.e.,
  // this widget is the parent to the handles).
  this->Point1Widget = vtkHandleWidget::New();
  this->Point1Widget->SetParent(this);
  this->Point2Widget = vtkHandleWidget::New();
  this->Point2Widget->SetParent(this);

  // Set up the callbacks on the two handles
  this->MeasureWidgetCallback1 = vtkDistanceWidgetCallback::New();
  this->MeasureWidgetCallback1->HandleNumber = 0;
  this->MeasureWidgetCallback1->MeasureWidget = this;
  this->Point1Widget->AddObserver(vtkCommand::StartInteractionEvent, this->MeasureWidgetCallback1, 
                                  this->Priority);
  this->Point1Widget->AddObserver(vtkCommand::InteractionEvent, this->MeasureWidgetCallback1,
                                  this->Priority);
  this->Point1Widget->AddObserver(vtkCommand::EndInteractionEvent, this->MeasureWidgetCallback1,
                                  this->Priority);

  this->MeasureWidgetCallback2 = vtkDistanceWidgetCallback::New();
  this->MeasureWidgetCallback2->HandleNumber = 1;
  this->MeasureWidgetCallback2->MeasureWidget = this;
  this->Point2Widget->AddObserver(vtkCommand::StartInteractionEvent, this->MeasureWidgetCallback2, 
                                  this->Priority);
  this->Point2Widget->AddObserver(vtkCommand::InteractionEvent, this->MeasureWidgetCallback2,
                                  this->Priority);
  this->Point2Widget->AddObserver(vtkCommand::EndInteractionEvent, this->MeasureWidgetCallback2,
                                  this->Priority);


  // These are the event callbacks supported by this widget
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent,
                                          vtkWidgetEvent::AddPoint,
                                          this, vtkDistanceWidget::AddPointAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MouseMoveEvent,
                                          vtkWidgetEvent::Move,
                                          this, vtkDistanceWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
                                          vtkWidgetEvent::EndSelect,
                                          this, vtkDistanceWidget::EndSelectAction);
}

//----------------------------------------------------------------------
vtkDistanceWidget::~vtkDistanceWidget()
{
  this->Point1Widget->RemoveObserver(this->MeasureWidgetCallback1);
  this->Point1Widget->Delete();
  this->MeasureWidgetCallback1->Delete();

  this->Point2Widget->RemoveObserver(this->MeasureWidgetCallback2);
  this->Point2Widget->Delete();
  this->MeasureWidgetCallback2->Delete();
}

//----------------------------------------------------------------------
void vtkDistanceWidget::CreateDefaultRepresentation()
{
  if ( ! this->WidgetRep )
    {
    this->WidgetRep = vtkDistanceRepresentation2D::New();
    }
  reinterpret_cast<vtkDistanceRepresentation*>(this->WidgetRep)->
    InstantiateHandleRepresentation();
}

//----------------------------------------------------------------------
void vtkDistanceWidget::SetEnabled(int enabling)
{
  // The handle widgets are not actually enabled until they are placed.
  // The handle widgets take their representation from the vtkDistanceRepresentation.
  if ( enabling )
    {
    if ( this->WidgetState == vtkDistanceWidget::Start )
      {
      reinterpret_cast<vtkDistanceRepresentation*>(this->WidgetRep)->VisibilityOff();    
      }
    else
      {
      this->Point1Widget->SetEnabled(1);
      this->Point2Widget->SetEnabled(1);
      }
    }

  // Done in this weird order to get everything to work right. This invocation creates the
  // default representation.
  this->Superclass::SetEnabled(enabling);

  if ( enabling )
    {
    this->Point1Widget->SetRepresentation(reinterpret_cast<vtkDistanceRepresentation*>
                                          (this->WidgetRep)->GetPoint1Representation());
    this->Point1Widget->SetInteractor(this->Interactor);
    this->Point1Widget->GetRepresentation()->SetRenderer(this->CurrentRenderer);
    
    this->Point2Widget->SetRepresentation(reinterpret_cast<vtkDistanceRepresentation*>
                                          (this->WidgetRep)->GetPoint2Representation());
    this->Point2Widget->SetInteractor(this->Interactor);
    this->Point2Widget->GetRepresentation()->SetRenderer(this->CurrentRenderer);

    }
  else
    {
    this->Point1Widget->SetEnabled(0);
    this->Point2Widget->SetEnabled(0);
    }
}

// The following methods are the callbacks that the measure widget responds to. 
//-------------------------------------------------------------------------
void vtkDistanceWidget::AddPointAction(vtkAbstractWidget *w)
{
  vtkDistanceWidget *self = reinterpret_cast<vtkDistanceWidget*>(w);
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  int state = self->WidgetRep->ComputeInteractionState(X,Y);

  // Freshly enabled and placing the first point
  if ( self->WidgetState == vtkDistanceWidget::Start )
    {
    self->Interactor->GrabFocus(self->EventCallbackCommand);
    self->WidgetState = vtkDistanceWidget::Define;
    self->Point1Widget->SetEnabled(0);
    self->Point2Widget->SetEnabled(0);
    reinterpret_cast<vtkDistanceRepresentation*>(self->WidgetRep)->VisibilityOn();    
    double e[2];
    e[0] = static_cast<double>(X);
    e[1] = static_cast<double>(Y);
    reinterpret_cast<vtkDistanceRepresentation*>(self->WidgetRep)->
      StartWidgetInteraction(e);
    self->CurrentHandle = 0;
    self->InvokeEvent(vtkCommand::PlacePointEvent,(void*)&(self->CurrentHandle));
    self->CurrentHandle++;
    }
  
  // Placing the second point is easy
  else if ( self->WidgetState == vtkDistanceWidget::Define )
    {
    self->InvokeEvent(vtkCommand::PlacePointEvent,(void*)&(self->CurrentHandle));
    self->WidgetState = vtkDistanceWidget::Manipulate;
    self->Point1Widget->SetEnabled(1);
    self->Point2Widget->SetEnabled(1);
    }
  
  // See if we are trying to manipulate the widget handles
  else if ( self->WidgetState == vtkDistanceWidget::Manipulate )
    {
    if ( state == vtkDistanceRepresentation::NearP1 )
      {
      self->Interactor->GrabFocus(self->EventCallbackCommand);
      self->CurrentHandle = 0;
      self->InvokeEvent(vtkCommand::LeftButtonPressEvent,NULL);
      }
    else if ( state == vtkDistanceRepresentation::NearP2 )
      {
      self->Interactor->GrabFocus(self->EventCallbackCommand);
      self->CurrentHandle = 1;
      self->InvokeEvent(vtkCommand::LeftButtonPressEvent,NULL);
      }

    else if ( state == vtkDistanceRepresentation::Outside )
      {
      return;
      }
    }

  // Clean up
  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(vtkCommand::InteractionEvent,NULL);
  self->Render();
}

//-------------------------------------------------------------------------
void vtkDistanceWidget::MoveAction(vtkAbstractWidget *w)
{
  vtkDistanceWidget *self = reinterpret_cast<vtkDistanceWidget*>(w);

  // Do nothing if outside
  if ( self->WidgetState == vtkDistanceWidget::Start || 
       (self->WidgetState == vtkDistanceWidget::Manipulate &&
        self->WidgetRep->GetInteractionState() == vtkDistanceRepresentation::Outside) )
    {
    return;
    }

  // compute some info we need for all cases
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Delegate the event consistent with the state
  if ( self->WidgetState == vtkDistanceWidget::Define )
    {
    double e[2];
    e[0] = static_cast<double>(X);
    e[1] = static_cast<double>(Y);
    reinterpret_cast<vtkDistanceRepresentation*>(self->WidgetRep)->
      WidgetInteraction(e);
    }
  else //must be moving a handle, invoke a event for the handle widgets
    {
    self->InvokeEvent(vtkCommand::MouseMoveEvent, NULL);
    }

  self->WidgetRep->BuildRepresentation();
  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(vtkCommand::InteractionEvent,NULL);
  self->Render();
}

//-------------------------------------------------------------------------
void vtkDistanceWidget::EndSelectAction(vtkAbstractWidget *w)
{
  vtkDistanceWidget *self = reinterpret_cast<vtkDistanceWidget*>(w);

  // Do nothing if outside
  if ( self->WidgetState != vtkDistanceWidget::Manipulate ||
       self->WidgetRep->GetInteractionState() == vtkDistanceRepresentation::Outside )
    {
    return;
    }

  self->Interactor->ReleaseFocus();
  self->InvokeEvent(vtkCommand::LeftButtonReleaseEvent,NULL);
  self->WidgetRep->BuildRepresentation();
  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(vtkCommand::InteractionEvent,NULL);
  self->Render();
}

// These are callbacks that are active when the user is manipulating the
// handles of the measure widget.
//----------------------------------------------------------------------
void vtkDistanceWidget::StartMeasureInteraction(int)
{
  this->Superclass::StartInteraction();
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
}

//----------------------------------------------------------------------
void vtkDistanceWidget::MeasureInteraction(int handle)
{
  double pos[3];
  if ( handle == 0 )
    {
    reinterpret_cast<vtkDistanceRepresentation*>(this->WidgetRep)->
      GetPoint1Representation()->GetDisplayPosition(pos);
    reinterpret_cast<vtkDistanceRepresentation*>(this->WidgetRep)->
      SetPoint1DisplayPosition(pos);
    }
  else
    {
    reinterpret_cast<vtkDistanceRepresentation*>(this->WidgetRep)->
      GetPoint2Representation()->GetDisplayPosition(pos);
    reinterpret_cast<vtkDistanceRepresentation*>(this->WidgetRep)->
      SetPoint2DisplayPosition(pos);
    }

  this->InvokeEvent(vtkCommand::InteractionEvent,NULL);
}

//----------------------------------------------------------------------
void vtkDistanceWidget::EndMeasureInteraction(int)
{
  this->Superclass::EndInteraction();

  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
}

//----------------------------------------------------------------------
void vtkDistanceWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);
  
}
