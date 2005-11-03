/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSeedWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSeedWidget.h"
#include "vtkSeedRepresentation.h"
#include "vtkCommand.h"
#include "vtkCallbackCommand.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkHandleWidget.h"
#include "vtkHandleRepresentation.h"
#include "vtkCoordinate.h"
#include "vtkWidgetCallbackMapper.h"
#include <vtkstd/vector>

vtkCxxRevisionMacro(vtkSeedWidget, "1.2");
vtkStandardNewMacro(vtkSeedWidget);


// The vtkSeedList is a PIMPLed vector<T>.
class vtkSeedList : public vtkstd::vector<vtkHandleWidget*> {};
typedef vtkstd::vector<vtkHandleWidget*>::iterator vtkSeedListIterator;


// The checkerboard simply observes the behavior of four vtkSliderWidgets.
// Here we create the command/observer classes to respond to the 
// slider widgets.
class vtkSeedWidgetCallback : public vtkCommand
{
public:
  static vtkSeedWidgetCallback *New() 
    { return new vtkSeedWidgetCallback; }
  virtual void Execute(vtkObject*, unsigned long eventId, void*)
    {
      switch (eventId)
        {
        case vtkCommand::StartInteractionEvent:
          this->SeedWidget->StartSeedInteraction(this->SeedNumber);
          break;
        case vtkCommand::InteractionEvent:
          this->SeedWidget->SeedInteraction(this->SeedNumber);
          break;
        case vtkCommand::EndInteractionEvent:
          this->SeedWidget->EndSeedInteraction(this->SeedNumber);
          break;
        }
    }
  int SeedNumber;
  vtkSeedWidget *SeedWidget;
};


//----------------------------------------------------------------------
vtkSeedWidget::vtkSeedWidget()
{
  this->ManagesCursor = 0;

  this->WidgetState = vtkSeedWidget::Start;
  this->CurrentHandleNumber = 0;

  // The widgets for moving the seeds.
  this->Seeds = new vtkSeedList;

  // These are the event callbacks supported by this widget
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent,
                                          vtkWidgetEvent::AddPoint,
                                          this, vtkSeedWidget::AddPointAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonPressEvent,
                                          vtkWidgetEvent::Completed,
                                          this, vtkSeedWidget::CompletedAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MouseMoveEvent,
                                          vtkWidgetEvent::Move,
                                          this, vtkSeedWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
                                          vtkWidgetEvent::EndSelect,
                                          this, vtkSeedWidget::EndSelectAction);
}

//----------------------------------------------------------------------
vtkSeedWidget::~vtkSeedWidget()
{
  // Loop over all seeds releasing their observes and deleting them
  vtkSeedListIterator siter;
  for (siter=this->Seeds->begin(); siter != this->Seeds->end(); ++siter )
    {
    (*siter)->RemoveObservers(vtkCommand::StartInteractionEvent);
    (*siter)->RemoveObservers(vtkCommand::InteractionEvent);
    (*siter)->RemoveObservers(vtkCommand::EndInteractionEvent);
    (*siter)->Delete();
    }
  delete this->Seeds;
}

//----------------------------------------------------------------------
void vtkSeedWidget::CreateDefaultRepresentation()
{
  if ( ! this->WidgetRep )
    {
    this->WidgetRep = vtkSeedRepresentation::New();
    }
}

//----------------------------------------------------------------------
void vtkSeedWidget::SetEnabled(int enabling)
{
  this->Superclass::SetEnabled(enabling);
  
  if ( ! enabling )
    {
    this->CurrentHandleNumber = 0;
    this->WidgetState = vtkSeedWidget::Start;
    vtkSeedListIterator siter;
    for (siter=this->Seeds->begin(); siter != this->Seeds->end(); ++siter )
      {
      (*siter)->SetEnabled(0);
      }
    }
}

//-------------------------------------------------------------------------
vtkHandleWidget *vtkSeedWidget::CreateHandleWidget(vtkSeedWidget *self, 
                                                   vtkSeedRepresentation *rep)
{
  // Create the handle widget or reuse an old one
  vtkHandleWidget *widget;
  if ( self->CurrentHandleNumber < self->Seeds->size() )
    {
    widget = (*self->Seeds)[self->CurrentHandleNumber];
    }
  else
    {
    widget = vtkHandleWidget::New();
    }
  
  // Configure the handle widget
  self->CurrentHandleWidget = widget;
  widget->SetParent(self);
  widget->SetInteractor(self->Interactor);

  vtkHandleRepresentation *handleRep = rep->GetHandleRepresentation(self->CurrentHandleNumber);
  handleRep->SetRenderer(self->CurrentRenderer);
  widget->SetRepresentation(handleRep);

  vtkSeedWidgetCallback *cbk = vtkSeedWidgetCallback::New();
  cbk->SeedNumber = self->CurrentHandleNumber;
  cbk->SeedWidget = self;
  widget->AddObserver(vtkCommand::StartInteractionEvent, cbk, self->Priority);
  widget->AddObserver(vtkCommand::InteractionEvent, cbk, self->Priority);
  widget->AddObserver(vtkCommand::EndInteractionEvent, cbk, self->Priority);
  cbk->Delete(); //okay reference counting
  
  // Now place the widget into the list of handle widgets (if not already there)
  if ( self->CurrentHandleNumber >= self->Seeds->size() )
    {
    self->Seeds->resize(self->CurrentHandleNumber+1,NULL);
    (*self->Seeds)[self->CurrentHandleNumber] = widget;
    }

  return widget;
}


