/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleFlight.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to John Biddiscombe of the Rutherford Appleton Laboratory
             who developed class.


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
#include "vtkInteractorStyleFlight.h"
#include "vtkMath.h"
#include "vtkCellPicker.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
vtkInteractorStyleFlight* vtkInteractorStyleFlight::New()
{
  // irst try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkInteractorStyleFlight");
  if(ret)
    {
    return (vtkInteractorStyleFlight*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkInteractorStyleFlight;
}
//---------------------------------------------------------------------------
vtkInteractorStyleFlight::vtkInteractorStyleFlight()
{
  this->KeysDown     = 0;
  this->Flying       = 0;
  this->Reversing    = 0;
  this->TimerRunning = 0;
  this->DiagonalLength           = 1.0;
  this->MotionStepSize           = 1.0/250.0;
  this->MotionUserScale          = 1.0;  // +/- key adjustment
  this->MotionAccelerationFactor = 10.0;
  this->AngleStepSize            = 1.0;
  this->AngleAccelerationFactor  = 5.0;
  this->OldX = 0.0;
  this->OldY = 0.0;
  this->AzimuthScanning = 0;
  this->DisableMotion   = 0;
  this->FixUpVector     = 0;
  this->FixedUpVector[0] = 0;
  this->FixedUpVector[1] = 0;
  this->FixedUpVector[2] = 1;
}
//---------------------------------------------------------------------------
vtkInteractorStyleFlight::~vtkInteractorStyleFlight() {

}
//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::DoTimerStart(void) {
  if (this->TimerRunning) 
    {
      return;
    }
  vtkRenderWindowInteractor *rwi = this->Interactor;
  rwi->CreateTimer(VTKI_TIMER_FIRST);
  this->TimerRunning = 1;
}
//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::DoTimerStop(void) {
  if (!this->TimerRunning) 
    {
      return;
    }
  vtkRenderWindowInteractor *rwi = this->Interactor;
  rwi->DestroyTimer();
  this->TimerRunning = 0;
}
//---------------------------------------------------------------------------
// Mouse event handlers
//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::OnLeftButtonDown(int ctrl, int shift, int X, int Y) {
  this->UpdateInternalState(ctrl, shift, X, Y);
  if (this->AzimuthScanning) 
    {
      return;
    }
  if (!this->Reversing) 
    {
      this->OldX        = this->X2 = X;
      this->OldY        = this->Y2 = Y;
      this->YawAngle   = 0;
      this->PitchAngle = 0;
      this->DoTimerStart();
    }
  this->Flying       = 1;
}
//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::OnLeftButtonUp(int ctrl, int shift, int X, int Y) {
  this->UpdateInternalState(ctrl, shift, X, Y);
  this->Flying       =  0;
}
//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::OnRightButtonDown(int ctrl, int shift, int X, int Y) {
  this->UpdateInternalState(ctrl, shift, X, Y);
  if (this->AzimuthScanning)
    {
      return;
    }
  if (!this->Flying) 
    {
      this->OldX        = this->X2 = X;
      this->OldY        = this->Y2 = Y;
      this->YawAngle   = 0;
      this->PitchAngle = 0;
      this->DoTimerStart();
    }
  this->Reversing    = 1;
}
//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::OnRightButtonUp(int ctrl, int shift, int X, int Y) {
  this->UpdateInternalState(ctrl, shift, X, Y);
  this->Reversing    =  0;
}
//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::OnMiddleButtonDown(int ctrl, int shift, int X, int Y) {
  this->UpdateInternalState(ctrl, shift, X, Y);
  if (this->AzimuthScanning) 
    {
      return;
    }
  // want to add some more functions???
  // I've got no middle mouse button :(
}
//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::OnMiddleButtonUp(int ctrl, int shift, int X, int Y) {
  this->UpdateInternalState(ctrl, shift, X, Y);
}
//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::OnMouseMove(int ctrl, int shift, int X, int Y) {
  this->UpdateInternalState(ctrl, shift, X, Y);
  if (this->AzimuthScanning) 
    {
      return;
    }
  if (this->Flying || this->Reversing) 
    {
      this->UpdateMouseSteering(X, Y);
    }
}
//---------------------------------------------------------------------------
// Keyboard event handlers
// Note, OnChar is a key press down and then up event
// Note, OnKeyDown/OnKeyUp are more sensitive for controlling motion
//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::OnChar(int ctrl, int shift,
                                char keycode, int vtkNotUsed(repeatcount))
{
  this->CtrlKey  = ctrl;
  this->ShiftKey = shift;
  switch (keycode)
    {
    // we are not overriding these keycodes
    case 'Q' :    case 'q' :
    case 'e' :    case 'E' :
    case 'u' :    case 'U' :
    case 'r' :    case 'R' :
    case 'w' :    case 'W' :
    case 's' :    case 'S' :
    case '3' :
    case 'p' :    case 'P' :
        vtkInteractorStyle::OnChar(ctrl, shift, keycode, 0);
        break;
    case '+' :
        this->MotionUserScale *= 2.0;
        break;
    case '-' :
        this->MotionUserScale *= 0.5;
        break;
    case 'L' :
    case 'l' :
        this->PerformAzimuthalScan(360);
        break;
    }
}
//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::OnKeyDown(int ctrl, int shift,
                                char keycode, int vtkNotUsed(repeatcount))
{
  this->CtrlKey  = ctrl;
  this->ShiftKey = shift;
  if (this->AzimuthScanning) 
    {
      return;
    }
  // New Flight mode behaviour
  // Note that we'll need #defines for ARROW key defs under non win32 OS
#ifdef _WIN32
  switch (keycode)
    {
    case VK_LEFT  	: this->KeysDown |=1;  break;
    case VK_RIGHT 	: this->KeysDown |=2;  break;
    case VK_UP    	: this->KeysDown |=4;  break;
    case VK_DOWN  	: this->KeysDown |=8;  break;
    case 'a':
    case 'A'	: this->KeysDown |=16; break;
    case 'z':
    case 'Z' 	: this->KeysDown |=32; break;
    }
  // it may already be started but it doesn't matter
	if (this->KeysDown) 
	  {
	    this->DoTimerStart();
	  }
#else
  // the following if statement is a dummy one to prevent keycode not used
  // warnings under unix, (until the correct keycodes are supplied)
  if (keycode==0x7F)
    {
      vtkWarningMacro(<<"Dummy test to prevent compiler warning");
    }
#endif
}

