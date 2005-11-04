/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAngleWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAngleWidget.h"
#include "vtkAngleRepresentation2D.h"
#include "vtkCommand.h"
#include "vtkCallbackCommand.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkHandleWidget.h"
#include "vtkHandleRepresentation.h"
#include "vtkCoordinate.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"

vtkCxxRevisionMacro(vtkAngleWidget, "1.3");
vtkStandardNewMacro(vtkAngleWidget);


// The checkerboard simply observes the behavior of four vtkSliderWidgets.
// Here we create the command/observer classes to respond to the 
// slider widgets.
class vtkAngleWidgetCallback : public vtkCommand
{
public:
  static vtkAngleWidgetCallback *New() 
    { return new vtkAngleWidgetCallback; }
  virtual void Execute(vtkObject*, unsigned long eventId, void*)
    {
      switch (eventId)
        {
        case vtkCommand::StartInteractionEvent:
          this->AngleWidget->StartAngleInteraction(this->HandleNumber);
          break;
        case vtkCommand::InteractionEvent:
          this->AngleWidget->AngleInteraction(this->HandleNumber);
          break;
        case vtkCommand::EndInteractionEvent:
          this->AngleWidget->EndAngleInteraction(this->HandleNumber);
          break;
        }
    }
  int HandleNumber;
  vtkAngleWidget *AngleWidget;
};


//----------------------------------------------------------------------
vtkAngleWidget::vtkAngleWidget()
{
  this->ManagesCursor = 0;

  this->WidgetState = vtkAngleWidget::Start;
  this->CurrentHandle = 0;

  // The widgets for moving the end points. They observe this widget (i.e.,
  // this widget is the parent to the handles).
  this->Point1Widget = vtkHandleWidget::New();
  this->Point1Widget->SetParent(this);
  this->CenterWidget = vtkHandleWidget::New();
  this->CenterWidget->SetParent(this);
  this->Point2Widget = vtkHandleWidget::New();
  this->Point2Widget->SetParent(this);

  // Set up the callbacks on the two handles
  this->AngleWidgetCallback1 = vtkAngleWidgetCallback::New();
  this->AngleWidgetCallback1->HandleNumber = 0;
  this->AngleWidgetCallback1->AngleWidget = this;
  this->Point1Widget->AddObserver(vtkCommand::StartInteractionEvent, this->AngleWidgetCallback1, 
                                  this->Priority);
  this->Point1Widget->AddObserver(vtkCommand::InteractionEvent, this->AngleWidgetCallback1,
                                  this->Priority);
  this->Point1Widget->AddObserver(vtkCommand::EndInteractionEvent, this->AngleWidgetCallback1,
                                  this->Priority);

  // Set up the callbacks on the two handles
  this->AngleWidgetCenterCallback = vtkAngleWidgetCallback::New();
  this->AngleWidgetCenterCallback->HandleNumber = 1;
  this->AngleWidgetCenterCallback->AngleWidget = this;
  this->CenterWidget->AddObserver(vtkCommand::StartInteractionEvent, this->AngleWidgetCenterCallback, 
                                  this->Priority);
  this->CenterWidget->AddObserver(vtkCommand::InteractionEvent, this->AngleWidgetCenterCallback,
                                  this->Priority);
  this->CenterWidget->AddObserver(vtkCommand::EndInteractionEvent, this->AngleWidgetCenterCallback,
                                  this->Priority);

  this->AngleWidgetCallback2 = vtkAngleWidgetCallback::New();
  this->AngleWidgetCallback2->HandleNumber = 2;
  this->AngleWidgetCallback2->AngleWidget = this;
  this->Point2Widget->AddObserver(vtkCommand::StartInteractionEvent, this->AngleWidgetCallback2, 
                                  this->Priority);
  this->Point2Widget->AddObserver(vtkCommand::InteractionEvent, this->AngleWidgetCallback2,
                                  this->Priority);
  this->Point2Widget->AddObserver(vtkCommand::EndInteractionEvent, this->AngleWidgetCallback2,
                                  this->Priority);


  // These are the event callbacks supported by this widget
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent,
                                          vtkWidgetEvent::AddPoint,
                                          this, vtkAngleWidget::AddPointAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MouseMoveEvent,
                                          vtkWidgetEvent::Move,
                                          this, vtkAngleWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
                                          vtkWidgetEvent::EndSelect,
                                          this, vtkAngleWidget::EndSelectAction);
}

//----------------------------------------------------------------------
vtkAngleWidget::~vtkAngleWidget()
{
  this->Point1Widget->RemoveObserver(this->AngleWidgetCallback1);
  this->Point1Widget->Delete();
  this->AngleWidgetCallback1->Delete();

  this->CenterWidget->RemoveObserver(this->AngleWidgetCenterCallback);
  this->CenterWidget->Delete();
  this->AngleWidgetCenterCallback->Delete();

  this->Point2Widget->RemoveObserver(this->AngleWidgetCallback2);
  this->Point2Widget->Delete();
  this->AngleWidgetCallback2->Delete();
}

