/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleSwitch.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInteractorStyleSwitch.h"

#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkInteractorStyleJoystickActor.h"
#include "vtkInteractorStyleJoystickCamera.h"
#include "vtkInteractorStyleTrackballActor.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindowInteractor.h"

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

//----------------------------------------------------------------------------
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
void vtkInteractorStyleSwitch::SetCurrentStyleToJoystickActor()
{
  this->JoystickOrTrackball = VTKIS_JOYSTICK;
  this->CameraOrActor = VTKIS_ACTOR;
  this->SetCurrentStyle();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleSwitch::SetCurrentStyleToJoystickCamera()
{
  this->JoystickOrTrackball = VTKIS_JOYSTICK;
  this->CameraOrActor = VTKIS_CAMERA;
  this->SetCurrentStyle();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleSwitch::SetCurrentStyleToTrackballActor()
{
  this->JoystickOrTrackball = VTKIS_TRACKBALL;
  this->CameraOrActor = VTKIS_ACTOR;
  this->SetCurrentStyle();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleSwitch::SetCurrentStyleToTrackballCamera()
{
  this->JoystickOrTrackball = VTKIS_TRACKBALL;
  this->CameraOrActor = VTKIS_CAMERA;
  this->SetCurrentStyle();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleSwitch::OnChar()
{
  switch (this->Interactor->GetKeyCode())
    {
    case 'j':
    case 'J':
      this->JoystickOrTrackball = VTKIS_JOYSTICK;
      this->EventCallbackCommand->SetAbortFlag(1);
      break;
    case 't':
    case 'T':
      this->JoystickOrTrackball = VTKIS_TRACKBALL;
      this->EventCallbackCommand->SetAbortFlag(1);
      break;
    case 'c':
    case 'C':
      this->CameraOrActor = VTKIS_CAMERA;
      this->EventCallbackCommand->SetAbortFlag(1);
      break;
    case 'a':
    case 'A':
      this->CameraOrActor = VTKIS_ACTOR;
      this->EventCallbackCommand->SetAbortFlag(1);
      break;
    }
  // Set the CurrentStyle pointer to the picked style
  this->SetCurrentStyle();
}

//----------------------------------------------------------------------------
// this will do nothing if the CurrentStyle matchs
// JoystickOrTrackball and CameraOrActor
// It should! If the this->Interactor was changed (using SetInteractor()),
// and the currentstyle should not change.
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
        }
    }
  if (this->CurrentStyle)
    {
    this->CurrentStyle->SetInteractor(this->Interactor);
    this->CurrentStyle->SetTDxStyle(this->TDxStyle);
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
    iren->AddObserver(vtkCommand::CharEvent,
                      this->EventCallbackCommand,
                      this->Priority);

    iren->AddObserver(vtkCommand::DeleteEvent,
                      this->EventCallbackCommand,
                      this->Priority);
    }
  this->SetCurrentStyle();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleSwitch::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "CurrentStyle " << this->CurrentStyle << "\n";
  if (this->CurrentStyle)
    {
    vtkIndent next_indent = indent.GetNextIndent();
    os << next_indent << this->CurrentStyle->GetClassName() << "\n";
    this->CurrentStyle->PrintSelf(os, indent.GetNextIndent());
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleSwitch::SetDefaultRenderer(vtkRenderer* renderer)
{
  this->vtkInteractorStyle::SetDefaultRenderer(renderer);
  this->JoystickActor->SetDefaultRenderer(renderer);
  this->JoystickCamera->SetDefaultRenderer(renderer);
  this->TrackballActor->SetDefaultRenderer(renderer);
  this->TrackballCamera->SetDefaultRenderer(renderer);
}

//----------------------------------------------------------------------------
void vtkInteractorStyleSwitch::SetCurrentRenderer(vtkRenderer* renderer)
{
  this->vtkInteractorStyle::SetCurrentRenderer(renderer);
  this->JoystickActor->SetCurrentRenderer(renderer);
  this->JoystickCamera->SetCurrentRenderer(renderer);
  this->TrackballActor->SetCurrentRenderer(renderer);
  this->TrackballCamera->SetCurrentRenderer(renderer);
}