//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::OnKeyUp(int ctrl, int shift,
                                char keycode, int vtkNotUsed(repeatcount))
{
  this->CtrlKey  = ctrl;
  this->ShiftKey = shift;
#ifdef _WIN32
  switch (keycode)
    {
    case VK_LEFT  	: this->KeysDown &= ~1;  break;
    case VK_RIGHT 	: this->KeysDown &= ~2;  break;
    case VK_UP    	: this->KeysDown &= ~4;  break;
    case VK_DOWN  	: this->KeysDown &= ~8;  break;
    case 'a':
    case 'A'	: this->KeysDown &= ~16; break;
    case 'z':
    case 'Z' 	: this->KeysDown &= ~32; break;
    }
#else
  // the following if statement is a dummy one to prevent keycode not used
  // warnings under unix, (until the correct keycodes are supplied)
  if (keycode==0x7F)
    {
      vtkWarningMacro(<<"Dummy test to prevent compiler warning");
    }
#endif
}
//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::JumpTo(double campos[3], double focpos[3]) {
    this->CurrentCamera->SetPosition(campos);
    this->CurrentCamera->SetFocalPoint(focpos);
    // Tidy up Camera stuff
    this->CurrentCamera->OrthogonalizeViewUp();
    if (this->FixUpVector)
      {
        this->CurrentCamera->SetViewUp(this->FixedUpVector);
      }
    this->CurrentRenderer->ResetCameraClippingRange();
    // Make sure light follows camera if desired
    vtkRenderWindowInteractor *rwi = this->Interactor;
    if (rwi->GetLightFollowCamera()) 
      {
        this->CurrentLight->SetPosition(campos);
        this->CurrentLight->SetFocalPoint(focpos);
      }
    rwi->Render();
}
//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::PerformAzimuthalScan(int numsteps) {
    this->AzimuthScanning = numsteps;
    this->Flying          = 0;
    this->Reversing       = 0;
    this->KeysDown        = 0;
    this->AzimuthStepSize = 360.0/numsteps;
    this->DoTimerStart();
}
//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::AzimuthScan(void) {
    this->AzimuthScanning -= 1;
    this->CurrentCamera->SetViewUp(0,0,1);
    this->CurrentCamera->Yaw(this->AzimuthStepSize);
    //
    if (this->AzimuthScanning == 0) 
      {
        this->DoTimerStop();
      }
}
//---------------------------------------------------------------------------
// All actual motion is performed in the timer
//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::OnTimer(void) {
    vtkRenderWindowInteractor *rwi = this->Interactor;
    // if we get a timer message we weren't expecting, just shut it down
	if (!this->Flying && !this->Reversing
        && !this->KeysDown && !this->AzimuthScanning) 
	  {
	    //
	    this->DoTimerStop();
	  }
	else 
	  {
	    // Make sure CurrentCamera variable is initialized
	    this->FindPokedCamera(this->OldX, this->OldY);
	    this->SetupMotionVars();
	    // What sort of motion do we want
	    if (this->AzimuthScanning) 
	      {
	      this->AzimuthScan();
	      } 
	    else 
	      {
		if (this->Flying || this->Reversing)
		  {
		    this->FlyByMouse();
		  }
		if (this->KeysDown) 
		  {
		    this->FlyByKey();
		  }
	      }
	    // Tidy up Camera stuff
	    this->CurrentCamera->OrthogonalizeViewUp();
	    if (this->FixUpVector) 
	      {
		this->CurrentCamera->SetViewUp(this->FixedUpVector);
	      }
	    this->CurrentRenderer->ResetCameraClippingRange();
	    // Make sure light follows camera if desired
	    if (rwi->GetLightFollowCamera()) 
	      {
		this->CurrentLight->SetPosition(this->CurrentCamera->GetPosition());
		this->CurrentLight->SetFocalPoint(this->CurrentCamera->GetFocalPoint());
	      }
	    rwi->Render();
	    rwi->CreateTimer(VTKI_TIMER_UPDATE);
	  }
}
//---------------------------------------------------------------------------
// Calculate angles for next redraw in timer event
//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::UpdateMouseSteering(int x, int y) {
	//
    double aspeed = this->AngleStepSize*(this->ShiftKey ? this->AngleAccelerationFactor : 1.0);
    //
    // we want to steer by an amount proportional to window viewangle and size
    vtkRenderWindowInteractor *rwi = this->Interactor;
    int *size = rwi->GetSize();
    double scalefactor = 5.0*this->CurrentCamera->GetViewAngle()/(double)size[0];
    double dx = - (x - this->OldX)*scalefactor;
    double dy =   (y - this->OldY)*scalefactor;
    this->YawAngle   = dx*aspeed;
    this->PitchAngle = dy*aspeed;
    this->X2 = x;
    this->Y2 = y;
}
//---------------------------------------------------------------------------
// useful utility functions
//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::SetupMotionVars(void) {
  float bounds[6];
  this->CurrentRenderer->ComputeVisiblePropBounds( bounds );
  if ( bounds[0] == VTK_LARGE_FLOAT ) 
    {
      this->DiagonalLength = 1.0;
    }
  else 
    {
      this->DiagonalLength =
	sqrt( (bounds[0] - bounds[1]) * (bounds[0] - bounds[1]) +
	      (bounds[2] - bounds[3]) * (bounds[2] - bounds[3]) +
	      (bounds[4] - bounds[5]) * (bounds[4] - bounds[5]) );
    }
}
//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::MotionAlongVector(double vector[3], double amount) {
    double oldcampos[3], oldcamfoc[3];
    //
    this->CurrentCamera->GetPosition(oldcampos);
    this->CurrentCamera->GetFocalPoint(oldcamfoc);
    // move camera and focus along viewplanenormal
    this->CurrentCamera->SetPosition(
            oldcampos[0] - amount * vector[0],
            oldcampos[1] - amount * vector[1],
            oldcampos[2] - amount * vector[2]);
    this->CurrentCamera->SetFocalPoint(
            oldcamfoc[0] - amount * vector[0],
            oldcamfoc[1] - amount * vector[1],
            oldcamfoc[2] - amount * vector[2]);
}
//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::ComputeLRVector(double vector[3]) {
  double viewplanenormal[3], viewup[3];
  this->CurrentCamera->GetViewPlaneNormal(viewplanenormal);
  this->CurrentCamera->GetViewUp(viewup);
  // Left Right axis
  vector[0] = (viewplanenormal[1] * viewup[2] -
               viewplanenormal[2] * viewup[1]);
  vector[1] = (viewplanenormal[2] * viewup[0] -
	           viewplanenormal[0] * viewup[2]);
  vector[2] = (viewplanenormal[0] * viewup[1] -
	           viewplanenormal[1] * viewup[0]);
}
//---------------------------------------------------------------------------
// Perform the motion
//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::FlyByMouse(void) {
    double a_vector[3];
    double speed  = this->DiagonalLength * this->MotionStepSize * this->MotionUserScale;
    speed = speed * ( this->ShiftKey ? this->MotionAccelerationFactor : 1.0);
    if (this->DisableMotion) 
      {
        speed = 0;
      }
    // Sidestep (convert steering angles to left right movement :
    // only because I added this after doing the angles earlier
    if (this->CtrlKey) 
      {
        if (this->YawAngle!=0.0) 
	  {
            this->ComputeLRVector(a_vector);
	    this->MotionAlongVector(a_vector,-this->YawAngle*speed/4.0);
	  }
        if (this->PitchAngle!=0.0) 
	  {
	    this->CurrentCamera->GetViewUp(a_vector);
	    this->MotionAlongVector(a_vector,-this->PitchAngle*speed/4.0);
	  }
      } 
    else 
      {
        this->CurrentCamera->Yaw(this->YawAngle);
        this->CurrentCamera->Pitch(this->PitchAngle);
      }
    this->OldX = this->X2;
    this->OldY = this->Y2;
    this->YawAngle   = 0;
    this->PitchAngle = 0;
    //
    if (!this->CtrlKey) 
      {
        this->CurrentCamera->GetViewPlaneNormal(a_vector);
        if (this->Flying) 
	  {
	    this->MotionAlongVector(a_vector, speed);
	  }
        if (this->Reversing)
	  {
	    this->MotionAlongVector(a_vector,-speed);
	  }
      }
}
//---------------------------------------------------------------------------
// Perform the motion
//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::FlyByKey(void) {

    double speed  = this->DiagonalLength * this->MotionStepSize * this->MotionUserScale;
    speed = speed * ( this->ShiftKey ? this->MotionAccelerationFactor : 1.0);
    if (this->DisableMotion) 
      {
        speed = 0;
      }
    //
    double aspeed = this->AngleStepSize* (this->ShiftKey ? this->AngleAccelerationFactor : 1.0);
    double a_vector[3];
    // Left and right
	if (this->CtrlKey) 
	  { // Sidestep
	    this->ComputeLRVector(a_vector);
	    if (this->KeysDown & 1) 
	      {
		this->MotionAlongVector(a_vector,-speed);
	      }
	    if (this->KeysDown & 2)
	      {
		this->MotionAlongVector(a_vector, speed);
	      }
	  } 
	else 
	  {
	    if (this->KeysDown & 1)
	      {
		this->CurrentCamera->Yaw( aspeed);
	      }
	    if (this->KeysDown & 2) 
	      {
		this->CurrentCamera->Yaw(-aspeed);
	      }
	  }

	// Up and Down
	if (this->CtrlKey) 
	  { // Sidestep
	    this->CurrentCamera->GetViewUp(a_vector);
	    if (this->KeysDown & 4) 
	      {
		this->MotionAlongVector(a_vector,-speed);
	      }
	    if (this->KeysDown & 8) 
	      {
		this->MotionAlongVector(a_vector, speed);
	      }
	  } 
	else 
	  {
	    if (this->KeysDown & 4) 
	      {
		this->CurrentCamera->Pitch(-aspeed);
	      }
	    if (this->KeysDown & 8) 
	      {
		this->CurrentCamera->Pitch( aspeed);
	      }
	  }

    // forward and backward
    this->CurrentCamera->GetViewPlaneNormal(a_vector);
    if (this->KeysDown & 16) 
      {
	this->MotionAlongVector(a_vector, speed);
      }
    if (this->KeysDown & 32)
      {
	this->MotionAlongVector(a_vector,-speed);
      }
}
//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::PrintSelf(ostream& os, vtkIndent indent) {
  vtkInteractorStyle::PrintSelf(os,indent);
  os << indent << "MotionStepSize: "
     << this->MotionStepSize << "\n";
  os << indent << "MotionAccelerationFactor: "
     << this->MotionAccelerationFactor << "\n";
  os << indent << "AngleStepSize: "
     << this->AngleStepSize << "\n";
  os << indent << "AngleAccelerationFactor: "
     << this->AngleAccelerationFactor << "\n";
  os << indent << "MotionUserScale: "
     << this->MotionUserScale << "\n";
  os << indent << "DisableMotion: "
     << this->DisableMotion << "\n";
  os << indent << "FixUpVector: "
     << this->FixUpVector << "\n";
  os << indent << "FixedUpVector: "
     << this->FixedUpVector[0] << " "
     << this->FixedUpVector[1] << " "
     << this->FixedUpVector[2] << "\n";
}


