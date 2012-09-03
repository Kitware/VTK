/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorObserver.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInteractorObserver.h"

#include "vtkAbstractPropPicker.h"
#include "vtkCallbackCommand.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkObserverMediator.h"
#include "vtkPickingManager.h"


vtkCxxSetObjectMacro(vtkInteractorObserver,DefaultRenderer,vtkRenderer);

//----------------------------------------------------------------------------
vtkInteractorObserver::vtkInteractorObserver()
{
  this->Enabled = 0;

  this->Interactor = NULL;

  this->EventCallbackCommand = vtkCallbackCommand::New();
  this->EventCallbackCommand->SetClientData(this);

  //subclass has to invoke SetCallback()

  this->KeyPressCallbackCommand = vtkCallbackCommand::New();
  this->KeyPressCallbackCommand->SetClientData(this);
  this->KeyPressCallbackCommand->SetCallback(vtkInteractorObserver::ProcessEvents);

  this->CurrentRenderer = NULL;
  this->DefaultRenderer = NULL;

  this->Priority = 0.0f;
  this->PickingManaged = true;

  this->KeyPressActivation = 1;
  this->KeyPressActivationValue = 'i';

  this->CharObserverTag = 0;
  this->DeleteObserverTag = 0;

  this->ObserverMediator = 0;
}

//----------------------------------------------------------------------------
vtkInteractorObserver::~vtkInteractorObserver()
{
  this->UnRegisterPickers();

  this->SetEnabled(0);
  this->SetCurrentRenderer(NULL);
  this->SetDefaultRenderer(NULL);
  this->EventCallbackCommand->Delete();
  this->KeyPressCallbackCommand->Delete();
  this->SetInteractor(0);
}

//----------------------------------------------------------------------------
void vtkInteractorObserver::SetCurrentRenderer(vtkRenderer *_arg)
{
  if (this->CurrentRenderer == _arg)
    {
    return;
    }

  if (this->CurrentRenderer != NULL)
    {
    this->CurrentRenderer->UnRegister(this);
    }

  // WARNING: see .h, if the DefaultRenderer is set, whatever the value
  // of _arg (except NULL), we are going to use DefaultRenderer
  // Normally when the widget is activated (SetEnabled(1) or when
  // keypress activation takes place), the renderer over which the mouse
  // pointer is positioned is used to call SetCurrentRenderer().
  // Alternatively, we may want to specify a user-defined renderer to bind the
  // interactor to when the interactor observer is activated.
  // The problem is that in many 3D widgets, when SetEnabled(0) is called,
  // the CurrentRender is set to NULL. In that case, the next time
  // SetEnabled(1) is called, the widget will try to set CurrentRenderer
  // to the renderer over which the mouse pointer is positioned, and we
  // will use our user-defined renderer. To solve that, we introduced the
  // DefaultRenderer ivar, which will be used to force the value of
  // CurrentRenderer each time SetCurrentRenderer is called (i.e., no matter
  // if SetCurrentRenderer is called with the renderer that was poked at
  // the mouse coords, the DefaultRenderer will be used).

  if (_arg && this->DefaultRenderer)
    {
    _arg = this->DefaultRenderer;
    }

  this->CurrentRenderer = _arg;

  if (this->CurrentRenderer != NULL)
    {
    this->CurrentRenderer->Register(this);
    }

  this->Modified();
}

