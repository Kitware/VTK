/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBiDimensionalWidget.cxx,v

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

vtkCxxRevisionMacro(vtkBiDimensionalWidget, "1.10");
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
          this->BiDimensionalWidget->StartBiDimensionalInteraction();
          break;
        case vtkCommand::EndInteractionEvent:
          this->BiDimensionalWidget->EndBiDimensionalInteraction();
          break;
        }
    }
  vtkBiDimensionalWidget *BiDimensionalWidget;
};


//----------------------------------------------------------------------
vtkBiDimensionalWidget::vtkBiDimensionalWidget()
{
  this->ManagesCursor = 1;

  this->WidgetState = vtkBiDimensionalWidget::Start;
  this->CurrentHandle = 0;

  // Manage priorities, we want the handles to be lower priority
  if ( this->Priority <= 0.0 )
    {
    this->Priority = 0.01;
    }

  // The widgets for moving the end points. They observe this widget (i.e.,
  // this widget is the parent to the handles).
  this->Point1Widget = vtkHandleWidget::New();
  this->Point1Widget->SetPriority(this->Priority-0.01);
  this->Point1Widget->SetParent(this);
  this->Point1Widget->ManagesCursorOff();

  this->Point2Widget = vtkHandleWidget::New();
  this->Point2Widget->SetPriority(this->Priority-0.01);
  this->Point2Widget->SetParent(this);
  this->Point2Widget->ManagesCursorOff();

  this->Point3Widget = vtkHandleWidget::New();
  this->Point3Widget->SetPriority(this->Priority-0.01);
  this->Point3Widget->SetParent(this);
  this->Point3Widget->ManagesCursorOff();

  this->Point4Widget = vtkHandleWidget::New();
  this->Point4Widget->SetPriority(this->Priority-0.01);
  this->Point4Widget->SetParent(this);
  this->Point4Widget->ManagesCursorOff();


  // Set up the callbacks on the two handles
  this->BiDimensionalWidgetCallback1 = vtkBiDimensionalWidgetCallback::New();
  this->BiDimensionalWidgetCallback1->BiDimensionalWidget = this;
  this->Point1Widget->AddObserver(vtkCommand::StartInteractionEvent, this->BiDimensionalWidgetCallback1, 
                                  this->Priority);
  this->Point1Widget->AddObserver(vtkCommand::EndInteractionEvent, this->BiDimensionalWidgetCallback1,
                                  this->Priority);

  this->BiDimensionalWidgetCallback2 = vtkBiDimensionalWidgetCallback::New();
  this->BiDimensionalWidgetCallback2->BiDimensionalWidget = this;
  this->Point2Widget->AddObserver(vtkCommand::StartInteractionEvent, this->BiDimensionalWidgetCallback2, 
                                  this->Priority);
  this->Point2Widget->AddObserver(vtkCommand::EndInteractionEvent, this->BiDimensionalWidgetCallback2,
                                  this->Priority);


  this->BiDimensionalWidgetCallback3 = vtkBiDimensionalWidgetCallback::New();
  this->BiDimensionalWidgetCallback3->BiDimensionalWidget = this;
  this->Point3Widget->AddObserver(vtkCommand::StartInteractionEvent, this->BiDimensionalWidgetCallback3, 
                                  this->Priority);
  this->Point3Widget->AddObserver(vtkCommand::EndInteractionEvent, this->BiDimensionalWidgetCallback3,
                                  this->Priority);


  this->BiDimensionalWidgetCallback4 = vtkBiDimensionalWidgetCallback::New();
  this->BiDimensionalWidgetCallback4->BiDimensionalWidget = this;
  this->Point4Widget->AddObserver(vtkCommand::StartInteractionEvent, this->BiDimensionalWidgetCallback4, 
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
  vtkBiDimensionalRepresentation2D::SafeDownCast(this->WidgetRep)->
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
        vtkBiDimensionalRepresentation2D::SafeDownCast(this->WidgetRep)->
          Line1VisibilityOff();
        vtkBiDimensionalRepresentation2D::SafeDownCast(this->WidgetRep)->
          Line2VisibilityOff();
        }
      }
    else
      {
      if (this->WidgetRep)
        {
        vtkBiDimensionalRepresentation2D::SafeDownCast(this->WidgetRep)->
          Line1VisibilityOn();
        vtkBiDimensionalRepresentation2D::SafeDownCast(this->WidgetRep)->
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

  if ( enabling )
    {
    // Done in this wierd order to get everything to work right. 
    // This invocation creates the default representation.
    this->Superclass::SetEnabled(enabling);

      if (this->Point1Widget)
        {
        this->Point1Widget->SetRepresentation(
          vtkBiDimensionalRepresentation2D::SafeDownCast
          (this->WidgetRep)->GetPoint1Representation());
        this->Point1Widget->SetInteractor(this->Interactor);
        this->Point1Widget->GetRepresentation()->SetRenderer(
          this->CurrentRenderer);
        }
      if (this->Point2Widget)
        {
        this->Point2Widget->SetRepresentation(
          vtkBiDimensionalRepresentation2D::SafeDownCast
          (this->WidgetRep)->GetPoint2Representation());
        this->Point2Widget->SetInteractor(this->Interactor);
        this->Point2Widget->GetRepresentation()->SetRenderer(
          this->CurrentRenderer);
        }
      if (this->Point3Widget)
        {
        this->Point3Widget->SetRepresentation(
          vtkBiDimensionalRepresentation2D::SafeDownCast
          (this->WidgetRep)->GetPoint3Representation());
        this->Point3Widget->SetInteractor(this->Interactor);
        this->Point3Widget->GetRepresentation()->SetRenderer(
          this->CurrentRenderer);
        }
      if (this->Point4Widget)
        {
        this->Point4Widget->SetRepresentation(
          vtkBiDimensionalRepresentation2D::SafeDownCast
          (this->WidgetRep)->GetPoint4Representation());
        this->Point4Widget->SetInteractor(this->Interactor);
        this->Point4Widget->GetRepresentation()->SetRenderer(
          this->CurrentRenderer);
        }
    }
  else //disabling widget
    {
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

    // Done in this wierd order to get everything right. The renderer is
    // set to null after we disable the sub-widgets. That should give the 
    // renderer a chance to remove the representation props before being
    // set to NULL.
    this->Superclass::SetEnabled(enabling);    
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

// The following methods are the callbacks that the bidimensional widget responds to. 
//-------------------------------------------------------------------------
void vtkBiDimensionalWidget::AddPointAction(vtkAbstractWidget *w)
{
  vtkBiDimensionalWidget *self = vtkBiDimensionalWidget::SafeDownCast(w);
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);

  // If we are placing the first point it's easy
  if ( self->WidgetState == vtkBiDimensionalWidget::Start )
    {
    self->GrabFocus(self->EventCallbackCommand);
    self->WidgetState = vtkBiDimensionalWidget::Define;
    self->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
    vtkBiDimensionalRepresentation2D::SafeDownCast(self->WidgetRep)->StartWidgetDefinition(e);
    self->CurrentHandle = 0;
    self->InvokeEvent(vtkCommand::PlacePointEvent,&(self->CurrentHandle));
    vtkBiDimensionalRepresentation2D::SafeDownCast(self->WidgetRep)->Line1VisibilityOn();
    self->Point1Widget->SetEnabled(1);
    self->CurrentHandle++;
    }

  // If defining we are placing the second or third point
  else if ( self->WidgetState == vtkBiDimensionalWidget::Define )
    {
    self->InvokeEvent(vtkCommand::PlacePointEvent,&(self->CurrentHandle));
    if ( self->CurrentHandle == 1 )
      {
      self->InvokeEvent(vtkCommand::PlacePointEvent,&(self->CurrentHandle));
      vtkBiDimensionalRepresentation2D::SafeDownCast(self->WidgetRep)->Point2WidgetInteraction(e);
      self->CurrentHandle++;
      self->Point2Widget->SetEnabled(1);
      self->Point3Widget->SetEnabled(1);
      self->Point4Widget->SetEnabled(1);
      vtkBiDimensionalRepresentation2D::SafeDownCast(self->WidgetRep)->Line2VisibilityOn();
      }
    else if ( self->CurrentHandle == 2 )
      {
      self->InvokeEvent(vtkCommand::PlacePointEvent,&(self->CurrentHandle));
      vtkBiDimensionalRepresentation2D::SafeDownCast(self->WidgetRep)->Point3WidgetInteraction(e);
      self->WidgetState = vtkBiDimensionalWidget::Manipulate;
      self->CurrentHandle = (-1);
      self->ReleaseFocus();
      self->InvokeEvent(vtkCommand::EndInteractionEvent,NULL); 
      }
    }
  
  // Maybe we are trying to manipulate the widget handles
  else //if ( self->WidgetState == vtkBiDimensionalWidget::Manipulate )
    {
    self->HandleLine1Selected = 0;
    self->HandleLine2Selected = 0;
    self->Line1InnerSelected = 0;
    self->Line1OuterSelected = 0;
    self->Line2InnerSelected = 0;
    self->Line2OuterSelected = 0;
    self->CenterSelected = 0;
    int modifier = self->Interactor->GetShiftKey() | self->Interactor->GetControlKey();
    int state = self->WidgetRep->ComputeInteractionState(X,Y,modifier);
    if ( state == vtkBiDimensionalRepresentation2D::Outside )
      {
      return;
      }

    self->GrabFocus(self->EventCallbackCommand);
    vtkBiDimensionalRepresentation2D::SafeDownCast(self->WidgetRep)->StartWidgetManipulation(e);
    if ( state == vtkBiDimensionalRepresentation2D::NearP1 ||
         state == vtkBiDimensionalRepresentation2D::NearP2 )
      {
      self->HandleLine1Selected = 1;
      self->InvokeEvent(vtkCommand::LeftButtonPressEvent,NULL);
      }
    else if ( state == vtkBiDimensionalRepresentation2D::NearP3 ||
              state == vtkBiDimensionalRepresentation2D::NearP4 )
      {
      self->HandleLine2Selected = 1;
      self->InvokeEvent(vtkCommand::LeftButtonPressEvent,NULL);
      }
    else if ( state == vtkBiDimensionalRepresentation2D::OnL1Inner)
      {
      self->WidgetRep->Highlight(1);
      self->Line1InnerSelected = 1;
      self->StartBiDimensionalInteraction();
      }
    else if ( state == vtkBiDimensionalRepresentation2D::OnL1Outer)
      {
      self->WidgetRep->Highlight(1);
      self->Line1OuterSelected = 1;
      self->StartBiDimensionalInteraction();
      }
    else if ( state == vtkBiDimensionalRepresentation2D::OnL2Inner)
      {
      self->WidgetRep->Highlight(1);
      self->Line2InnerSelected = 1;
      self->StartBiDimensionalInteraction();
      }
    else if ( state == vtkBiDimensionalRepresentation2D::OnL2Outer)
      {
      self->WidgetRep->Highlight(1);
      self->Line2OuterSelected = 1;
      self->StartBiDimensionalInteraction();
      }
    else if ( state == vtkBiDimensionalRepresentation2D::OnCenter )
      {
      self->WidgetRep->Highlight(1);
      self->CenterSelected = 1;
      self->StartBiDimensionalInteraction();
      }
    }
  
  self->EventCallbackCommand->SetAbortFlag(1);
  self->Render();
}

//-------------------------------------------------------------------------
void vtkBiDimensionalWidget::MoveAction(vtkAbstractWidget *w)
{
  vtkBiDimensionalWidget *self = vtkBiDimensionalWidget::SafeDownCast(w);

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
  double p1[3], p2[3], slope;

  if ( self->WidgetState == vtkBiDimensionalWidget::Define )
    {
    if ( self->CurrentHandle == 1 )
      {
      vtkBiDimensionalRepresentation2D::SafeDownCast(self->WidgetRep)->
        Point2WidgetInteraction(e);
      }
    else
      {
      vtkBiDimensionalRepresentation2D::SafeDownCast(self->WidgetRep)->
        Point3WidgetInteraction(e);
      }
    self->InvokeEvent(vtkCommand::InteractionEvent,NULL);
    self->EventCallbackCommand->SetAbortFlag(1);
    }

  else if ( self->Line1OuterSelected || self->Line2OuterSelected )
    {
    // moving outer portion of line -- rotating
    self->RequestCursorShape(VTK_CURSOR_HAND);
    vtkBiDimensionalRepresentation2D::SafeDownCast(self->WidgetRep)->
      WidgetInteraction(e);
    self->InvokeEvent(vtkCommand::InteractionEvent,NULL);
    }
  
  else if ( self->Line1InnerSelected )
    {//must be moving inner portion of line 1 -- line translation
    reinterpret_cast<vtkBiDimensionalRepresentation2D*>(self->WidgetRep)->
      GetPoint1DisplayPosition(p1);
    reinterpret_cast<vtkBiDimensionalRepresentation2D*>(self->WidgetRep)->
      GetPoint2DisplayPosition(p2);
    slope = VTK_DOUBLE_MAX;
    if (p1[0] != p2[0])
      {
      slope = (p2[1] - p1[1]) / (p2[0] - p1[0]);
      }
    if ( slope > -1 && slope < 1)
      {
      self->RequestCursorShape(VTK_CURSOR_SIZENS);
      }
    else
      {
      self->RequestCursorShape(VTK_CURSOR_SIZEWE);
      }

    reinterpret_cast<vtkBiDimensionalRepresentation2D*>(self->WidgetRep)->
      WidgetInteraction(e);
    self->InvokeEvent(vtkCommand::InteractionEvent,NULL);
    }

  else if ( self->Line2InnerSelected )
    {//must be moving inner portion of line 2 -- line translation
    reinterpret_cast<vtkBiDimensionalRepresentation2D*>(self->WidgetRep)->
      GetPoint3DisplayPosition(p1);
    reinterpret_cast<vtkBiDimensionalRepresentation2D*>(self->WidgetRep)->
      GetPoint4DisplayPosition(p2);
    slope = VTK_DOUBLE_MAX;
    if (p1[0] != p2[0])
      {
      slope = (p2[1] - p1[1]) / (p2[0] - p1[0]);
      }
    if ( slope > -1 && slope < 1)
      {
      self->RequestCursorShape(VTK_CURSOR_SIZENS);
      }
    else
      {
      self->RequestCursorShape(VTK_CURSOR_SIZEWE);
      }

    reinterpret_cast<vtkBiDimensionalRepresentation2D*>(self->WidgetRep)->
      WidgetInteraction(e);
    self->InvokeEvent(vtkCommand::InteractionEvent,NULL);
    }

  else if ( self->HandleLine1Selected )
    { // moving one of the endpoints of line 1
    reinterpret_cast<vtkBiDimensionalRepresentation2D*>(self->WidgetRep)->
      GetPoint1DisplayPosition(p1);
    reinterpret_cast<vtkBiDimensionalRepresentation2D*>(self->WidgetRep)->
      GetPoint2DisplayPosition(p2);
    slope = VTK_DOUBLE_MAX;
    if (p1[0] != p2[0])
      {
      slope = (p2[1] - p1[1]) / (p2[0] - p1[0]);
      }
    if ( slope > -1 && slope < 1)
      {
      self->RequestCursorShape(VTK_CURSOR_SIZEWE);
      }
    else
      {
      self->RequestCursorShape(VTK_CURSOR_SIZENS);
      }

    reinterpret_cast<vtkBiDimensionalRepresentation2D*>(self->WidgetRep)->
      WidgetInteraction(e);
    self->InvokeEvent(vtkCommand::InteractionEvent,NULL);
    }

  else if ( self->HandleLine2Selected )
    { // moving one of the endpoints of line 2
    reinterpret_cast<vtkBiDimensionalRepresentation2D*>(self->WidgetRep)->
      GetPoint3DisplayPosition(p1);
    reinterpret_cast<vtkBiDimensionalRepresentation2D*>(self->WidgetRep)->
      GetPoint4DisplayPosition(p2);
    slope = VTK_DOUBLE_MAX;
    if (p1[0] != p2[0])
      {
      slope = (p2[1] - p1[1]) / (p2[0] - p1[0]);
      }
    if ( slope > -1 && slope < 1)
      {
      self->RequestCursorShape(VTK_CURSOR_SIZEWE);
      }
    else
      {
      self->RequestCursorShape(VTK_CURSOR_SIZENS);
      }

    reinterpret_cast<vtkBiDimensionalRepresentation2D*>(self->WidgetRep)->
      WidgetInteraction(e);
    self->InvokeEvent(vtkCommand::InteractionEvent,NULL);
    }

  else if ( self->CenterSelected )
    {//grabbing center intersection point
    self->RequestCursorShape(VTK_CURSOR_SIZEALL);
    vtkBiDimensionalRepresentation2D::SafeDownCast(self->WidgetRep)->
      WidgetInteraction(e);
    self->InvokeEvent(vtkCommand::InteractionEvent,NULL);
    }

  else // just moving around, nothing yet selected
    {
    int state = self->WidgetRep->ComputeInteractionState(X,Y);
    if ( state == vtkBiDimensionalRepresentation2D::Outside )
      {
      self->RequestCursorShape(VTK_CURSOR_DEFAULT);
      }
    else if ( state == vtkBiDimensionalRepresentation2D::OnCenter )
      {
      self->RequestCursorShape(VTK_CURSOR_SIZEALL);
      }
    else if ( state == vtkBiDimensionalRepresentation2D::NearP1 ||
              state == vtkBiDimensionalRepresentation2D::NearP2 )
      {
      reinterpret_cast<vtkBiDimensionalRepresentation2D*>(self->WidgetRep)->
        GetPoint1DisplayPosition(p1);
      reinterpret_cast<vtkBiDimensionalRepresentation2D*>(self->WidgetRep)->
        GetPoint2DisplayPosition(p2);
      slope = VTK_DOUBLE_MAX;
      if (p1[0] != p2[0])
        {
        slope = (p2[1] - p1[1]) / (p2[0] - p1[0]);
        }
      if ( slope > -1 && slope < 1)
        {
        self->RequestCursorShape(VTK_CURSOR_SIZEWE);
        }
      else
        {
        self->RequestCursorShape(VTK_CURSOR_SIZENS);
        }
      }
    else if ( state == vtkBiDimensionalRepresentation2D::NearP3 ||
              state == vtkBiDimensionalRepresentation2D::NearP4 )
      {
      reinterpret_cast<vtkBiDimensionalRepresentation2D*>(self->WidgetRep)->
        GetPoint3DisplayPosition(p1);
      reinterpret_cast<vtkBiDimensionalRepresentation2D*>(self->WidgetRep)->
        GetPoint4DisplayPosition(p2);
      slope = VTK_DOUBLE_MAX;
      if (p1[0] != p2[0])
        {
        slope = (p2[1] - p1[1]) / (p2[0] - p1[0]);
        }
      if ( slope > -1 && slope < 1)
        {
        self->RequestCursorShape(VTK_CURSOR_SIZEWE);
        }
      else
        {
        self->RequestCursorShape(VTK_CURSOR_SIZENS);
        }
      }
    else if ( state == vtkBiDimensionalRepresentation2D::OnL1Inner )
      {
      reinterpret_cast<vtkBiDimensionalRepresentation2D*>(self->WidgetRep)->
        GetPoint1DisplayPosition(p1);
      reinterpret_cast<vtkBiDimensionalRepresentation2D*>(self->WidgetRep)->
        GetPoint2DisplayPosition(p2);
      slope = VTK_DOUBLE_MAX;
      if (p1[0] != p2[0])
        {
        slope = (p2[1] - p1[1]) / (p2[0] - p1[0]);
        }
      if ( slope > -1 && slope < 1)
        {
        self->RequestCursorShape(VTK_CURSOR_SIZENS);
        }
      else
        {
        self->RequestCursorShape(VTK_CURSOR_SIZEWE);
        }
      }
    else if ( state == vtkBiDimensionalRepresentation2D::OnL2Inner )
      {
      reinterpret_cast<vtkBiDimensionalRepresentation2D*>(self->WidgetRep)->
        GetPoint3DisplayPosition(p1);
      reinterpret_cast<vtkBiDimensionalRepresentation2D*>(self->WidgetRep)->
        GetPoint4DisplayPosition(p2);
      slope = VTK_DOUBLE_MAX;
      if (p1[0] != p2[0])
        {
        slope = (p2[1] - p1[1]) / (p2[0] - p1[0]);
        }
      if ( slope > -1 && slope < 1)
        {
        self->RequestCursorShape(VTK_CURSOR_SIZENS);
        }
      else
        {
        self->RequestCursorShape(VTK_CURSOR_SIZEWE);
        }
      }
    else
      {
      self->RequestCursorShape(VTK_CURSOR_HAND);
      }
    }

  self->WidgetRep->BuildRepresentation();
  self->Render();
}

//-------------------------------------------------------------------------
void vtkBiDimensionalWidget::EndSelectAction(vtkAbstractWidget *w)
{
  vtkBiDimensionalWidget *self = vtkBiDimensionalWidget::SafeDownCast(w);

  // Do nothing if outside
  if ( self->WidgetState == vtkBiDimensionalWidget::Start ||
       self->WidgetState == vtkBiDimensionalWidget::Define ||
       (!self->HandleLine1Selected && !self->HandleLine2Selected &&
        !self->Line1InnerSelected && !self->Line1OuterSelected &&
        !self->Line2InnerSelected && !self->Line2OuterSelected &&
        !self->CenterSelected) )
    {
    return;
    }

  self->Line1InnerSelected = 0;
  self->Line1OuterSelected = 0;
  self->Line2InnerSelected = 0;
  self->Line2OuterSelected = 0;
  self->HandleLine1Selected = 0;
  self->HandleLine2Selected = 0;
  self->CenterSelected = 0;
  self->WidgetRep->Highlight(0);
  self->ReleaseFocus();
  self->CurrentHandle = (-1);
  self->WidgetRep->BuildRepresentation();
  int state = self->WidgetRep->GetInteractionState();
  if ( state == vtkBiDimensionalRepresentation2D::NearP1 ||
       state == vtkBiDimensionalRepresentation2D::NearP2 ||
       state == vtkBiDimensionalRepresentation2D::NearP3 ||
       state == vtkBiDimensionalRepresentation2D::NearP4 )
    {
    self->InvokeEvent(vtkCommand::LeftButtonReleaseEvent,NULL);
    }
  else
    {
    self->EndBiDimensionalInteraction();
    }
  self->EventCallbackCommand->SetAbortFlag(1);
  self->Render();
}

// These are callbacks that are active when the user is manipulating the
// handles of the angle widget.
//----------------------------------------------------------------------
void vtkBiDimensionalWidget::StartBiDimensionalInteraction()
{
  this->Superclass::StartInteraction();
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
}

//----------------------------------------------------------------------
void vtkBiDimensionalWidget::EndBiDimensionalInteraction()
{
  this->Superclass::EndInteraction();
  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
}

//----------------------------------------------------------------------
void vtkBiDimensionalWidget::SetProcessEvents(int pe)
{
  this->Superclass::SetProcessEvents(pe);

  this->Point1Widget->SetProcessEvents(pe);
  this->Point2Widget->SetProcessEvents(pe);
  this->Point3Widget->SetProcessEvents(pe);
  this->Point4Widget->SetProcessEvents(pe);
}

//----------------------------------------------------------------------
void vtkBiDimensionalWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);
}
