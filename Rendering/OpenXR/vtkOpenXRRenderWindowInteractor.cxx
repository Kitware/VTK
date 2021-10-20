/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenXRRenderWindowInteractor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenXRRenderWindowInteractor.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLState.h"
#include "vtkOpenXR.h"
#include "vtkOpenXRInteractorStyle.h"
#include "vtkOpenXRManager.h"
#include "vtkOpenXRRenderWindow.h"
#include "vtkRendererCollection.h"

#include "vtkEventData.h"

#include "vtkOpenXRUtilities.h"
#include "vtkTransform.h"

#include "vtk_jsoncpp.h"
#include <vtksys/FStream.hxx>
#include <vtksys/SystemTools.hxx>

vtkStandardNewMacro(vtkOpenXRRenderWindowInteractor);

void (*vtkOpenXRRenderWindowInteractor::ClassExitMethod)(void*) = (void (*)(void*)) nullptr;
void* vtkOpenXRRenderWindowInteractor::ClassExitMethodArg = (void*)nullptr;
void (*vtkOpenXRRenderWindowInteractor::ClassExitMethodArgDelete)(
  void*) = (void (*)(void*)) nullptr;

//------------------------------------------------------------------------------
// Construct object so that light follows camera motion.
vtkOpenXRRenderWindowInteractor::vtkOpenXRRenderWindowInteractor()
{
  // This will create the actions name and store it in ActionMap
  vtkNew<vtkOpenXRInteractorStyle> style;
  this->SetInteractorStyle(style);

  for (int i = 0; i < vtkEventDataNumberOfDevices; i++)
  {
    this->DeviceInputDownCount[i] = 0;
  }
  this->ActionManifestFileName = "./vtk_openxr_actions.json";

  // OpenXR can't have slashes in the action set name (as well as action names)
  this->ActionSetName = "vtk-actions";
}

