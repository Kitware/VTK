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

vtkCxxRevisionMacro(vtkInteractorStyleSwitch, "1.9");
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
}

//----------------------------------------------------------------------------
void vtkInteractorStyleSwitch::OnMouseMove(int ctrl, int shift, int x, int y) 
{
  // Call the parent so the LastPos is set
  vtkInteractorStyle::OnMouseMove(ctrl, shift, x, y);
  if (this->JoystickOrTrackball == VTKIS_JOYSTICK &&
      this->CameraOrActor == VTKIS_CAMERA)
    {
    this->JoystickCamera->OnMouseMove(ctrl, shift, x, y);
    }
  else if (this->JoystickOrTrackball == VTKIS_JOYSTICK &&
           this->CameraOrActor == VTKIS_ACTOR)
    {
    this->JoystickActor->OnMouseMove(ctrl, shift, x, y);
    }
  else if (this->JoystickOrTrackball == VTKIS_TRACKBALL &&
           this->CameraOrActor == VTKIS_CAMERA)
    {
    this->TrackballCamera->OnMouseMove(ctrl, shift, x, y);
    }
  else if (this->JoystickOrTrackball == VTKIS_TRACKBALL &&
           this->CameraOrActor == VTKIS_ACTOR)
    {
    this->TrackballActor->OnMouseMove(ctrl, shift, x, y);
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleSwitch::OnLeftButtonDown(int ctrl, int shift, 
                                                int x, int y) 
{
  if (this->HasObserver(vtkCommand::LeftButtonPressEvent)) 
    {
    this->InvokeEvent(vtkCommand::LeftButtonPressEvent,NULL);
    return;
    }

  if (this->JoystickOrTrackball == VTKIS_JOYSTICK && 
      this->CameraOrActor == VTKIS_CAMERA)
    {
    this->JoystickCamera->OnLeftButtonDown(ctrl, shift, x, y);
    }
  else if (this->JoystickOrTrackball == VTKIS_JOYSTICK &&
           this->CameraOrActor == VTKIS_ACTOR)
    {
    this->JoystickActor->OnLeftButtonDown(ctrl, shift, x, y);
    }
  else if (this->JoystickOrTrackball == VTKIS_TRACKBALL &&
           this->CameraOrActor == VTKIS_CAMERA)
    {
    this->TrackballCamera->OnLeftButtonDown(ctrl, shift, x, y);
    }
  else if (this->JoystickOrTrackball == VTKIS_TRACKBALL &&
           this->CameraOrActor == VTKIS_ACTOR)
    {
    this->TrackballActor->OnLeftButtonDown(ctrl, shift, x, y);
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleSwitch::OnLeftButtonUp(int ctrl, int shift,
                                              int x, int y) 
{
  if (this->HasObserver(vtkCommand::LeftButtonReleaseEvent)) 
    {
    this->InvokeEvent(vtkCommand::LeftButtonReleaseEvent,NULL);
    return;
    }

  if (this->JoystickOrTrackball == VTKIS_JOYSTICK &&
      this->CameraOrActor == VTKIS_CAMERA)
    {
    this->JoystickCamera->OnLeftButtonUp(ctrl, shift, x, y);
    }
  else if (this->JoystickOrTrackball == VTKIS_JOYSTICK &&
           this->CameraOrActor == VTKIS_ACTOR)
    {
    this->JoystickActor->OnLeftButtonUp(ctrl, shift, x, y);
    }
  else if (this->JoystickOrTrackball == VTKIS_TRACKBALL &&
           this->CameraOrActor == VTKIS_CAMERA)
    {
    this->TrackballCamera->OnLeftButtonUp(ctrl, shift, x, y);
    }
  else if (this->JoystickOrTrackball == VTKIS_TRACKBALL &&
           this->CameraOrActor == VTKIS_ACTOR)
    {
    this->TrackballActor->OnLeftButtonUp(ctrl, shift, x, y);
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleSwitch::OnMiddleButtonDown(int ctrl, int shift, 
                                                  int x, int y) 
{
  if (this->HasObserver(vtkCommand::MiddleButtonPressEvent)) 
    {
    this->InvokeEvent(vtkCommand::MiddleButtonPressEvent,NULL);
    return;
    }

  if (this->JoystickOrTrackball == VTKIS_JOYSTICK &&
      this->CameraOrActor == VTKIS_CAMERA)
    {
    this->JoystickCamera->OnMiddleButtonDown(ctrl, shift, x, y);
    }
  else if (this->JoystickOrTrackball == VTKIS_JOYSTICK &&
           this->CameraOrActor == VTKIS_ACTOR)
    {
    this->JoystickActor->OnMiddleButtonDown(ctrl, shift, x, y);
    }
  else if (this->JoystickOrTrackball == VTKIS_TRACKBALL &&
           this->CameraOrActor == VTKIS_CAMERA)
    {
    this->TrackballCamera->OnMiddleButtonDown(ctrl, shift, x, y);
    }
  else if (this->JoystickOrTrackball == VTKIS_TRACKBALL &&
           this->CameraOrActor == VTKIS_ACTOR)
    {
    this->TrackballActor->OnMiddleButtonDown(ctrl, shift, x, y);
    }
}
//----------------------------------------------------------------------------
void vtkInteractorStyleSwitch::OnMiddleButtonUp(int ctrl, int shift, 
                                                int x, int y) 
{
  if (this->HasObserver(vtkCommand::MiddleButtonReleaseEvent)) 
    {
    this->InvokeEvent(vtkCommand::MiddleButtonReleaseEvent,NULL);
    return;
    }

  if (this->JoystickOrTrackball == VTKIS_JOYSTICK &&
      this->CameraOrActor == VTKIS_CAMERA)
    {
    this->JoystickCamera->OnMiddleButtonUp(ctrl, shift, x, y);
    }
  else if (this->JoystickOrTrackball == VTKIS_JOYSTICK &&
           this->CameraOrActor == VTKIS_ACTOR)
    {
    this->JoystickActor->OnMiddleButtonUp(ctrl, shift, x, y);
    }
  else if (this->JoystickOrTrackball == VTKIS_TRACKBALL &&
           this->CameraOrActor == VTKIS_CAMERA)
    {
    this->TrackballCamera->OnMiddleButtonUp(ctrl, shift, x, y);
    }
  else if (this->JoystickOrTrackball == VTKIS_TRACKBALL &&
           this->CameraOrActor == VTKIS_ACTOR)
    {
    this->TrackballActor->OnMiddleButtonUp(ctrl, shift, x, y);
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleSwitch::OnRightButtonDown(int ctrl, int shift,
                                                 int x, int y)
{
  if (this->HasObserver(vtkCommand::RightButtonPressEvent)) 
    {
    this->InvokeEvent(vtkCommand::RightButtonPressEvent,NULL);
    return;
    }

  if (this->JoystickOrTrackball == VTKIS_JOYSTICK &&
      this->CameraOrActor == VTKIS_CAMERA)
    {
    this->JoystickCamera->OnRightButtonDown(ctrl, shift, x, y);
    }
  else if (this->JoystickOrTrackball == VTKIS_JOYSTICK &&
           this->CameraOrActor == VTKIS_ACTOR)
    {
    this->JoystickActor->OnRightButtonDown(ctrl, shift, x, y);
    }
  else if (this->JoystickOrTrackball == VTKIS_TRACKBALL &&
           this->CameraOrActor == VTKIS_CAMERA)
    {
    this->TrackballCamera->OnRightButtonDown(ctrl, shift, x, y);
    }
  else if (this->JoystickOrTrackball == VTKIS_TRACKBALL &&
           this->CameraOrActor == VTKIS_ACTOR)
    {
    this->TrackballActor->OnRightButtonDown(ctrl, shift, x, y);
    }
}
//----------------------------------------------------------------------------
void vtkInteractorStyleSwitch::OnRightButtonUp(int ctrl, int shift,
                                               int x, int y)
{
  if (this->HasObserver(vtkCommand::RightButtonReleaseEvent)) 
    {
    this->InvokeEvent(vtkCommand::RightButtonReleaseEvent,NULL);
    return;
    }

  if (this->JoystickOrTrackball == VTKIS_JOYSTICK &&
      this->CameraOrActor == VTKIS_CAMERA)
    {
    this->JoystickCamera->OnRightButtonUp(ctrl, shift, x, y);
    }
  else if (this->JoystickOrTrackball == VTKIS_JOYSTICK &&
           this->CameraOrActor == VTKIS_ACTOR)
    {
    this->JoystickActor->OnRightButtonUp(ctrl, shift, x, y);
    }
  else if (this->JoystickOrTrackball == VTKIS_TRACKBALL &&
           this->CameraOrActor == VTKIS_CAMERA)
    {
    this->TrackballCamera->OnRightButtonUp(ctrl, shift, x, y);
    }
  else if (this->JoystickOrTrackball == VTKIS_TRACKBALL &&
           this->CameraOrActor == VTKIS_ACTOR)
    {
    this->TrackballActor->OnRightButtonUp(ctrl, shift, x, y);
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleSwitch::SetInteractor(vtkRenderWindowInteractor *iren)
{
  this->JoystickActor->SetInteractor(iren);
  this->JoystickCamera->SetInteractor(iren);
  this->TrackballActor->SetInteractor(iren);
  this->TrackballCamera->SetInteractor(iren);
  
  this->vtkInteractorStyle::SetInteractor(iren);
}

//----------------------------------------------------------------------------
void vtkInteractorStyleSwitch::OnTimer(void)
{
  if (this->JoystickOrTrackball == VTKIS_JOYSTICK &&
      this->CameraOrActor == VTKIS_CAMERA)
    {
    this->JoystickCamera->OnTimer();
    }
  else if (this->JoystickOrTrackball == VTKIS_JOYSTICK &&
           this->CameraOrActor == VTKIS_ACTOR)
    {
    this->JoystickActor->OnTimer();
    }
}
