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

vtkCxxRevisionMacro(vtkInteractorObserver, "1.5");

vtkInteractorObserver::vtkInteractorObserver()
{
  this->Enabled = 0;

  this->Interactor = NULL;

  this->EventCallbackCommand = vtkCallbackCommand::New();
  this->EventCallbackCommand->SetClientData(this); 
  //subclass has to invoke SetCallback()

  this->KeypressCallbackCommand = vtkCallbackCommand::New();
  this->KeypressCallbackCommand->SetClientData(this); 
  this->KeypressCallbackCommand->SetCallback(vtkInteractorObserver::ProcessEvents);
 
  this->CurrentRenderer = NULL;
  this->OldX = 0.0;
  this->OldY = 0.0;
  
  this->Priority = 1.0;
  this->KeyPressActivation = 1;
  this->KeyPressActivationValue = 'i';
}

vtkInteractorObserver::~vtkInteractorObserver()
{
  this->EventCallbackCommand->Delete();
  this->KeypressCallbackCommand->Delete();
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
    this->Interactor->RemoveObserver(this->KeypressCallbackCommand);
    }

  this->Interactor = i;

  // add observers for each of the events handled in ProcessEvents
  if (i)
    {
    i->AddObserver(vtkCommand::CharEvent, this->KeypressCallbackCommand);
    i->AddObserver(vtkCommand::DeleteEvent, this->KeypressCallbackCommand);
    }
  
  this->Modified();
}

void vtkInteractorObserver::ProcessEvents(vtkObject* object, 
                                          unsigned long event,
                                          void* clientdata, 
                                          void* vtkNotUsed(calldata))
{
  vtkInteractorObserver* self 
    = reinterpret_cast<vtkInteractorObserver *>( clientdata );
  vtkRenderWindowInteractor* rwi 
    = static_cast<vtkRenderWindowInteractor *>( object );

  //look for char and delete events
  switch(event)
    {
    case vtkCommand::CharEvent:
      self->OnChar(rwi->GetControlKey(), rwi->GetShiftKey(),
                   rwi->GetKeyCode(), rwi->GetRepeatCount());
      break;
    case vtkCommand::DeleteEvent:
      self->Interactor = NULL; //its going bye bye
      self->Enabled = 0;
      break;
    }
}

// Description:
// Transform from display to world coordinates.
// WorldPt has to be allocated as 4 vector
void vtkInteractorObserver::ComputeDisplayToWorld(double x, double y,
                                                  double z, double *worldPt)
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
void vtkInteractorObserver::ComputeWorldToDisplay(double x, double y,
                                         double z, double *displayPt)
{
  if ( !this->CurrentRenderer ) 
    {
    return;
    }
  
  this->CurrentRenderer->SetWorldPoint(x, y, z, 1.0);
  this->CurrentRenderer->WorldToDisplay();
  this->CurrentRenderer->GetDisplayPoint(displayPt);
}

void vtkInteractorObserver::OnChar(int vtkNotUsed(ctrl), int vtkNotUsed(shift),
                                   char keycode, int vtkNotUsed(repeatcount))
{
  // catch additional keycodes otherwise
  if ( this->KeyPressActivation )
    {
    if (keycode == this->KeyPressActivationValue )
      {
      if ( !this->Enabled )
        {
        this->On();
        }
      else
        {
        this->Off();
        }
      this->EventCallbackCommand->SetAbortFlag(1);
      }
    }//if activation enabled
}

void vtkInteractorObserver::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "Enabled: " << this->Enabled << "\n";
  os << indent << "Priority: " << this->Priority << "\n";
  os << indent << "Interactor: " << this->Interactor << "\n";
  os << indent << "Key Press Activation: " 
     << (this->KeyPressActivation ? "On" : "Off") << "\n";
  os << indent << "Key Press Activation Value: " 
     << this->KeyPressActivationValue << "\n";
}


