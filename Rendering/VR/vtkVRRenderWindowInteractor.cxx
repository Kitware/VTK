/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVRRenderWindowInteractor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVRRenderWindowInteractor.h"

#include "vtkActor.h"
#include "vtkCommand.h"
#include "vtkEventData.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLState.h"
#include "vtkRendererCollection.h"
#include "vtkTextureObject.h"
#include "vtkTransform.h"
#include "vtkVRInteractorStyle.h"
#include "vtkVRRenderWindow.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vtksys/SystemTools.hxx>

void (*vtkVRRenderWindowInteractor::ClassExitMethod)(void*) = (void (*)(void*)) nullptr;
void* vtkVRRenderWindowInteractor::ClassExitMethodArg = (void*)nullptr;
void (*vtkVRRenderWindowInteractor::ClassExitMethodArgDelete)(void*) = (void (*)(void*)) nullptr;

//------------------------------------------------------------------------------
vtkVRRenderWindowInteractor::vtkVRRenderWindowInteractor()
{
  for (int i = 0; i < vtkEventDataNumberOfDevices; i++)
  {
    this->DeviceInputDownCount[i] = 0;
  }
}

//------------------------------------------------------------------------------
void vtkVRRenderWindowInteractor::SetPhysicalScale(double scale)
{
  vtkVRRenderWindow* win = vtkVRRenderWindow::SafeDownCast(this->RenderWindow);
  win->SetPhysicalScale(scale);
}

//------------------------------------------------------------------------------
double vtkVRRenderWindowInteractor::GetPhysicalScale()
{
  vtkVRRenderWindow* win = vtkVRRenderWindow::SafeDownCast(this->RenderWindow);
  return win->GetPhysicalScale();
}

//------------------------------------------------------------------------------
void vtkVRRenderWindowInteractor::SetPhysicalTranslation(
  vtkCamera*, double t1, double t2, double t3)
{
  vtkVRRenderWindow* win = vtkVRRenderWindow::SafeDownCast(this->RenderWindow);
  win->SetPhysicalTranslation(t1, t2, t3);
}

//------------------------------------------------------------------------------
double* vtkVRRenderWindowInteractor::GetPhysicalTranslation(vtkCamera*)
{
  vtkVRRenderWindow* win = vtkVRRenderWindow::SafeDownCast(this->RenderWindow);
  return win->GetPhysicalTranslation();
}

