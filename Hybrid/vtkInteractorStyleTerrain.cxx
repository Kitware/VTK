/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleTerrain.cxx
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
#include "vtkInteractorStyleTerrain.h"
#include "vtkMath.h"
#include "vtkCallbackCommand.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkInteractorStyleTerrain, "1.1");
vtkStandardNewMacro(vtkInteractorStyleTerrain);

vtkInteractorStyleTerrain::vtkInteractorStyleTerrain()
{
  this->State = vtkInteractorStyleTerrain::Start;
  this->EventCallbackCommand->SetCallback(
    vtkInteractorStyleTerrain::ProcessEvents);
  
}

vtkInteractorStyleTerrain::~vtkInteractorStyleTerrain()
{
}

// NOTE!!! This does not do any reference counting!!!
// This is to avoid some ugly reference counting loops 
// and the benefit of being able to hold only an entire
// renderwindow from an interactor style doesn't seem worth the
// mess.   Instead the vtkInteractorStyle sets up a DeleteEvent callback, so
// that it can tell when the vtkRenderWindowInteractor is going away.
void vtkInteractorStyleTerrain::SetInteractor(vtkRenderWindowInteractor *i)
{
  if(i == this->Interactor)
    {
    return;
    }

  // if we already have an Interactor then stop observing it
  if(this->Interactor)
    {
    this->Interactor->RemoveObserver(this->EventCallbackCommand);
    }
  this->Interactor = i;

  // add observers for each of the events handled in ProcessEvents
  if(i)
    {
    i->AddObserver(vtkCommand::MouseMoveEvent, this->EventCallbackCommand);
    i->AddObserver(vtkCommand::LeftButtonPressEvent, 
                   this->EventCallbackCommand);
    i->AddObserver(vtkCommand::LeftButtonReleaseEvent, 
                   this->EventCallbackCommand);
    i->AddObserver(vtkCommand::MiddleButtonPressEvent, 
                   this->EventCallbackCommand);
    i->AddObserver(vtkCommand::MiddleButtonReleaseEvent, 
                   this->EventCallbackCommand);
    i->AddObserver(vtkCommand::RightButtonPressEvent, 
                   this->EventCallbackCommand);
    i->AddObserver(vtkCommand::RightButtonReleaseEvent, 
                   this->EventCallbackCommand);
    i->AddObserver(vtkCommand::CharEvent, this->EventCallbackCommand);
    i->AddObserver(vtkCommand::DeleteEvent, this->EventCallbackCommand);
    }
}


void vtkInteractorStyleTerrain::ProcessEvents(vtkObject* object, 
                                              unsigned long event,
                                              void* clientdata, 
                                              void* vtkNotUsed(calldata))
{
  vtkInteractorStyleTerrain* self = 
    reinterpret_cast<vtkInteractorStyleTerrain *>( clientdata );
  vtkRenderWindowInteractor* rwi = 
    static_cast<vtkRenderWindowInteractor *>( object );
  int* XY = rwi->GetEventPosition();

  //okay, let's do the right thing
  switch(event)
    {
    case vtkCommand::LeftButtonPressEvent:
      self->OnLeftButtonDown(rwi->GetControlKey(), rwi->GetShiftKey(), 
                             XY[0], XY[1]);
      break;
    case vtkCommand::LeftButtonReleaseEvent:
      self->OnLeftButtonUp(rwi->GetControlKey(), rwi->GetShiftKey(), 
                           XY[0], XY[1]);
      break;
    case vtkCommand::MouseMoveEvent:
      self->OnMouseMove(rwi->GetControlKey(), rwi->GetShiftKey(), 
                        XY[0], XY[1]);
      break;
    case vtkCommand::CharEvent:
      self->OnChar(rwi->GetControlKey(), rwi->GetShiftKey(),
                   rwi->GetKeyCode(), rwi->GetRepeatCount());
      break;
    case vtkCommand::DeleteEvent:
      self->Interactor = 0;
      break;
    }
}



void vtkInteractorStyleTerrain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
}


void vtkInteractorStyleTerrain::OnLeftButtonDown (int vtkNotUsed(ctrl), 
                                                  int vtkNotUsed(shift), 
                                                  int X, int Y)
{
  // We're only here is we are enabled
  this->State = vtkInteractorStyleTerrain::LeftDown;
  
  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  this->Interactor->Render();

  this->OldX = X;
  this->OldY = Y;
}


void vtkInteractorStyleTerrain::OnMouseMove (int vtkNotUsed(ctrl), int shift, 
                                             int X, int Y)
{
  // See whether we're active
  if ( this->State == vtkInteractorStyleTerrain::Outside || 
       this->State == vtkInteractorStyleTerrain::Start )
    {
    return;
    }

  // Get the vector of motion
  int *size = this->Interactor->GetSize();
  float a = (float)(OldX-X) / size[0] * 180.0f;
  float e = (float)(OldY-Y) / size[1] * 180.0f;
  
  if ( shift )
    {
    if ( fabs(OldX-X) >= fabs(OldY-Y) )
      {
      e = 0.0;
      }
    else
      {
      a = 0.0;
      }
    }

  this->CurrentCamera = this->Interactor->FindPokedCamera(X,Y);
  if ( !this->CurrentCamera )
    {
    return;
    }

  this->CurrentCamera->Elevation(e);
  this->CurrentCamera->Azimuth(a);

  // Interact, if desired
  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::InteractionEvent,NULL);
  
  this->Interactor->Render();
  this->OldX = X;
  this->OldY = Y;
}


void vtkInteractorStyleTerrain::OnLeftButtonUp (int vtkNotUsed(ctrl), 
                                                int vtkNotUsed(shift), 
                                                int vtkNotUsed(X), 
                                                int vtkNotUsed(Y))
{
  if ( this->State == vtkInteractorStyleTerrain::Outside )
    {
    return;
    }

  this->State = vtkInteractorStyleTerrain::Start;
  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  this->Interactor->Render();
}


void vtkInteractorStyleTerrain::OnChar(int vtkNotUsed(ctrl), 
                                       int vtkNotUsed(shift),
                                       char keycode, 
                                       int vtkNotUsed(repeatcount))
{
  switch (keycode)
    {
    case 'Q' :
    case 'q' :
    case 'e' :
    case 'E' :
      this->Interactor->ExitCallback();
      break;

    //-----
    case 'u' :
    case 'U' :
      this->Interactor->UserCallback();
      break;

    //-----
    case 'r' :
    case 'R' :
      this->FindPokedRenderer(this->OldX, this->OldY);
      this->CurrentRenderer->ResetCamera();
      this->Interactor->Render();
      break;

    //-----
    case '3' :
      if (this->Interactor->GetRenderWindow()->GetStereoRender()) 
        {
        this->Interactor->GetRenderWindow()->StereoRenderOff();
        }
      else 
        {
        this->Interactor->GetRenderWindow()->StereoRenderOn();
        }
      this->Interactor->Render();
      break;
    }

  this->KeyPressCallbackCommand->SetAbortFlag(1);
}


