/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleSwitch.cxx
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
#include "vtkInteractorStyleSwitch.h"
#include "vtkObjectFactory.h"
#include "vtkCommand.h"
#include "vtkCallbackCommand.h"
#include "vtkInteractorStyleJoystickActor.h"
#include "vtkInteractorStyleJoystickCamera.h"
#include "vtkInteractorStyleTrackballActor.h"
#include "vtkInteractorStyleTrackballCamera.h"

vtkCxxRevisionMacro(vtkInteractorStyleSwitch, "1.10");
vtkStandardNewMacro(vtkInteractorStyleSwitch);

//----------------------------------------------------------------------------
vtkInteractorStyleSwitch::vtkInteractorStyleSwitch() 
{
  this->JoystickActor = vtkInteractorStyleJoystickActor::New();
  this->JoystickCamera = vtkInteractorStyleJoystickCamera::New();
  this->TrackballActor = vtkInteractorStyleTrackballActor::New();
  this->TrackballCamera = vtkInteractorStyleTrackballCamera::New();
  this->JoystickOrTrackball = VTKIS_JOYSTICK;
  this->CameraOrActor = VTKIS_CAMERA;
  this->CurrentStyle = 0;
}

//----------------------------------------------------------------------------
vtkInteractorStyleSwitch::~vtkInteractorStyleSwitch() 
{
  this->JoystickActor->Delete();
  this->JoystickActor = NULL;
  
  this->JoystickCamera->Delete();
  this->JoystickCamera = NULL;
  
  this->TrackballActor->Delete();
  this->TrackballActor = NULL;
  
  this->TrackballCamera->Delete();
  this->TrackballCamera = NULL;
}

void vtkInteractorStyleSwitch::SetAutoAdjustCameraClippingRange( int value )
{
  if ( value == this->AutoAdjustCameraClippingRange )
    {
    return;
    }
  
  if ( value < 0 || value > 1 )
    {
    vtkErrorMacro("Value must be between 0 and 1 for" <<
                  " SetAutoAdjustCameraClippingRange");
    return;
    }
  
  this->AutoAdjustCameraClippingRange = value;
  this->JoystickActor->SetAutoAdjustCameraClippingRange( value );
  this->JoystickCamera->SetAutoAdjustCameraClippingRange( value );
  this->TrackballActor->SetAutoAdjustCameraClippingRange( value );
  this->TrackballCamera->SetAutoAdjustCameraClippingRange( value );
  
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleSwitch::OnChar(int ctrl, int shift, 
                                      char keycode,
                                      int repeatcount) 
{
  switch (keycode)
    {
    case 'j':
    case 'J':
      this->JoystickOrTrackball = VTKIS_JOYSTICK;
      break;
    case 't':
    case 'T':
      this->JoystickOrTrackball = VTKIS_TRACKBALL;
      break;
    case 'c':
    case 'C':  
      this->CameraOrActor = VTKIS_CAMERA;
      break;
    case 'a':
    case 'A':
      this->CameraOrActor = VTKIS_ACTOR;
      break;
    default:
      vtkInteractorStyle::OnChar(ctrl, shift, keycode, repeatcount);
      break;
    }
  // Set the CurrentStyle pointer to the picked style
  this->SetCurrentStyle();
}


// this will do nothing if the CurrentStyle matchs
// JoystickOrTrackball and CameraOrActor
void vtkInteractorStyleSwitch::SetCurrentStyle()
{
  // if the currentstyle does not match JoystickOrTrackball 
  // and CameraOrActor ivars, then call SetInteractor(0)
  // on the Currentstyle to remove all of the observers.
  // Then set the Currentstyle and call SetInteractor with 
  // this->Interactor so the callbacks are set for the 
  // currentstyle.
  if (this->JoystickOrTrackball == VTKIS_JOYSTICK &&
      this->CameraOrActor == VTKIS_CAMERA)
    {
    if(this->CurrentStyle != this->JoystickCamera)
      {
      if(this->CurrentStyle)
        {
        this->CurrentStyle->SetInteractor(0);
        }
      this->CurrentStyle = this->JoystickCamera;
      this->CurrentStyle->SetInteractor(this->Interactor);
      }
    }
  else if (this->JoystickOrTrackball == VTKIS_JOYSTICK &&
           this->CameraOrActor == VTKIS_ACTOR)
    { 
    if(this->CurrentStyle != this->JoystickActor)
      {
      if(this->CurrentStyle)
        {
        this->CurrentStyle->SetInteractor(0);
        }
      this->CurrentStyle = this->JoystickActor;
      this->CurrentStyle->SetInteractor(this->Interactor);
      }
    }
  else if (this->JoystickOrTrackball == VTKIS_TRACKBALL &&
           this->CameraOrActor == VTKIS_CAMERA)
    {
    if(this->CurrentStyle != this->TrackballCamera)
      {
      if(this->CurrentStyle)
        {
        this->CurrentStyle->SetInteractor(0);
        }
      this->CurrentStyle = this->TrackballCamera;
      this->CurrentStyle->SetInteractor(this->Interactor);
      }
    }
  else if (this->JoystickOrTrackball == VTKIS_TRACKBALL &&
           this->CameraOrActor == VTKIS_ACTOR)
    { 
      if(this->CurrentStyle != this->TrackballActor)
        {
        if(this->CurrentStyle)
          {
          this->CurrentStyle->SetInteractor(0);
          }
        this->CurrentStyle = this->TrackballActor;
        this->CurrentStyle->SetInteractor(this->Interactor);
        }
    }
}


//----------------------------------------------------------------------------
void vtkInteractorStyleSwitch::SetInteractor(vtkRenderWindowInteractor *iren)
{
  if(iren == this->Interactor)
    {
    return;
    }
  // if we already have an Interactor then stop observing it
  if(this->Interactor)
    {
    this->Interactor->RemoveObserver(this->EventCallbackCommand);
    }
  this->Interactor = iren;
  // add observers for each of the events handled in ProcessEvents
  if(iren)
    {
    iren->AddObserver(vtkCommand::CharEvent, this->EventCallbackCommand);
    }
  this->SetCurrentStyle();
}


void vtkInteractorStyleSwitch::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "CurrentStyle " << this->CurrentStyle << "\n";
}