//------------------------------------------------------------------------------
void vtkVRRenderWindowInteractor::ConvertPoseToWorldCoordinates(
  // const float poseInTrackingCoordinates[3][4],
  vtkMatrix4x4* poseInTrackingCoordinates,
  double pos[3],  // Output world position
  double wxyz[4], // Output world orientation quaternion
  double ppos[3], // Output physical position
  double wdir[3]) // Output world view direction (-Z)
{
  vtkVRRenderWindow* win = vtkVRRenderWindow::SafeDownCast(this->RenderWindow);
  double physicalScale = win->GetPhysicalScale();
  double* trans = win->GetPhysicalTranslation();

  // Vive to world axes
  double* vup = win->GetPhysicalViewUp();
  double* dop = win->GetPhysicalViewDirection();
  double vright[3];
  vtkMath::Cross(dop, vup, vright);

  // extract HMD axes
  double hvright[3];
  // hvright[0] = poseInTrackingCoordinates[0][0];
  // hvright[1] = poseInTrackingCoordinates[1][0];
  // hvright[2] = poseInTrackingCoordinates[2][0];
  hvright[0] = poseInTrackingCoordinates->GetElement(0, 0);
  hvright[1] = poseInTrackingCoordinates->GetElement(1, 0);
  hvright[2] = poseInTrackingCoordinates->GetElement(2, 0);
  double hvup[3];
  // hvup[0] = poseInTrackingCoordinates[0][1];
  // hvup[1] = poseInTrackingCoordinates[1][1];
  // hvup[2] = poseInTrackingCoordinates[2][1];
  hvup[0] = poseInTrackingCoordinates->GetElement(0, 1);
  hvup[1] = poseInTrackingCoordinates->GetElement(1, 1);
  hvup[2] = poseInTrackingCoordinates->GetElement(2, 1);

  // convert position to world coordinates
  // get the position and orientation of the button press
  for (int i = 0; i < 3; i++)
  {
    // ppos[i] = poseInTrackingCoordinates[i][3];
    ppos[i] = poseInTrackingCoordinates->GetElement(i, 3);
  }

  pos[0] = ppos[0] * vright[0] + ppos[1] * vup[0] - ppos[2] * dop[0];
  pos[1] = ppos[0] * vright[1] + ppos[1] * vup[1] - ppos[2] * dop[1];
  pos[2] = ppos[0] * vright[2] + ppos[1] * vup[2] - ppos[2] * dop[2];
  // now adjust for scale and translation
  for (int i = 0; i < 3; i++)
  {
    pos[i] = pos[i] * physicalScale - trans[i];
  }

  // convert axes to world coordinates
  double fvright[3]; // final vright
  fvright[0] = hvright[0] * vright[0] + hvright[1] * vup[0] - hvright[2] * dop[0];
  fvright[1] = hvright[0] * vright[1] + hvright[1] * vup[1] - hvright[2] * dop[1];
  fvright[2] = hvright[0] * vright[2] + hvright[1] * vup[2] - hvright[2] * dop[2];
  double fvup[3]; // final vup
  fvup[0] = hvup[0] * vright[0] + hvup[1] * vup[0] - hvup[2] * dop[0];
  fvup[1] = hvup[0] * vright[1] + hvup[1] * vup[1] - hvup[2] * dop[1];
  fvup[2] = hvup[0] * vright[2] + hvup[1] * vup[2] - hvup[2] * dop[2];
  vtkMath::Cross(fvup, fvright, wdir);

  double ortho[3][3];
  for (int i = 0; i < 3; i++)
  {
    ortho[i][0] = fvright[i];
    ortho[i][1] = fvup[i];
    ortho[i][2] = -wdir[i];
  }

  vtkMath::Matrix3x3ToQuaternion(ortho, wxyz);

  // Compute the return value wxyz
  double mag = sqrt(wxyz[1] * wxyz[1] + wxyz[2] * wxyz[2] + wxyz[3] * wxyz[3]);

  if (mag != 0.0)
  {
    wxyz[0] = 2.0 * vtkMath::DegreesFromRadians(atan2(mag, wxyz[0]));
    wxyz[1] /= mag;
    wxyz[2] /= mag;
    wxyz[3] /= mag;
  }
  else
  {
    wxyz[0] = 0.0;
    wxyz[1] = 0.0;
    wxyz[2] = 0.0;
    wxyz[3] = 1.0;
  }
}

//------------------------------------------------------------------------------
void vtkVRRenderWindowInteractor::StartEventLoop()
{
  this->StartedMessageLoop = 1;
  this->Done = false;

  vtkVRRenderWindow* renWin = vtkVRRenderWindow::SafeDownCast(this->RenderWindow);

  vtkRenderer* ren = static_cast<vtkRenderer*>(renWin->GetRenderers()->GetItemAsObject(0));

  while (!this->Done)
  {
    this->DoOneEvent(renWin, ren);
  }
}

//------------------------------------------------------------------------------
void vtkVRRenderWindowInteractor::ProcessEvents()
{
  vtkVRRenderWindow* renWin = vtkVRRenderWindow::SafeDownCast(this->RenderWindow);

  vtkRenderer* ren = static_cast<vtkRenderer*>(renWin->GetRenderers()->GetItemAsObject(0));
  this->DoOneEvent(renWin, ren);
}