//----------------------------------------------------------------------------
// This adds the keypress event observer and the delete event observer
void vtkInteractorObserver::SetInteractor(vtkRenderWindowInteractor* i)
{
  if (i == this->Interactor)
    {
    return;
    }

  // Since the observer mediator is bound to the interactor, reset it to
  // 0 so that the next time it is requested, it is queried from the
  // new interactor.
  // Furthermore, remove ourself from the mediator queue.

  if (this->ObserverMediator)
    {
    this->ObserverMediator->RemoveAllCursorShapeRequests(this);
    this->ObserverMediator = 0;
    }

  // if we already have an Interactor then stop observing it
  if (this->Interactor)
    {
    this->SetEnabled(0); //disable the old interactor
    this->Interactor->RemoveObserver(this->CharObserverTag);
    this->CharObserverTag = 0;
    this->Interactor->RemoveObserver(this->DeleteObserverTag);
    this->DeleteObserverTag = 0;
    }

  this->Interactor = i;

  // add observers for each of the events handled in ProcessEvents
  if (i)
    {
    this->CharObserverTag = i->AddObserver(vtkCommand::CharEvent,
                                           this->KeyPressCallbackCommand,
                                           this->Priority);
    this->DeleteObserverTag = i->AddObserver(vtkCommand::DeleteEvent,
                                             this->KeyPressCallbackCommand,
                                             this->Priority);

    this->RegisterPickers();
    }

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkInteractorObserver::RegisterPickers()
{
}

//----------------------------------------------------------------------------
void vtkInteractorObserver::UnRegisterPickers()
{
  vtkPickingManager* pm = this->GetPickingManager();
  if (!pm)
    {
    return;
    }

  pm->RemoveObject(this);
}

//----------------------------------------------------------------------------
void vtkInteractorObserver::PickersModified()
{
  if (!this->GetPickingManager())
    {
    return;
    }

  this->UnRegisterPickers();
  this->RegisterPickers();
}

//----------------------------------------------------------------------------
vtkPickingManager* vtkInteractorObserver::GetPickingManager()
{
  return this->Interactor ? this->Interactor->GetPickingManager() : 0;
}

//------------------------------------------------------------------------------
vtkAssemblyPath* vtkInteractorObserver::
GetAssemblyPath(double X, double Y, double Z, vtkAbstractPropPicker* picker)
{
  if (!this->GetPickingManager())
    {
    picker->Pick(X, Y, Z, this->CurrentRenderer);
    return picker->GetPath();
    }

  return this->GetPickingManager()->GetAssemblyPath(
    X, Y, Z, picker, this->CurrentRenderer, this);
}

//----------------------------------------------------------------------------
void vtkInteractorObserver::ProcessEvents(vtkObject* vtkNotUsed(object),
                                          unsigned long event,
                                          void* clientdata,
                                          void* vtkNotUsed(calldata))
{
  if (event == vtkCommand::CharEvent ||
      event == vtkCommand::DeleteEvent)
    {
    vtkObject *vobj = reinterpret_cast<vtkObject *>( clientdata );
    vtkInteractorObserver* self
      = vtkInteractorObserver::SafeDownCast(vobj);
    if (self)
      {
      if (event == vtkCommand::CharEvent)
        {
        self->OnChar();
        }
      else // delete event
        {
        self->SetInteractor(0);
        }
      }
    else
      {
      vtkGenericWarningMacro("Process Events received a bad client data. The client data class name was " << vobj->GetClassName());
      }
    }
}

//----------------------------------------------------------------------------
void vtkInteractorObserver::StartInteraction()
{
  this->Interactor->GetRenderWindow()->SetDesiredUpdateRate(this->Interactor->GetDesiredUpdateRate());
}

//----------------------------------------------------------------------------
void vtkInteractorObserver::EndInteraction()
{
  this->Interactor->GetRenderWindow()->SetDesiredUpdateRate(this->Interactor->GetStillUpdateRate());
}

//----------------------------------------------------------------------------
// Description:
// Transform from display to world coordinates.
// WorldPt has to be allocated as 4 vector
void vtkInteractorObserver::ComputeDisplayToWorld(vtkRenderer *ren,
                                                  double x,
                                                  double y,
                                                  double z,
                                                  double worldPt[4])
{
  ren->SetDisplayPoint(x, y, z);
  ren->DisplayToWorld();
  ren->GetWorldPoint(worldPt);
  if (worldPt[3])
    {
    worldPt[0] /= worldPt[3];
    worldPt[1] /= worldPt[3];
    worldPt[2] /= worldPt[3];
    worldPt[3] = 1.0;
    }
}


//----------------------------------------------------------------------------
// Description:
// Transform from world to display coordinates.
// displayPt has to be allocated as 3 vector
void vtkInteractorObserver::ComputeWorldToDisplay(vtkRenderer *ren,
                                                  double x,
                                                  double y,
                                                  double z,
                                                  double displayPt[3])
{
  ren->SetWorldPoint(x, y, z, 1.0);
  ren->WorldToDisplay();
  ren->GetDisplayPoint(displayPt);
}

//----------------------------------------------------------------------------
// Description:
// Transform from display to world coordinates.
// WorldPt has to be allocated as 4 vector
void vtkInteractorObserver::ComputeDisplayToWorld(double x,
                                                  double y,
                                                  double z,
                                                  double worldPt[4])
{
  if ( !this->CurrentRenderer )
    {
    return;
    }

  this->ComputeDisplayToWorld(this->CurrentRenderer, x, y, z, worldPt);
}


//----------------------------------------------------------------------------
// Description:
// Transform from world to display coordinates.
// displayPt has to be allocated as 3 vector
void vtkInteractorObserver::ComputeWorldToDisplay(double x,
                                                  double y,
                                                  double z,
                                                  double displayPt[3])
{
  if ( !this->CurrentRenderer )
    {
    return;
    }

  this->ComputeWorldToDisplay(this->CurrentRenderer, x, y, z, displayPt);
}

//----------------------------------------------------------------------------
void vtkInteractorObserver::OnChar()
{
  // catch additional keycodes otherwise
  if ( this->KeyPressActivation )
    {
    if (this->Interactor->GetKeyCode() == this->KeyPressActivationValue )
      {
      if ( !this->Enabled )
        {
        this->On();
        }
      else
        {
        this->Off();
        }
      this->KeyPressCallbackCommand->SetAbortFlag(1);
      }
    }//if activation enabled
}

//----------------------------------------------------------------------------
void vtkInteractorObserver::GrabFocus(vtkCommand *mouseEvents, vtkCommand *keypressEvents)
{
  if ( this->Interactor )
    {
    this->Interactor->GrabFocus(mouseEvents,keypressEvents);
    }
}


//----------------------------------------------------------------------------
void vtkInteractorObserver::ReleaseFocus()
{
  if ( this->Interactor )
    {
    this->Interactor->ReleaseFocus();
    }
}

//----------------------------------------------------------------------------
int vtkInteractorObserver::RequestCursorShape(int requestedShape)
{
  if ( !this->Interactor )
    {
    return 0;
    }

  if ( ! this->ObserverMediator )
    {
    this->ObserverMediator = this->Interactor->GetObserverMediator();
    }
  int status = this->ObserverMediator->RequestCursorShape(this,requestedShape);
  if ( status )
    {
    this->InvokeEvent(vtkCommand::CursorChangedEvent,NULL);
    }
  return status;
}

//----------------------------------------------------------------------------
void vtkInteractorObserver::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Current Renderer: " << this->CurrentRenderer << "\n";
  os << indent << "Default Renderer: " << this->DefaultRenderer << "\n";
  os << indent << "Enabled: " << this->Enabled << "\n";
  os << indent << "Priority: " << this->Priority << "\n";
  os << indent << "Interactor: " << this->Interactor << "\n";
  os << indent << "Key Press Activation: "
     << (this->KeyPressActivation ? "On" : "Off") << "\n";
  os << indent << "Key Press Activation Value: "
     << this->KeyPressActivationValue << "\n";
}