// The following methods are the callbacks that the seed widget responds to. 
//-------------------------------------------------------------------------
void vtkSeedWidget::AddPointAction(vtkAbstractWidget *w)
{
  vtkSeedWidget *self = reinterpret_cast<vtkSeedWidget*>(w);

  // Need to distinguish between placing handles and manipulating handles
  if ( self->WidgetState == vtkSeedWidget::MovingSeed )
    {
    return;
    }

  // compute some info we need for all cases
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // When a seed is placed, a new handle widget must be created and enabled.
  int state = self->WidgetRep->ComputeInteractionState(X,Y);
  if ( state == vtkSeedRepresentation::NearSeed )
    {
    self->WidgetState = vtkSeedWidget::MovingSeed;
    self->CurrentHandleNumber = reinterpret_cast<vtkSeedRepresentation*>(self->WidgetRep)->
      GetActiveHandle();

    // Invoke an event on ourself for the handles 
    self->InvokeEvent(vtkCommand::LeftButtonPressEvent,NULL);
    }

  else //we are placing a new seed
    {
    self->WidgetState = vtkSeedWidget::PlacingSeed;
    double e[3]; e[2]=0.0;
    e[0] = static_cast<double>(X);
    e[1] = static_cast<double>(Y);
    vtkSeedRepresentation *rep = reinterpret_cast<vtkSeedRepresentation*>(self->WidgetRep);
    self->CurrentHandleNumber = rep->CreateHandle(e);
    self->CurrentHandleWidget = self->CreateHandleWidget(self,rep);
    rep->SetSeedDisplayPosition(self->CurrentHandleNumber,e);
    self->CurrentHandleWidget->SetEnabled(1);
    self->InvokeEvent(vtkCommand::PlacePointEvent,(void*)&(self->CurrentHandleNumber));
    }

  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(vtkCommand::InteractionEvent,NULL);
  self->Render();
}


//-------------------------------------------------------------------------
void vtkSeedWidget::CompletedAction(vtkAbstractWidget *w)
{
  vtkSeedWidget *self = reinterpret_cast<vtkSeedWidget*>(w);

  // Do something only if we are in the middle of placing the seeds
  if ( self->WidgetState != vtkSeedWidget::PlacingSeed )
    {
    return;
    }
  
  // All we do is set the state to placed
  self->WidgetState = vtkSeedWidget::Placed;
  self->EventCallbackCommand->SetAbortFlag(1);
}

//-------------------------------------------------------------------------
void vtkSeedWidget::MoveAction(vtkAbstractWidget *w)
{
  vtkSeedWidget *self = reinterpret_cast<vtkSeedWidget*>(w);

  // Do nothing if outside
  if ( self->WidgetState == vtkSeedWidget::Start ||
       self->WidgetState == vtkSeedWidget::Placed ||
       self->WidgetState == vtkSeedWidget::PlacingSeed )
    {
    return;
    }

  // else we are moving a seed
  self->InvokeEvent(vtkCommand::MouseMoveEvent, NULL);

  self->WidgetRep->BuildRepresentation();
  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(vtkCommand::InteractionEvent,NULL);
  self->Render();
}

//-------------------------------------------------------------------------
void vtkSeedWidget::EndSelectAction(vtkAbstractWidget *w)
{
  vtkSeedWidget *self = reinterpret_cast<vtkSeedWidget*>(w);

  // Do nothing if outside
  if ( self->WidgetState != vtkSeedWidget::MovingSeed )
    {
    return;
    }

  self->WidgetState = vtkSeedWidget::Placed;
  self->InvokeEvent(vtkCommand::LeftButtonReleaseEvent,NULL);

  self->WidgetRep->BuildRepresentation();
  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(vtkCommand::InteractionEvent,NULL);
  self->Render();
}

// These are callbacks that are active when the user is manipulating the
// handles of the seed widget.
//----------------------------------------------------------------------
void vtkSeedWidget::StartSeedInteraction(int)
{
  this->Superclass::StartInteraction();
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
}

//----------------------------------------------------------------------
void vtkSeedWidget::SeedInteraction(int)
{
//  double pos[3];
//  reinterpret_cast<vtkSeedRepresentation*>(this->WidgetRep)->
//    GetHandleRepresentation(handle)->GetDisplayPosition(pos);
//  reinterpret_cast<vtkSeedRepresentation*>(this->WidgetRep)->
//    SetSeedDisplayPosition(handle,pos);

  this->InvokeEvent(vtkCommand::InteractionEvent,NULL);
}

//----------------------------------------------------------------------
void vtkSeedWidget::EndSeedInteraction(int)
{
  this->Superclass::EndInteraction();

  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
}

//----------------------------------------------------------------------
void vtkSeedWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);
}