//------------------------------------------------------------------------------
void vtkVRRenderWindowInteractor::HandleGripEvents(vtkEventData* ed)
{
  vtkEventDataDevice3D* edata = ed->GetAsEventDataDevice3D();
  if (!edata)
  {
    return;
  }

  this->PointerIndex = static_cast<int>(edata->GetDevice());
  if (edata->GetAction() == vtkEventDataAction::Press)
  {
    this->DeviceInputDownCount[this->PointerIndex] = 1;

    this->StartingPhysicalEventPositions[this->PointerIndex][0] =
      this->PhysicalEventPositions[this->PointerIndex][0];
    this->StartingPhysicalEventPositions[this->PointerIndex][1] =
      this->PhysicalEventPositions[this->PointerIndex][1];
    this->StartingPhysicalEventPositions[this->PointerIndex][2] =
      this->PhysicalEventPositions[this->PointerIndex][2];

    vtkVRRenderWindow* renWin = vtkVRRenderWindow::SafeDownCast(this->RenderWindow);
    renWin->GetPhysicalToWorldMatrix(this->StartingPhysicalToWorldMatrix);

    // Both controllers have the grip down, start multitouch
    if (this->DeviceInputDownCount[static_cast<int>(vtkEventDataDevice::LeftController)] &&
      this->DeviceInputDownCount[static_cast<int>(vtkEventDataDevice::RightController)])
    {
      // we do not know what the gesture is yet
      this->CurrentGesture = vtkCommand::StartEvent;
    }
    return;
  }
  // end the gesture if needed
  if (edata->GetAction() == vtkEventDataAction::Release)
  {
    this->DeviceInputDownCount[this->PointerIndex] = 0;

    if (edata->GetInput() == vtkEventDataDeviceInput::Grip)
    {
      if (this->CurrentGesture == vtkCommand::PinchEvent)
      {
        this->EndPinchEvent();
      }
      if (this->CurrentGesture == vtkCommand::PanEvent)
      {
        this->EndPanEvent();
      }
      if (this->CurrentGesture == vtkCommand::RotateEvent)
      {
        this->EndRotateEvent();
      }
      this->CurrentGesture = vtkCommand::NoEvent;
      return;
    }
  }
}

