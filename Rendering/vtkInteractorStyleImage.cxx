/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleImage.cxx
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
#include "vtkInteractorStyleImage.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"
#include "vtkCommand.h"

//----------------------------------------------------------------------------
vtkInteractorStyleImage *vtkInteractorStyleImage::New() 
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkInteractorStyleImage");
  if(ret)
    {
    return (vtkInteractorStyleImage*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkInteractorStyleImage;
}


//----------------------------------------------------------------------------
vtkInteractorStyleImage::vtkInteractorStyleImage() 
{
  this->MotionFactor = 10.0;
  this->State = VTK_INTERACTOR_STYLE_IMAGE_NONE;
  this->RadianToDegree = 180.0 / vtkMath::Pi();
  this->WindowLevelStartPosition[0] = 0;
  this->WindowLevelStartPosition[1] = 0;  
  this->WindowLevelCurrentPosition[0] = 0;
  this->WindowLevelCurrentPosition[1] = 0;  
}

//----------------------------------------------------------------------------
vtkInteractorStyleImage::~vtkInteractorStyleImage() 
{
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::OnMouseMove(int vtkNotUsed(ctrl), 
					   int vtkNotUsed(shift),
					   int x, int y) 
{
  if (this->State == VTK_INTERACTOR_STYLE_IMAGE_WINDOW_LEVEL)
    {
    this->FindPokedCamera(x, y);
    this->WindowLevelXY(x, y);
    }
  else if (this->State == VTK_INTERACTOR_STYLE_IMAGE_PAN)
    {
    this->FindPokedCamera(x, y);
    this->PanXY(x, y, this->LastPos[0], this->LastPos[1]);
    }
  else if (this->State == VTK_INTERACTOR_STYLE_IMAGE_ZOOM)
    {
    this->FindPokedCamera(x, y);
    this->DollyXY(x - this->LastPos[0], y - this->LastPos[1]);
    }
  else if (this->State == VTK_INTERACTOR_STYLE_IMAGE_SPIN)
    {
    this->FindPokedCamera(x, y);
    this->SpinXY(x, y, this->LastPos[0], this->LastPos[1]);
    }
  
  this->LastPos[0] = x;
  this->LastPos[1] = y;
}


//----------------------------------------------------------------------------
void vtkInteractorStyleImage::WindowLevelXY(int x, int y)
{
  this->WindowLevelCurrentPosition[0] = x;
  this->WindowLevelCurrentPosition[1] = y;
  
  if (this->HasObserver(vtkCommand::WindowLevelEvent)) 
    {
    this->InvokeEvent(vtkCommand::WindowLevelEvent,NULL);
    return;
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::PanXY(int x, int y, int oldX, int oldY)
{
  vtkCamera *cam;
  double viewFocus[4], focalDepth, viewPoint[3];
  float newPickPoint[4], oldPickPoint[4], motionVector[3];
  
  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  // calculate the focal depth since we'll be using it a lot
  cam = this->CurrentRenderer->GetActiveCamera();
  cam->GetFocalPoint(viewFocus);
  this->ComputeWorldToDisplay(viewFocus[0], viewFocus[1],
			      viewFocus[2], viewFocus);
  focalDepth = viewFocus[2];

  this->ComputeDisplayToWorld(double(x), double(y),
			      focalDepth, newPickPoint);
    
  // has to recalc old mouse point since the viewport has moved,
  // so can't move it outside the loop
  this->ComputeDisplayToWorld(double(oldX),double(oldY),
			      focalDepth, oldPickPoint);
  
  // camera motion is reversed
  motionVector[0] = oldPickPoint[0] - newPickPoint[0];
  motionVector[1] = oldPickPoint[1] - newPickPoint[1];
  motionVector[2] = oldPickPoint[2] - newPickPoint[2];
  
  cam->GetFocalPoint(viewFocus);
  cam->GetPosition(viewPoint);
  cam->SetFocalPoint(motionVector[0] + viewFocus[0],
		     motionVector[1] + viewFocus[1],
		     motionVector[2] + viewFocus[2]);
  cam->SetPosition(motionVector[0] + viewPoint[0],
		   motionVector[1] + viewPoint[1],
		   motionVector[2] + viewPoint[2]);
      
  vtkRenderWindowInteractor *rwi = this->Interactor;
  if (this->CurrentLight)
    {
    /* get the first light */
    this->CurrentLight->SetPosition(cam->GetPosition());
    this->CurrentLight->SetFocalPoint(cam->GetFocalPoint());
    }
    
  rwi->Render();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::DollyXY(int vtkNotUsed(dx), int dy)
{
  vtkCamera *cam;
  double dyf = this->MotionFactor * (double)(dy) / (double)(this->Center[1]);
  double zoomFactor = pow((double)1.1, dyf);
  
  if (this->CurrentRenderer == NULL)
    {
    return;
    }
  
  cam = this->CurrentRenderer->GetActiveCamera();
  if (cam->GetParallelProjection())
    {
    cam->SetParallelScale(cam->GetParallelScale()/zoomFactor);
    }
  else
    {
    cam->Dolly(zoomFactor);
    this->CurrentRenderer->ResetCameraClippingRange();
    }
  
  vtkRenderWindowInteractor *rwi = this->Interactor;
  if (this->CurrentLight)
    {
    /* get the first light */
    this->CurrentLight->SetPosition(cam->GetPosition());
    this->CurrentLight->SetFocalPoint(cam->GetFocalPoint());
    }
  
  rwi->Render();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::SpinXY(int x, int y, int oldX, int oldY)
{
  vtkRenderWindowInteractor *rwi = this->Interactor;
  vtkCamera *cam;

  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  double newAngle = atan2((double)(y - this->Center[1]),
                         (double)(x - this->Center[0]));
  double oldAngle = atan2((double)(oldY -this->Center[1]),
                         (double)(oldX - this->Center[0]));
  
  newAngle *= this->RadianToDegree;
  oldAngle *= this->RadianToDegree;

  cam = this->CurrentRenderer->GetActiveCamera();
  cam->Roll(newAngle - oldAngle);
  cam->OrthogonalizeViewUp();
      
  rwi->Render();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::OnLeftButtonDown(int ctrl, int shift, 
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
    return;
    }

  this->UpdateInternalState(ctrl, shift, x, y);

  if (shift)
    {
    if (ctrl)
      {
      this->State = VTK_INTERACTOR_STYLE_IMAGE_ZOOM;
      }
    else
      {
      this->State = VTK_INTERACTOR_STYLE_IMAGE_PAN;
      }
    }
  else 
    {
    if (this->CtrlKey)
      {
      this->State = VTK_INTERACTOR_STYLE_IMAGE_SPIN;
      }
    else
      {
      this->State = VTK_INTERACTOR_STYLE_IMAGE_WINDOW_LEVEL;
      this->WindowLevelStartPosition[0] = x;
      this->WindowLevelStartPosition[1] = y;      
      if (this->HasObserver(vtkCommand::WindowLevelEvent)) 
        {
        this->InvokeEvent(vtkCommand::WindowLevelEvent,this);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::OnLeftButtonUp(int vtkNotUsed(ctrl),
						       int vtkNotUsed(shift), 
						       int vtkNotUsed(x),
						       int vtkNotUsed(y))
{
  this->State = VTK_INTERACTOR_STYLE_IMAGE_NONE;
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::OnMiddleButtonDown(int vtkNotUsed(ctrl),
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
  
  this->State = VTK_INTERACTOR_STYLE_IMAGE_PAN;
}
//----------------------------------------------------------------------------
void vtkInteractorStyleImage::OnMiddleButtonUp(int vtkNotUsed(ctrl),
							 int vtkNotUsed(shift), 
							 int vtkNotUsed(x),
							 int vtkNotUsed(y))
{
  this->State = VTK_INTERACTOR_STYLE_IMAGE_NONE;
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::OnRightButtonDown(int vtkNotUsed(ctrl),
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
  
  this->State = VTK_INTERACTOR_STYLE_IMAGE_ZOOM;
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::OnRightButtonUp(int vtkNotUsed(ctrl),
							int vtkNotUsed(shift), 
							int vtkNotUsed(x),
							int vtkNotUsed(y))
{
  this->State = VTK_INTERACTOR_STYLE_IMAGE_NONE;
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkInteractorStyle::PrintSelf(os,indent);

}
