/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleFlight.cxx
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
#include "vtkInteractorStyleFlight.h"
#include "vtkMath.h"
#include "vtkCellPicker.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkInteractorStyleFlight, "1.20");
vtkStandardNewMacro(vtkInteractorStyleFlight);

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

  this->AzimuthScanning  = 0;
  this->DisableMotion    = 0;
  this->FixUpVector      = 0;
  this->FixedUpVector[0] = 0;
  this->FixedUpVector[1] = 0;
  this->FixedUpVector[2] = 1;
}

//---------------------------------------------------------------------------
vtkInteractorStyleFlight::~vtkInteractorStyleFlight() 
{

}

//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::DoTimerStart(void) 
{
  if (!this->UseTimers || this->TimerRunning) 
    {
    return;
    }
  vtkRenderWindowInteractor *rwi = this->Interactor;
  rwi->CreateTimer(VTKI_TIMER_FIRST);
  this->TimerRunning = 1;
}

//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::DoTimerStop(void) 
{
  if (!this->UseTimers || !this->TimerRunning) 
    {
    return;
    }
  vtkRenderWindowInteractor *rwi = this->Interactor;
  rwi->DestroyTimer();
  this->TimerRunning = 0;
}

//---------------------------------------------------------------------------
// All actual motion is performed in the timer
//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::OnTimer(void) 
{
  vtkRenderWindowInteractor *rwi = this->Interactor;
  // if we get a timer message we weren't expecting, just shut it down
  if (!this->Flying && 
      !this->Reversing &&
      !this->KeysDown && 
      !this->AzimuthScanning) 
    {
    this->DoTimerStop();
    }
  else 
    {
    // Make sure CurrentCamera variable is initialized
    this->FindPokedCamera(this->LastPos[0], this->LastPos[1]);
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
    this->ResetCameraClippingRange();
    // Make sure light follows camera if desired
    if (rwi->GetLightFollowCamera()) 
      {
      this->CurrentLight->SetPosition(this->CurrentCamera->GetPosition());
      this->CurrentLight->SetFocalPoint(this->CurrentCamera->GetFocalPoint());
      }
    rwi->Render();
    if (this->UseTimers) 
      {
      rwi->CreateTimer(VTKI_TIMER_UPDATE);
      }
    }
}

//---------------------------------------------------------------------------
// Mouse event handlers
//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::OnMouseMove(int vtkNotUsed(ctrl), 
                                           int vtkNotUsed(shift),
                                           int x, 
                                           int y) 
{
  if (this->AzimuthScanning) 
    {
    return;
    }

  if (this->Flying || this->Reversing) 
    {
    this->UpdateMouseSteering(x, y);
    }
}

//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::OnLeftButtonDown(int vtkNotUsed(ctrl), 
                                                int vtkNotUsed(shift), 
                                                int x, 
                                                int y)
{
  if (this->AzimuthScanning) 
    {
    return;
    }
  if (!this->Reversing) 
    {
    this->LastPos[0]        = this->X2 = x;
    this->LastPos[1]        = this->Y2 = y;
    this->YawAngle   = 0;
    this->PitchAngle = 0;
    this->DoTimerStart();
    }
  this->Flying       = 1;
}

//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::OnLeftButtonUp(int vtkNotUsed(ctrl), 
                                              int vtkNotUsed(shift), 
                                              int vtkNotUsed(x), 
                                              int vtkNotUsed(y))
{
  this->Flying       =  0;
}

//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::OnMiddleButtonDown(int vtkNotUsed(ctrl), 
                                                  int vtkNotUsed(shift), 
                                                  int vtkNotUsed(x), 
                                                  int vtkNotUsed(y))
{
  if (this->AzimuthScanning) 
    {
    return;
    }
  // want to add some more functions???
  // I've got no middle mouse button :(
}

//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::OnMiddleButtonUp(int vtkNotUsed(ctrl), 
                                                int vtkNotUsed(shift), 
                                                int vtkNotUsed(x), 
                                                int vtkNotUsed(y))
{
}

//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::OnRightButtonDown(int vtkNotUsed(ctrl), 
                                                 int vtkNotUsed(shift), 
                                                 int x, 
                                                 int y)
{
  if (this->AzimuthScanning)
    {
    return;
    }
  if (!this->Flying) 
    {
    this->LastPos[0]        = this->X2 = x;
    this->LastPos[1]        = this->Y2 = y;
    this->YawAngle   = 0;
    this->PitchAngle = 0;
    this->DoTimerStart();
    }
  this->Reversing    = 1;
}

//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::OnRightButtonUp(int vtkNotUsed(ctrl), 
                                               int vtkNotUsed(shift), 
                                               int vtkNotUsed(x), 
                                               int vtkNotUsed(y))
{
  this->Reversing    =  0;
}

