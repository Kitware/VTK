/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleJoystickCamera.cxx
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
#include "vtkInteractorStyleJoystickCamera.h"
#include "vtkObjectFactory.h"
#include "vtkCommand.h"

//----------------------------------------------------------------------------
vtkInteractorStyleJoystickCamera *vtkInteractorStyleJoystickCamera::New() 
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkInteractorStyleJoystickCamera");
  if(ret)
    {
    return (vtkInteractorStyleJoystickCamera*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkInteractorStyleJoystickCamera;
}


//----------------------------------------------------------------------------
vtkInteractorStyleJoystickCamera::vtkInteractorStyleJoystickCamera() 
{
  this->MotionFactor = 10.0;
  this->State = VTK_INTERACTOR_STYLE_CAMERA_NONE;
}

//----------------------------------------------------------------------------
vtkInteractorStyleJoystickCamera::~vtkInteractorStyleJoystickCamera() 
{
}


//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickCamera::OnTimer(void) 
{
  vtkRenderWindowInteractor *rwi = this->Interactor;
  switch (this->State) 
    {
    //-----
    case VTKIS_START:
      // JCP Animation control
      if (this->AnimState == VTKIS_ANIM_ON)
        {
                rwi->DestroyTimer();
                rwi->Render();
                rwi->CreateTimer(VTKI_TIMER_FIRST);
         }
      // JCP Animation control 
      break;
      //-----
    case VTK_INTERACTOR_STYLE_CAMERA_ROTATE:  // rotate with respect to an axis perp to look
      this->FindPokedCamera(this->LastPos[0], this->LastPos[1]);
      this->RotateCamera(this->LastPos[0], this->LastPos[1]);
      rwi->CreateTimer(VTKI_TIMER_UPDATE);
      break;
      //-----
    case VTK_INTERACTOR_STYLE_CAMERA_PAN: // move perpendicular to camera's look vector
      this->FindPokedCamera(this->LastPos[0], this->LastPos[1]);
      this->PanCamera(this->LastPos[0], this->LastPos[1]);
      rwi->CreateTimer(VTKI_TIMER_UPDATE);
      break;
      //-----
    case VTK_INTERACTOR_STYLE_CAMERA_ZOOM:
      this->FindPokedCamera(this->LastPos[0], this->LastPos[1]);
      this->DollyCamera(this->LastPos[0], this->LastPos[1]);
      rwi->CreateTimer(VTKI_TIMER_UPDATE);
      break;
      //-----
    case VTK_INTERACTOR_STYLE_CAMERA_SPIN:
      this->FindPokedCamera(this->LastPos[0], this->LastPos[1]);
      this->SpinCamera(this->LastPos[0], this->LastPos[1]);
      rwi->CreateTimer(VTKI_TIMER_UPDATE);
      break;
      //-----
    case VTKIS_TIMER:
                rwi->Render();
      rwi->CreateTimer(VTKI_TIMER_UPDATE);
      break;
      //-----
    default :

      break;
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickCamera::OnMouseMove(int vtkNotUsed(ctrl), 
					   int vtkNotUsed(shift),
					   int x, int y) 
{
  this->LastPos[0] = x;
  this->LastPos[1] = y;
}


//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickCamera::OnLeftButtonDown(int ctrl, int shift, 
						int x, int y) 
{
  if (this->HasObserver(vtkCommand::LeftButtonPressEvent)) 
    {
    this->InvokeEvent(vtkCommand::LeftButtonPressEvent,NULL);
    return;
    }
  this->FindPokedRenderer(x, y);

  if (this->CurrentRenderer == NULL)
    {
    vtkErrorMacro("CurrentRenderer is NULL");
    return;
    }

  this->UpdateInternalState(ctrl, shift, x, y);
  
  if (shift)
    {
    if (ctrl)
      {
      this->StartDolly();
      this->State = VTK_INTERACTOR_STYLE_CAMERA_ZOOM;
      }
    else
      {
      this->StartPan();
      this->State = VTK_INTERACTOR_STYLE_CAMERA_PAN;
      }
    }
  else 
    {
    if (this->CtrlKey)
      {
      this->StartSpin();
      this->State = VTK_INTERACTOR_STYLE_CAMERA_SPIN;
      }
    else
      {
      this->StartRotate();
      this->State = VTK_INTERACTOR_STYLE_CAMERA_ROTATE;
      }
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickCamera::OnLeftButtonUp(int vtkNotUsed(ctrl),
						      int vtkNotUsed(shift),
						      int vtkNotUsed(x),
						      int vtkNotUsed(y))
{
  if (this->State == VTK_INTERACTOR_STYLE_CAMERA_ROTATE)
    {
    this->EndRotate();
    }
  else
    {
    this->EndSpin();
    }

  this->State = VTK_INTERACTOR_STYLE_CAMERA_NONE;
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickCamera::OnMiddleButtonDown(int vtkNotUsed(ctrl),
							  int vtkNotUsed(shift), 
							  int x, int y)
{
  if (this->HasObserver(vtkCommand::MiddleButtonPressEvent)) 
    {
    this->InvokeEvent(vtkCommand::MiddleButtonPressEvent,NULL);
    return;
    }
  this->FindPokedRenderer(x, y);
  if (this->CurrentRenderer == NULL)
    {
    return;
    }
  
  this->StartPan();
  this->State = VTK_INTERACTOR_STYLE_CAMERA_PAN;
}
//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickCamera::OnMiddleButtonUp(int vtkNotUsed(ctrl),
							int vtkNotUsed(shift), 
							int vtkNotUsed(x),
							int vtkNotUsed(y)) 
{
  this->EndPan();
  this->State = VTK_INTERACTOR_STYLE_CAMERA_NONE;
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickCamera::OnRightButtonDown(int vtkNotUsed(ctrl),
							 int vtkNotUsed(shift), 
							 int x, int y)
{
  if (this->HasObserver(vtkCommand::RightButtonPressEvent)) 
    {
    this->InvokeEvent(vtkCommand::RightButtonPressEvent,NULL);
    return;
    }
  this->FindPokedRenderer(x, y);
  if (this->CurrentRenderer == NULL)
    {
    return;
    }
  
  this->StartZoom();
  this->State = VTK_INTERACTOR_STYLE_CAMERA_ZOOM;
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickCamera::OnRightButtonUp(int vtkNotUsed(ctrl),
						       int vtkNotUsed(shift), 
						       int vtkNotUsed(x),
						       int vtkNotUsed(y))
{
  this->EndZoom();
  this->State = VTK_INTERACTOR_STYLE_CAMERA_NONE;
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickCamera::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkInteractorStyle::PrintSelf(os,indent);
}