//------------------------------------------------------------------------------
void vtkVRRenderWindowInteractor::RecognizeComplexGesture(vtkEventDataDevice3D*)
{
  // Recognize gesture only if one button is pressed per controller
  int lhand = static_cast<int>(vtkEventDataDevice::LeftController);
  int rhand = static_cast<int>(vtkEventDataDevice::RightController);

  if (this->DeviceInputDownCount[lhand] > 1 || this->DeviceInputDownCount[lhand] == 0 ||
    this->DeviceInputDownCount[rhand] > 1 || this->DeviceInputDownCount[rhand] == 0)
  {
    this->CurrentGesture = vtkCommand::NoEvent;
    return;
  }

  double* posVals[2];
  double* startVals[2];
  posVals[0] = this->PhysicalEventPositions[lhand];
  posVals[1] = this->PhysicalEventPositions[rhand];

  startVals[0] = this->StartingPhysicalEventPositions[lhand];
  startVals[1] = this->StartingPhysicalEventPositions[rhand];

  // The meat of the algorithm
  // on move events we analyze them to determine what type
  // of movement it is and then deal with it.
  if (this->CurrentGesture != vtkCommand::NoEvent)
  {
    // calculate the distances
    double originalDistance = sqrt(vtkMath::Distance2BetweenPoints(startVals[0], startVals[1]));
    double newDistance = sqrt(vtkMath::Distance2BetweenPoints(posVals[0], posVals[1]));

    // calculate the translations
    double t0[3];
    t0[0] = posVals[0][0] - startVals[0][0];
    t0[1] = posVals[0][1] - startVals[0][1];
    t0[2] = posVals[0][2] - startVals[0][2];

    double t1[3];
    t1[0] = posVals[1][0] - startVals[1][0];
    t1[1] = posVals[1][1] - startVals[1][1];
    t1[2] = posVals[1][2] - startVals[1][2];

    double trans[3];
    trans[0] = (t0[0] + t1[0]) / 2.0;
    trans[1] = (t0[1] + t1[1]) / 2.0;
    trans[2] = (t0[2] + t1[2]) / 2.0;

    // calculate rotations
    double originalAngle = vtkMath::DegreesFromRadians(
      atan2((double)startVals[1][2] - startVals[0][2], (double)startVals[1][0] - startVals[0][0]));
    double newAngle = vtkMath::DegreesFromRadians(
      atan2((double)posVals[1][2] - posVals[0][2], (double)posVals[1][0] - posVals[0][0]));

    // angles are cyclic so watch for that, -179 and 179 are only 2 apart :)
    if (newAngle - originalAngle > 180.0)
    {
      newAngle -= 360;
    }
    if (newAngle - originalAngle < -180.0)
    {
      newAngle += 360;
    }
    double angleDeviation = newAngle - originalAngle;

    // do we know what gesture we are doing yet? If not
    // see if we can figure it out
    if (this->CurrentGesture == vtkCommand::StartEvent)
    {
      // pinch is a move to/from the center point
      // rotate is a move along the circumference
      // pan is a move of the center point
      // compute the distance along each of these axes in meters
      // the first to break thresh wins
      double thresh = 0.05; // in meters

      double pinchDistance = fabs(newDistance - originalDistance);
      double panDistance = sqrt(trans[0] * trans[0] + trans[1] * trans[1] + trans[2] * trans[2]);
      double rotateDistance = originalDistance * 3.1415926 * fabs(angleDeviation) / 180.0;

      if (pinchDistance > thresh && pinchDistance > panDistance && pinchDistance > rotateDistance)
      {
        this->CurrentGesture = vtkCommand::PinchEvent;
        this->Scale = 1.0;
        this->StartPinchEvent();
      }
      else if (rotateDistance > thresh && rotateDistance > panDistance)
      {
        this->CurrentGesture = vtkCommand::RotateEvent;
        this->Rotation = 0.0;
        this->StartRotateEvent();
      }
      else if (panDistance > thresh)
      {
        this->CurrentGesture = vtkCommand::PanEvent;
        this->Translation3D[0] = 0.0;
        this->Translation3D[1] = 0.0;
        this->Translation3D[2] = 0.0;
        this->StartPanEvent();
      }
    }
    // if we have found a specific type of movement then
    // handle it
    if (this->CurrentGesture == vtkCommand::RotateEvent)
    {
      this->SetRotation(angleDeviation);
      this->RotateEvent();
    }
    if (this->CurrentGesture == vtkCommand::PinchEvent)
    {
      this->SetScale(newDistance / originalDistance);
      this->PinchEvent();
    }
    if (this->CurrentGesture == vtkCommand::PanEvent)
    {
      // Vive to world axes
      vtkVRRenderWindow* win = vtkVRRenderWindow::SafeDownCast(this->RenderWindow);
      double* vup = win->GetPhysicalViewUp();
      double* dop = win->GetPhysicalViewDirection();
      double physicalScale = win->GetPhysicalScale();
      double vright[3];
      vtkMath::Cross(dop, vup, vright);
      double wtrans[3];

      // convert translation to world coordinates
      // now adjust for scale
      for (int i = 0; i < 3; i++)
      {
        wtrans[i] = trans[0] * vright[i] + trans[1] * vup[i] - trans[2] * dop[i];
        wtrans[i] = wtrans[i] * physicalScale;
      }

      this->SetTranslation3D(wtrans);
      this->PanEvent();
    }
  }
}

