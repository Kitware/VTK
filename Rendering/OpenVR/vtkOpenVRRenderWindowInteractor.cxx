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
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "vtkOpenGLState.h"
#include "vtkOpenVROverlay.h"
#include "vtkOpenVRRenderWindow.h"
#include "vtkOpenVRRenderWindowInteractor.h"
#include "vtkRendererCollection.h"
#include "vtkVRRenderWindow.h"

#include "vtkEventData.h"

#include "vtkActor.h"
#include "vtkCommand.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenVRCamera.h"
#include "vtkOpenVRInteractorStyle.h"
#include "vtkTextureObject.h"
#include "vtkTransform.h"
#include <vtksys/SystemTools.hxx>

vtkStandardNewMacro(vtkOpenVRRenderWindowInteractor);

void (*vtkOpenVRRenderWindowInteractor::ClassExitMethod)(void*) = (void (*)(void*)) nullptr;
void* vtkOpenVRRenderWindowInteractor::ClassExitMethodArg = (void*)nullptr;
void (*vtkOpenVRRenderWindowInteractor::ClassExitMethodArgDelete)(
  void*) = (void (*)(void*)) nullptr;

//------------------------------------------------------------------------------
// Construct object so that light follows camera motion.
vtkOpenVRRenderWindowInteractor::vtkOpenVRRenderWindowInteractor()
{
  vtkNew<vtkOpenVRInteractorStyle> style;
  this->SetInteractorStyle(style);

  for (int i = 0; i < vtkEventDataNumberOfDevices; i++)
  {
    this->DeviceInputDownCount[i] = 0;
  }
  this->ActionManifestFileName = "./vtk_openvr_actions.json";
  this->ActionSetName = "/actions/vtk";
}

//------------------------------------------------------------------------------
vtkOpenVRRenderWindowInteractor::~vtkOpenVRRenderWindowInteractor() = default;

void vtkOpenVRRenderWindowInteractor::SetPhysicalScale(double scale)
{
  vtkVRRenderWindow* win = vtkVRRenderWindow::SafeDownCast(this->RenderWindow);
  win->SetPhysicalScale(scale);
}

double vtkOpenVRRenderWindowInteractor::GetPhysicalScale()
{
  vtkVRRenderWindow* win = vtkVRRenderWindow::SafeDownCast(this->RenderWindow);
  return win->GetPhysicalScale();
}

void vtkOpenVRRenderWindowInteractor::SetPhysicalTranslation(
  vtkCamera*, double t1, double t2, double t3)
{
  vtkVRRenderWindow* win = vtkVRRenderWindow::SafeDownCast(this->RenderWindow);
  win->SetPhysicalTranslation(t1, t2, t3);
}

double* vtkOpenVRRenderWindowInteractor::GetPhysicalTranslation(vtkCamera*)
{
  vtkVRRenderWindow* win = vtkVRRenderWindow::SafeDownCast(this->RenderWindow);
  return win->GetPhysicalTranslation();
}

void vtkOpenVRRenderWindowInteractor::ConvertOpenVRPoseToMatrices(
  const vr::TrackedDevicePose_t& tdPose, vtkMatrix4x4* poseMatrixWorld,
  vtkMatrix4x4* poseMatrixPhysical /*=nullptr*/)
{
  if (!poseMatrixWorld && !poseMatrixPhysical)
  {
    return;
  }

  vtkNew<vtkMatrix4x4> poseMatrixPhysicalTemp;
  for (int row = 0; row < 3; ++row)
  {
    for (int col = 0; col < 4; ++col)
    {
      poseMatrixPhysicalTemp->SetElement(row, col, tdPose.mDeviceToAbsoluteTracking.m[row][col]);
    }
  }
  if (poseMatrixPhysical)
  {
    poseMatrixPhysical->DeepCopy(poseMatrixPhysicalTemp);
  }

  if (poseMatrixWorld)
  {
    vtkVRRenderWindow* win = vtkVRRenderWindow::SafeDownCast(this->RenderWindow);
    vtkNew<vtkMatrix4x4> physicalToWorldMatrix;
    win->GetPhysicalToWorldMatrix(physicalToWorldMatrix);
    vtkMatrix4x4::Multiply4x4(physicalToWorldMatrix, poseMatrixPhysicalTemp, poseMatrixWorld);
  }
}

