/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorObserver.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInteractorObserver.h"

#include "vtkCallbackCommand.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"

vtkCxxRevisionMacro(vtkInteractorObserver, "1.26");

vtkCxxSetObjectMacro(vtkInteractorObserver,DefaultRenderer,vtkRenderer);

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

  this->Priority = 0.0;

  this->KeyPressActivation = 1;
  this->KeyPressActivationValue = 'i';
}

vtkInteractorObserver::~vtkInteractorObserver()
{
  this->SetEnabled(0);
  this->SetCurrentRenderer(NULL);
  this->SetDefaultRenderer(NULL);
  this->EventCallbackCommand->Delete();
  this->KeyPressCallbackCommand->Delete();
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

// This adds the keypress event observer and the delete event observer
void vtkInteractorObserver::SetInteractor(vtkRenderWindowInteractor* i)
{
  if (i == this->Interactor)
    {
    return;
    }

  // if we already have an Interactor then stop observing it
  if (this->Interactor)
    {
    this->SetEnabled(0); //disable the old interactor
    this->Interactor->RemoveObserver(this->KeyPressCallbackCommand);
    }

  this->Interactor = i;

  // add observers for each of the events handled in ProcessEvents
  if (i)
    {
    i->AddObserver(vtkCommand::CharEvent, 
                   this->KeyPressCallbackCommand, 
                   this->Priority);

    i->AddObserver(vtkCommand::DeleteEvent, 
                   this->KeyPressCallbackCommand, 
                   this->Priority);
    }
  
  this->Modified();
}

void vtkInteractorObserver::ProcessEvents(vtkObject* vtkNotUsed(object), 
                                          unsigned long event,
                                          void* clientdata, 
                                          void* vtkNotUsed(calldata))
{
  vtkInteractorObserver* self 
    = reinterpret_cast<vtkInteractorObserver *>( clientdata );

  //look for char and delete events
  switch(event)
    {
    case vtkCommand::CharEvent:
      self->OnChar();
      break;
    case vtkCommand::DeleteEvent:
      //self->Interactor = NULL; //commented out, can't write to a 
      //self->Enabled = 0;       //deleted object
      break;
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
void vtkInteractorObserver::ComputeDisplayToWorld(double x, 
                                                  double y,
                                                  double z, 
                                                  double worldPt[4])
{
  if ( !this->CurrentRenderer ) 
    {
    return;
    }
  
  this->CurrentRenderer->SetDisplayPoint(x, y, z);
  this->CurrentRenderer->DisplayToWorld();
  this->CurrentRenderer->GetWorldPoint(worldPt);
  if (worldPt[3])
    {
    worldPt[0] /= worldPt[3];
    worldPt[1] /= worldPt[3];
    worldPt[2] /= worldPt[3];
    worldPt[3] = 1.0;
    }
}

void vtkInteractorObserver::ComputeDisplayToWorld(double x, 
                                                  double y,
                                                  double z, 
                                                  float worldPt[4])
{
  if ( !this->CurrentRenderer ) 
    {
    return;
    }
  
  this->CurrentRenderer->SetDisplayPoint(x, y, z);
  this->CurrentRenderer->DisplayToWorld();
  this->CurrentRenderer->GetWorldPoint(worldPt);
  if (worldPt[3])
    {
    worldPt[0] /= worldPt[3];
    worldPt[1] /= worldPt[3];
    worldPt[2] /= worldPt[3];
    worldPt[3] = 1.0;
    }
}

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
  
  this->CurrentRenderer->SetWorldPoint(x, y, z, 1.0);
  this->CurrentRenderer->WorldToDisplay();
  this->CurrentRenderer->GetDisplayPoint(displayPt);
}

// Description:
// transform from world to display coordinates.
// displayPt has to be allocated as 3 vector
void vtkInteractorObserver::ComputeWorldToDisplay(double x, 
                                                  double y,
                                                  double z, 
                                                  float displayPt[3])
{
  if ( !this->CurrentRenderer ) 
    {
    return;
    }
  
  this->CurrentRenderer->SetWorldPoint(x, y, z, 1.0);
  this->CurrentRenderer->WorldToDisplay();
  this->CurrentRenderer->GetDisplayPoint(displayPt);
}

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


