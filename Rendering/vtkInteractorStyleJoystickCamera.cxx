/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleJoystickCamera.cxx
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
#include "vtkInteractorStyleJoystickCamera.h"
#include "vtkObjectFactory.h"
#include "vtkCommand.h"

vtkCxxRevisionMacro(vtkInteractorStyleJoystickCamera, "1.15");
vtkStandardNewMacro(vtkInteractorStyleJoystickCamera);

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
  else if (this->State == VTK_INTERACTOR_STYLE_CAMERA_PAN) 
    {
    this->EndPan();
    } 
  else if (this->State == VTK_INTERACTOR_STYLE_CAMERA_ZOOM) 
    {
    this->EndDolly();
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
  this->Superclass::PrintSelf(os,indent);
}