//------------------------------------------------------------------------------
void vtkVRRenderWindowInteractor::Initialize()
{
  // make sure we have a RenderWindow and camera
  if (!this->RenderWindow)
  {
    vtkErrorMacro(<< "No renderer defined!");
    return;
  }
  if (this->Initialized)
  {
    return;
  }

  vtkVRRenderWindow* ren = vtkVRRenderWindow::SafeDownCast(this->RenderWindow);
  int* size;

  // get the info we need from the RenderingWindow
  size = ren->GetSize();
  ren->GetPosition();
  this->Enable();
  this->Size[0] = size[0];
  this->Size[1] = size[1];

  this->Initialized = 1;
}

//------------------------------------------------------------------------------
int vtkVRRenderWindowInteractor::InternalCreateTimer(
  int vtkNotUsed(timerId), int vtkNotUsed(timerType), unsigned long vtkNotUsed(duration))
{
  // todo
  return 0;
}

//------------------------------------------------------------------------------
int vtkVRRenderWindowInteractor::InternalDestroyTimer(int vtkNotUsed(platformTimerId))
{
  // todo
  return 0;
}

//------------------------------------------------------------------------------
// Specify the default function to be called when an interactor needs to exit.
// This callback is overridden by an instance ExitMethod that is defined.
void vtkVRRenderWindowInteractor::SetClassExitMethod(void (*f)(void*), void* arg)
{
  if (f != vtkVRRenderWindowInteractor::ClassExitMethod ||
    arg != vtkVRRenderWindowInteractor::ClassExitMethodArg)
  {
    // delete the current arg if there is a delete method
    if ((vtkVRRenderWindowInteractor::ClassExitMethodArg) &&
      (vtkVRRenderWindowInteractor::ClassExitMethodArgDelete))
    {
      (*vtkVRRenderWindowInteractor::ClassExitMethodArgDelete)(
        vtkVRRenderWindowInteractor::ClassExitMethodArg);
    }
    vtkVRRenderWindowInteractor::ClassExitMethod = f;
    vtkVRRenderWindowInteractor::ClassExitMethodArg = arg;

    // no call to this->Modified() since this is a class member function
  }
}

//------------------------------------------------------------------------------
// Set the arg delete method.  This is used to free user memory.
void vtkVRRenderWindowInteractor::SetClassExitMethodArgDelete(void (*f)(void*))
{
  if (f != vtkVRRenderWindowInteractor::ClassExitMethodArgDelete)
  {
    vtkVRRenderWindowInteractor::ClassExitMethodArgDelete = f;

    // no call to this->Modified() since this is a class member function
  }
}

//------------------------------------------------------------------------------
void vtkVRRenderWindowInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "StartedMessageLoop: " << this->StartedMessageLoop << endl;
}

//------------------------------------------------------------------------------
void vtkVRRenderWindowInteractor::ExitCallback()
{
  if (this->HasObserver(vtkCommand::ExitEvent))
  {
    this->InvokeEvent(vtkCommand::ExitEvent, nullptr);
  }
  else if (vtkVRRenderWindowInteractor::ClassExitMethod)
  {
    (*vtkVRRenderWindowInteractor::ClassExitMethod)(
      vtkVRRenderWindowInteractor::ClassExitMethodArg);
  }

  this->TerminateApp();
}

//------------------------------------------------------------------------------
vtkEventDataDevice vtkVRRenderWindowInteractor::GetPointerDevice()
{
  if (this->PointerIndex == 0)
  {
    return vtkEventDataDevice::RightController;
  }
  if (this->PointerIndex == 1)
  {
    return vtkEventDataDevice::LeftController;
  }
  return vtkEventDataDevice::Unknown;
}

//------------------------------------------------------------------------------
void vtkVRRenderWindowInteractor::GetStartingPhysicalToWorldMatrix(
  vtkMatrix4x4* startingPhysicalToWorldMatrix)
{
  if (!startingPhysicalToWorldMatrix)
  {
    return;
  }
  startingPhysicalToWorldMatrix->DeepCopy(this->StartingPhysicalToWorldMatrix);
}
