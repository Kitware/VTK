/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleJoystickCamera.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkInteractorStyleJoystickCamera.h"
#include "vtkObjectFactory.h"

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
void vtkInteractorStyleJoystickCamera::OnMouseMove(int vtkNotUsed(ctrl), 
					   int vtkNotUsed(shift),
					   int x, int y) 
{
  if (this->State == VTK_INTERACTOR_STYLE_CAMERA_ROTATE)
    {
    this->FindPokedCamera(x, y);
    this->RotateXY(x, y);
    }
  else if (this->State == VTK_INTERACTOR_STYLE_CAMERA_PAN)
    {
    this->FindPokedCamera(x, y);
    this->PanXY(x, y);
    }
  else if (this->State == VTK_INTERACTOR_STYLE_CAMERA_ZOOM)
    {
    this->FindPokedCamera(x, y);
    this->DollyXY(x, y);
    }
  else if (this->State == VTK_INTERACTOR_STYLE_CAMERA_SPIN)
    {
    this->FindPokedCamera(x, y);
    this->SpinXY(x, y);
    }

  this->LastPos[0] = x;
  this->LastPos[1] = y;
}


//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickCamera::RotateXY(int x, int y)
{
  vtkCamera *cam;
  vtkRenderWindowInteractor *rwi = this->Interactor;

  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  double rxf = (double)(x - this->Center[0]) * this->DeltaAzimuth;
  double ryf = (double)(y - this->Center[1]) * this->DeltaElevation;

  cam = this->CurrentRenderer->GetActiveCamera();
  cam->Azimuth(rxf);
  cam->Elevation(ryf);
  cam->OrthogonalizeViewUp();
  this->CurrentRenderer->ResetCameraClippingRange();
  if (this->CurrentLight)
    {
    this->CurrentLight->SetPosition(cam->GetPosition());
    this->CurrentLight->SetFocalPoint(cam->GetFocalPoint());
    }
  rwi->Render();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickCamera::PanXY(int x, int y)
{
  vtkCamera *cam;

  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  cam = this->CurrentRenderer->GetActiveCamera();

  vtkRenderWindowInteractor *rwi = this->Interactor;
  double viewFocus[4];
  
  // calculate the focal depth since we'll be using it a lot
  cam->GetFocalPoint(viewFocus);
  this->ComputeWorldToDisplay(viewFocus[0], viewFocus[1],
                              viewFocus[2], viewFocus);
  double focalDepth = viewFocus[2];

  double newPickPoint[4];
  this->ComputeDisplayToWorld((float)x, (float)y,
                              focalDepth, newPickPoint);

  // get the current focal point and position
  cam->GetFocalPoint(viewFocus);
  double *viewPoint = cam->GetPosition();

  /*
   * Compute a translation vector, moving everything 1/10
   * the distance to the cursor. (Arbitrary scale factor)
   */
  double motionVector[3];
  motionVector[0] = 0.1*(viewFocus[0] - newPickPoint[0]);
  motionVector[1] = 0.1*(viewFocus[1] - newPickPoint[1]);
  motionVector[2] = 0.1*(viewFocus[2] - newPickPoint[2]);

  cam->SetFocalPoint(motionVector[0] + viewFocus[0],
                     motionVector[1] + viewFocus[1],
                     motionVector[2] + viewFocus[2]);
  cam->SetPosition(motionVector[0] + viewPoint[0],
                   motionVector[1] + viewPoint[1],
                   motionVector[2] + viewPoint[2]);

  if (this->CurrentLight)
    {
    /* get the first light */
    this->CurrentLight->SetPosition(cam->GetPosition());
    this->CurrentLight->SetFocalPoint(cam->GetFocalPoint());
    }
  rwi->Render();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickCamera::DollyXY(int x, int y)
{
  vtkCamera *cam;
  vtkRenderWindowInteractor *rwi = this->Interactor;

  double dyf = 0.5 * (double)(y - this->Center[1]) /
    (double)(this->Center[1]);
  double zoomFactor = pow((double)1.1, dyf);
  
  if (this->CurrentRenderer == NULL)
    {
    return;
    }
  
  cam = this->CurrentRenderer->GetActiveCamera();

  if (zoomFactor < 0.5 || zoomFactor > 1.5)
    {
    vtkErrorMacro("Bad zoom factor encountered");
    }
  
  if (cam->GetParallelProjection())
    {
    cam->SetParallelScale(this->CurrentCamera->GetParallelScale()/zoomFactor);
    }
  else
    {
    cam->Dolly(zoomFactor);
    this->CurrentRenderer->ResetCameraClippingRange();
    }

  if (this->CurrentLight)
    {
    /* get the first light */
    this->CurrentLight->SetPosition(this->CurrentCamera->GetPosition());
    this->CurrentLight->SetFocalPoint(this->CurrentCamera->GetFocalPoint());
    }
  rwi->Render();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickCamera::SpinXY(int x, int y)
{
  vtkRenderWindowInteractor *rwi = this->Interactor;
  vtkCamera *cam;

  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  cam = this->CurrentRenderer->GetActiveCamera();
  // spin is based on y value
  double yf = (double)(y - this->Center[1]) / (double)(this->Center[1]);
  if (yf > 1)
    {
    yf = 1;
    }
  else if (yf < -1)
    {
    yf = -1;
    }

  double newAngle = asin(yf) * 180.0 / 3.1415926;

  cam->Roll(newAngle);
  cam->OrthogonalizeViewUp();
  rwi->Render();
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

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickCamera::OnLeftButtonUp(int ctrl, int shift, 
					      int x, int y) 
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
void vtkInteractorStyleJoystickCamera::OnMiddleButtonDown(int ctrl, int shift, 
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
void vtkInteractorStyleJoystickCamera::OnMiddleButtonUp(int ctrl, int shift, 
					       int x, int y) 
{
  this->EndPan();
  this->State = VTK_INTERACTOR_STYLE_CAMERA_NONE;
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickCamera::OnRightButtonDown(int ctrl, int shift, 
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
void vtkInteractorStyleJoystickCamera::OnRightButtonUp(int ctrl, int shift, 
					      int x, int y) 
{
  this->EndZoom();
  this->State = VTK_INTERACTOR_STYLE_CAMERA_NONE;
}

//----------------------------------------------------------------------------
void vtkInteractorStyleJoystickCamera::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkInteractorStyle::PrintSelf(os,indent);
}

