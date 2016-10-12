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



//----------------------------------------------------------------------
vtkAbstractWidget::vtkAbstractWidget()
{
  // Setup event processing
  this->EventCallbackCommand->SetCallback(
    vtkAbstractWidget::ProcessEventsHandler);

  // There is no parent to this widget currently
  this->Parent = NULL;

  // Set up the geometry
  this->WidgetRep = NULL;

  // Set priority higher than interactor styles
  this->Priority = 0.5;

  // Does this widget manage a cursor?
  this->ManagesCursor = 1;

  // Does this widget respond to interaction?
  this->ProcessEvents = 1;

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
    // Remove the representation from the renderer.
    if (this->CurrentRenderer)
    {
      this->CurrentRenderer->RemoveViewProp(this->WidgetRep);
    }
    this->WidgetRep->Delete();
  }

  this->EventTranslator->Delete();
  this->CallbackMapper->Delete();
  this->SetEnabled(0);
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

    int X=this->Interactor->GetEventPosition()[0];
    int Y=this->Interactor->GetEventPosition()[1];

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

    if (this->CurrentRenderer)
    {
      this->CurrentRenderer->RemoveViewProp(this->WidgetRep);
    }

    this->InvokeEvent(vtkCommand::DisableEvent,NULL);
    this->SetCurrentRenderer(NULL);
  }

  // We no longer call render when enabled state changes. It's the applications
  // resposibility to explicitly call render after changing enable state.
  //// Should only render if there is no parent
  //if ( this->Interactor && !this->Parent )
  //  {
  //  this->Interactor->Render();
  //  }
}

//-------------------------------------------------------------------------
void vtkAbstractWidget::ProcessEventsHandler(vtkObject* vtkNotUsed(object),
                                       unsigned long vtkEvent,
                                       void* clientdata,
                                       void* calldata)
{
  vtkAbstractWidget* self =
    reinterpret_cast<vtkAbstractWidget *>( clientdata );

  // if ProcessEvents is Off, we ignore all interaction events.
  if (!self->GetProcessEvents())
  {
    return;
  }

  int modifier = vtkEvent::GetModifier(self->Interactor);
  unsigned long widgetEvent = vtkWidgetEvent::NoEvent;

  // If neither the ctrl nor the shift keys are pressed, give
  // NoModifier a preference over AnyModifer.
  if (modifier == vtkEvent::AnyModifier)
  {
    widgetEvent = self->EventTranslator->GetTranslation(vtkEvent,
                                          vtkEvent::NoModifier,
                                          self->Interactor->GetKeyCode(),
                                          self->Interactor->GetRepeatCount(),
                                          self->Interactor->GetKeySym());
  }

  if ( widgetEvent == vtkWidgetEvent::NoEvent)
  {
    widgetEvent = self->EventTranslator->GetTranslation(vtkEvent,
                                        modifier,
                                        self->Interactor->GetKeyCode(),
                                        self->Interactor->GetRepeatCount(),
                                        self->Interactor->GetKeySym());
  }

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
void vtkAbstractWidget::SetPriority( float f )
{
  if (f != this->Priority)
  {
    this->Superclass::SetPriority(f);

    // We are going to re-add all the events to the interactor. The
    // interactor observer maintains a sorted list of command-observers, sorted
    // by prioirty. The sorting happens only during insertion of a command-
    // observer into the list. Yeah.. Look at the documentation of SetPriority
    // in vtkInteractorObserver. That documentation recommends setting the
    // interactor to NULL and back again. We won't do that because it will
    // cause two unnecessary re-renders, (cause we'd have had to enable and
    // disable the widgets).
    if (this->Enabled)
    {
      if (this->Interactor)
      {
        this->Interactor->RemoveObserver(this->CharObserverTag);
        this->Interactor->RemoveObserver(this->DeleteObserverTag);
        this->CharObserverTag = this->Interactor->AddObserver(
            vtkCommand::CharEvent,
            this->KeyPressCallbackCommand,
            this->Priority);
        this->DeleteObserverTag = this->Interactor->AddObserver(
            vtkCommand::DeleteEvent,
            this->KeyPressCallbackCommand,
            this->Priority);
      }

      if ( !this->Parent )
      {
        if(this->Interactor)
        {
          this->Interactor->RemoveObserver(this->EventCallbackCommand);
        }
      }
      else
      {
        this->Parent->RemoveObserver(this->EventCallbackCommand);
      }

      if ( ! this->Parent )
      {
        if(this->Interactor)
        {
          this->EventTranslator->AddEventsToInteractor(this->Interactor,
            this->EventCallbackCommand,this->Priority);
        }
      }
      else
      {
        this->EventTranslator->AddEventsToParent(this->Parent,
          this->EventCallbackCommand,this->Priority);
      }
    }
  }
}

//----------------------------------------------------------------------
void vtkAbstractWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ProcessEvents: "
    << (this->ProcessEvents? "On" : "Off") << "\n";

  if ( this->WidgetRep )
  {
    os << indent << "Widget Representation: " << this->WidgetRep << "\n";
  }
  else
  {
    os << indent << "Widget Representation: (none)\n";
  }

  os << indent << "Manages Cursor: "
    << (this->ManagesCursor? "On" : "Off") << "\n";

  os << indent << "Parent: " << this->Parent << "\n";
}
