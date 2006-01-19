/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContourWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkContourWidget.h"
#include "vtkOrientedGlyphContourRepresentation.h"
#include "vtkCommand.h"
#include "vtkCallbackCommand.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkSphereSource.h"
#include "vtkProperty.h"
#include "vtkProperty2D.h"
#include "vtkEvent.h"
#include "vtkWidgetEvent.h"

vtkCxxRevisionMacro(vtkContourWidget, "1.11");
vtkStandardNewMacro(vtkContourWidget);

//----------------------------------------------------------------------
vtkContourWidget::vtkContourWidget()
{
  this->ManagesCursor = 0;

  this->WidgetState = vtkContourWidget::Start;
  this->CurrentHandle = 0;

  // These are the event callbacks supported by this widget
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent,
                                          vtkWidgetEvent::Select,
                                          this, vtkContourWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonPressEvent,
                                          vtkWidgetEvent::AddFinalPoint,
                                          this, vtkContourWidget::AddFinalPointAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MouseMoveEvent,
                                          vtkWidgetEvent::Move,
                                          this, vtkContourWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
                                          vtkWidgetEvent::EndSelect,
                                          this, vtkContourWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent,
                                          vtkEvent::NoModifier, 46, 1, NULL,
                                          vtkWidgetEvent::Delete,
                                          this, vtkContourWidget::DeleteAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::CharEvent,
                                          vtkEvent::NoModifier, 46, 1, NULL,
                                          vtkWidgetEvent::Delete,
                                          this, vtkContourWidget::DeleteAction);
  
  this->CreateDefaultRepresentation();
  
}

//----------------------------------------------------------------------
vtkContourWidget::~vtkContourWidget()
{
}

//----------------------------------------------------------------------
void vtkContourWidget::CreateDefaultRepresentation()
{
  if ( ! this->WidgetRep )
    {
    vtkOrientedGlyphContourRepresentation *rep =
      vtkOrientedGlyphContourRepresentation::New();
    
    this->WidgetRep = rep;
    
    vtkSphereSource *ss = vtkSphereSource::New();
    ss->SetRadius(0.5);
    rep->SetActiveCursorShape( ss->GetOutput() );
    ss->Delete();
    
    rep->GetProperty()->SetColor(.25,1.0,.25);
    
    vtkProperty *property = 
        dynamic_cast< vtkProperty * >(rep->GetActiveProperty());
    if (property)
      {
      property->SetRepresentationToSurface();
      property->SetAmbient(0.1);
      property->SetDiffuse(0.9);
      property->SetSpecular(0.0);
      }
    }
}

//----------------------------------------------------------------------
void vtkContourWidget::SetEnabled(int enabling)
{
  // The handle widgets are not actually enabled until they are placed.
  // The handle widgets take their representation from the vtkContourRepresentation.
  if ( enabling )
    {
    if ( this->WidgetState == vtkContourWidget::Start )
      {
      reinterpret_cast<vtkContourRepresentation*>(this->WidgetRep)->VisibilityOff();    
      }
    else
      {
      reinterpret_cast<vtkContourRepresentation*>(this->WidgetRep)->VisibilityOn();    
      }
    }

  this->Superclass::SetEnabled(enabling);  
}

// The following methods are the callbacks that the measure widget responds to. 
//-------------------------------------------------------------------------
void vtkContourWidget::SelectAction(vtkAbstractWidget *w)
{
  vtkContourWidget *self = reinterpret_cast<vtkContourWidget*>(w);
  vtkContourRepresentation *rep = 
    reinterpret_cast<vtkContourRepresentation*>(self->WidgetRep);

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  double pos[2];
  pos[0] = X;
  pos[1] = Y;

  
  switch ( self->WidgetState )
    {
    case vtkContourWidget::Start:
    case vtkContourWidget::Define:
      self->AddNode();
      break;
    case vtkContourWidget::Manipulate:
      if ( rep->ActivateNode(X,Y) )
        {
        self->StartInteraction();
        rep->SetCurrentOperationToTranslate();
        rep->StartWidgetInteraction(pos);
        self->EventCallbackCommand->SetAbortFlag(1);
        }
      else if ( rep->AddNodeOnContour( X, Y ) )
        {
        if ( rep->ActivateNode(X,Y) )
          {
          rep->SetCurrentOperationToTranslate();
          rep->StartWidgetInteraction(pos);
          }
        self->EventCallbackCommand->SetAbortFlag(1);        
        }
      break;
    }
  
  if ( rep->GetNeedToRender() )
    {
    self->Render();
    rep->NeedToRenderOff();
    }
}

//-------------------------------------------------------------------------
void vtkContourWidget::AddFinalPointAction(vtkAbstractWidget *w)
{
  vtkContourWidget *self = reinterpret_cast<vtkContourWidget*>(w);
  vtkContourRepresentation *rep = 
    reinterpret_cast<vtkContourRepresentation*>(self->WidgetRep);

  if ( self->WidgetState ==  vtkContourWidget::Define &&
       rep->GetNumberOfNodes() >= 1 )
    {
    self->AddNode();
    self->WidgetState = vtkContourWidget::Manipulate;
    self->EventCallbackCommand->SetAbortFlag(1);
    }
  
  if ( rep->GetNeedToRender() )
    {
    self->Render();
    rep->NeedToRenderOff();
    }
}

