/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractWidget.cxx,v

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

vtkCxxRevisionMacro(vtkAbstractWidget, "1.8");


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
  this->Visibility = 1;
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
  if (enabling == this->Enabled)
    {
    return;
    }

  // Enforce the requirement that if a widget is enabled, it is also visible
  if (enabling && !this->Visibility)
    {
    return;
    }
  this->Enabled = enabling;
  this->SetVisibilityAndEnabledState();
}

//----------------------------------------------------------------------
void vtkAbstractWidget::SetVisibility(int v)
{
  // Enforce the requirement that if a widget is invisible, it is also disabled
  if ((v == this->Visibility) && ((v*this->Enabled) == this->Enabled))
    {
    return;
    }
  
  this->Visibility = v;
  this->Enabled    = v * this->Enabled;

  this->SetVisibilityAndEnabledState();
}

//----------------------------------------------------------------------
void vtkAbstractWidget::SetVisibilityAndEnabledState()
{
  if (this->Enabled) //----------------
    {
    if ( ! this->Interactor )
      {
      // The interactor must be set prior to enabling the widget
      this->Enabled = 0;
      return;
      }

    int X=this->Interactor->GetEventPosition()[0];
    int Y=this->Interactor->GetEventPosition()[1];

    if ( ! this->CurrentRenderer )
      {
      this->SetCurrentRenderer(this->Interactor->FindPokedRenderer(X,Y));

      if (this->CurrentRenderer == NULL)
        {
        this->Enabled = 0;
        this->Visibility = 0;
        return;
        }
      }

    // We're ready to enable
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
      this->WidgetRep->ComputeInteractionState(X, Y);
      this->SetCursor(this->WidgetRep->GetInteractionState());
      }

    this->WidgetRep->BuildRepresentation();
    if (this->Visibility)
      {
      this->CurrentRenderer->AddViewProp(this->WidgetRep);
      }

    this->InvokeEvent(vtkCommand::EnableEvent,NULL);
    }
  
  else //disabling------------------
    {
    vtkDebugMacro(<<"Disabling widget");

    // don't listen for events any more
    if ( !this->Parent  )
      {
      if (this->Interactor)
        {
        this->Interactor->RemoveObserver(this->EventCallbackCommand);
        }
      }
    else
      {
      this->Parent->RemoveObserver(this->EventCallbackCommand);
      }

    if (this->CurrentRenderer)
      {
      if (!this->Visibility)
        {
        this->CurrentRenderer->RemoveViewProp(this->WidgetRep);
        this->SetCurrentRenderer(NULL);
        }
      else
        {
        this->CurrentRenderer->AddViewProp(this->WidgetRep);   
        }
      }
    else if (this->Visibility && this->Interactor)
      {
      int X=this->Interactor->GetEventPosition()[0];
      int Y=this->Interactor->GetEventPosition()[1];
      this->SetCurrentRenderer(this->Interactor->FindPokedRenderer(X,Y));
      if (this->CurrentRenderer == NULL)
        {
        this->Visibility = 0;
        }
      else
        {
        this->CurrentRenderer->AddViewProp(this->WidgetRep);
        }
      }
    else
      {
      this->Visibility = 0;
      }
    this->InvokeEvent(vtkCommand::DisableEvent,NULL);
    }

  if (this->Interactor)
    {
    this->Interactor->Render();
    }
}

//-------------------------------------------------------------------------
void vtkAbstractWidget::ProcessEvents(vtkObject* vtkNotUsed(object), 
                                       unsigned long vtkEvent,
                                       void* clientdata, 
                                       void* calldata)
{
  vtkAbstractWidget* self = 
    reinterpret_cast<vtkAbstractWidget *>( clientdata );
  
  unsigned long widgetEvent = 
    self->EventTranslator->GetTranslation(vtkEvent,
                                          vtkEvent::GetModifier(self->Interactor),
                                          self->Interactor->GetKeyCode(),
                                          self->Interactor->GetRepeatCount(),
                                          self->Interactor->GetKeySym());

  // Save the call data for widgets if needed
  self->CallData = calldata;

  // Invoke the widget callback
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
