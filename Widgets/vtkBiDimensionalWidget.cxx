/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBiDimensionalWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBiDimensionalWidget.h"
#include "vtkBiDimensionalRepresentation2D.h"
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
#include "vtkRenderWindow.h"

vtkCxxRevisionMacro(vtkBiDimensionalWidget, "1.1");
vtkStandardNewMacro(vtkBiDimensionalWidget);


// The bidimensional widget observes the handles.
// Here we create the command/observer classes to respond to the 
// slider widgets.
class vtkBiDimensionalWidgetCallback : public vtkCommand
{
public:
  static vtkBiDimensionalWidgetCallback *New() 
    { return new vtkBiDimensionalWidgetCallback; }
  virtual void Execute(vtkObject*, unsigned long eventId, void*)
    {
      switch (eventId)
        {
        case vtkCommand::StartInteractionEvent:
          this->BiDimensionalWidget->StartBiDimensionalInteraction(this->HandleNumber);
          break;
        case vtkCommand::InteractionEvent:
          this->BiDimensionalWidget->BiDimensionalInteraction(this->HandleNumber);
          break;
        case vtkCommand::EndInteractionEvent:
          this->BiDimensionalWidget->EndBiDimensionalInteraction(this->HandleNumber);
          break;
        }
    }
  int HandleNumber;
  vtkBiDimensionalWidget *BiDimensionalWidget;
};


//----------------------------------------------------------------------
vtkBiDimensionalWidget::vtkBiDimensionalWidget()
{
  this->ManagesCursor = 0;

  this->WidgetState = vtkBiDimensionalWidget::Start;
  this->CurrentHandle = 0;

  // The widgets for moving the end points. They observe this widget (i.e.,
  // this widget is the parent to the handles).
  this->Point1Widget = vtkHandleWidget::New();
  this->Point1Widget->SetParent(this);
  this->Point2Widget = vtkHandleWidget::New();
  this->Point2Widget->SetParent(this);
  this->Point3Widget = vtkHandleWidget::New();
  this->Point3Widget->SetParent(this);
  this->Point4Widget = vtkHandleWidget::New();
  this->Point4Widget->SetParent(this);

  // Set up the callbacks on the two handles
  this->BiDimensionalWidgetCallback1 = vtkBiDimensionalWidgetCallback::New();
  this->BiDimensionalWidgetCallback1->HandleNumber = 0;
  this->BiDimensionalWidgetCallback1->BiDimensionalWidget = this;
  this->Point1Widget->AddObserver(vtkCommand::StartInteractionEvent, this->BiDimensionalWidgetCallback1, 
                                  this->Priority);
  this->Point1Widget->AddObserver(vtkCommand::InteractionEvent, this->BiDimensionalWidgetCallback1,
                                  this->Priority);
  this->Point1Widget->AddObserver(vtkCommand::EndInteractionEvent, this->BiDimensionalWidgetCallback1,
                                  this->Priority);

  this->BiDimensionalWidgetCallback2 = vtkBiDimensionalWidgetCallback::New();
  this->BiDimensionalWidgetCallback2->HandleNumber = 1;
  this->BiDimensionalWidgetCallback2->BiDimensionalWidget = this;
  this->Point2Widget->AddObserver(vtkCommand::StartInteractionEvent, this->BiDimensionalWidgetCallback2, 
                                  this->Priority);
  this->Point2Widget->AddObserver(vtkCommand::InteractionEvent, this->BiDimensionalWidgetCallback2,
                                  this->Priority);
  this->Point2Widget->AddObserver(vtkCommand::EndInteractionEvent, this->BiDimensionalWidgetCallback2,
                                  this->Priority);


  this->BiDimensionalWidgetCallback3 = vtkBiDimensionalWidgetCallback::New();
  this->BiDimensionalWidgetCallback3->HandleNumber = 2;
  this->BiDimensionalWidgetCallback3->BiDimensionalWidget = this;
  this->Point3Widget->AddObserver(vtkCommand::StartInteractionEvent, this->BiDimensionalWidgetCallback3, 
                                  this->Priority);
  this->Point3Widget->AddObserver(vtkCommand::InteractionEvent, this->BiDimensionalWidgetCallback3,
                                  this->Priority);
  this->Point3Widget->AddObserver(vtkCommand::EndInteractionEvent, this->BiDimensionalWidgetCallback3,
                                  this->Priority);


  this->BiDimensionalWidgetCallback4 = vtkBiDimensionalWidgetCallback::New();
  this->BiDimensionalWidgetCallback4->HandleNumber = 3;
  this->BiDimensionalWidgetCallback4->BiDimensionalWidget = this;
  this->Point4Widget->AddObserver(vtkCommand::StartInteractionEvent, this->BiDimensionalWidgetCallback4, 
                                  this->Priority);
  this->Point4Widget->AddObserver(vtkCommand::InteractionEvent, this->BiDimensionalWidgetCallback4,
                                  this->Priority);
  this->Point4Widget->AddObserver(vtkCommand::EndInteractionEvent, this->BiDimensionalWidgetCallback4,
                                  this->Priority);


  // These are the event callbacks supported by this widget
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent,
                                          vtkWidgetEvent::AddPoint,
                                          this, vtkBiDimensionalWidget::AddPointAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MouseMoveEvent,
                                          vtkWidgetEvent::Move,
                                          this, vtkBiDimensionalWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
                                          vtkWidgetEvent::EndSelect,
                                          this, vtkBiDimensionalWidget::EndSelectAction);
}

