/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleSwitch.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkInteractorStyleSwitch.h"
#include "vtkObjectFactory.h"
#include "vtkCommand.h"

//----------------------------------------------------------------------------
vtkInteractorStyleSwitch *vtkInteractorStyleSwitch::New() 
{
// First try to create the object from the vtkObjectFactory
  vtkObject* ret =
    vtkObjectFactory::CreateInstance("vtkInteractorStyleSwitch");
  if(ret)
    {
    return (vtkInteractorStyleSwitch*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkInteractorStyleSwitch;
}

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