//----------------------------------------------------------------------
void vtkAngleWidget::CreateDefaultRepresentation()
{
  if ( ! this->WidgetRep )
    {
    this->WidgetRep = vtkAngleRepresentation2D::New();
    }
  reinterpret_cast<vtkAngleRepresentation*>(this->WidgetRep)->
    InstantiateHandleRepresentation();
}

//----------------------------------------------------------------------
void vtkAngleWidget::SetEnabled(int enabling)
{
  // The handle widgets are not actually enabled until they are placed.
  // The handle widgets take their representation from the vtkAngleRepresentation.
  if ( enabling )
    {
    if ( this->WidgetState == vtkAngleWidget::Start )
      {
      reinterpret_cast<vtkAngleRepresentation*>(this->WidgetRep)->VisibilityOff();    
      }
    else
      {
      reinterpret_cast<vtkAngleRepresentation*>(this->WidgetRep)->Ray1VisibilityOn();
      reinterpret_cast<vtkAngleRepresentation*>(this->WidgetRep)->Ray2VisibilityOn();
      reinterpret_cast<vtkAngleRepresentation*>(this->WidgetRep)->ArcVisibilityOn();
      this->Point1Widget->SetEnabled(1);
      this->CenterWidget->SetEnabled(1);
      this->Point2Widget->SetEnabled(1);
      }
    }

  // Done in this wierd order to get everything to work right. This invocation creates the
  // default representation.
  this->Superclass::SetEnabled(enabling);

  if ( enabling )
    {
    this->Point1Widget->SetRepresentation(reinterpret_cast<vtkAngleRepresentation*>
                                          (this->WidgetRep)->GetPoint1Representation());
    this->Point1Widget->SetInteractor(this->Interactor);
    this->Point1Widget->GetRepresentation()->SetRenderer(this->CurrentRenderer);
    
    this->CenterWidget->SetRepresentation(reinterpret_cast<vtkAngleRepresentation*>
                                          (this->WidgetRep)->GetCenterRepresentation());
    this->CenterWidget->SetInteractor(this->Interactor);
    this->CenterWidget->GetRepresentation()->SetRenderer(this->CurrentRenderer);
    
    this->Point2Widget->SetRepresentation(reinterpret_cast<vtkAngleRepresentation*>
                                          (this->WidgetRep)->GetPoint2Representation());
    this->Point2Widget->SetInteractor(this->Interactor);
    this->Point2Widget->GetRepresentation()->SetRenderer(this->CurrentRenderer);

    }
  else
    {
    reinterpret_cast<vtkAngleRepresentation*>(this->WidgetRep)->Ray1VisibilityOff();
    reinterpret_cast<vtkAngleRepresentation*>(this->WidgetRep)->Ray2VisibilityOff();
    reinterpret_cast<vtkAngleRepresentation*>(this->WidgetRep)->ArcVisibilityOff();
    this->Point1Widget->SetEnabled(0);
    this->CenterWidget->SetEnabled(0);
    this->Point2Widget->SetEnabled(0);
    }
}