//------------------------------------------------------------------------
void vtkContourWidget::AddNode()
{
  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];
  
  // If the rep already has at least 2 nodes, check how close we are to 
  // the first
  if ( reinterpret_cast<vtkContourRepresentation*>(this->WidgetRep)->
       GetNumberOfNodes() > 1 )
    {
    int pixelTolerance2 = 
      reinterpret_cast<vtkContourRepresentation*>(this->WidgetRep)->
      GetPixelTolerance();
    pixelTolerance2 *= pixelTolerance2;
    
    double displayPos[2];
    if ( !reinterpret_cast<vtkContourRepresentation*>(this->WidgetRep)->
         GetNthNodeDisplayPosition( 0, displayPos ) )
      {
      vtkErrorMacro("Can't get first node display position!");
      return;      
      }
    
    if ( (X - displayPos[0]) * (X - displayPos[0]) +
         (Y - displayPos[1]) * (Y - displayPos[1]) < 
         pixelTolerance2 )
      {
      // yes - we have made a loop. Stop defining and switch to 
      // manipulate mode
      this->WidgetState = vtkContourWidget::Manipulate;
      reinterpret_cast<vtkContourRepresentation*>(this->WidgetRep)->ClosedLoopOn();    
      this->Render();
      this->EventCallbackCommand->SetAbortFlag(1);
      this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
      return;
      }
    }
  
  if ( reinterpret_cast<vtkContourRepresentation*>(this->WidgetRep)->
       AddNodeAtDisplayPosition( X, Y ) )
    {
    if ( this->WidgetState == vtkContourWidget::Start )
      {
      this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
      }
    
    this->WidgetState = vtkContourWidget::Define;
    reinterpret_cast<vtkContourRepresentation*>(this->WidgetRep)->VisibilityOn();    
    this->EventCallbackCommand->SetAbortFlag(1);
    this->InvokeEvent(vtkCommand::InteractionEvent,NULL);
    }
}

//-------------------------------------------------------------------------
void vtkContourWidget::DeleteAction(vtkAbstractWidget *w)
{
  vtkContourWidget *self = reinterpret_cast<vtkContourWidget*>(w);

  if ( self->WidgetState == vtkContourWidget::Start )
    {
    return;
    }
  
  vtkContourRepresentation *rep = 
    reinterpret_cast<vtkContourRepresentation*>(self->WidgetRep);
  
  if ( self->WidgetState == vtkContourWidget::Define )
    {
    rep->DeleteLastNode();
    }
  else
    {
    int X = self->Interactor->GetEventPosition()[0];
    int Y = self->Interactor->GetEventPosition()[1];
    rep->ActivateNode( X, Y );
    if ( rep->DeleteActiveNode() )
      {
      self->InvokeEvent(vtkCommand::InteractionEvent,NULL);      
      }
    rep->ActivateNode( X, Y );
    if ( rep->GetNumberOfNodes() < 3 )
      {
      rep->ClosedLoopOff();
      self->WidgetState = vtkContourWidget::Define;
      }
    }
  
  if ( rep->GetNeedToRender() )
    {
    self->Render();
    rep->NeedToRenderOff();
    }
}

//-------------------------------------------------------------------------
void vtkContourWidget::MoveAction(vtkAbstractWidget *w)
{
  vtkContourWidget *self = reinterpret_cast<vtkContourWidget*>(w);

  if ( self->WidgetState == vtkContourWidget::Start ||
       self->WidgetState == vtkContourWidget::Define )
    {
    return;
    }

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  vtkContourRepresentation *rep = 
    reinterpret_cast<vtkContourRepresentation*>(self->WidgetRep);
  

  if ( rep->GetCurrentOperation() == vtkContourRepresentation::Inactive )
    {
    reinterpret_cast<vtkContourRepresentation*>(self->WidgetRep)->
      ComputeInteractionState( X, Y );
    reinterpret_cast<vtkContourRepresentation*>(self->WidgetRep)->
      ActivateNode( X, Y );
    }
  else
    {
    double pos[2];
    pos[0] = X;
    pos[1] = Y;
    self->WidgetRep->WidgetInteraction(pos);
    self->Interaction();
    }
  
  if ( self->WidgetRep->GetNeedToRender() )
    {
    self->Render();
    self->WidgetRep->NeedToRenderOff();
    }
}

//-------------------------------------------------------------------------
void vtkContourWidget::EndSelectAction(vtkAbstractWidget *w)
{
  vtkContourWidget *self = reinterpret_cast<vtkContourWidget*>(w);
  vtkContourRepresentation *rep = 
    reinterpret_cast<vtkContourRepresentation*>(self->WidgetRep);

  // Do nothing if inactive
  if ( rep->GetCurrentOperation() == vtkContourRepresentation::Inactive )
    {
    return;
    }

  rep->SetCurrentOperationToInactive();
  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  
  if ( self->WidgetRep->GetNeedToRender() )
    {
    self->Render();
    self->WidgetRep->NeedToRenderOff();
    }
}

// These are callbacks that are active when the user is manipulating the
// handles of the measure widget.
//----------------------------------------------------------------------
void vtkContourWidget::StartInteraction()
{
  this->Superclass::StartInteraction();
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
}

//----------------------------------------------------------------------
void vtkContourWidget::Interaction()
{
  this->InvokeEvent(vtkCommand::InteractionEvent,NULL);
}

//----------------------------------------------------------------------
void vtkContourWidget::EndInteraction()
{
  this->Superclass::EndInteraction();
  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
}

//----------------------------------------------------------------------
void vtkContourWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);
  
}
