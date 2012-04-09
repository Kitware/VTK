/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCaptionWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCaptionWidget.h"
#include "vtkCaptionRepresentation.h"
#include "vtkObjectFactory.h"
#include "vtkCommand.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkHandleWidget.h"
#include "vtkPointHandleRepresentation3D.h"
#include "vtkCallbackCommand.h"
#include "vtkRenderWindow.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"

vtkStandardNewMacro(vtkCaptionWidget);

// The point widget invokes events that we watch for. Basically
// the attachment/anchor point is moved with the point widget.
class vtkCaptionAnchorCallback : public vtkCommand
{
public:
  static vtkCaptionAnchorCallback *New()
    { return new vtkCaptionAnchorCallback; }
  virtual void Execute(vtkObject*, unsigned long eventId, void*)
    {
      switch (eventId)
        {
        case vtkCommand::StartInteractionEvent:
          this->CaptionWidget->StartAnchorInteraction();
          break;
        case vtkCommand::InteractionEvent:
          this->CaptionWidget->AnchorInteraction();
          break;
        case vtkCommand::EndInteractionEvent:
          this->CaptionWidget->EndAnchorInteraction();
          break;
        }
    }
  vtkCaptionAnchorCallback():CaptionWidget(0) {}
  vtkCaptionWidget *CaptionWidget;
};


//-------------------------------------------------------------------------
vtkCaptionWidget::vtkCaptionWidget()
{
  // The priority of the point widget is set a little higher than me.
  // This is so Enable/Disable events are caught by the anchor and then
  // dispatched to the BorderWidget.
  this->HandleWidget = vtkHandleWidget::New();
  this->HandleWidget->SetPriority(this->Priority+0.01);
  this->HandleWidget->KeyPressActivationOff();

  // over ride the call back mapper on the border widget superclass to move
  // the caption widget using the left mouse button (still moves on middle
  // mouse button press). Release is already mapped to end select action
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent,
                                          vtkWidgetEvent::Select,
                                          this, vtkBorderWidget::TranslateAction);

  this->AnchorCallback = vtkCaptionAnchorCallback::New();
  this->AnchorCallback->CaptionWidget = this;
  this->HandleWidget->AddObserver(vtkCommand::StartInteractionEvent, this->AnchorCallback, 1.0);
  this->HandleWidget->AddObserver(vtkCommand::InteractionEvent, this->AnchorCallback, 1.0);
  this->HandleWidget->AddObserver(vtkCommand::EndInteractionEvent, this->AnchorCallback, 1.0);
}

//-------------------------------------------------------------------------
vtkCaptionWidget::~vtkCaptionWidget()
{
  this->HandleWidget->Delete();
  this->AnchorCallback->Delete();
}

//----------------------------------------------------------------------
void vtkCaptionWidget::SetEnabled(int enabling)
{
  if ( this->Interactor )
    {
    this->Interactor->Disable(); //avoid extra renders
    }

  if ( enabling )
    {
    this->CreateDefaultRepresentation();
    this->HandleWidget->SetRepresentation(reinterpret_cast<vtkCaptionRepresentation*>
                                          (this->WidgetRep)->GetAnchorRepresentation());
    this->HandleWidget->SetInteractor(this->Interactor);
    this->HandleWidget->SetEnabled(1);
    }
  else
    {
    this->HandleWidget->SetEnabled(0);
    }
  if ( this->Interactor )
    {
    this->Interactor->Enable();
    }

  this->Superclass::SetEnabled(enabling);
}

//----------------------------------------------------------------------
void vtkCaptionWidget::CreateDefaultRepresentation()
{
  if ( ! this->WidgetRep )
    {
    this->WidgetRep = vtkCaptionRepresentation::New();
    }
}

//-------------------------------------------------------------------------
void vtkCaptionWidget::SetCaptionActor2D(vtkCaptionActor2D *capActor)
{
  vtkCaptionRepresentation *capRep = reinterpret_cast<vtkCaptionRepresentation*>(this->WidgetRep);
  if ( ! capRep )
    {
    this->CreateDefaultRepresentation();
    capRep = reinterpret_cast<vtkCaptionRepresentation*>(this->WidgetRep);
    }

  if ( capRep->GetCaptionActor2D() != capActor )
    {
    capRep->SetCaptionActor2D(capActor);
    this->Modified();
    }
}

//-------------------------------------------------------------------------
vtkCaptionActor2D *vtkCaptionWidget::GetCaptionActor2D()
{
  vtkCaptionRepresentation *capRep = reinterpret_cast<vtkCaptionRepresentation*>(this->WidgetRep);
  if ( ! capRep )
    {
    return NULL;
    }
  else
    {
    return capRep->GetCaptionActor2D();
    }
}

//----------------------------------------------------------------------
void vtkCaptionWidget::StartAnchorInteraction()
{
  this->Superclass::StartInteraction();
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
}

//----------------------------------------------------------------------
void vtkCaptionWidget::AnchorInteraction()
{
  vtkCaptionRepresentation *rep=  reinterpret_cast<vtkCaptionRepresentation*>(this->WidgetRep);
  double pos[3];
  rep->GetAnchorRepresentation()->GetWorldPosition(pos);
  rep->SetAnchorPosition(pos);
  this->InvokeEvent(vtkCommand::InteractionEvent,NULL);
}

//----------------------------------------------------------------------
void vtkCaptionWidget::EndAnchorInteraction()
{
  this->Superclass::EndInteraction();
  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
}

//-------------------------------------------------------------------------
void vtkCaptionWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

}
