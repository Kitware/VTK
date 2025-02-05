// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOpenVRRenderWindowInteractor.h"

#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLState.h"
#include "vtkOpenVRInteractorStyle.h"
#include "vtkOpenVROverlay.h"
#include "vtkOpenVRRenderWindow.h"
#include "vtkTextureObject.h"

#include <vtksys/SystemTools.hxx>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOpenVRRenderWindowInteractor);

//------------------------------------------------------------------------------
vtkOpenVRRenderWindowInteractor::vtkOpenVRRenderWindowInteractor()
{
  vtkNew<vtkOpenVRInteractorStyle> style;
  this->SetInteractorStyle(style);
  this->ActionManifestFileName = "vtk_openvr_actions.json";
  this->ActionSetName = "/actions/vtk";
}

//------------------------------------------------------------------------------
void vtkOpenVRRenderWindowInteractor::Initialize()
{
  // Start with superclass initialization
  this->Superclass::Initialize();

  std::string fullpath = vtksys::SystemTools::CollapseFullPath(
    this->ActionManifestDirectory + this->ActionManifestFileName);
  vr::VRInput()->SetActionManifestPath(fullpath.c_str());
  vr::VRInput()->GetActionSetHandle(this->ActionSetName.c_str(), &this->ActionsetVTK);

  // add in pose events
  vr::VRInput()->GetInputSourceHandle(
    "/user/hand/left", &this->Trackers[vtkOpenVRRenderWindowInteractor::LEFT_HAND].Source);
  vr::VRInput()->GetInputSourceHandle(
    "/user/hand/right", &this->Trackers[vtkOpenVRRenderWindowInteractor::RIGHT_HAND].Source);
  vr::VRInput()->GetInputSourceHandle(
    "/user/head", &this->Trackers[vtkOpenVRRenderWindowInteractor::HEAD].Source);

  this->AddAction("/actions/vtk/in/ComplexGestureAction", false,
    [this](vtkEventData* ed) { this->HandleComplexGestureEvents(ed); });

  // This additional action is needed to support the recognition of complex gestures
  // in cases where the two complex gesture buttons are pressed consecutively. In such
  // scenarios, when the user is already pressing one button, the `actionData.bChanged`
  // condition (used in `DoOneEvent`) is already false and no additional event is synthesized.
  // By using another action named `ComplexGestureAction_Event2`, we ensure a second event
  // is synthesized. This leads to a second invocation of `HandleComplexGestureEvents`,
  // enabling the function `RecognizeComplexGesture()` to effectively recognize the associated
  // gesture.
  this->AddAction("/actions/vtk/in/ComplexGestureAction_Event2", false,
    [this](vtkEventData* ed) { this->HandleComplexGestureEvents(ed); });

  // add extra event actions
  for (auto& it : this->ActionMap)
  {
    vr::VRInput()->GetActionHandle(it.first.c_str(), &it.second.ActionHandle);
  }
}