//---------------------------------------------------------------------------
// Keyboard event handlers
// Note, OnChar is a key press down and then up event
// Note, OnKeyDown/OnKeyUp are more sensitive for controlling motion
//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::OnKeyDown(int vtkNotUsed(ctrl), 
                                         int vtkNotUsed(shift), 
                                         char keycode, 
                                         int vtkNotUsed(repeatcount))
{
  if (this->AzimuthScanning) 
    {
    return;
    }
  // New Flight mode behaviour
  // Note that we'll need #defines for ARROW key defs under non win32 OS
#ifdef _WIN32
  switch (keycode)
    {
    case VK_LEFT        : this->KeysDown |=1;  break;
    case VK_RIGHT       : this->KeysDown |=2;  break;
    case VK_UP          : this->KeysDown |=4;  break;
    case VK_DOWN        : this->KeysDown |=8;  break;
    case 'a':
    case 'A'            : this->KeysDown |=16; break;
    case 'z':
    case 'Z'            : this->KeysDown |=32; break;
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
void vtkInteractorStyleFlight::OnKeyUp(int vtkNotUsed(ctrl), 
                                       int vtkNotUsed(shift), 
                                       char keycode, 
                                       int vtkNotUsed(repeatcount))
{
#ifdef _WIN32
  switch (keycode)
    {
    case VK_LEFT        : this->KeysDown &= ~1;  break;
    case VK_RIGHT       : this->KeysDown &= ~2;  break;
    case VK_UP          : this->KeysDown &= ~4;  break;
    case VK_DOWN        : this->KeysDown &= ~8;  break;
    case 'a':
    case 'A'            : this->KeysDown &= ~16; break;
    case 'z':
    case 'Z'            : this->KeysDown &= ~32; break;
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
void vtkInteractorStyleFlight::OnChar(int ctrl, 
                                      int shift, 
                                      char keycode, 
                                      int repeatcount)
{
  switch (keycode)
    {
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
    default:
      this->Superclass::OnChar(ctrl, shift, keycode, repeatcount);
      break;
    }
}

//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::JumpTo(double campos[3], double focpos[3]) 
{
  this->CurrentCamera->SetPosition(campos);
  this->CurrentCamera->SetFocalPoint(focpos);
  // Tidy up Camera stuff
  this->CurrentCamera->OrthogonalizeViewUp();
  if (this->FixUpVector)
    {
    this->CurrentCamera->SetViewUp(this->FixedUpVector);
    }
  this->ResetCameraClippingRange();
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
void vtkInteractorStyleFlight::PerformAzimuthalScan(int numsteps) 
{
  this->AzimuthScanning = numsteps;
  this->Flying          = 0;
  this->Reversing       = 0;
  this->KeysDown        = 0;
  this->AzimuthStepSize = 360.0/numsteps;
  this->DoTimerStart();
}

//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::AzimuthScan(void) 
{
  this->AzimuthScanning -= 1;
  this->CurrentCamera->SetViewUp(0,0,1);
  this->CurrentCamera->Yaw(this->AzimuthStepSize);

  if (this->AzimuthScanning == 0) 
    {
    this->DoTimerStop();
    }
}

//---------------------------------------------------------------------------
// Calculate angles for next redraw in timer event
//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::UpdateMouseSteering(int x, int y) 
{
  vtkRenderWindowInteractor *rwi = this->Interactor;
  double aspeed = this->AngleStepSize*(rwi->GetShiftKey() ? this->AngleAccelerationFactor : 1.0);
  //
  // we want to steer by an amount proportional to window viewangle and size
  int *size = rwi->GetSize();
  double scalefactor = 5.0*this->CurrentCamera->GetViewAngle()/(double)size[0];
  double dx = - (x - this->LastPos[0])*scalefactor;
  double dy =   (y - this->LastPos[1])*scalefactor;
  this->YawAngle   = dx*aspeed;
  this->PitchAngle = dy*aspeed;
  this->X2 = x;
  this->Y2 = y;
}

//---------------------------------------------------------------------------
// useful utility functions
//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::SetupMotionVars(void) 
{
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
void vtkInteractorStyleFlight::MotionAlongVector(double vector[3], double amount) 
{
  double oldcampos[3], oldcamfoc[3];

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
void vtkInteractorStyleFlight::ComputeLRVector(double vector[3]) 
{
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
void vtkInteractorStyleFlight::FlyByMouse(void) 
{
  double a_vector[3];
  double speed  = this->DiagonalLength * this->MotionStepSize * this->MotionUserScale;
  speed = speed * ( this->Interactor->GetShiftKey() ? this->MotionAccelerationFactor : 1.0);
  if (this->DisableMotion) 
    {
    speed = 0;
    }
  // Sidestep (convert steering angles to left right movement :
  // only because I added this after doing the angles earlier
  if (this->Interactor->GetControlKey()) 
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
  this->LastPos[0] = this->X2;
  this->LastPos[1] = this->Y2;
  this->YawAngle   = 0;
  this->PitchAngle = 0;
  //
  if (!this->Interactor->GetControlKey()) 
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
void vtkInteractorStyleFlight::FlyByKey(void) 
{

  double speed  = this->DiagonalLength * this->MotionStepSize * this->MotionUserScale;
  speed = speed * ( this->Interactor->GetShiftKey() ? this->MotionAccelerationFactor : 1.0);
  if (this->DisableMotion) 
    {
    speed = 0;
    }
  //
  double aspeed = this->AngleStepSize* (this->Interactor->GetShiftKey() ? this->AngleAccelerationFactor : 1.0);
  double a_vector[3];
  // Left and right
  if (this->Interactor->GetControlKey()) 
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
  if (this->Interactor->GetControlKey()) 
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
void vtkInteractorStyleFlight::PrintSelf(ostream& os, vtkIndent indent) 
{
  this->Superclass::PrintSelf(os,indent);
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


