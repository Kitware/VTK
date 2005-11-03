/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMeasureWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMeasureWidget.h"
#include "vtkMeasureRepresentation2D.h"
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

vtkCxxRevisionMacro(vtkMeasureWidget, "1.2");
vtkStandardNewMacro(vtkMeasureWidget);


// The checkerboard simply observes the behavior of four vtkSliderWidgets.
// Here we create the command/observer classes to respond to the 
// slider widgets.
class vtkMeasureWidgetCallback : public vtkCommand
{
public:
  static vtkMeasureWidgetCallback *New() 
    { return new vtkMeasureWidgetCallback; }
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
  vtkMeasureWidget *MeasureWidget;
};


//----------------------------------------------------------------------
vtkMeasureWidget::vtkMeasureWidget()
{
  this->ManagesCursor = 0;

  this->WidgetState = vtkMeasureWidget::Start;
  this->CurrentHandle = 0;

  // The widgets for moving the end points. They observe this widget (i.e.,
  // this widget is the parent to the handles).
  this->Point1Widget = vtkHandleWidget::New();
  this->Point1Widget->SetParent(this);
  this->Point2Widget = vtkHandleWidget::New();
  this->Point2Widget->SetParent(this);

  // Set up the callbacks on the two handles
  this->MeasureWidgetCallback1 = vtkMeasureWidgetCallback::New();
  this->MeasureWidgetCallback1->HandleNumber = 0;
  this->MeasureWidgetCallback1->MeasureWidget = this;
  this->Point1Widget->AddObserver(vtkCommand::StartInteractionEvent, this->MeasureWidgetCallback1, 
                                  this->Priority);
  this->Point1Widget->AddObserver(vtkCommand::InteractionEvent, this->MeasureWidgetCallback1,
                                  this->Priority);
  this->Point1Widget->AddObserver(vtkCommand::EndInteractionEvent, this->MeasureWidgetCallback1,
                                  this->Priority);

  this->MeasureWidgetCallback2 = vtkMeasureWidgetCallback::New();
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
                                          this, vtkMeasureWidget::AddPointAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MouseMoveEvent,
                                          vtkWidgetEvent::Move,
                                          this, vtkMeasureWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
                                          vtkWidgetEvent::EndSelect,
                                          this, vtkMeasureWidget::EndSelectAction);
}

//----------------------------------------------------------------------
vtkMeasureWidget::~vtkMeasureWidget()
{
  this->Point1Widget->RemoveObserver(this->MeasureWidgetCallback1);
  this->Point1Widget->Delete();
  this->MeasureWidgetCallback1->Delete();

  this->Point2Widget->RemoveObserver(this->MeasureWidgetCallback2);
  this->Point2Widget->Delete();
  this->MeasureWidgetCallback2->Delete();
}

//----------------------------------------------------------------------
void vtkMeasureWidget::CreateDefaultRepresentation()
{
  if ( ! this->WidgetRep )
    {
    this->WidgetRep = vtkMeasureRepresentation2D::New();
    }
  reinterpret_cast<vtkMeasureRepresentation*>(this->WidgetRep)->
    InstantiateHandleRepresentation();
}