//----------------------------------------------------------------------
vtkBiDimensionalWidget::~vtkBiDimensionalWidget()
{
  this->Point1Widget->RemoveObserver(this->BiDimensionalWidgetCallback1);
  this->Point1Widget->Delete();
  this->BiDimensionalWidgetCallback1->Delete();

  this->Point2Widget->RemoveObserver(this->BiDimensionalWidgetCallback2);
  this->Point2Widget->Delete();
  this->BiDimensionalWidgetCallback2->Delete();

  this->Point3Widget->RemoveObserver(this->BiDimensionalWidgetCallback3);
  this->Point3Widget->Delete();
  this->BiDimensionalWidgetCallback3->Delete();

  this->Point4Widget->RemoveObserver(this->BiDimensionalWidgetCallback4);
  this->Point4Widget->Delete();
  this->BiDimensionalWidgetCallback4->Delete();
}

//----------------------------------------------------------------------
void vtkBiDimensionalWidget::CreateDefaultRepresentation()
{
  if ( ! this->WidgetRep )
    {
    this->WidgetRep = vtkBiDimensionalRepresentation2D::New();
    }
  reinterpret_cast<vtkBiDimensionalRepresentation2D*>(this->WidgetRep)->
    InstantiateHandleRepresentation();
}

//----------------------------------------------------------------------
void vtkBiDimensionalWidget::SetEnabled(int enabling)
{
  // The handle widgets are not actually enabled until they are placed.
  // The handle widgets take their representation from the vtkBiDimensionalRepresentation2D.
  if ( enabling )
    {
    if ( this->WidgetState == vtkBiDimensionalWidget::Start )
      {
      if (this->WidgetRep)
        {
        reinterpret_cast<vtkBiDimensionalRepresentation2D*>(this->WidgetRep)->
          Line1VisibilityOff();
        reinterpret_cast<vtkBiDimensionalRepresentation2D*>(this->WidgetRep)->
          Line2VisibilityOff();
        }
      }
    else
      {
      if (this->WidgetRep)
        {
        reinterpret_cast<vtkBiDimensionalRepresentation2D*>(this->WidgetRep)->
          Line1VisibilityOn();
        reinterpret_cast<vtkBiDimensionalRepresentation2D*>(this->WidgetRep)->
          Line2VisibilityOn();
        }
      if (this->Point1Widget)
        {
        this->Point1Widget->SetEnabled(1);
        }
      if (this->Point2Widget)
        {
        this->Point2Widget->SetEnabled(1);
        }
      if (this->Point3Widget)
        {
        this->Point3Widget->SetEnabled(1);
        }
      if (this->Point4Widget)
        {
        this->Point4Widget->SetEnabled(1);
        }
      }
    }

  // Done in this wierd order to get everything to work right. This invocation creates the
  // default representation.
  this->Superclass::SetEnabled(enabling);

  if ( enabling )
    {
      if (this->Point1Widget)
        {
        this->Point1Widget->SetRepresentation(
          reinterpret_cast<vtkBiDimensionalRepresentation2D*>
          (this->WidgetRep)->GetPoint1Representation());
        this->Point1Widget->SetInteractor(this->Interactor);
        this->Point1Widget->GetRepresentation()->SetRenderer(
          this->CurrentRenderer);
        }
      if (this->Point2Widget)
        {
        this->Point2Widget->SetRepresentation(
          reinterpret_cast<vtkBiDimensionalRepresentation2D*>
          (this->WidgetRep)->GetPoint2Representation());
        this->Point2Widget->SetInteractor(this->Interactor);
        this->Point2Widget->GetRepresentation()->SetRenderer(
          this->CurrentRenderer);
        }
      if (this->Point3Widget)
        {
        this->Point3Widget->SetRepresentation(
          reinterpret_cast<vtkBiDimensionalRepresentation2D*>
          (this->WidgetRep)->GetPoint3Representation());
        this->Point3Widget->SetInteractor(this->Interactor);
        this->Point3Widget->GetRepresentation()->SetRenderer(
          this->CurrentRenderer);
        }
      if (this->Point4Widget)
        {
        this->Point4Widget->SetRepresentation(
          reinterpret_cast<vtkBiDimensionalRepresentation2D*>
          (this->WidgetRep)->GetPoint4Representation());
        this->Point4Widget->SetInteractor(this->Interactor);
        this->Point4Widget->GetRepresentation()->SetRenderer(
          this->CurrentRenderer);
        }
    }
  else //disabling widget
    {
    if (this->WidgetRep)
      {
      reinterpret_cast<vtkBiDimensionalRepresentation2D*>(this->WidgetRep)->
        Line1VisibilityOff();
      reinterpret_cast<vtkBiDimensionalRepresentation2D*>(this->WidgetRep)->
        Line2VisibilityOff();
      }
      if (this->Point1Widget)
        {
        this->Point1Widget->SetEnabled(0);
        }
      if (this->Point2Widget)
        {
        this->Point2Widget->SetEnabled(0);
        }
      if (this->Point3Widget)
        {
        this->Point3Widget->SetEnabled(0);
        }
      if (this->Point4Widget)
        {
        this->Point4Widget->SetEnabled(0);
        }
    }
}

