/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenVRRenderWindowInteractor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cassert>

#include "vtkOpenVROverlay.h"
#include "vtkOpenVRRenderWindowInteractor.h"
#include "vtkOpenVRRenderWindow.h"
#include "vtkRendererCollection.h"

#include "vtkEventData.h"

#include "vtkActor.h"
#include "vtkCommand.h"
#include "vtkOpenVRInteractorStyle.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenVRCamera.h"
#include "vtkTextureObject.h"

vtkStandardNewMacro(vtkOpenVRRenderWindowInteractor);

void (*vtkOpenVRRenderWindowInteractor::ClassExitMethod)(void *) = (void (*)(void *))nullptr;
void *vtkOpenVRRenderWindowInteractor::ClassExitMethodArg = (void *)nullptr;
void (*vtkOpenVRRenderWindowInteractor::ClassExitMethodArgDelete)(void *) = (void (*)(void *))nullptr;

//----------------------------------------------------------------------------
// Construct object so that light follows camera motion.
vtkOpenVRRenderWindowInteractor::vtkOpenVRRenderWindowInteractor()
{
    vtkNew<vtkOpenVRInteractorStyle> style;
    this->SetInteractorStyle(style);

    for (int i = 0; i < VTKI_MAX_POINTERS; i++)
    {
      this->DeviceInputDown[i][0] = 0;
      this->DeviceInputDown[i][1] = 0;
    }
    this->DeviceInputDownCount[0] = 0;
    this->DeviceInputDownCount[1] = 0;
}

//----------------------------------------------------------------------------
vtkOpenVRRenderWindowInteractor::~vtkOpenVRRenderWindowInteractor()
{
}

void vtkOpenVRRenderWindowInteractor::SetPhysicalScale(double scale)
{
  vtkOpenVRRenderWindow *win =
    vtkOpenVRRenderWindow::SafeDownCast(this->RenderWindow);
  win->SetPhysicalScale(scale);
}

double vtkOpenVRRenderWindowInteractor::GetPhysicalScale()
{
  vtkOpenVRRenderWindow *win =
    vtkOpenVRRenderWindow::SafeDownCast(this->RenderWindow);
  return win->GetPhysicalScale();
}

void vtkOpenVRRenderWindowInteractor::SetPhysicalTranslation(vtkCamera *, double t1, double t2, double t3)
{
  vtkOpenVRRenderWindow *win =
    vtkOpenVRRenderWindow::SafeDownCast(this->RenderWindow);
  win->SetPhysicalTranslation(t1,t2,t3);
}

double *vtkOpenVRRenderWindowInteractor::GetPhysicalTranslation(vtkCamera *)
{
  vtkOpenVRRenderWindow *win =
    vtkOpenVRRenderWindow::SafeDownCast(this->RenderWindow);
  return win->GetPhysicalTranslation();
}