//----------------------------------------------------------------------
void vtkMeasureWidget::SetEnabled(int enabling)
{
  // The handle widgets are not actually enabled until they are placed.
  // The handle widgets take their representation from the vtkMeasureRepresentation.
  if ( enabling )
    {
    if ( this->WidgetState == vtkMeasureWidget::Start )
      {
      reinterpret_cast<vtkMeasureRepresentation*>(this->WidgetRep)->VisibilityOff();    
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
    this->Point1Widget->SetRepresentation(reinterpret_cast<vtkMeasureRepresentation*>
                                          (this->WidgetRep)->GetPoint1Representation());
    this->Point1Widget->SetInteractor(this->Interactor);
    this->Point1Widget->GetRepresentation()->SetRenderer(this->CurrentRenderer);
    
    this->Point2Widget->SetRepresentation(reinterpret_cast<vtkMeasureRepresentation*>
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
void vtkMeasureWidget::AddPointAction(vtkAbstractWidget *w)
{
  vtkMeasureWidget *self = reinterpret_cast<vtkMeasureWidget*>(w);

  // Need to distinguish between placing handles and manipulating handles
  if ( self->WidgetState == vtkMeasureWidget::MovingHandle )
    {
    return;
    }

  // Placing the second point is easy
  if ( self->WidgetState == vtkMeasureWidget::PlacingPoints )
    {
    self->InvokeEvent(vtkCommand::PlacePointEvent,(void*)&(self->CurrentHandle));
    self->WidgetState = vtkMeasureWidget::Placed;
    self->Point1Widget->SetEnabled(1);
    self->Point2Widget->SetEnabled(1);
    }
  
  else //need to see whether we are placing the first point or manipulating a handle
    {
    // compute some info we need for all cases
    int X = self->Interactor->GetEventPosition()[0];
    int Y = self->Interactor->GetEventPosition()[1];
    int state = self->WidgetRep->ComputeInteractionState(X,Y);
    if ( self->WidgetState == vtkMeasureWidget::Start ||
         (self->WidgetState == vtkMeasureWidget::Placed && state == vtkMeasureRepresentation::Outside ) )
      { //putting down the first point
      self->WidgetState = vtkMeasureWidget::PlacingPoints;
      self->Point1Widget->SetEnabled(0);
      self->Point2Widget->SetEnabled(0);
      reinterpret_cast<vtkMeasureRepresentation*>(self->WidgetRep)->VisibilityOn();    
      double e[2];
      e[0] = static_cast<double>(X);
      e[1] = static_cast<double>(Y);
      reinterpret_cast<vtkMeasureRepresentation*>(self->WidgetRep)->
        StartWidgetInteraction(e);
      self->CurrentHandle = 0;
      self->InvokeEvent(vtkCommand::PlacePointEvent,(void*)&(self->CurrentHandle));
      self->CurrentHandle++;
      }
    else if ( state == vtkMeasureRepresentation::NearP1 ||
              state == vtkMeasureRepresentation::NearP2 )
      {
      if ( state == vtkMeasureRepresentation::NearP1 )
        {
        self->WidgetState = vtkMeasureWidget::MovingHandle;
        self->CurrentHandle = 0;
        }
      else if ( state == vtkMeasureRepresentation::NearP2 )
        {
        self->WidgetState = vtkMeasureWidget::MovingHandle;
        self->CurrentHandle = 1;
        }
      // Invoke an event on ourself for the handles 
      self->InvokeEvent(vtkCommand::LeftButtonPressEvent,NULL);
      }
    }

  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(vtkCommand::InteractionEvent,NULL);
  self->Render();
}

//-------------------------------------------------------------------------
void vtkMeasureWidget::MoveAction(vtkAbstractWidget *w)
{
  vtkMeasureWidget *self = reinterpret_cast<vtkMeasureWidget*>(w);

  // Do nothing if outside
  if ( self->WidgetState == vtkMeasureWidget::Start ||
       self->WidgetState == vtkMeasureWidget::Placed )
    {
    return;
    }

  // compute some info we need for all cases
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Delegate the event consistent with the state
  if ( self->WidgetState == vtkMeasureWidget::PlacingPoints )
    {
    double e[2];
    e[0] = static_cast<double>(X);
    e[1] = static_cast<double>(Y);
    reinterpret_cast<vtkMeasureRepresentation*>(self->WidgetRep)->
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
void vtkMeasureWidget::EndSelectAction(vtkAbstractWidget *w)
{
  vtkMeasureWidget *self = reinterpret_cast<vtkMeasureWidget*>(w);

  // Do nothing if outside
  if ( self->WidgetState != vtkMeasureWidget::MovingHandle )
    {
    return;
    }

  self->WidgetState = vtkMeasureWidget::Placed;
  self->InvokeEvent(vtkCommand::LeftButtonReleaseEvent,NULL);

  self->WidgetRep->BuildRepresentation();
  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(vtkCommand::InteractionEvent,NULL);
  self->Render();
}

// These are callbacks that are active when the user is manipulating the
// handles of the measure widget.
//----------------------------------------------------------------------
void vtkMeasureWidget::StartMeasureInteraction(int)
{
  this->Superclass::StartInteraction();
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
}

//----------------------------------------------------------------------
void vtkMeasureWidget::MeasureInteraction(int handle)
{
  double pos[3];
  if ( handle == 0 )
    {
    reinterpret_cast<vtkMeasureRepresentation*>(this->WidgetRep)->
      GetPoint1Representation()->GetDisplayPosition(pos);
    reinterpret_cast<vtkMeasureRepresentation*>(this->WidgetRep)->
      SetPoint1DisplayPosition(pos);
    }
  else
    {
    reinterpret_cast<vtkMeasureRepresentation*>(this->WidgetRep)->
      GetPoint2Representation()->GetDisplayPosition(pos);
    reinterpret_cast<vtkMeasureRepresentation*>(this->WidgetRep)->
      SetPoint2DisplayPosition(pos);
    }

  this->InvokeEvent(vtkCommand::InteractionEvent,NULL);
}

//----------------------------------------------------------------------
void vtkMeasureWidget::EndMeasureInteraction(int)
{
  this->Superclass::EndInteraction();

  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
}

//----------------------------------------------------------------------
void vtkMeasureWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);
  
}