//----------------------------------------------------------------------
int vtkBiDimensionalWidget::IsMeasureValid()
{
  if ( this->WidgetState == vtkBiDimensionalWidget::Manipulate ||
       (this->WidgetState == vtkBiDimensionalWidget::Define && this->CurrentHandle == 2) )
    {
    return 1;
    }
  else
    {
    return 0;
    }
}

// The following methods are the callbacks that the angle widget responds to. 
//-------------------------------------------------------------------------
void vtkBiDimensionalWidget::AddPointAction(vtkAbstractWidget *w)
{
  vtkBiDimensionalWidget *self = reinterpret_cast<vtkBiDimensionalWidget*>(w);
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // If we are placing the first point it's easy
  if ( self->WidgetState == vtkBiDimensionalWidget::Start )
    {
    self->GrabFocus(self->EventCallbackCommand);
    self->WidgetState = vtkBiDimensionalWidget::Define;
    self->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
    double e[2];
    e[0] = static_cast<double>(X);
    e[1] = static_cast<double>(Y);
    reinterpret_cast<vtkBiDimensionalRepresentation2D*>(self->WidgetRep)->StartWidgetInteraction(e);
    self->CurrentHandle = 0;
    self->InvokeEvent(vtkCommand::PlacePointEvent,(void*)&(self->CurrentHandle));
    reinterpret_cast<vtkBiDimensionalRepresentation2D*>(self->WidgetRep)->Line1VisibilityOn();
    self->Point1Widget->SetEnabled(1);
    self->CurrentHandle++;
    }

  // If defining we are placing the second or third point
  else if ( self->WidgetState == vtkBiDimensionalWidget::Define )
    {
    self->InvokeEvent(vtkCommand::PlacePointEvent,(void*)&(self->CurrentHandle));
    if ( self->CurrentHandle == 1 )
      {
      self->InvokeEvent(vtkCommand::PlacePointEvent,(void*)&(self->CurrentHandle));
      double e[2];
      e[0] = static_cast<double>(X);
      e[1] = static_cast<double>(Y);
      reinterpret_cast<vtkBiDimensionalRepresentation2D*>(self->WidgetRep)->Point2WidgetInteraction(e);
      self->CurrentHandle++;
      self->Point2Widget->SetEnabled(1);
      self->Point3Widget->SetEnabled(1);
      self->Point4Widget->SetEnabled(1);
      reinterpret_cast<vtkBiDimensionalRepresentation2D*>(self->WidgetRep)->Line2VisibilityOn();
      }
    else if ( self->CurrentHandle == 2 )
      {
      self->InvokeEvent(vtkCommand::PlacePointEvent,(void*)&(self->CurrentHandle));
      self->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
      self->WidgetState = vtkBiDimensionalWidget::Manipulate;
      self->CurrentHandle = (-1);
      self->ReleaseFocus();
      }
    }
  
  // Maybe we are trying to manipulate the widget handles
  else //if ( self->WidgetState == vtkBiDimensionalWidget::Manipulate )
    {
    self->HandleSelected = 0;
    self->LineSelected = 0;
    int state = self->WidgetRep->ComputeInteractionState(X,Y);
    if ( state == vtkBiDimensionalRepresentation2D::Outside )
      {
      return;
      }

    self->GrabFocus(self->EventCallbackCommand);
    if ( state == vtkBiDimensionalRepresentation2D::NearP1 ||
         state == vtkBiDimensionalRepresentation2D::NearP2 ||
         state == vtkBiDimensionalRepresentation2D::NearP3 ||
         state == vtkBiDimensionalRepresentation2D::NearP4 )
      {
      self->HandleSelected = 1;
      self->InvokeEvent(vtkCommand::LeftButtonPressEvent,NULL);
      }
    else if ( state == vtkBiDimensionalRepresentation2D::OnL1 ||
              state == vtkBiDimensionalRepresentation2D::OnL2 )
      {
      self->LineSelected = 1;
      }
    }
  
  self->EventCallbackCommand->SetAbortFlag(1);
  self->Render();
}