void vtkOpenVRRenderWindowInteractor::ConvertPoseToWorldCoordinates(
  const vr::TrackedDevicePose_t &tdPose,
  double pos[3],
  double wxyz[4],
  double ppos[3],
  double wdir[3])
{
  vtkOpenVRRenderWindow *win =
    vtkOpenVRRenderWindow::SafeDownCast(this->RenderWindow);
  double distance = win->GetPhysicalScale();
  double *trans = win->GetPhysicalTranslation();

  // Vive to world axes
  double *vup = win->GetPhysicalViewUp();
  double *dop = win->GetPhysicalViewDirection();
  double vright[3];
  vtkMath::Cross(dop, vup, vright);

  // extract HMD axes
  double hvright[3];
  hvright[0] = tdPose.mDeviceToAbsoluteTracking.m[0][0];
  hvright[1] = tdPose.mDeviceToAbsoluteTracking.m[1][0];
  hvright[2] = tdPose.mDeviceToAbsoluteTracking.m[2][0];
  double hvup[3];
  hvup[0] = tdPose.mDeviceToAbsoluteTracking.m[0][1];
  hvup[1] = tdPose.mDeviceToAbsoluteTracking.m[1][1];
  hvup[2] = tdPose.mDeviceToAbsoluteTracking.m[2][1];

  // convert position to world coordinates
  // get the position and orientation of the button press
  for (int i = 0; i < 3; i++)
  {
    pos[i] = tdPose.mDeviceToAbsoluteTracking.m[i][3];
  }

  ppos[0] = pos[0]*vright[0] + pos[1]*vup[0] - pos[2]*dop[0];
  ppos[1] = pos[0]*vright[1] + pos[1]*vup[1] - pos[2]*dop[1];
  ppos[2] = pos[0]*vright[2] + pos[1]*vup[2] - pos[2]*dop[2];
  // now adjust for scale and translation
  for (int i = 0; i < 3; i++)
  {
    pos[i] = ppos[i]*distance - trans[i];
  }

  // convert axes to world coordinates
  double fvright[3]; // final vright
  fvright[0] = hvright[0]*vright[0] + hvright[1]*vup[0] - hvright[2]*dop[0];
  fvright[1] = hvright[0]*vright[1] + hvright[1]*vup[1] - hvright[2]*dop[1];
  fvright[2] = hvright[0]*vright[2] + hvright[1]*vup[2] - hvright[2]*dop[2];
  double fvup[3]; // final vup
  fvup[0] = hvup[0]*vright[0] + hvup[1]*vup[0] - hvup[2]*dop[0];
  fvup[1] = hvup[0]*vright[1] + hvup[1]*vup[1] - hvup[2]*dop[1];
  fvup[2] = hvup[0]*vright[2] + hvup[1]*vup[2] - hvup[2]*dop[2];
  vtkMath::Cross(fvup, fvright, wdir);

  double ortho[3][3];
  for (int i = 0; i < 3; i++)
  {
    ortho[i][0] = fvright[i];
    ortho[i][1] = fvup[i];
    ortho[i][2] = -wdir[i];
  }

  vtkMath::Matrix3x3ToQuaternion(ortho, wxyz);

  // calc the return value wxyz
 double mag = sqrt( wxyz[1] * wxyz[1] + wxyz[2] * wxyz[2] + wxyz[3] * wxyz[3] );

  if ( mag != 0.0 )
  {
    wxyz[0] = 2.0 * vtkMath::DegreesFromRadians( atan2( mag, wxyz[0] ) );
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

void vtkOpenVRRenderWindowInteractor::UpdateTouchPadPosition(
  vr::IVRSystem *pHMD,
   vr::TrackedDeviceIndex_t tdi)
{
  vr::VRControllerState_t cstate;
  pHMD->GetControllerState(tdi, &cstate, sizeof(cstate));

  for (unsigned int i = 0; i < vr::k_unControllerStateAxisCount; i++)
  {
    if (pHMD->GetInt32TrackedDeviceProperty(tdi,
      static_cast<vr::ETrackedDeviceProperty>(vr::ETrackedDeviceProperty::Prop_Axis0Type_Int32 + i))
      == vr::EVRControllerAxisType::k_eControllerAxis_TrackPad)
    {
      this->SetTouchPadPosition(cstate.rAxis[i].x,cstate.rAxis[i].y);
    }
  }
}

//----------------------------------------------------------------------------
void  vtkOpenVRRenderWindowInteractor::StartEventLoop()
{
  this->StartedMessageLoop = 1;
  this->Done = false;

  vtkOpenVRRenderWindow *renWin =
    vtkOpenVRRenderWindow::SafeDownCast(this->RenderWindow);

  vtkRenderer *ren = static_cast<vtkRenderer *>(
    renWin->GetRenderers()->GetItemAsObject(0));

  while (!this->Done)
  {
    this->DoOneEvent(renWin, ren);
  }
}

void vtkOpenVRRenderWindowInteractor::DoOneEvent(vtkOpenVRRenderWindow *renWin, vtkRenderer *ren)
{
  if (!renWin || !ren)
  {
    return;
  }
  vr::IVRSystem *pHMD = renWin->GetHMD();

  if (!pHMD)
  {
    // try rendering to create the HMD connection
    renWin->Render();
    return;
  }

  vr::VREvent_t event;
  vtkOpenVROverlay *ovl = renWin->GetDashboardOverlay();
  bool result = false;

  if (vr::VROverlay() &&
     vr::VROverlay()->IsOverlayVisible( ovl->GetOverlayHandle() ))
  {
    result = vr::VROverlay()->PollNextOverlayEvent(
      ovl->GetOverlayHandle(),
      &event, sizeof( vr::VREvent_t ) );

    if (result)
    {
      int height = ovl->GetOverlayTexture()->GetHeight();
      switch( event.eventType )
      {
        case vr::VREvent_MouseButtonDown:
        {
          if (event.data.mouse.button == vr::VRMouseButton_Left)
          {
            ovl->MouseButtonPress(event.data.mouse.x, height - event.data.mouse.y - 1);
          }
        }
        break;

        case vr::VREvent_MouseButtonUp:
        {
          if (event.data.mouse.button == vr::VRMouseButton_Left)
          {
            ovl->MouseButtonRelease(event.data.mouse.x, height - event.data.mouse.y - 1);
          }
        }
        break;

        case vr::VREvent_MouseMove:
        {
          ovl->MouseMoved(event.data.mouse.x, height - event.data.mouse.y - 1);
        }
        break;

        case vr::VREvent_OverlayShown:
        {
          renWin->RenderOverlay();
        }
        break;

        case vr::VREvent_Quit:
          this->Done = true;
        break;
      }
    }

    // eat up any pending events
    while(pHMD->PollNextEvent(&event, sizeof(vr::VREvent_t))) { };
  }
  else
  {
    result = pHMD->PollNextEvent(&event, sizeof(vr::VREvent_t));

    // process all pending events
    while (result)
    {
      vr::TrackedDeviceIndex_t tdi = event.trackedDeviceIndex;

      // is it a controller button action?
      if (pHMD->GetTrackedDeviceClass(tdi) ==
          vr::ETrackedDeviceClass::TrackedDeviceClass_Controller &&
            (event.eventType == vr::VREvent_ButtonPress ||
             event.eventType == vr::VREvent_ButtonUnpress))
      {
        vr::ETrackedControllerRole role = pHMD->GetControllerRoleForTrackedDeviceIndex(tdi);

        this->UpdateTouchPadPosition(pHMD,tdi);

        // 0 = right hand 1 = left
        int pointerIndex =
          (role == vr::ETrackedControllerRole::TrackedControllerRole_RightHand ? 0 : 1);
        this->PointerIndexLookup[pointerIndex] = tdi;

        vr::TrackedDevicePose_t &tdPose = renWin->GetTrackedDevicePose(tdi);
        double pos[3];
        double ppos[3];
        double wxyz[4];
        double wdir[3];
        this->ConvertPoseToWorldCoordinates(tdPose, pos, wxyz, ppos, wdir);

        // so even though we have world coordinates we have to convert them to
        // screen coordinates because all of VTKs picking code is currently
        // based on screen coordinates
        ren->SetWorldPoint(pos[0],pos[1],pos[2],1.0);
        ren->WorldToDisplay();
        double *displayCoords = ren->GetDisplayPoint();

        this->SetEventPosition(displayCoords[0],displayCoords[1],pointerIndex);
        this->SetWorldEventPosition(pos[0],pos[1],pos[2],pointerIndex);
        this->SetPhysicalEventPosition(ppos[0], ppos[1], ppos[2], pointerIndex);
        this->SetWorldEventOrientation(wxyz[0],wxyz[1],wxyz[2],wxyz[3],pointerIndex);
        this->SetPointerIndex(pointerIndex);

        vtkNew<vtkEventDataButton3D> ed;
        ed->SetDevice(pointerIndex ? vtkEventDataDevice::LeftController : vtkEventDataDevice::RightController);
        ed->SetAction(event.eventType == vr::VREvent_ButtonPress
          ? vtkEventDataAction::Press : vtkEventDataAction::Release);
        ed->SetWorldPosition(pos);
        ed->SetWorldOrientation(wxyz);
        ed->SetWorldDirection(wdir);

        switch (event.data.controller.button)
        {
          default:
          case vr::EVRButtonId::k_EButton_Axis1:
            ed->SetInput(vtkEventDataDeviceInput::Trigger);
            break;
          case vr::EVRButtonId::k_EButton_Axis0:
            ed->SetInput(vtkEventDataDeviceInput::TrackPad);
            vr::VRControllerState_t cstate;
            pHMD->GetControllerState(tdi, &cstate, sizeof(cstate));
            for (unsigned int i = 0; i < vr::k_unControllerStateAxisCount; i++)
            {
              if (pHMD->GetInt32TrackedDeviceProperty(tdi,
                static_cast<vr::ETrackedDeviceProperty>(vr::ETrackedDeviceProperty::Prop_Axis0Type_Int32 + i))
                == vr::EVRControllerAxisType::k_eControllerAxis_TrackPad)
              {
                ed->SetTrackPadPosition(cstate.rAxis[i].x,cstate.rAxis[i].y);
              }
            }
            break;
          case vr::EVRButtonId::k_EButton_Grip:
            ed->SetInput(vtkEventDataDeviceInput::Grip);
            break;
          case vr::EVRButtonId::k_EButton_ApplicationMenu:
            ed->SetInput(vtkEventDataDeviceInput::ApplicationMenu);
            break;
        }

        if (this->Enabled)
        {
          this->InvokeEvent(vtkCommand::Button3DEvent, ed);
          //----------------------------------------------------------------------------
          //Handle Multitouch
          if (this->RecognizeGestures)
          {
            int iInput = static_cast<int>(ed->GetInput());
            if (ed->GetAction() == vtkEventDataAction::Press)
            {
              if (!this->DeviceInputDown[iInput][pointerIndex])
              {
                this->DeviceInputDown[iInput][pointerIndex] = 1;
                this->DeviceInputDownCount[pointerIndex]++;
              }
            }
            if (ed->GetAction() == vtkEventDataAction::Release)
            {
              if (this->DeviceInputDown[iInput][pointerIndex])
              {
                this->DeviceInputDown[iInput][pointerIndex] = 0;
                this->DeviceInputDownCount[pointerIndex]--;
              }
            }
            this->RecognizeComplexGesture(ed);
          }
          //----------------------------------------------------------------------------
        }
      }

      result = pHMD->PollNextEvent(&event, sizeof(vr::VREvent_t));
    }

    // for each controller create mouse move event
    for (uint32_t unTrackedDevice = vr::k_unTrackedDeviceIndex_Hmd + 1;
         unTrackedDevice < vr::k_unMaxTrackedDeviceCount; unTrackedDevice++ )
    {
      // is it not connected?
      if (!pHMD->IsTrackedDeviceConnected( unTrackedDevice ) )
      {
        continue;
      }
      if (pHMD->GetTrackedDeviceClass( unTrackedDevice ) != vr::TrackedDeviceClass_Controller)
      {
        continue;
      }

      const vr::TrackedDevicePose_t &tdPose = renWin->GetTrackedDevicePose(unTrackedDevice);
      // is the model's pose not valid?
      if( !tdPose.bPoseIsValid )
      {
        continue;
      }

      vr::ETrackedControllerRole role = pHMD->GetControllerRoleForTrackedDeviceIndex(unTrackedDevice);

      // 0 = right hand 1 = left
      int pointerIndex =
        (role == vr::ETrackedControllerRole::TrackedControllerRole_RightHand ? 0 : 1);
      this->PointerIndexLookup[pointerIndex] = unTrackedDevice;
      this->SetPointerIndex(pointerIndex);

      if (this->PointersDown[pointerIndex])
      {
        this->UpdateTouchPadPosition(pHMD, unTrackedDevice);
      }

      double pos[3];
      double ppos[3];
      double wxyz[4];
      double wdir[3];
      this->ConvertPoseToWorldCoordinates(tdPose, pos, wxyz, ppos, wdir);

      // so even though we have world coordinates we have to convert them to
      // screen coordinates because all of VTKs picking code is currently
      // based on screen coordinates
      ren->SetWorldPoint(pos[0],pos[1],pos[2],1.0);
      ren->WorldToDisplay();
      double *displayCoords = ren->GetDisplayPoint();
      this->SetEventPosition(displayCoords[0], displayCoords[1], pointerIndex);
      this->SetWorldEventPosition(pos[0],pos[1],pos[2],pointerIndex);
      this->SetWorldEventOrientation(wxyz[0],wxyz[1],wxyz[2],wxyz[3],pointerIndex);
      this->SetPhysicalEventPosition(ppos[0], ppos[1], ppos[2], pointerIndex);
      vtkNew<vtkEventDataMove3D> ed;
      ed->SetDevice(pointerIndex ? vtkEventDataDevice::LeftController : vtkEventDataDevice::RightController);
      ed->SetWorldPosition(pos);
      ed->SetWorldOrientation(wxyz);
      ed->SetWorldDirection(wdir);
      if (this->Enabled)
      {
        this->InvokeEvent(vtkCommand::Move3DEvent, ed);
        if (this->RecognizeGestures)
        {
          this->RecognizeComplexGesture(ed);
        }
      }
    }

    this->InvokeEvent(vtkCommand::RenderEvent);
    renWin->Render();
  }
}

//----------------------------------------------------------------------------
void vtkOpenVRRenderWindowInteractor::RecognizeComplexGesture(vtkEventDataDevice3D* edata)
{
  //Recognize gesture only if one button is pressed per controller
  if (this->DeviceInputDownCount[this->PointerIndex] > 2 ||
    this->DeviceInputDownCount[this->PointerIndex] == 0)
  {
    this->CurrentGesture = vtkCommand::NoEvent;
    return;
  }

  // store the initial positions
  if (edata->GetType() == vtkCommand::Button3DEvent)
  {
    if (edata->GetAction() == vtkEventDataAction::Press)
    {
        int iInput = static_cast<int>(vtkEventDataDeviceInput::Grip);

        this->StartingPhysicalEventPositions[this->PointerIndex][0] =
          this->PhysicalEventPositions[this->PointerIndex][0];
        this->StartingPhysicalEventPositions[this->PointerIndex][1] =
          this->PhysicalEventPositions[this->PointerIndex][1];
        this->StartingPhysicalEventPositions[this->PointerIndex][2] =
          this->PhysicalEventPositions[this->PointerIndex][2];

        //Both controllers have the grip down, start multitouch
        if (this->DeviceInputDown[iInput][0] && this->DeviceInputDown[iInput][1])
        {
          // we do not know what the gesture is yet
          this->CurrentGesture = vtkCommand::StartEvent;
        }
      return;
    }
    // end the gesture if needed
    if (edata->GetAction() == vtkEventDataAction::Release)
    {
      if (edata->GetInput() == vtkEventDataDeviceInput::Trigger)
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

  double *posVals[2];
  double *startVals[2];
  posVals[0] = this->PhysicalEventPositions[0];
  posVals[1] = this->PhysicalEventPositions[1];

  startVals[0] = this->StartingPhysicalEventPositions[0];
  startVals[1] = this->StartingPhysicalEventPositions[1];


  // The meat of the algorithm
  // on move events we analyze them to determine what type
  // of movement it is and then deal with it.
  if (edata->GetType() == vtkCommand::Move3DEvent &&
    this->CurrentGesture != vtkCommand::NoEvent)
  {
    //Reduce computation
    if (!this->PointerIndex)
    {
      return;
    }

    // calculate the distances
    double originalDistance = sqrt(
      vtkMath::Distance2BetweenPoints(startVals[0], startVals[1]));
    double newDistance = sqrt(
      vtkMath::Distance2BetweenPoints(posVals[0], posVals[1]));

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
    double originalAngle =
      vtkMath::DegreesFromRadians(atan2((double)startVals[1][2] - startVals[0][2],
      (double)startVals[1][0] - startVals[0][0]));
    double newAngle =
      vtkMath::DegreesFromRadians(atan2((double)posVals[1][2] - posVals[0][2],
      (double)posVals[1][0] - posVals[0][0]));

    // angles are cyclic so watch for that, 1 and 359 are only 2 apart :)
    double angleDeviation = newAngle - originalAngle;
    newAngle = (newAngle + 180.0 >= 360.0 ? newAngle - 180.0 : newAngle + 180.0);
    originalAngle = (originalAngle + 180.0 >= 360.0 ? originalAngle - 180.0 : originalAngle + 180.0);
    if (fabs(newAngle - originalAngle) < fabs(angleDeviation))
    {
      angleDeviation = newAngle - originalAngle;
    }

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
      double rotateDistance = originalDistance*3.1415926*fabs(angleDeviation) / 180.0;

      if (pinchDistance > thresh
        && pinchDistance > panDistance
        && pinchDistance > rotateDistance)
      {
        this->CurrentGesture = vtkCommand::PinchEvent;
        this->Scale = 1.0;
        this->StartPinchEvent();
      }
      else if (rotateDistance > thresh
        && rotateDistance > panDistance)
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
      this->SetTranslation3D(trans);
      this->PanEvent();
    }
  }
}

//----------------------------------------------------------------------------
void vtkOpenVRRenderWindowInteractor::Initialize()
{
  // make sure we have a RenderWindow and camera
  if ( ! this->RenderWindow)
  {
    vtkErrorMacro(<<"No renderer defined!");
    return;
  }
  if (this->Initialized)
  {
    return;
  }

  vtkOpenVRRenderWindow *ren =
    vtkOpenVRRenderWindow::SafeDownCast(this->RenderWindow);
  int *size;

  this->Initialized = 1;
  // get the info we need from the RenderingWindow

  size = ren->GetSize();
  ren->GetPosition();
  this->Enable();
  this->Size[0] = size[0];
  this->Size[1] = size[1];
}

//----------------------------------------------------------------------------
void vtkOpenVRRenderWindowInteractor::TerminateApp(void)
{
  this->Done = true;
}

//----------------------------------------------------------------------------
int vtkOpenVRRenderWindowInteractor::InternalCreateTimer(
  int vtkNotUsed(timerId),
  int vtkNotUsed(timerType),
  unsigned long vtkNotUsed(duration))
{
  // todo
  return 0;
}

//----------------------------------------------------------------------------
int vtkOpenVRRenderWindowInteractor::InternalDestroyTimer(
  int vtkNotUsed(platformTimerId))
{
  // todo
  return 0;
}


//----------------------------------------------------------------------------
// Specify the default function to be called when an interactor needs to exit.
// This callback is overridden by an instance ExitMethod that is defined.
void
vtkOpenVRRenderWindowInteractor::SetClassExitMethod(void (*f)(void *),void *arg)
{
  if ( f != vtkOpenVRRenderWindowInteractor::ClassExitMethod
       || arg != vtkOpenVRRenderWindowInteractor::ClassExitMethodArg)
  {
    // delete the current arg if there is a delete method
    if ((vtkOpenVRRenderWindowInteractor::ClassExitMethodArg)
        && (vtkOpenVRRenderWindowInteractor::ClassExitMethodArgDelete))
    {
      (*vtkOpenVRRenderWindowInteractor::ClassExitMethodArgDelete)
        (vtkOpenVRRenderWindowInteractor::ClassExitMethodArg);
    }
    vtkOpenVRRenderWindowInteractor::ClassExitMethod = f;
    vtkOpenVRRenderWindowInteractor::ClassExitMethodArg = arg;

    // no call to this->Modified() since this is a class member function
  }
}

//----------------------------------------------------------------------------
// Set the arg delete method.  This is used to free user memory.
void
vtkOpenVRRenderWindowInteractor::SetClassExitMethodArgDelete(void (*f)(void *))
{
  if (f != vtkOpenVRRenderWindowInteractor::ClassExitMethodArgDelete)
  {
    vtkOpenVRRenderWindowInteractor::ClassExitMethodArgDelete = f;

    // no call to this->Modified() since this is a class member function
  }
}

//----------------------------------------------------------------------------
void vtkOpenVRRenderWindowInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "StartedMessageLoop: " << this->StartedMessageLoop << endl;
}

//----------------------------------------------------------------------------
void vtkOpenVRRenderWindowInteractor::ExitCallback()
{
  if (this->HasObserver(vtkCommand::ExitEvent))
  {
    this->InvokeEvent(vtkCommand::ExitEvent,nullptr);
  }
  else if (this->ClassExitMethod)
  {
    (*this->ClassExitMethod)(this->ClassExitMethodArg);
  }

  this->TerminateApp();
}

vtkEventDataDevice vtkOpenVRRenderWindowInteractor::GetPointerDevice()
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