//------------------------------------------------------------------------------
void vtkOpenVRRenderWindowInteractor::DoOneEvent(vtkVRRenderWindow* renWin, vtkRenderer* ren)
{
  vtkOpenVRRenderWindow* oRenWin = vtkOpenVRRenderWindow::SafeDownCast(renWin);

  if (!oRenWin || !ren)
  {
    return;
  }
  vr::IVRSystem* pHMD = oRenWin->GetHMD();

  if (!pHMD)
  {
    // try rendering to create the HMD connection
    oRenWin->Render();
    return;
  }

  vr::VREvent_t event;
  vtkOpenVROverlay* ovl = oRenWin->GetDashboardOverlay();
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
          oRenWin->RenderOverlay();
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
      if (event.eventType == vr::VREvent_Quit)
      {
        this->Done = true;
        return;
      }
    }
  }
  else
  {
    result = pHMD->PollNextEvent(&event, sizeof(vr::VREvent_t));

    // process all pending events
    while (result)
    {
      if (event.eventType == vr::VREvent_Quit)
      {
        this->Done = true;
        return;
      }
      result = pHMD->PollNextEvent(&event, sizeof(vr::VREvent_t));
    }

    // Process SteamVR action state
    // UpdateActionState is called each frame to update the state of the actions themselves. The
    // application controls which action sets are active with the provided array of
    // VRActiveActionSet_t structs.
    vr::VRActiveActionSet_t actionSet = { 0 };
    actionSet.ulActionSet = this->ActionsetVTK;
    vr::VRInput()->UpdateActionState(&actionSet, sizeof(actionSet), 1);

    for (int tracker = 0; tracker < vtkOpenVRRenderWindowInteractor::NUMBER_OF_TRACKERS; tracker++)
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

        vtkNew<vtkMatrix4x4> lastPose;
        oRenWin->SetMatrixFromOpenVRPose(lastPose, this->Trackers[tracker].LastPose);
        this->ConvertPoseToWorldCoordinates(lastPose, pos, wxyz, ppos, wdir);
        vtkNew<vtkEventDataDevice3D> ed;
        ed->SetWorldPosition(pos);
        ed->SetWorldOrientation(wxyz);
        ed->SetWorldDirection(wdir);

        switch (tracker)
        {
          case vtkOpenVRRenderWindowInteractor::LEFT_HAND:
            ed->SetDevice(vtkEventDataDevice::LeftController);
            break;
          case vtkOpenVRRenderWindowInteractor::RIGHT_HAND:
            ed->SetDevice(vtkEventDataDevice::RightController);
            break;
          case vtkOpenVRRenderWindowInteractor::HEAD:
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
        vr::VRInput()->GetActionBindingInfo(
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
          if (originInfo.devicePath ==
            this->Trackers[vtkOpenVRRenderWindowInteractor::LEFT_HAND].Source)
          {
            edp->SetDevice(vtkEventDataDevice::LeftController);

            vtkNew<vtkMatrix4x4> lastPose;
            oRenWin->SetMatrixFromOpenVRPose(lastPose, this->Trackers[0].LastPose);
            this->ConvertPoseToWorldCoordinates(lastPose, pos, wxyz, ppos, wdir);
          }
          if (originInfo.devicePath ==
            this->Trackers[vtkOpenVRRenderWindowInteractor::RIGHT_HAND].Source)
          {
            edp->SetDevice(vtkEventDataDevice::RightController);

            vtkNew<vtkMatrix4x4> lastPose;
            oRenWin->SetMatrixFromOpenVRPose(lastPose, this->Trackers[1].LastPose);
            this->ConvertPoseToWorldCoordinates(lastPose, pos, wxyz, ppos, wdir);
          }
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
    auto ostate = oRenWin->GetState();
    oRenWin->MakeCurrent();
    ostate->Reset();
    ostate->Push();
    oRenWin->Render();
    ostate->Pop();
  }
}

//------------------------------------------------------------------------------
void vtkOpenVRRenderWindowInteractor::AddAction(
  const std::string& path, const vtkCommand::EventIds& eid, bool isAnalog)
{
  // Path example: "/user/hand/right/input/trackpad"
  auto& am = this->ActionMap[path];
  am.EventId = eid;
  am.UseFunction = false;
  am.IsAnalog = isAnalog;
  if (this->Initialized)
  {
    vr::VRInput()->GetActionHandle(path.c_str(), &am.ActionHandle);
  }
}

//------------------------------------------------------------------------------
void vtkOpenVRRenderWindowInteractor::AddAction(
  const std::string& path, bool isAnalog, const std::function<void(vtkEventData*)>& func)
{
  // Path example: "/user/hand/right/input/trackpad"
  auto& am = this->ActionMap[path];
  am.UseFunction = true;
  am.Function = func;
  am.IsAnalog = isAnalog;
  if (this->Initialized)
  {
    vr::VRInput()->GetActionHandle(path.c_str(), &am.ActionHandle);
  }
}

VTK_ABI_NAMESPACE_END
