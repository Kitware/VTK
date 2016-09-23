/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleFlight.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInteractorStyleFlight.h"

#include "vtkCamera.h"
#include "vtkPerspectiveTransform.h"
#include "vtkRenderer.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkCallbackCommand.h"
#include "vtkWindows.h"

vtkStandardNewMacro(vtkInteractorStyleFlight);

class CPIDControl
{
public:
  int    m_iDeltaT;
  int    m_iDeltaTf;
  double m_dPrevX;
  double m_dKp;
  double m_dKd;
  double m_dKi;
  double m_dXSum;
  double m_dDelta;

  double m_dVelSum;
  int    m_iVelCount;
  double m_dVelAvg;
public:
  CPIDControl(double dKp, double dKd, double dKi);
  double PIDCalc(double dX,double dFinalX);
  void SetCoefficients(double dKp, double dKd, double dKi);
};

CPIDControl::CPIDControl(double dKp, double dKd, double dKi)
{
  m_dXSum     = 0.0;
  m_dPrevX    = 0.0;
  m_dDelta    = 0.0;
  m_iDeltaT   = 0;
  m_iDeltaTf  = 0;
  m_dVelSum   = 0.0;
  m_iVelCount = 0;
  m_dVelAvg   = 0.0;
  this->SetCoefficients(dKp, dKd, dKi);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
double CPIDControl::PIDCalc(double dX, double dFinalX)
{
  double dVal = dX - dFinalX;
  m_dXSum     = m_dXSum + dVal;
  // Calculate the velocity
  dVal = m_dPrevX - dX;
  // Average the velocity
  m_dVelSum += dVal;
  m_iVelCount++;
  if(m_iVelCount >= 10)
  {
    m_dVelAvg = m_dVelSum/m_iVelCount;
    m_iVelCount = 0;
    m_dVelSum = 0.0;
  }
  m_dDelta = m_dKp*dX + m_dKd*m_dVelAvg + m_dKi*m_dXSum;
  m_dPrevX = dX;

  return m_dDelta;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void CPIDControl::SetCoefficients(double dKp, double dKd, double dKi)
{
  m_dKp = dKp;
  m_dKd = dKd;
  m_dKi = dKi;
  // should reset internal params here, but no need for this simple usage
}


//---------------------------------------------------------------------------
vtkInteractorStyleFlight::vtkInteractorStyleFlight()
{
  this->KeysDown     = 0;
  this->UseTimers    = 1;

  this->DiagonalLength           = 1.0;
  this->MotionStepSize           = 1.0/250.0;
  this->MotionUserScale          = 1.0;  // +/- key adjustment
  this->MotionAccelerationFactor = 10.0;
  this->AngleStepSize            = 1.0;
  this->AngleAccelerationFactor  = 5.0;

  this->DisableMotion      = 0;
  this->RestoreUpVector    = 1;
  this->DefaultUpVector[0] = 0;
  this->DefaultUpVector[1] = 0;
  this->DefaultUpVector[2] = 1;
  //
  PID_Yaw     = new CPIDControl(-0.05, 0.0, -0.0008);
  PID_Pitch   = new CPIDControl(-0.05, 0.0, -0.0008);
  Transform = vtkPerspectiveTransform::New();
}

//---------------------------------------------------------------------------
vtkInteractorStyleFlight::~vtkInteractorStyleFlight()
{
    Transform->Delete();
    delete PID_Yaw;
    delete PID_Pitch;
}
//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::ForwardFly()
{
  if (this->CurrentRenderer == NULL)
  {
    return;
  }
  vtkCamera* camera = this->CurrentRenderer->GetActiveCamera();
  //
  if (this->KeysDown)
  {
    this->FlyByKey(camera);
  }
  else
  {
    this->UpdateSteering(camera);
    this->FlyByMouse(camera);
  }
  //
  this->FinishCamera(camera);
}
//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::ReverseFly()
{
    // The code is the same, just the state variable that is tracked...
    ForwardFly();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleFlight::StartForwardFly()
{
  if (this->State != VTKIS_NONE)
  {
    return;
  }
  this->StartState(VTKIS_FORWARDFLY);
}

//----------------------------------------------------------------------------
void vtkInteractorStyleFlight::EndForwardFly()
{
  if (this->State != VTKIS_FORWARDFLY)
  {
    return;
  }
  this->StopState();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleFlight::StartReverseFly()
{
  if (this->State != VTKIS_NONE)
  {
    return;
  }
  this->StartState(VTKIS_REVERSEFLY);
}

//----------------------------------------------------------------------------
void vtkInteractorStyleFlight::EndReverseFly()
{
  if (this->State != VTKIS_REVERSEFLY)
  {
    return;
  }
  this->StopState();
}

//---------------------------------------------------------------------------
// All actual motion is performed in the timer
//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::OnTimer()
{
  switch (this->State)
  {
    case VTKIS_FORWARDFLY:
      this->ForwardFly();
      break;

    case VTKIS_REVERSEFLY:
      this->ReverseFly();
      break;

    default:
      break;
  }
}

//---------------------------------------------------------------------------
// Mouse event handlers
//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::OnMouseMove()
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];
  this->FindPokedRenderer(x, y);
  vtkCamera* cam = this->CurrentRenderer->GetActiveCamera();
  switch (this->State)
  {
    case VTKIS_FORWARDFLY:
    case VTKIS_REVERSEFLY:
      this->UpdateMouseSteering(cam);
      this->InvokeEvent(vtkCommand::InteractionEvent, NULL);
      break;
  }
}

//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::OnLeftButtonDown()
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];
  this->FindPokedRenderer(x, y);
  if (this->CurrentRenderer == NULL)
  {
    return;
  }

  //
  this->GrabFocus(this->EventCallbackCommand);
  vtkCamera* cam = this->CurrentRenderer->GetActiveCamera();
  switch (this->State)
  {
    case VTKIS_REVERSEFLY:
      this->State = VTKIS_FORWARDFLY;
      break;
    default :
      this->SetupMotionVars(cam);
      this->StartForwardFly();
      break;
  }
}

