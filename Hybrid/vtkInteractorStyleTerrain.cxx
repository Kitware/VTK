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
#include "vtkPropPicker.h"
#include "vtkSphereSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkInteractorStyleTerrain, "1.2");
vtkStandardNewMacro(vtkInteractorStyleTerrain);

vtkInteractorStyleTerrain::vtkInteractorStyleTerrain()
{
  this->State = vtkInteractorStyleTerrain::Start;
  this->EventCallbackCommand->SetCallback(
    vtkInteractorStyleTerrain::ProcessEvents);
  
  this->LatLongLines = 0;
  this->Picker = vtkPropPicker::New();
  this->LatLongSphere = vtkSphereSource::New();
  this->LatLongSphere->SetPhiResolution(13);
  this->LatLongSphere->SetThetaResolution(25);
  this->LatLongSphere->LatLongTessellationOn();
  this->LatLongMapper = vtkPolyDataMapper::New();
  this->LatLongMapper->SetInput(this->LatLongSphere->GetOutput());
  this->LatLongActor = vtkActor::New();
  this->LatLongActor->SetMapper(this->LatLongMapper);
  this->LatLongActor->PickableOff();

}

vtkInteractorStyleTerrain::~vtkInteractorStyleTerrain()
{
  this->Picker->Delete();
  this->LatLongSphere->Delete();
  this->LatLongMapper->Delete();
  this->LatLongActor->Delete();
}

void vtkInteractorStyleTerrain::SetEnabled(int enabling)
{
  if ( ! this->Interactor )
    {
    vtkErrorMacro(<<"The interactor must be set prior to enabling/disabling widget");
    return;
    }

  if ( enabling ) //----------------------------------------------------------
    {
    vtkDebugMacro(<<"Enabling widget");

    if ( this->Enabled ) //already enabled, just return
      {
      return;
      }
    
    this->Enabled = 1;
    this->InvokeEvent(vtkCommand::EnableEvent,NULL);
    }

  else //disabling-------------------------------------------------------------
    {
    vtkDebugMacro(<<"Disabling widget");

    if ( ! this->Enabled ) //already disabled, just return
      {
      return;
      }
    
    this->Enabled = 0;
    this->InvokeEvent(vtkCommand::DisableEvent,NULL);
    }
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
  
  os << indent << "Latitude/Longitude Lines: " 
     << (this->LatLongLines ? "On\n" : "Off\n");
}


void vtkInteractorStyleTerrain::OnLeftButtonDown (int vtkNotUsed(ctrl), 
                                                  int vtkNotUsed(shift), 
                                                  int X, int Y)
{
  // We're only here is we are enabled
  this->State = vtkInteractorStyleTerrain::Rotating;
  
  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
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

void vtkInteractorStyleTerrain::OnMiddleButtonDown (int vtkNotUsed(ctrl), 
                                                  int vtkNotUsed(shift), 
                                                  int X, int Y)
{
  // We're only here is we are enabled
  this->State = vtkInteractorStyleTerrain::Panning;
  
  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  this->Interactor->Render();

  this->OldX = X;
  this->OldY = Y;
}

void vtkInteractorStyleTerrain::OnMiddleButtonUp (int vtkNotUsed(ctrl), 
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

void vtkInteractorStyleTerrain::OnRightButtonDown (int vtkNotUsed(ctrl), 
                                                  int vtkNotUsed(shift), 
                                                  int X, int Y)
{
  // We're only here is we are enabled
  this->State = vtkInteractorStyleTerrain::Zooming;
  
  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  this->Interactor->Render();

  this->OldX = X;
  this->OldY = Y;
}

void vtkInteractorStyleTerrain::OnRightButtonUp (int vtkNotUsed(ctrl), 
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

void vtkInteractorStyleTerrain::OnMouseMove (int vtkNotUsed(ctrl), int shift, 
                                             int X, int Y)
{
  // See whether we're active
  if ( this->State == vtkInteractorStyleTerrain::Outside || 
       this->State == vtkInteractorStyleTerrain::Start )
    {
    return;
    }

  // Make sure that we have a camera
  this->CurrentCamera = this->Interactor->FindPokedCamera(X,Y);
  if ( !this->CurrentCamera )
    {
    return;
    }

  // Gather necessary information
  int *size = this->Interactor->GetSize();

  // Do the right thing depending on the mouse button
  if ( this->State == vtkInteractorStyleTerrain::Rotating ) //left mouse
    {
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
    // Move the camera
    this->CurrentCamera->Elevation(e);
    this->CurrentCamera->Azimuth(a);
    }

  else if ( this->State == vtkInteractorStyleTerrain::Panning ) //middle mouse
    {
    }

  else if ( this->State == vtkInteractorStyleTerrain::Zooming ) //right mouse
    {
    }

  // Interact, if desired
  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::InteractionEvent,NULL);
  
  this->Interactor->Render();
  this->OldX = X;
  this->OldY = Y;
}


void vtkInteractorStyleTerrain::OnChar(int vtkNotUsed(ctrl), 
                                       int vtkNotUsed(shift),
                                       char keycode, 
                                       int vtkNotUsed(repeatcount))
{
  switch (keycode)
    {
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
      this->CurrentRenderer = 
        this->Interactor->FindPokedRenderer(this->OldX,this->OldY);
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

    //-----
    case 'f' :
      break;

    //-----
    case 'l' :
      this->CurrentRenderer = 
        this->Interactor->FindPokedRenderer(this->OldX,this->OldY);
      if ( this->LatLongLines ) 
        {
        this->LatLongLinesOff();
        }
      else 
        {
        float bounds[6];
        this->CurrentRenderer->ComputeVisiblePropBounds(bounds);
        float radius = sqrt((bounds[1]-bounds[0])*(bounds[1]-bounds[0]) +
                            (bounds[3]-bounds[2])*(bounds[3]-bounds[2]) +
                            (bounds[5]-bounds[4])*(bounds[5]-bounds[4])) / 2.0;
        this->LatLongSphere->SetRadius(radius);
        this->LatLongSphere->SetCenter((bounds[0]+bounds[1])/2.0,
                                       (bounds[2]+bounds[3])/2.0,
                                       (bounds[4]+bounds[5])/2.0);        
        this->LatLongLinesOn();
        }
      this->SelectRepresentation();

      this->Interactor->Render();
      break;
    }

  this->KeyPressCallbackCommand->SetAbortFlag(1);
}

void vtkInteractorStyleTerrain::SelectRepresentation()
{
  if ( ! this->CurrentRenderer )
    {
    return;
    }

  this->CurrentRenderer->RemoveActor(this->LatLongActor);
  this->LatLongActor->VisibilityOff();
  
  if ( this->LatLongLines )
    {
    this->LatLongActor->VisibilityOn();
    this->CurrentRenderer->AddActor(this->LatLongActor);
    this->LatLongActor->GetProperty()->SetRepresentationToWireframe();
    }
  
}