//-------------------------------------------------------------------------
void vtkBiDimensionalWidget::MoveAction(vtkAbstractWidget *w)
{
  vtkBiDimensionalWidget *self = reinterpret_cast<vtkBiDimensionalWidget*>(w);

  // Do nothing if outside
  if ( self->WidgetState == vtkBiDimensionalWidget::Start )
    {
    return;
    }

  // Delegate the event consistent with the state
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  if ( self->WidgetState == vtkBiDimensionalWidget::Define )
    {
    if ( self->CurrentHandle == 1 )
      {
      reinterpret_cast<vtkBiDimensionalRepresentation2D*>(self->WidgetRep)->
        Point2WidgetInteraction(e);
      }
    else
      {
      reinterpret_cast<vtkBiDimensionalRepresentation2D*>(self->WidgetRep)->
        Point3WidgetInteraction(e);
      }
    self->InvokeEvent(vtkCommand::InteractionEvent,NULL);
    self->EventCallbackCommand->SetAbortFlag(1);
    }

  else //must be moving a handle or line, i.e., we are in manipulate state
    {
    int state = self->WidgetRep->ComputeInteractionState(X,Y);
    if ( state != vtkBiDimensionalRepresentation2D::Outside )
      {
      self->RequestCursorShape(VTK_CURSOR_HAND);
      }
    else
      {
      self->RequestCursorShape(VTK_CURSOR_DEFAULT);
      }

    // If moving a line, we deal with the events
    if ( self->LineSelected )
      {
      reinterpret_cast<vtkBiDimensionalRepresentation2D*>(self->WidgetRep)->
        WidgetInteraction(e);
      }
    else if ( self->HandleSelected )
      {
      self->CurrentHandle = 0;
      // We invoke a mouse move and the handle(s) take care of it
      self->InvokeEvent(vtkCommand::MouseMoveEvent, NULL);
      }
    }

  self->WidgetRep->BuildRepresentation();
  self->Render();
}