//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::OnLeftButtonUp()
{
  switch (this->State)
  {
    case VTKIS_FORWARDFLY:
      this->EndForwardFly();
      break;
    default :
      break;
  }
  if ( this->Interactor )
  {
    this->ReleaseFocus();
  }
}

//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::OnMiddleButtonDown()
{
}

//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::OnMiddleButtonUp()
{
}

//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::OnRightButtonDown()
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];
  this->FindPokedRenderer(x, y);
  if (this->CurrentRenderer == NULL)
  {
    return;
  }

  //
  this->GrabFocus(this->EventCallbackCommand);
  vtkCamera* cam = this->CurrentRenderer->GetActiveCamera();
  switch (this->State)
  {
    case VTKIS_FORWARDFLY:
      this->State = VTKIS_REVERSEFLY;
      break;
    default :
      this->SetupMotionVars(cam);
      this->StartReverseFly();
      break;
  }
}

//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::OnRightButtonUp()
{
  switch (this->State)
  {
    case VTKIS_REVERSEFLY:
      this->EndReverseFly();
      break;
    default :
      break;
  }
  if ( this->Interactor )
  {
    this->ReleaseFocus();
  }
}

//---------------------------------------------------------------------------
// Keyboard event handlers
// Note, OnChar is a key press down and then up event
// Note, OnKeyDown/OnKeyUp are more sensitive for controlling motion
//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::OnKeyDown()
{
  // New Flight mode behaviour
  // Note that we'll need #defines for ARROW key defs under non win32 OS
#ifdef _WIN32
  switch (this->Interactor->GetKeyCode())
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
  if ((this->KeysDown & (32+16)) == (32+16))
  {
    if (this->State==VTKIS_FORWARDFLY)
    {
      this->EndForwardFly();
    }
    if (this->State==VTKIS_REVERSEFLY)
    {
      this->EndReverseFly();
    }
  }
  else if ((this->KeysDown & 32) == 32)
  {
    if (this->State==VTKIS_FORWARDFLY)
    {
      this->EndForwardFly();
    }
    this->StartReverseFly();
  }
  else if ((this->KeysDown & 16) == 16)
  {
    if (this->State==VTKIS_REVERSEFLY)
    {
      this->EndReverseFly();
    }
    this->StartForwardFly();
  }
#else
  // the following if statement is a dummy one to prevent keycode not used
  // warnings under unix, (until the correct keycodes are supplied)
  if (this->Interactor->GetKeyCode() == 0x7F)
  {
    vtkWarningMacro(<<"Dummy test to prevent compiler warning");
  }
#endif
}