//------------------------------------------------------------------------------
vtkOpenXRRenderWindowInteractor::~vtkOpenXRRenderWindowInteractor()
{
  MapAction::iterator it;
  for (it = this->MapActionStruct_Name.begin(); it != this->MapActionStruct_Name.end(); ++it)
  {
    ActionData* actionData = it->second;
    delete actionData;
  }

  MapActionStruct_Name.clear();
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindowInteractor::StartEventLoop()
{
  this->StartedMessageLoop = 1;
  this->Done = false;

  vtkOpenXRRenderWindow* renWin = vtkOpenXRRenderWindow::SafeDownCast(this->RenderWindow);

  vtkRenderer* ren = static_cast<vtkRenderer*>(renWin->GetRenderers()->GetItemAsObject(0));

  while (!this->Done)
  {
    this->DoOneEvent(renWin, ren);
  }
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindowInteractor::ProcessEvents()
{
  vtkOpenXRRenderWindow* renWin = vtkOpenXRRenderWindow::SafeDownCast(this->RenderWindow);

  vtkRenderer* ren = static_cast<vtkRenderer*>(renWin->GetRenderers()->GetItemAsObject(0));
  this->DoOneEvent(renWin, ren);
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindowInteractor::DoOneEvent(
  vtkOpenXRRenderWindow* renWin, vtkRenderer* vtkNotUsed(ren))
{
  this->ProcessXrEvents();

  if (this->Done || !vtkOpenXRManager::GetInstance()->IsSessionRunning())
  {
    return;
  }

  this->PollXrActions(renWin);

  // Start a render
  this->InvokeEvent(vtkCommand::RenderEvent);
  auto ostate = renWin->GetState();
  renWin->MakeCurrent();
  ostate->Reset();
  ostate->Push();
  renWin->Render();
  ostate->Pop();
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindowInteractor::ProcessXrEvents()
{
  vtkOpenXRManager* xrManager = vtkOpenXRManager::GetInstance();

  XrEventDataBuffer eventData{};
  while (xrManager->PollEvent(eventData))
  {
    switch (eventData.type)
    {
      // We lost some data
      case XR_TYPE_EVENT_DATA_EVENTS_LOST:
      {
        const auto stateEvent = *reinterpret_cast<const XrEventDataEventsLost*>(&eventData);
        vtkDebugMacro(<< "OpenXR event [XR_TYPE_EVENT_DATA_EVENTS_LOST] : "
                      << stateEvent.lostEventCount << " events data lost!");
        // do we care if the runtime loses events?
        break;
      }

      //
      case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
      {
        vtkWarningMacro(
          << "OpenXR event [XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING] : exit render loop.");
        this->Done = true;
        return;
      }

      //
      case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
      {
        const auto stateEvent =
          *reinterpret_cast<const XrEventDataSessionStateChanged*>(&eventData);
        if (stateEvent.session != xrManager->GetSession())
        {
          vtkErrorMacro(<< "OpenXR event [XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED] : session is "
                           "different than this->Session. Aborting.");
          this->Done = true;
          return;
        }
        switch (stateEvent.state)
        {
          case XR_SESSION_STATE_READY:
          {
            vtkDebugMacro(<< "OpenXR event [XR_SESSION_STATE_READY] : Begin session");
            xrManager->BeginSession();
            break;
          }
          case XR_SESSION_STATE_STOPPING:
            vtkDebugMacro(<< "OpenXR event [XR_SESSION_STATE_STOPPING]");
          case XR_SESSION_STATE_LOSS_PENDING:
            // Session was lost, so start over and poll for new systemId.
            vtkDebugMacro(<< "OpenXR event [XR_SESSION_STATE_LOSS_PENDING]");
          case XR_SESSION_STATE_EXITING:
          {
            // Do not attempt to restart, because user closed this session.
            vtkDebugMacro(<< "OpenXR event [XR_SESSION_STATE_EXITING]");
            vtkDebugMacro(<< "Exit render loop.");
            this->Done = true;
            break;
          }
        }
        break;
      }
      case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING:
      {
        vtkDebugMacro(<< "OpenXR event [XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING]");
        const auto stateEvent =
          *reinterpret_cast<const XrEventDataReferenceSpaceChangePending*>(&eventData);
        (void)stateEvent;
        break;
      }

      case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
      {
        vtkDebugMacro(<< "OpenXR event [XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED]");
        const auto stateEvent =
          *reinterpret_cast<const XrEventDataInteractionProfileChanged*>(&eventData);
        (void)stateEvent;

        XrInteractionProfileState state{ XR_TYPE_INTERACTION_PROFILE_STATE };

        for (uint32_t hand :
          { vtkOpenXRManager::ControllerIndex::Left, vtkOpenXRManager::ControllerIndex::Right })
        {
          if (!xrManager->XrCheckWarn(xrGetCurrentInteractionProfile(xrManager->GetSession(),
                                        xrManager->GetSubactionPaths()[hand], &state),
                "Failed to get interaction profile for hand " + hand))
          {
            continue;
          }

          XrPath interactionProfile = state.interactionProfile;

          uint32_t strLength;
          char profileString[XR_MAX_PATH_LENGTH];
          if (!xrManager->XrCheckWarn(
                xrPathToString(xrManager->GetXrRuntimeInstance(), interactionProfile,
                  XR_MAX_PATH_LENGTH, &strLength, profileString),
                "Failed to get interaction profile path string for hand " + hand))
          {
            continue;
          }

          vtkDebugMacro(<< "Interaction profile changed for " << hand << ": " << profileString);
        }
        break;
      }
      default:
      {
        vtkWarningMacro(<< "Unhandled event type "
                        << vtkOpenXRUtilities::GetStructureTypeAsString(eventData.type));
        break;
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindowInteractor::PollXrActions(vtkOpenXRRenderWindow* renWin)
{
  // Update the action states by syncing using the active action set
  vtkOpenXRManager::GetInstance()->SyncActions();

  // Iterate over all actions and update their data
  MapAction::iterator it;
  for (it = this->MapActionStruct_Name.begin(); it != this->MapActionStruct_Name.end(); ++it)
  {
    ActionData* actionData = it->second;

    // Update the state of the actions for left and right hands separately.
    for (uint32_t hand :
      { vtkOpenXRManager::ControllerIndex::Left, vtkOpenXRManager::ControllerIndex::Right })
    {
      vtkOpenXRManager::GetInstance()->UpdateActionData(actionData->ActionStruct, hand);
    }
  }

  // Construct the event data that contains position and orientation of each hand
  double pos[3] = { 0.0 };
  double ppos[3] = { 0.0 };
  double wxyz[4] = { 0.0 };
  double wdir[3] = { 0.0 };
  std::array<vtkSmartPointer<vtkEventDataDevice3D>, 2> eventDatas;
  for (const uint32_t hand :
    { vtkOpenXRManager::ControllerIndex::Left, vtkOpenXRManager::ControllerIndex::Right })
  {
    XrPosef& handPose = this->GetHandPose(hand);
    renWin->ConvertOpenXRPoseToWorldCoordinates(handPose, pos, wxyz, ppos, wdir);
    auto edHand = vtkEventDataDevice3D::New();
    edHand->SetDevice(hand == vtkOpenXRManager::ControllerIndex::Right
        ? vtkEventDataDevice::RightController
        : vtkEventDataDevice::LeftController);
    edHand->SetWorldPosition(pos);
    edHand->SetWorldOrientation(wxyz);
    edHand->SetWorldDirection(wdir);
    eventDatas[hand].TakeReference(edHand);

    // We should remove this and use event data directly
    int pointerIndex = static_cast<int>(edHand->GetDevice());
    this->SetPhysicalEventPosition(ppos[0], ppos[1], ppos[2], pointerIndex);
    this->SetWorldEventPosition(pos[0], pos[1], pos[2], pointerIndex);
    this->SetWorldEventOrientation(wxyz[0], wxyz[1], wxyz[2], wxyz[3], pointerIndex);
  }

  // All actions are now updated, handle them now
  for (it = this->MapActionStruct_Name.begin(); it != this->MapActionStruct_Name.end(); ++it)
  {
    ActionData* actionData = it->second;

    for (uint32_t hand :
      { vtkOpenXRManager::ControllerIndex::Left, vtkOpenXRManager::ControllerIndex::Right })
    {
      vtkEventDataDevice3D* eventData = eventDatas[hand];

      eventData->SetInput(actionData->DeviceInput);
      eventData->SetType(actionData->EventId);

      this->HandleAction(*actionData, hand, eventData);
    }
  }
}

//------------------------------------------------------------------------------
XrPosef& vtkOpenXRRenderWindowInteractor::GetHandPose(const uint32_t hand)
{
  ActionData* adHandPose = this->MapActionStruct_Name["handpose"];

  return adHandPose->ActionStruct.PoseLocations[hand].pose;
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindowInteractor::HandleAction(
  const ActionData& actionData, const int hand, vtkEventDataDevice3D* ed)
{
  const Action_t& actionT = actionData.ActionStruct;
  switch (actionT.ActionType)
  {
    /*case XR_ACTION_TYPE_FLOAT_INPUT:
      actionT.States[hand]._float.type = XR_TYPE_ACTION_STATE_FLOAT;
      actionT.States[hand]._float.next = nullptr;
      if (!this->XrCheckError(xrGetActionStateFloat(Session, &info, &action_t.States[hand]._float),
        "Failed to get float value"))
      {
        return false;
      }
      break;*/
    case XR_ACTION_TYPE_BOOLEAN_INPUT:
      this->HandleBooleanAction(actionData, hand, ed);
      break;
    case XR_ACTION_TYPE_VECTOR2F_INPUT:
      this->HandleVector2fAction(actionData, hand, ed);
      break;
    case XR_ACTION_TYPE_POSE_INPUT:
      this->HandlePoseAction(actionData, hand, ed);
      break;
    default:
      break;
  }
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindowInteractor::ApplyAction(
  const ActionData& actionData, vtkEventDataDevice3D* ed)
{
  this->SetPointerIndex(static_cast<int>(ed->GetDevice()));

  if (actionData.UseFunction)
  {
    actionData.Function(ed);
  }
  else
  {
    this->InvokeEvent(actionData.EventId, ed);
  }
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindowInteractor::HandleBooleanAction(
  const ActionData& actionData, const int hand, vtkEventDataDevice3D* ed)
{
  XrActionStateBoolean value = actionData.ActionStruct.States[hand]._boolean;

  // Set the active state of the model
  vtkOpenXRRenderWindow::SafeDownCast(this->RenderWindow)
    ->SetModelActiveState(hand, value.isActive);

  // Do nothing if the controller is inactive
  if (!value.isActive)
  {
    return;
  }

  if (value.changedSinceLastSync)
  {
    vtkDebugMacro(<< "Boolean action \"" << actionData.Name << "\" is triggered with value "
                  << value.currentState << " for hand " << hand);

    if (value.currentState == 1)
    {
      ed->SetAction(vtkEventDataAction::Press);
    }
    else
    {
      ed->SetAction(vtkEventDataAction::Release);
    }

    this->ApplyAction(actionData, ed);
  }
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindowInteractor::HandlePoseAction(
  const ActionData& actionData, const int hand, vtkEventDataDevice3D* ed)
{
  XrActionStatePose pose = actionData.ActionStruct.States[hand]._pose;

  // Set the active state of the model
  vtkOpenXRRenderWindow::SafeDownCast(this->RenderWindow)->SetModelActiveState(hand, pose.isActive);
  // Do nothing if the controller is inactive
  if (!pose.isActive)
  {
    return;
  }

  this->ApplyAction(actionData, ed);
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindowInteractor::HandleVector2fAction(
  const ActionData& actionData, const int hand, vtkEventDataDevice3D* ed)
{
  XrActionStateVector2f vec2f = actionData.ActionStruct.States[hand]._vec2f;

  // Set the active state of the model
  vtkOpenXRRenderWindow::SafeDownCast(this->RenderWindow)
    ->SetModelActiveState(hand, vec2f.isActive);
  // Do nothing if the controller is inactive
  if (!vec2f.isActive)
  {
    return;
  }

  if (vec2f.changedSinceLastSync)
  {
    vtkDebugMacro(<< "Vector2f : " << actionData.Name << ", x = " << vec2f.currentState.x
                  << " / y = " << vec2f.currentState.y);

    if (vec2f.currentState.y == 0 || vec2f.currentState.x == 0)
    {
      return;
    }

    ed->SetTrackPadPosition(vec2f.currentState.x, vec2f.currentState.y);

    this->ApplyAction(actionData, ed);
  }
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindowInteractor::HandleGripEvents(vtkEventData* ed)
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

    vtkOpenXRRenderWindow* renWin = vtkOpenXRRenderWindow::SafeDownCast(this->RenderWindow);
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
void vtkOpenXRRenderWindowInteractor::RecognizeComplexGesture(vtkEventDataDevice3D*)
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
      // HMD to world axes
      vtkOpenXRRenderWindow* win = vtkOpenXRRenderWindow::SafeDownCast(this->RenderWindow);
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
void vtkOpenXRRenderWindowInteractor::AddAction(
  const std::string& path, const vtkCommand::EventIds& eid)
{
  if (this->MapActionStruct_Name.count(path) == 0)
  {
    ActionData* am = new ActionData();
    am->EventId = eid;
    am->UseFunction = false;
    this->MapActionStruct_Name[path] = am;
  }
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindowInteractor::AddAction(
  const std::string& path, const std::function<void(vtkEventData*)>& func)
{
  if (this->MapActionStruct_Name.count(path) == 0)
  {
    ActionData* am = new ActionData();
    am->UseFunction = true;
    am->Function = func;
    this->MapActionStruct_Name[path] = am;
  }
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindowInteractor::Initialize()
{
  // make sure we have a RenderWindow and camera
  if (!this->RenderWindow)
  {
    vtkErrorMacro(<< "No render window defined!");
    return;
  }
  if (this->Initialized)
  {
    return;
  }

  vtkOpenXRRenderWindow* renWin = vtkOpenXRRenderWindow::SafeDownCast(this->RenderWindow);

  // Make sure the render window is initialized
  renWin->Initialize();

  if (!renWin->GetInitialized())
  {
    return;
  }

  this->Initialized = true;

  // Get the info we need from the RenderingWindow
  int* size;
  size = renWin->GetSize();
  renWin->GetPosition();
  this->Enable();
  this->Size[0] = size[0];
  this->Size[1] = size[1];

  // Grip actions are handled by the interactor directly (why?)
  this->AddAction("leftgripaction", [this](vtkEventData* ed) { this->HandleGripEvents(ed); });

  this->AddAction("rightgripaction", [this](vtkEventData* ed) { this->HandleGripEvents(ed); });

  // Create an entry for pose actions that are used to retrieve
  // Orientation and locations of trackers
  this->AddAction("handpose", vtkCommand::Move3DEvent);
  // this->AddAction("handposehandgrip", vtkCommand::Move3DEvent);

  std::string fullpath =
    vtksys::SystemTools::CollapseFullPath(this->ActionManifestFileName.c_str());

  if (!this->LoadActions(fullpath))
  {
    vtkErrorMacro(<< "Failed to load actions.");
    this->Initialized = false;
    return;
  }

  // All action sets have been created, so
  // We can now attach the action sets to the session
  if (!vtkOpenXRManager::GetInstance()->AttachSessionActionSets())
  {
    this->Initialized = false;
    return;
  }
}

//------------------------------------------------------------------------------
bool vtkOpenXRRenderWindowInteractor::LoadActions(const std::string& actionFilename)
{
  vtkDebugMacro(<< "LoadActions from file : " << actionFilename);
  // As OpenXR does not yet have a way to pass a file to create actions
  // We need to create them programmatically, so we parse it with JsonCpp
  Json::Value root;

  // Open the file
  vtksys::ifstream file;
  file.open(actionFilename.c_str());
  if (!file.is_open())
  {
    vtkErrorMacro(<< "Unable to open openXR action file : " << actionFilename);
    return false;
  }

  Json::CharReaderBuilder builder;

  std::string formattedErrors;
  // parse the entire data into the Json::Value root
  if (!Json::parseFromStream(builder, file, &root, &formattedErrors))
  {
    // Report failures and their locations in the document
    vtkErrorMacro(<< "Failed to parse action file with errors :" << endl << formattedErrors);
    return false;
  }

  // Create an action set
  std::string localizedActionSetName = "VTK actions";
  vtkOpenXRManager::GetInstance()->CreateActionSet(this->ActionSetName, localizedActionSetName);

  // We must select an action set to create actions
  // For instance only one action set so select it
  // Improvement: select each action set and create all actions
  // that belong to it
  vtkOpenXRManager::GetInstance()->SelectActiveActionSet(0);

  // Create actions
  Json::Value actions = root["actions"];
  if (actions.isNull())
  {
    vtkErrorMacro(<< "Parse openxr_actions: Missing actions node");
    return false;
  }
  Json::Value localization = root["localization"];
  if (localization.isNull())
  {
    vtkErrorMacro(<< "Parse openxr_actions: Missing localization node");
    return false;
  }
  localization = localization[0];

  for (Json::Value::ArrayIndex i = 0; i < actions.size(); ++i)
  {
    // Create one action per json value
    Json::Value action = actions[i];

    std::string name = action["name"].asString();
    std::string localizedName = localization[name].asString();
    std::string type = action["type"].asString();

    // Check if the action is used by the interactor style
    // or ourself. If that's the case, create it
    // Else do nothing
    if (this->MapActionStruct_Name.count(name) == 0)
    {
      vtkWarningMacro(
        << "An action with name " << name
        << " is available but the interactor style or the interactor does not use it.");
      continue;
    }

    vtkDebugMacro(<< "Creating an action of type=" << type << ", with name=" << name
                  << ", localizedName=" << localizedName);

    XrActionType xrActionType = this->GetActionTypeFromString(type);
    if (!xrActionType)
    {
      return false;
    }

    // Create the action using the selected action set
    Action_t actionStruct;
    actionStruct.ActionType = xrActionType;
    if (!vtkOpenXRManager::GetInstance()->CreateOneAction(actionStruct, name, localizedName))
    {
      return false;
    }

    // Store it to retrieve actions by their name
    this->MapActionStruct_Name[name]->ActionStruct = actionStruct;
    this->MapActionStruct_Name[name]->Name = name;
  }

  // Select default binding depending on the selected controllerType (currently,
  // only htc vive is supported).
  const std::string controllerType = "vive_controller";

  Json::Value defaultBindings = root["default_bindings"];
  if (defaultBindings.isNull())
  {
    vtkErrorMacro(<< "Parse openxr_actions: Missing default_bindings node");
    return false;
  }
  for (Json::Value::ArrayIndex i = 0; i < defaultBindings.size(); ++i)
  {
    Json::Value binding = defaultBindings[i];

    // Load the json file corresponding to our selected controller type
    if (binding["controller_type"] == controllerType)
    {
      std::string bindingUrl = binding["binding_url"].asString();
      std::string bindingFilename = vtksys::SystemTools::CollapseFullPath(bindingUrl.c_str());
      if (!this->LoadDefaultBinding(bindingFilename))
      {
        return false;
      }
    }
  }

  return true;
}

//------------------------------------------------------------------------------
XrActionType vtkOpenXRRenderWindowInteractor::GetActionTypeFromString(const std::string& type)
{
  if (type == "boolean")
  {
    return XR_ACTION_TYPE_BOOLEAN_INPUT;
  }
  else if (type == "float")
  {
    return XR_ACTION_TYPE_FLOAT_INPUT;
  }
  else if (type == "vector2")
  {
    return XR_ACTION_TYPE_VECTOR2F_INPUT;
  }
  else if (type == "pose")
  {
    return XR_ACTION_TYPE_POSE_INPUT;
  }
  else if (type == "vibration")
  {
    return XR_ACTION_TYPE_VIBRATION_OUTPUT;
  }
  else
  {
    vtkErrorMacro(<< "Unrecognized action type: " << type);
    return (XrActionType)0;
  }
}

//------------------------------------------------------------------------------
bool vtkOpenXRRenderWindowInteractor::LoadDefaultBinding(const std::string& bindingFilename)
{
  Json::Value root;
  // Open the file
  vtksys::ifstream file;
  file.open(bindingFilename.c_str());
  if (!file.is_open())
  {
    vtkErrorMacro(<< "Unable to open openXR binding file : " << bindingFilename);
    return false;
  }

  Json::CharReaderBuilder builder;
  std::string formattedErrors;
  // parse the entire data into the Json::Value root
  if (!Json::parseFromStream(builder, file, &root, &formattedErrors))
  {
    // Report failures and their locations in the document
    vtkErrorMacro(<< "Failed to parse binding file with errors :" << endl << formattedErrors);
    return false;
  }

  // Get the interaction profile name
  std::string interactionProfile = root["interaction_profile"].asString();

  Json::Value bindings = root["bindings"];

  Json::Value actionSet = bindings[this->ActionSetName];
  if (actionSet.isNull())
  {
    vtkErrorMacro(<< "Selected action set : " << this->ActionSetName
                  << " is not in binding file : " << bindingFilename);
    return false;
  }

  // We need to fill this vector to suggest interaction profile bindings
  std::vector<XrActionSuggestedBinding> actionSuggestedBindings;

  // Get the XrPath from path string,
  // Get the XrAction from the string jsonValue["output"]
  // Store in the actionData the device input guessed from the path
  // And fill actionSuggestedBindings
  auto fillActionSuggestedBindings = [&](const std::string& path, const Json::Value& jsonValue) {
    // Get the action
    std::string action = jsonValue["output"].asString();

    // Only suggest a binding for an action that is used by the interactor style
    // or ourself
    if (this->MapActionStruct_Name.count(action) == 0)
    {
      return;
    }
    vtkDebugMacro(<< "Add action : " << action << ", with path : " << path);
    ActionData* actionData = this->GetActionDataFromName(action);

    if (actionData != nullptr)
    {
      // Use the path to guess the device input
      if (path.find("trigger") != std::string::npos)
      {
        actionData->DeviceInput = vtkEventDataDeviceInput::Trigger;
      }
      else if (path.find("trackpad") != std::string::npos)
      {
        actionData->DeviceInput = vtkEventDataDeviceInput::TrackPad;
      }
      else if (path.find("grip") != std::string::npos)
      {
        actionData->DeviceInput = vtkEventDataDeviceInput::Grip;
      }
      else if (path.find("thumbstick") != std::string::npos)
      {
        actionData->DeviceInput = vtkEventDataDeviceInput::Joystick;
      }

      Action_t& actionT = actionData->ActionStruct;

      if (actionT.Action == XR_NULL_HANDLE)
      {
        vtkErrorMacro(<< "Action " << action << ", with path : " << path << " has a null handle !");
        return;
      }

      XrPath xrPath = vtkOpenXRManager::GetInstance()->GetXrPath(path);
      actionSuggestedBindings.push_back({ actionT.Action, xrPath });
    }
  };

  // First, look after all sources inputs, ie. boolean/float/vector2f actions
  Json::Value sources = actionSet["sources"];
  for (Json::Value::ArrayIndex i = 0; i < sources.size(); ++i)
  {
    Json::Value source = sources[i];

    // The path for this action
    std::string path = source["path"].asString();

    // Iterate over all inputs and append to the path the selected input
    // For example, if the input is "click", then append click to the path
    // if the input is "position", add nothing as openxr binds the position as a vector2f
    // (for example if we want to retrieve the position of the trackpad as a vector2f directly)
    Json::Value inputs = source["inputs"];
    for (auto const& inputStr : inputs.getMemberNames())
    {
      Json::Value action = inputs[inputStr];
      if (inputStr == "position")
      {
        fillActionSuggestedBindings(path, action);
      }
      else
      {
        fillActionSuggestedBindings(path + "/" + inputStr, action);
      }
    }
  }

  // Submit all suggested bindings
  return vtkOpenXRManager::GetInstance()->SuggestActions(
    interactionProfile, actionSuggestedBindings);
}

//------------------------------------------------------------------------------
vtkOpenXRRenderWindowInteractor::ActionData* vtkOpenXRRenderWindowInteractor::GetActionDataFromName(
  const std::string& actionName)
{
  // Check if action data exists
  if (this->MapActionStruct_Name.count(actionName) == 0)
  {
    vtkWarningMacro(<< "vtkOpenXRRenderWindowInteractor: Attempt to get an action data with name "
                    << actionName << " that does not exist in the map.");
    return nullptr;
  }
  return this->MapActionStruct_Name[actionName];
}

//------------------------------------------------------------------------------
int vtkOpenXRRenderWindowInteractor::InternalCreateTimer(
  int vtkNotUsed(timerId), int vtkNotUsed(timerType), unsigned long vtkNotUsed(duration))
{
  return 0;
}

//------------------------------------------------------------------------------
int vtkOpenXRRenderWindowInteractor::InternalDestroyTimer(int vtkNotUsed(platformTimerId))
{
  return 0;
}

//------------------------------------------------------------------------------
// Specify the default function to be called when an interactor needs to exit.
// This callback is overridden by an instance ExitMethod that is defined.
void vtkOpenXRRenderWindowInteractor::SetClassExitMethod(void (*f)(void*), void* arg)
{
  if (f != vtkOpenXRRenderWindowInteractor::ClassExitMethod ||
    arg != vtkOpenXRRenderWindowInteractor::ClassExitMethodArg)
  {
    // delete the current arg if there is a delete method
    if ((vtkOpenXRRenderWindowInteractor::ClassExitMethodArg) &&
      (vtkOpenXRRenderWindowInteractor::ClassExitMethodArgDelete))
    {
      (*vtkOpenXRRenderWindowInteractor::ClassExitMethodArgDelete)(
        vtkOpenXRRenderWindowInteractor::ClassExitMethodArg);
    }
    vtkOpenXRRenderWindowInteractor::ClassExitMethod = f;
    vtkOpenXRRenderWindowInteractor::ClassExitMethodArg = arg;

    // no call to this->Modified() since this is a class member function
  }
}

//------------------------------------------------------------------------------
// Set the arg delete method.  This is used to free user memory.
void vtkOpenXRRenderWindowInteractor::SetClassExitMethodArgDelete(void (*f)(void*))
{
  if (f != vtkOpenXRRenderWindowInteractor::ClassExitMethodArgDelete)
  {
    vtkOpenXRRenderWindowInteractor::ClassExitMethodArgDelete = f;

    // no call to this->Modified() since this is a class member function
  }
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindowInteractor::ExitCallback()
{
  if (this->HasObserver(vtkCommand::ExitEvent))
  {
    this->InvokeEvent(vtkCommand::ExitEvent, nullptr);
  }
  else if (this->ClassExitMethod)
  {
    (*this->ClassExitMethod)(this->ClassExitMethodArg);
  }

  this->TerminateApp();
}

//------------------------------------------------------------------------------
vtkEventDataDevice vtkOpenXRRenderWindowInteractor::GetPointerDevice()
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
void vtkOpenXRRenderWindowInteractor::GetStartingPhysicalToWorldMatrix(
  vtkMatrix4x4* startingPhysicalToWorldMatrix)
{
  if (!startingPhysicalToWorldMatrix)
  {
    return;
  }
  startingPhysicalToWorldMatrix->DeepCopy(this->StartingPhysicalToWorldMatrix);
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindowInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "vtkOpenXRRenderWindowInteractor"
     << "\n";
  this->Superclass::PrintSelf(os, indent);
}
