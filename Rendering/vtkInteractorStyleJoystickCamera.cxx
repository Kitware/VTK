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

vtkCxxRevisionMacro(vtkInteractorStyleJoystickCamera, "1.17");
vtkStandardNewMacro(vtkInteractorStyleJoystickCamera);

//----------------------------------------------------------------------------
vtkInteractorStyleJoystickCamera::vtkInteractorStyleJoystickCamera() 
{
}

//----------------------------------------------------------------------------
vtkInteractorStyleJoystickCamera::~vtkInteractorStyleJoystickCamera() 
{
}


//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickCamera::OnTimer(void) 
{
  switch (this->State) 
    {
    case VTKIS_ROTATE:
    case VTKIS_PAN:
    case VTKIS_ZOOM:
    case VTKIS_SPIN:
      this->FindPokedCamera(this->LastPos[0], this->LastPos[1]);
      break;
    }

  this->Superclass::OnTimer();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickCamera::OnLeftButtonDown(int ctrl, 
                                                        int shift, 
                                                        int x, 
                                                        int y) 
{
  this->FindPokedRenderer(x, y);
  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  if (shift) 
    {
    if (ctrl) 
      {
      this->StartDolly();
      }
    else 
      {
      this->StartPan();
      }
    } 
  else 
    {
    if (ctrl) 
      {
      this->StartSpin();
      }
    else 
      {
      this->StartRotate();
      }
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickCamera::OnLeftButtonUp(int vtkNotUsed(ctrl), 
                                                      int vtkNotUsed(shift), 
                                                      int vtkNotUsed(x), 
                                                      int vtkNotUsed(y))
{
  switch (this->State) 
    {
    case VTKIS_DOLLY:
      this->EndDolly();
      break;

    case VTKIS_PAN:
      this->EndPan();
      break;

    case VTKIS_SPIN:
      this->EndSpin();
      break;

    case VTKIS_ROTATE:
      this->EndRotate();
      break;
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickCamera::OnMiddleButtonDown(int vtkNotUsed(ctrl),
                                                          int vtkNotUsed(shift), 
                                                          int x, 
                                                          int y)
{
  this->FindPokedRenderer(x, y);
  if (this->CurrentRenderer == NULL)
    {
    return;
    }
  
  this->StartPan();
}
//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickCamera::OnMiddleButtonUp(int vtkNotUsed(ctrl),
                                                        int vtkNotUsed(shift), 
                                                        int vtkNotUsed(x),
                                                        int vtkNotUsed(y)) 
{
  switch (this->State) 
    {
    case VTKIS_PAN:
      this->EndPan();
      break;
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickCamera::OnRightButtonDown(int vtkNotUsed(ctrl),
                                                         int vtkNotUsed(shift), 
                                                         int x, 
                                                         int y)
{
  this->FindPokedRenderer(x, y);
  if (this->CurrentRenderer == NULL)
    {
    return;
    }
  
  this->StartZoom();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickCamera::OnRightButtonUp(int vtkNotUsed(ctrl), 
                                                       int vtkNotUsed(shift), 
                                                       int vtkNotUsed(x), 
                                                       int vtkNotUsed(y))
{
  switch (this->State) 
    {
    case VTKIS_ZOOM:
      this->EndZoom();
      break;
    }
}
//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickCamera::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