//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::OnKeyUp()
{
#ifdef _WIN32
  switch (this->Interactor->GetKeyCode())
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
  switch (this->State)
  {
    case VTKIS_FORWARDFLY:
      if ((this->KeysDown & 16) == 0)
      {
        this->EndForwardFly();
      }
      break;
    case VTKIS_REVERSEFLY:
      if ((this->KeysDown & 32) == 0)
      {
        this->EndReverseFly();
      }
      break;
  }
#else
  // the following if statement is a dummy one to prevent keycode not used
  // warnings under unix, (until the correct keycodes are supplied)
  if (this->Interactor->GetKeyCode() == 0x7F)
  {
    vtkWarningMacro(<<"Dummy test to prevent compiler warning");
  }
#endif
}

//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::OnChar()
{
  switch (this->Interactor->GetKeyCode())
  {
    case '+' :
      this->MotionUserScale *= 2.0;
      break;
    case '-' :
      this->MotionUserScale *= 0.5;
      break;
    default:
      this->Superclass::OnChar();
      break;
  }
}

//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::JumpTo(double campos[3], double focpos[3])
{
  if (this->CurrentRenderer == NULL)
  {
    return;
  }
  vtkCamera* cam = this->CurrentRenderer->GetActiveCamera();
  cam->SetPosition(campos);
  cam->SetFocalPoint(focpos);
  FinishCamera(cam);
  this->Interactor->Render();
}

void vtkInteractorStyleFlight::FinishCamera(vtkCamera* cam)
{
    cam->OrthogonalizeViewUp();
    if (this->RestoreUpVector)
    {
      double delta[3];
      cam->GetViewUp(delta);
      double weight = vtkMath::Dot(this->DefaultUpVector,delta);
      // only correct up if we're close to it already...
      if (weight>0.3) {
        weight = 0.25*fabs(weight);
        delta[0] = delta[0] + (this->DefaultUpVector[0]-delta[0])*weight;
        delta[1] = delta[1] + (this->DefaultUpVector[1]-delta[1])*weight;
        delta[2] = delta[2] + (this->DefaultUpVector[2]-delta[2])*weight;
        cam->SetViewUp(delta);
      }
    }
    if (this->AutoAdjustCameraClippingRange)
    {
      this->CurrentRenderer->ResetCameraClippingRange();
    }
    if (this->Interactor->GetLightFollowCamera())
    {
      this->CurrentRenderer->UpdateLightsGeometryToFollowCamera();
    }
}

//---------------------------------------------------------------------------
// Use this mouse pos and last mouse pos to get the amount of motion
// Compute an "Ideal" focal point, The flight will sterr towards this ideal
// point, but will be damped in Yaw/Pitch by our PID Controllers.
// The damping and motion is done in the timer event.
//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::UpdateMouseSteering(vtkCamera *cam)
{
  int *thispos = this->Interactor->GetEventPosition();
  int *lastpos = this->Interactor->GetLastEventPosition();

  double aspeed = this->AngleStepSize*(this->Interactor->GetShiftKey() ?
      this->AngleAccelerationFactor : 1.0);
  //
  // we want to steer by an amount proportional to window viewangle and size
  // compute dx and dy increments relative to last mouse click
  int *size = this->Interactor->GetSize();
  double scalefactor = 5*cam->GetViewAngle()/size[0];
  double dx = - (thispos[0] - lastpos[0])*scalefactor*aspeed;
  double dy =   (thispos[1] - lastpos[1])*scalefactor*aspeed;

  // Temporary until I get smooth flight working
  this->DeltaPitch = dy;
  this->DeltaYaw = dx;
/*
  Not happy with smooth flight yet, please levae this code here
  until I get around to experimenting : JB July 2002

  // dx and dy need to be converted to a new 'ideal' camera focal point
  // we maintain an actual FocalPoint and an IdealFocalPoint so that we can
  // smooth the motion using our PID controllers (one for yaw, one for pitch).
  cam->OrthogonalizeViewUp();
  double LRaxis[3];
  double *ViewUp     = cam->GetViewUp();
  double *Position   = cam->GetPosition();
  double *FocalPoint = cam->GetFocalPoint();
  this->GetLRVector(LRaxis, cam);
  //
  this->Transform->Identity();
  this->Transform->Translate(+Position[0],+Position[1],+Position[2]);
  this->Transform->RotateWXYZ(dx, ViewUp);
  this->Transform->RotateWXYZ(dy, LRaxis);
  this->Transform->Translate(-Position[0],-Position[1],-Position[2]);
  this->Transform->TransformPoint(this->IdealFocalPoint,this->IdealFocalPoint);
*/
}