// The following methods are the callbacks that the angle widget responds to. 
//-------------------------------------------------------------------------
void vtkAngleWidget::AddPointAction(vtkAbstractWidget *w)
{
  vtkAngleWidget *self = reinterpret_cast<vtkAngleWidget*>(w);

  // Need to distinguish between placing handles and manipulating handles
  if ( self->WidgetState == vtkAngleWidget::MovingHandle )
    {
    return;
    }

  // Placing the second and third points is easy
  if ( self->WidgetState == vtkAngleWidget::PlacingPoints )
    {
    self->InvokeEvent(vtkCommand::PlacePointEvent,(void*)&(self->CurrentHandle));
    if ( self->CurrentHandle == 1 )
      {
      int X = self->Interactor->GetEventPosition()[0];
      int Y = self->Interactor->GetEventPosition()[1];
      double e[2];
      e[0] = static_cast<double>(X);
      e[1] = static_cast<double>(Y);
      reinterpret_cast<vtkAngleRepresentation*>(self->WidgetRep)->
        CenterWidgetInteraction(e);
      self->CurrentHandle++;
      self->CenterWidget->SetEnabled(1);
      reinterpret_cast<vtkAngleRepresentation*>(self->WidgetRep)->Ray2VisibilityOn();
      reinterpret_cast<vtkAngleRepresentation*>(self->WidgetRep)->ArcVisibilityOn();
      }
    else if ( self->CurrentHandle == 2 )
      {
      self->WidgetState = vtkAngleWidget::Placed;
      self->Point2Widget->SetEnabled(1);
      }
    }
  
  else //need to see whether we are placing the first point or manipulating a handle
    {
    // compute some info we need for all cases
    int X = self->Interactor->GetEventPosition()[0];
    int Y = self->Interactor->GetEventPosition()[1];
    int state = self->WidgetRep->ComputeInteractionState(X,Y);
    if ( self->WidgetState == vtkAngleWidget::Start ||
         (self->WidgetState == vtkAngleWidget::Placed && state == vtkAngleRepresentation::Outside ) )
      { //putting down the first point
      self->WidgetState = vtkAngleWidget::PlacingPoints;
      self->CenterWidget->SetEnabled(0);
      self->Point2Widget->SetEnabled(0);
      reinterpret_cast<vtkAngleRepresentation*>(self->WidgetRep)->Ray2VisibilityOff();
      reinterpret_cast<vtkAngleRepresentation*>(self->WidgetRep)->ArcVisibilityOff();
      double e[2];
      e[0] = static_cast<double>(X);
      e[1] = static_cast<double>(Y);
      reinterpret_cast<vtkAngleRepresentation*>(self->WidgetRep)->
        StartWidgetInteraction(e);
      self->CurrentHandle = 0;
      self->InvokeEvent(vtkCommand::PlacePointEvent,(void*)&(self->CurrentHandle));
      self->CurrentHandle++;
      reinterpret_cast<vtkAngleRepresentation*>(self->WidgetRep)->Ray1VisibilityOn();
      self->Point1Widget->SetEnabled(1);
      }
    else if ( state == vtkAngleRepresentation::NearP1 ||
              state == vtkAngleRepresentation::NearCenter ||
              state == vtkAngleRepresentation::NearP2 )
      {
      if ( state == vtkAngleRepresentation::NearP1 )
        {
        self->WidgetState = vtkAngleWidget::MovingHandle;
        self->CurrentHandle = 0;
        }
      else if ( state == vtkAngleRepresentation::NearCenter )
        {
        self->WidgetState = vtkAngleWidget::MovingHandle;
        self->CurrentHandle = 1;
        }
      else if ( state == vtkAngleRepresentation::NearP2 )
        {
        self->WidgetState = vtkAngleWidget::MovingHandle;
        self->CurrentHandle = 2;
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
void vtkAngleWidget::MoveAction(vtkAbstractWidget *w)
{
  vtkAngleWidget *self = reinterpret_cast<vtkAngleWidget*>(w);

  // Do nothing if outside
  if ( self->WidgetState == vtkAngleWidget::Start ||
       self->WidgetState == vtkAngleWidget::Placed )
    {
    return;
    }

  // compute some info we need for all cases
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Delegate the event consistent with the state
  if ( self->WidgetState == vtkAngleWidget::PlacingPoints )
    {
    double e[2];
    e[0] = static_cast<double>(X);
    e[1] = static_cast<double>(Y);
    if ( self->CurrentHandle == 1 )
      {
      reinterpret_cast<vtkAngleRepresentation*>(self->WidgetRep)->
        CenterWidgetInteraction(e);
      }
    else
      {
      reinterpret_cast<vtkAngleRepresentation*>(self->WidgetRep)->
        WidgetInteraction(e);
      }
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
void vtkAngleWidget::EndSelectAction(vtkAbstractWidget *w)
{
  vtkAngleWidget *self = reinterpret_cast<vtkAngleWidget*>(w);

  // Do nothing if outside
  if ( self->WidgetState != vtkAngleWidget::MovingHandle )
    {
    return;
    }

  self->WidgetState = vtkAngleWidget::Placed;
  self->InvokeEvent(vtkCommand::LeftButtonReleaseEvent,NULL);

  self->WidgetRep->BuildRepresentation();
  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(vtkCommand::InteractionEvent,NULL);
  self->Render();
}

// These are callbacks that are active when the user is manipulating the
// handles of the angle widget.
//----------------------------------------------------------------------
void vtkAngleWidget::StartAngleInteraction(int)
{
  this->Superclass::StartInteraction();
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
}

//----------------------------------------------------------------------
void vtkAngleWidget::AngleInteraction(int handle)
{
  double pos[3];
  if ( handle == 0 )
    {
    reinterpret_cast<vtkAngleRepresentation*>(this->WidgetRep)->
      GetPoint1Representation()->GetDisplayPosition(pos);
    reinterpret_cast<vtkAngleRepresentation*>(this->WidgetRep)->
      SetPoint1DisplayPosition(pos);
    }
  else if ( handle == 1 )
    {
    reinterpret_cast<vtkAngleRepresentation*>(this->WidgetRep)->
      GetCenterRepresentation()->GetDisplayPosition(pos);
    reinterpret_cast<vtkAngleRepresentation*>(this->WidgetRep)->
      SetCenterDisplayPosition(pos);
    }
  else //if ( handle == 2 )
    {
    reinterpret_cast<vtkAngleRepresentation*>(this->WidgetRep)->
      GetPoint2Representation()->GetDisplayPosition(pos);
    reinterpret_cast<vtkAngleRepresentation*>(this->WidgetRep)->
      SetPoint2DisplayPosition(pos);
    }

  this->InvokeEvent(vtkCommand::InteractionEvent,NULL);
}

//----------------------------------------------------------------------
void vtkAngleWidget::EndAngleInteraction(int)
{
  this->Superclass::EndInteraction();

  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
}

//----------------------------------------------------------------------
void vtkAngleWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);
}