void vtkOpenVRRenderWindowInteractor::ConvertPoseToWorldCoordinates(
  const vr::TrackedDevicePose_t& tdPose,
  double pos[3],  // Output world position
  double wxyz[4], // Output world orientation quaternion
  double ppos[3], // Output physical position
  double wdir[3]) // Output world view direction (-Z)
{
  // Convenience function to use the openvr-independant function
  // TODO: remove it
  this->ConvertPoseMatrixToWorldCoordinates(
    tdPose.mDeviceToAbsoluteTracking.m, pos, wxyz, ppos, wdir);
}

void vtkOpenVRRenderWindowInteractor::ConvertPoseMatrixToWorldCoordinates(
  const float poseMatrix[3][4],
  double pos[3],  // Output world position
  double wxyz[4], // Output world orientation quaternion
  double ppos[3], // Output physical position
  double wdir[3]) // Output world view direction (-Z)
{
  // TODO: define it in generic superclass VRRenderWindowInteractor
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
  hvright[0] = poseMatrix[0][0];
  hvright[1] = poseMatrix[1][0];
  hvright[2] = poseMatrix[2][0];
  double hvup[3];
  hvup[0] = poseMatrix[0][1];
  hvup[1] = poseMatrix[1][1];
  hvup[2] = poseMatrix[2][1];

  // convert position to world coordinates
  // get the position and orientation of the button press
  for (int i = 0; i < 3; i++)
  {
    ppos[i] = poseMatrix[i][3];
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

  // calc the return value wxyz
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

//---------------------------------------------------------------------------------------------------------------------
// Purpose: Returns true if the action is active and its state is true
//---------------------------------------------------------------------------------------------------------------------
bool GetDigitalActionState(
  vr::VRActionHandle_t action, vr::VRInputValueHandle_t* pDevicePath = nullptr)
{
  vr::InputDigitalActionData_t actionData;
  vr::VRInput()->GetDigitalActionData(
    action, &actionData, sizeof(actionData), vr::k_ulInvalidInputValueHandle);
  if (pDevicePath)
  {
    *pDevicePath = vr::k_ulInvalidInputValueHandle;
    if (actionData.bActive)
    {
      vr::InputOriginInfo_t originInfo;
      if (vr::VRInputError_None ==
        vr::VRInput()->GetOriginTrackedDeviceInfo(
          actionData.activeOrigin, &originInfo, sizeof(originInfo)))
      {
        *pDevicePath = originInfo.devicePath;
      }
    }
  }
  return actionData.bActive && actionData.bState;
}

//------------------------------------------------------------------------------
void vtkOpenVRRenderWindowInteractor::StartEventLoop()
{
  this->StartedMessageLoop = 1;
  this->Done = false;

  vtkOpenVRRenderWindow* renWin = vtkOpenVRRenderWindow::SafeDownCast(this->RenderWindow);

  vtkRenderer* ren = static_cast<vtkRenderer*>(renWin->GetRenderers()->GetItemAsObject(0));

  while (!this->Done)
  {
    this->DoOneEvent(renWin, ren);
  }
}

void vtkOpenVRRenderWindowInteractor::ProcessEvents()
{
  vtkOpenVRRenderWindow* renWin = vtkOpenVRRenderWindow::SafeDownCast(this->RenderWindow);

  vtkRenderer* ren = static_cast<vtkRenderer*>(renWin->GetRenderers()->GetItemAsObject(0));
  this->DoOneEvent(renWin, ren);
}

void vtkOpenVRRenderWindowInteractor::DoOneEvent(vtkOpenVRRenderWindow* renWin, vtkRenderer* ren)
{
  if (!renWin || !ren)
  {
    return;
  }
  vr::IVRSystem* pHMD = renWin->GetHMD();

  if (!pHMD)
  {
    // try rendering to create the HMD connection
    renWin->Render();
    return;
  }

  vr::VREvent_t event;
  vtkOpenVROverlay* ovl = renWin->GetDashboardOverlay();
  bool result = false;

  if (vr::VROverlay() && vr::VROverlay()->IsOverlayVisible(ovl->GetOverlayHandle()))
  {
    result =
      vr::VROverlay()->PollNextOverlayEvent(ovl->GetOverlayHandle(), &event, sizeof(vr::VREvent_t));

    if (result)
    {
      int height = ovl->GetOverlayTexture()->GetHeight();
      switch (event.eventType)
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
    while (pHMD->PollNextEvent(&event, sizeof(vr::VREvent_t)))
    {
    }
  }
  else
  {
    result = pHMD->PollNextEvent(&event, sizeof(vr::VREvent_t));

    // process all pending events
    while (result)
    {
      result = pHMD->PollNextEvent(&event, sizeof(vr::VREvent_t));
    }

    // Process SteamVR action state
    // UpdateActionState is called each frame to update the state of the actions themselves. The
    // application controls which action sets are active with the provided array of
    // VRActiveActionSet_t structs.
    vr::VRActiveActionSet_t actionSet = { 0 };
    actionSet.ulActionSet = this->ActionsetVTK;
    vr::VRInput()->UpdateActionState(&actionSet, sizeof(actionSet), 1);

    for (int tracker = 0; tracker < vtkOpenVRRenderWindowInteractor::NumberOfTrackers; tracker++)
    {
      vr::InputOriginInfo_t originInfo;
      vr::EVRInputError evriError = vr::VRInput()->GetOriginTrackedDeviceInfo(
        this->Trackers[tracker].Source, &originInfo, sizeof(vr::InputOriginInfo_t));
      if (evriError != vr::VRInputError_None)
      {
        // this can happen when a tracker isn't online
        continue;
      }

      vr::EVRCompositorError evrcError = vr::VRCompositor()->GetLastPoseForTrackedDeviceIndex(
        originInfo.trackedDeviceIndex, &this->Trackers[tracker].LastPose, nullptr);
      if (evrcError != vr::VRCompositorError_None)
      {
        vtkErrorMacro("Error in GetLastPoseForTrackedDeviceIndex: " << evrcError);
        continue;
      }

      if (this->Trackers[tracker].LastPose.bPoseIsValid)
      {
        double pos[3] = { 0.0 };
        double ppos[3] = { 0.0 };
        double wxyz[4] = { 0.0 };
        double wdir[3] = { 0.0 };
        this->ConvertPoseToWorldCoordinates(
          this->Trackers[tracker].LastPose, pos, wxyz, ppos, wdir);
        vtkNew<vtkEventDataDevice3D> ed;
        ed->SetWorldPosition(pos);
        ed->SetWorldOrientation(wxyz);
        ed->SetWorldDirection(wdir);

        switch (tracker)
        {
          case LeftHand:
            ed->SetDevice(vtkEventDataDevice::LeftController);
            break;
          case RightHand:
            ed->SetDevice(vtkEventDataDevice::RightController);
            break;
          case Head:
            ed->SetDevice(vtkEventDataDevice::HeadMountedDisplay);
            break;
        }
        ed->SetType(vtkCommand::Move3DEvent);

        // would like to remove the three following lines, they are there mostly to support
        // multitouch and event handling where the handler doesn;t use the data in the event
        // the first doesn't seem to be a common use pattern for VR, the second isn't used
        // to my knowledge
        this->SetPointerIndex(static_cast<int>(ed->GetDevice()));
        this->SetPhysicalEventPosition(ppos[0], ppos[1], ppos[2], this->PointerIndex);
        this->SetWorldEventPosition(pos[0], pos[1], pos[2], this->PointerIndex);
        this->SetWorldEventOrientation(wxyz[0], wxyz[1], wxyz[2], wxyz[3], this->PointerIndex);

        if (this->Enabled)
        {
          this->InvokeEvent(vtkCommand::Move3DEvent, ed);
        }
      }
    }

    // handle other actions
    for (auto& it : this->ActionMap)
    {
      vr::VRActionHandle_t& actionHandle = it.second.ActionHandle;
      vtkEventDataDevice3D* edp = nullptr;
      vr::VRInputValueHandle_t activeOrigin = 0;

      if (it.second.IsAnalog)
      {
        vr::InputAnalogActionData_t analogData;
        if (vr::VRInput()->GetAnalogActionData(actionHandle, &analogData, sizeof(analogData),
              vr::k_ulInvalidInputValueHandle) == vr::VRInputError_None &&
          analogData.bActive)
        {
          // send a movement event
          edp = vtkEventDataDevice3D::New();
          edp->SetTrackPadPosition(analogData.x, analogData.y);
          activeOrigin = analogData.activeOrigin;
        }
      }
      else
      {
        vr::InputDigitalActionData_t actionData;
        auto vrresult = vr::VRInput()->GetDigitalActionData(
          actionHandle, &actionData, sizeof(actionData), vr::k_ulInvalidInputValueHandle);
        if (vrresult == vr::VRInputError_None && actionData.bActive && actionData.bChanged)
        {
          edp = vtkEventDataDevice3D::New();
          edp->SetAction(
            actionData.bState ? vtkEventDataAction::Press : vtkEventDataAction::Release);
          activeOrigin = actionData.activeOrigin;
        }
      }

      if (edp)
      {
        double pos[3] = { 0.0 };
        double ppos[3] = { 0.0 };
        double wxyz[4] = { 0.0 };
        double wdir[3] = { 0.0 };

        vr::InputBindingInfo_t inBindingInfo;
        uint32_t returnedBindingInfoCount = 0;
        /*vr::EVRInputError abinfo = */ vr::VRInput()->GetActionBindingInfo(
          actionHandle, &inBindingInfo, sizeof(inBindingInfo), 1, &returnedBindingInfoCount);

        std::string inputSource = inBindingInfo.rchInputSourceType;
        if (inputSource == "trackpad")
        {
          edp->SetInput(vtkEventDataDeviceInput::TrackPad);
        }
        else if (inputSource == "joystick")
        {
          edp->SetInput(vtkEventDataDeviceInput::Joystick);
        }
        else if (inputSource == "trigger")
        {
          edp->SetInput(vtkEventDataDeviceInput::Trigger);
        }
        else if (inputSource == "grip")
        {
          edp->SetInput(vtkEventDataDeviceInput::Grip);
        }
        else
        {
          edp->SetInput(vtkEventDataDeviceInput::Unknown);
        }

        vr::InputOriginInfo_t originInfo;
        if (vr::VRInputError_None ==
          vr::VRInput()->GetOriginTrackedDeviceInfo(activeOrigin, &originInfo, sizeof(originInfo)))
        {
          if (originInfo.devicePath == this->Trackers[LeftHand].Source)
          {
            edp->SetDevice(vtkEventDataDevice::LeftController);
            this->ConvertPoseToWorldCoordinates(this->Trackers[0].LastPose, pos, wxyz, ppos, wdir);
          }
          if (originInfo.devicePath == this->Trackers[RightHand].Source)
          {
            edp->SetDevice(vtkEventDataDevice::RightController);
            this->ConvertPoseToWorldCoordinates(this->Trackers[1].LastPose, pos, wxyz, ppos, wdir);
          }
          // edp->SetInput(originInfo.);
        }
        edp->SetWorldPosition(pos);
        edp->SetWorldOrientation(wxyz);
        edp->SetWorldDirection(wdir);
        edp->SetType(it.second.EventId);

        if (it.second.UseFunction)
        {
          it.second.Function(edp);
        }
        else
        {
          this->InvokeEvent(it.second.EventId, edp);
        }
        edp->Delete();
      }
    }

    if (this->RecognizeGestures)
    {
      this->RecognizeComplexGesture(nullptr);
    }
    this->InvokeEvent(vtkCommand::RenderEvent);
    auto ostate = renWin->GetState();
    renWin->MakeCurrent();
    ostate->Reset();
    ostate->Push();
    renWin->Render();
    ostate->Pop();
  }
}

void vtkOpenVRRenderWindowInteractor::HandleGripEvents(vtkEventData* ed)
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
void vtkOpenVRRenderWindowInteractor::RecognizeComplexGesture(vtkEventDataDevice3D*)
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
void vtkOpenVRRenderWindowInteractor::AddAction(
  std::string path, vtkCommand::EventIds eid, bool isAnalog)
{
  auto& am = this->ActionMap[path];
  am.EventId = eid;
  am.UseFunction = false;
  am.IsAnalog = isAnalog;
  if (this->Initialized)
  {
    vr::VRInput()->GetActionHandle(path.c_str(), &am.ActionHandle);

    // "path" : "/user/hand/right/input/trackpad"
  }
}

void vtkOpenVRRenderWindowInteractor::AddAction(
  std::string path, bool isAnalog, std::function<void(vtkEventData*)> func)
{
  auto& am = this->ActionMap[path];
  am.UseFunction = true;
  am.Function = func;
  am.IsAnalog = isAnalog;
  if (this->Initialized)
  {
    vr::VRInput()->GetActionHandle(path.c_str(), &am.ActionHandle);
  }
}

//------------------------------------------------------------------------------
void vtkOpenVRRenderWindowInteractor::Initialize()
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

  this->Initialized = 1;
  // get the info we need from the RenderingWindow

  size = ren->GetSize();
  ren->GetPosition();
  this->Enable();
  this->Size[0] = size[0];
  this->Size[1] = size[1];

  std::string fullpath = vtksys::SystemTools::CollapseFullPath(this->ActionManifestFileName);
  vr::VRInput()->SetActionManifestPath(fullpath.c_str());
  vr::VRInput()->GetActionSetHandle(this->ActionSetName.c_str(), &this->ActionsetVTK);

  // add in pose events
  vr::VRInput()->GetInputSourceHandle("/user/hand/left", &this->Trackers[LeftHand].Source);
  // vr::VRInput()->GetActionHandle("/actions/vtk/in/HandLeft",
  // &this->Trackers[LeftHand].ActionPose);
  vr::VRInput()->GetInputSourceHandle("/user/hand/right", &this->Trackers[RightHand].Source);
  // vr::VRInput()->GetActionHandle(
  //   "/actions/vtk/in/HandRight", &this->Trackers[RightHand].ActionPose);
  vr::VRInput()->GetInputSourceHandle("/user/head", &this->Trackers[Head].Source);
  // vr::VRInput()->GetActionHandle("/actions/vtk/in/head", &this->Trackers[Head].ActionPose);

  this->AddAction("/actions/vtk/in/LeftGripAction", false,
    [this](vtkEventData* ed) { this->HandleGripEvents(ed); });
  this->AddAction("/actions/vtk/in/RightGripAction", false,
    [this](vtkEventData* ed) { this->HandleGripEvents(ed); });

  // add extra event actions
  for (auto& it : this->ActionMap)
  {
    vr::VRInput()->GetActionHandle(it.first.c_str(), &it.second.ActionHandle);
  }
}

//------------------------------------------------------------------------------
int vtkOpenVRRenderWindowInteractor::InternalCreateTimer(
  int vtkNotUsed(timerId), int vtkNotUsed(timerType), unsigned long vtkNotUsed(duration))
{
  // todo
  return 0;
}

//------------------------------------------------------------------------------
int vtkOpenVRRenderWindowInteractor::InternalDestroyTimer(int vtkNotUsed(platformTimerId))
{
  // todo
  return 0;
}

//------------------------------------------------------------------------------
// Specify the default function to be called when an interactor needs to exit.
// This callback is overridden by an instance ExitMethod that is defined.
void vtkOpenVRRenderWindowInteractor::SetClassExitMethod(void (*f)(void*), void* arg)
{
  if (f != vtkOpenVRRenderWindowInteractor::ClassExitMethod ||
    arg != vtkOpenVRRenderWindowInteractor::ClassExitMethodArg)
  {
    // delete the current arg if there is a delete method
    if ((vtkOpenVRRenderWindowInteractor::ClassExitMethodArg) &&
      (vtkOpenVRRenderWindowInteractor::ClassExitMethodArgDelete))
    {
      (*vtkOpenVRRenderWindowInteractor::ClassExitMethodArgDelete)(
        vtkOpenVRRenderWindowInteractor::ClassExitMethodArg);
    }
    vtkOpenVRRenderWindowInteractor::ClassExitMethod = f;
    vtkOpenVRRenderWindowInteractor::ClassExitMethodArg = arg;

    // no call to this->Modified() since this is a class member function
  }
}

//------------------------------------------------------------------------------
// Set the arg delete method.  This is used to free user memory.
void vtkOpenVRRenderWindowInteractor::SetClassExitMethodArgDelete(void (*f)(void*))
{
  if (f != vtkOpenVRRenderWindowInteractor::ClassExitMethodArgDelete)
  {
    vtkOpenVRRenderWindowInteractor::ClassExitMethodArgDelete = f;

    // no call to this->Modified() since this is a class member function
  }
}

//------------------------------------------------------------------------------
void vtkOpenVRRenderWindowInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "StartedMessageLoop: " << this->StartedMessageLoop << endl;
}

//------------------------------------------------------------------------------
void vtkOpenVRRenderWindowInteractor::ExitCallback()
{
  if (this->HasObserver(vtkCommand::ExitEvent))
  {
    this->InvokeEvent(vtkCommand::ExitEvent, nullptr);
  }
  else if (vtkOpenVRRenderWindowInteractor::ClassExitMethod)
  {
    (*vtkOpenVRRenderWindowInteractor::ClassExitMethod)(
      vtkOpenVRRenderWindowInteractor::ClassExitMethodArg);
  }

  this->TerminateApp();
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
void vtkOpenVRRenderWindowInteractor::GetStartingPhysicalToWorldMatrix(
  vtkMatrix4x4* startingPhysicalToWorldMatrix)
{
  if (!startingPhysicalToWorldMatrix)
  {
    return;
  }
  startingPhysicalToWorldMatrix->DeepCopy(this->StartingPhysicalToWorldMatrix);
}