// We know the ideal and actual focal points. We now want to reduce these
// to a 2D Yaw+Pitch form so that we can smooth the motion on each
void vtkInteractorStyleFlight::UpdateSteering(vtkCamera *vtkNotUsed(cam))
{
/*
  #define D2R 0.01745329251994329576923690768    // degrees to radians
  #define R2D 57.2957795130823208767981548141    // radians to degrees

  Not happy with smooth flight yet, please levae this code here
  until I get around to experimenting : JB July 2002

  cam->OrthogonalizeViewUp();
  double *Position = cam->GetPosition();
  double *FocalPoint = cam->GetFocalPoint();
  double vec1[3], vec2[3], norm1[3], norm2[3];
  vec1[0] = FocalPoint[0]-Position[0];
  vec1[1] = FocalPoint[1]-Position[1];
  vec1[2] = FocalPoint[2]-Position[2];
  vec2[0] = this->IdealFocalPoint[0]-Position[0];
  vec2[1] = this->IdealFocalPoint[1]-Position[1];
  vec2[2] = this->IdealFocalPoint[2]-Position[2];
  // what's the azimuthal angle between the Ideal and actual focal points
  // angle between planes given by Up/Focus and Up/IdealFocus
  double *ViewUp = cam->GetViewUp();
  vtkMath::Cross(vec1, ViewUp, norm1);
  vtkMath::Cross(vec2, ViewUp, norm2);
  vtkMath::Normalize(norm1);
  vtkMath::Normalize(norm2);
  //
  double dot = vtkMath::Dot(norm1, norm2);
  if (dot>1.0) {
    dot = 1.0;
  }
  else if (dot<-1.0) {
    dot =-1.0;
  }
  double yaw = R2D*acos(dot);
  // we know the angle, but is it +ve or -ve (which side is new focal point)
  double d1,d2;
  d1 = vtkMath::Dot(norm1,this->IdealFocalPoint);
  d2 = vtkMath::Dot(norm1,FocalPoint);
  if (d1-d2>0) yaw = -yaw;
  //
  // what's the elevation angle between the Ideal and actual focal points
  // angle between planes given by LR/Focus and LR/IdealFocus
  double LRaxis[3];
  this->GetLRVector(LRaxis, cam);
  vtkMath::Cross(vec1, LRaxis, norm1);
  vtkMath::Cross(vec2, LRaxis, norm2);
  vtkMath::Normalize(norm1);
  vtkMath::Normalize(norm2);
  //
  dot = vtkMath::Dot(norm1, norm2);
  if (dot>1.0) {
    dot = 1.0;
  } else if (dot<-1.0) {
    dot =-1.0;
  }
  double pitch = R2D*acos(dot);
  d1 = vtkMath::Dot(norm1,this->IdealFocalPoint);
  d2 = vtkMath::Dot(norm1,FocalPoint);
  if (d1-d2>0) pitch = -pitch;

  // use our motion dampers to reduce the yaw/pitch errors to zero gradually
//  this->DeltaYaw   = PID_Yaw->PIDCalc(yaw, 0);
//  this->DeltaPitch = PID_Pitch->PIDCalc(pitch, 0);

  this->DeltaPitch = 0.75*pitch + 0.25*lPitch;
  lPitch = pitch;

  this->DeltaYaw = 0.75*yaw + 0.25*lYaw;
  lYaw = yaw;

*/
}

