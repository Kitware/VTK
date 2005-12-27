/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAbstractWidget.h"
#include "vtkWidgetRepresentation.h"
#include "vtkCommand.h"
#include "vtkCallbackCommand.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkWidgetEventTranslator.h"
#include "vtkWidgetCallbackMapper.h" 
#include "vtkRenderer.h"
#include "vtkEvent.h"
#include "vtkWidgetEvent.h"
#include "vtkObserverMediator.h"

vtkCxxRevisionMacro(vtkAbstractWidget, "1.5");


//----------------------------------------------------------------------
vtkAbstractWidget::vtkAbstractWidget()
{
  // Setup event processing
  this->EventCallbackCommand->SetCallback(vtkAbstractWidget::ProcessEvents);

  // There is no parent to this widget currently
  this->Parent = NULL;

  // Set up the geometry
  this->WidgetRep = NULL;

  // Set priority higher than interactor styles
  this->Priority = 0.5;

  // Does this widget manage a cursor?
  this->ManagesCursor = 1;

  // Okay, set up the event translations for the subclasses.
  this->EventTranslator = vtkWidgetEventTranslator::New(); 
  this->CallbackMapper = vtkWidgetCallbackMapper::New();
  this->CallbackMapper->SetEventTranslator(this->EventTranslator);
}

//----------------------------------------------------------------------
vtkAbstractWidget::~vtkAbstractWidget()
{
  if ( this->WidgetRep )
    {
    this->WidgetRep->Delete();
    }

  this->EventTranslator->Delete();
  this->CallbackMapper->Delete();
}

//----------------------------------------------------------------------
void vtkAbstractWidget::SetWidgetRepresentation(vtkWidgetRepresentation *r)
{
  if ( r != this->WidgetRep )
    {
    int enabled=0;
    if ( this->Enabled )
      {
      enabled = 1;
      this->SetEnabled(0);
      }

    if ( this->WidgetRep )
      {
      this->WidgetRep->Delete();
      }
    this->WidgetRep = r;
    if ( this->WidgetRep )
      {
      this->WidgetRep->Register(this);
      }
    this->Modified();

    if ( enabled )
      {
      this->SetEnabled(1);
      }
    }
}

//----------------------------------------------------------------------
void vtkAbstractWidget::SetEnabled(int enabling)
{
  if ( enabling ) //----------------
    {
    vtkDebugMacro(<<"Enabling widget");

    if ( this->Enabled ) //already enabled, just return
      {
      return;
      }
    
    if ( ! this->Interactor )
      {
      vtkErrorMacro(<<"The interactor must be set prior to enabling the widget");
      return;
      }

    int X=this->Interactor->GetLastEventPosition()[0];
    int Y=this->Interactor->GetLastEventPosition()[1];

    if ( ! this->CurrentRenderer )
      {
      this->SetCurrentRenderer(this->Interactor->FindPokedRenderer(X,Y));

      if (this->CurrentRenderer == NULL)
        {
        return;
        }
      }

    // We're ready to enable
    this->Enabled = 1;
    this->CreateDefaultRepresentation();
    this->WidgetRep->SetRenderer(this->CurrentRenderer);

    // listen for the events found in the EventTranslator
    if ( ! this->Parent )
      {
      this->EventTranslator->AddEventsToInteractor(this->Interactor,
                                                   this->EventCallbackCommand,this->Priority);
      }
    else
      {
      this->EventTranslator->AddEventsToParent(this->Parent,
                                               this->EventCallbackCommand,this->Priority);
      }

    if ( this->ManagesCursor )
      {
      this->ObserverMediator = this->Interactor->GetObserverMediator();
      this->WidgetRep->ComputeInteractionState(X, Y);
      this->SetCursor(this->WidgetRep->GetInteractionState());
      }

    this->WidgetRep->BuildRepresentation();
    this->CurrentRenderer->AddViewProp(this->WidgetRep);

    this->InvokeEvent(vtkCommand::EnableEvent,NULL);
    }
  
  else //disabling------------------
    {
    vtkDebugMacro(<<"Disabling widget");

    if ( ! this->Enabled ) //already disabled, just return
      {
      return;
      }
    
    this->Enabled = 0;

    // don't listen for events any more
    if ( ! this->Parent )
      {
      this->Interactor->RemoveObserver(this->EventCallbackCommand);
      }
    else
      {
      this->Parent->RemoveObserver(this->EventCallbackCommand);
      }

    this->CurrentRenderer->RemoveViewProp(this->WidgetRep);

    this->InvokeEvent(vtkCommand::DisableEvent,NULL);
    this->SetCurrentRenderer(NULL);
    }

  this->Interactor->Render();
}

//-------------------------------------------------------------------------
void vtkAbstractWidget::ProcessEvents(vtkObject* vtkNotUsed(object), 
                                       unsigned long vtkEvent,
                                       void* clientdata, 
                                       void* vtkNotUsed(calldata))
{
  vtkAbstractWidget* self = 
    reinterpret_cast<vtkAbstractWidget *>( clientdata );
  
  unsigned long widgetEvent = 
    self->EventTranslator->GetTranslation(vtkEvent,
                                          vtkEvent::GetModifier(self->Interactor),
                                          self->Interactor->GetKeyCode(),
                                          self->Interactor->GetRepeatCount(),
                                          self->Interactor->GetKeySym());

  if ( widgetEvent != vtkWidgetEvent::NoEvent)
    {
    self->CallbackMapper->InvokeCallback(widgetEvent);
    }
}

//----------------------------------------------------------------------
void vtkAbstractWidget::Render()
{
  if ( ! this->Parent && this->Interactor )
    {
    this->Interactor->Render();
    }
}

//----------------------------------------------------------------------
void vtkAbstractWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);
  
  if ( this->WidgetRep )
    {
    os << indent << "Widget Representation: " << this->WidgetRep << "\n";
    }
  else
    {
    os << indent << "Widget Representation: (none)\n";
    }
}