//-------------------------------------------------------------------------
void vtkBiDimensionalWidget::EndSelectAction(vtkAbstractWidget *w)
{
  vtkBiDimensionalWidget *self = reinterpret_cast<vtkBiDimensionalWidget*>(w);

  // Do nothing if outside
  if ( self->WidgetState == vtkBiDimensionalWidget::Start ||
       self->WidgetState == vtkBiDimensionalWidget::Define ||
       (!self->HandleSelected && !self->LineSelected) )
    {
    return;
    }

  self->ReleaseFocus();
  self->InvokeEvent(vtkCommand::LeftButtonReleaseEvent,NULL);
  self->CurrentHandle = (-1);
  self->WidgetRep->BuildRepresentation();
  self->EventCallbackCommand->SetAbortFlag(1);
  self->Render();
}

// These are callbacks that are active when the user is manipulating the
// handles of the angle widget.
//----------------------------------------------------------------------
void vtkBiDimensionalWidget::StartBiDimensionalInteraction(int)
{
  this->Superclass::StartInteraction();
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
}

//----------------------------------------------------------------------
void vtkBiDimensionalWidget::BiDimensionalInteraction(int vtkNotUsed(handle))
{
  double pos[3];
  int state = this->WidgetRep->GetInteractionState();
  if ( state == vtkBiDimensionalRepresentation2D::NearP1 )
    {
    reinterpret_cast<vtkBiDimensionalRepresentation2D*>(this->WidgetRep)->
      GetPoint1Representation()->GetDisplayPosition(pos);
    reinterpret_cast<vtkBiDimensionalRepresentation2D*>(this->WidgetRep)->
      SetPoint1DisplayPosition(pos);
    }
  else if ( state == vtkBiDimensionalRepresentation2D::NearP2 )
    {
    reinterpret_cast<vtkBiDimensionalRepresentation2D*>(this->WidgetRep)->
      GetPoint2Representation()->GetDisplayPosition(pos);
    reinterpret_cast<vtkBiDimensionalRepresentation2D*>(this->WidgetRep)->
      SetPoint2DisplayPosition(pos);
    }
  else if ( state == vtkBiDimensionalRepresentation2D::NearP3 )
    {
    reinterpret_cast<vtkBiDimensionalRepresentation2D*>(this->WidgetRep)->
      GetPoint3Representation()->GetDisplayPosition(pos);
    reinterpret_cast<vtkBiDimensionalRepresentation2D*>(this->WidgetRep)->
      SetPoint3DisplayPosition(pos);
    }
  else if ( state == vtkBiDimensionalRepresentation2D::NearP4 )
    {
    reinterpret_cast<vtkBiDimensionalRepresentation2D*>(this->WidgetRep)->
      GetPoint4Representation()->GetDisplayPosition(pos);
    reinterpret_cast<vtkBiDimensionalRepresentation2D*>(this->WidgetRep)->
      SetPoint4DisplayPosition(pos);
    }

  this->InvokeEvent(vtkCommand::InteractionEvent,NULL);
}

//----------------------------------------------------------------------
void vtkBiDimensionalWidget::EndBiDimensionalInteraction(int)
{
  this->Superclass::EndInteraction();

  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
}

//----------------------------------------------------------------------
void vtkBiDimensionalWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);
}