//---------------------------------------------------------------------------
// useful utility functions
//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::SetupMotionVars(vtkCamera *cam)
{
  lPitch = 0;
  lYaw   = 0;
  cam->GetFocalPoint(IdealFocalPoint);

  double bounds[6];
  this->CurrentRenderer->ComputeVisiblePropBounds( bounds );
  if ( !vtkMath::AreBoundsInitialized(bounds) )
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
void vtkInteractorStyleFlight::MotionAlongVector(double vector[3],
    double amount, vtkCamera* cam)
{
  double oldcampos[3], oldcamfoc[3];
  cam->GetPosition(oldcampos);
  cam->GetFocalPoint(oldcamfoc);
  // move camera and focus along DirectionOfProjection
  cam->SetPosition(
    oldcampos[0] - amount * vector[0],
    oldcampos[1] - amount * vector[1],
    oldcampos[2] - amount * vector[2]);
  cam->SetFocalPoint(
    oldcamfoc[0] - amount * vector[0],
    oldcamfoc[1] - amount * vector[1],
    oldcamfoc[2] - amount * vector[2]);
}

//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::GetLRVector(double vector[3], vtkCamera* cam)
{
  vtkMatrix4x4 *vtm;
  vtm = cam->GetViewTransformMatrix();
  vector[0] = vtm->GetElement(0,0);
  vector[1] = vtm->GetElement(0,1);
  vector[2] = vtm->GetElement(0,2);
}

//---------------------------------------------------------------------------
// Perform the motion
//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::FlyByMouse(vtkCamera* cam)
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
    if (this->DeltaYaw!=0.0)
    {
      this->GetLRVector(a_vector, cam);
      this->MotionAlongVector(a_vector,-this->DeltaYaw*speed/2.0, cam);
    }
    if (this->DeltaPitch!=0.0)
    {
      cam->GetViewUp(a_vector);
      this->MotionAlongVector(a_vector,-this->DeltaPitch*speed/2.0, cam);
    }
  }
  else
  {
    cam->Yaw(this->DeltaYaw);
    cam->Pitch(this->DeltaPitch);
    this->DeltaYaw = 0;
    this->DeltaPitch = 0;
    // cam->SetFocalPoint(this->IdealFocalPoint);
  }
  //
  if (!this->Interactor->GetControlKey())
  {
    cam->GetDirectionOfProjection(a_vector); // reversed (use -speed)
    switch (this->State)
    {
      case VTKIS_FORWARDFLY:
        this->MotionAlongVector(a_vector, -speed, cam);
        break;
      case VTKIS_REVERSEFLY:
        this->MotionAlongVector(a_vector, speed, cam);
        break;
    }
  }
}

//---------------------------------------------------------------------------
void vtkInteractorStyleFlight::FlyByKey(vtkCamera* cam)
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
    this->GetLRVector(a_vector, cam);
    if (this->KeysDown & 1)
    {
      this->MotionAlongVector(a_vector,-speed, cam);
    }
    if (this->KeysDown & 2)
    {
      this->MotionAlongVector(a_vector, speed, cam);
    }
  }
  else
  {
    if (this->KeysDown & 1)
    {
      cam->Yaw( aspeed);
    }
    if (this->KeysDown & 2)
    {
      cam->Yaw(-aspeed);
    }
  }

  // Up and Down
  if (this->Interactor->GetControlKey())
  { // Sidestep
    cam->GetViewUp(a_vector);
    if (this->KeysDown & 4)
    {
      this->MotionAlongVector(a_vector,-speed, cam);
    }
    if (this->KeysDown & 8)
    {
      this->MotionAlongVector(a_vector, speed, cam);
    }
  }
  else
  {
    if (this->KeysDown & 4)
    {
      cam->Pitch(-aspeed);
    }
    if (this->KeysDown & 8)
    {
      cam->Pitch( aspeed);
    }
  }

  // forward and backward
  cam->GetDirectionOfProjection(a_vector);
  if (this->KeysDown & 16)
  {
    this->MotionAlongVector(a_vector, speed, cam);
  }
  if (this->KeysDown & 32)
  {
    this->MotionAlongVector(a_vector,-speed, cam);
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
  os << indent << "RestoreUpVector: "
     << this->RestoreUpVector << "\n";
  os << indent << "DefaultUpVector: "
     << this->DefaultUpVector[0] << " "
     << this->DefaultUpVector[1] << " "
     << this->DefaultUpVector[2] << "\n";
}


