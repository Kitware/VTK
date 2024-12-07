// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOpenXRRenderWindowInteractor.h"

#include "vtkObjectFactory.h"
#include "vtkOpenGLState.h"
#include "vtkOpenXRInteractorStyle.h"
#include "vtkOpenXRRenderWindow.h"
#include "vtkOpenXRUtilities.h"
#include "vtkResourceFileLocator.h"
#include "vtkVersion.h"

#include "vtk_jsoncpp.h"
#include <vtksys/FStream.hxx>
#include <vtksys/SystemTools.hxx>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOpenXRRenderWindowInteractor);

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
  this->ActionManifestFileName = "vtk_openxr_actions.json";

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
void vtkOpenXRRenderWindowInteractor::DoOneEvent(
  vtkVRRenderWindow* renWin, vtkRenderer* vtkNotUsed(ren))
{
  this->ProcessXrEvents();

  if (this->Done || !vtkOpenXRManager::GetInstance().IsSessionRunning())
  {
    return;
  }

  this->PollXrActions();

  if (this->RecognizeGestures)
  {
    this->RecognizeComplexGesture(nullptr);
  }

  // Start a render
  this->InvokeEvent(vtkCommand::RenderEvent);
  renWin->Render();
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindowInteractor::ProcessXrEvents()
{
  vtkOpenXRManager& xrManager = vtkOpenXRManager::GetInstance();

  XrEventDataBuffer eventData{};
  while (xrManager.PollEvent(eventData))
  {
    switch (eventData.type)
    {
      // We lost some data
      case XR_TYPE_EVENT_DATA_EVENTS_LOST:
      {
        vtkDebugMacro(<< "OpenXR event [XR_TYPE_EVENT_DATA_EVENTS_LOST] : some events data lost!");
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
        if (stateEvent.session != xrManager.GetSession())
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
            xrManager.BeginSession();
            break;
          }
          case XR_SESSION_STATE_STOPPING:
            vtkDebugMacro(<< "OpenXR event [XR_SESSION_STATE_STOPPING]");
            VTK_FALLTHROUGH;
          case XR_SESSION_STATE_LOSS_PENDING:
            // Session was lost, so start over and poll for new systemId.
            vtkDebugMacro(<< "OpenXR event [XR_SESSION_STATE_LOSS_PENDING]");
            VTK_FALLTHROUGH;
          case XR_SESSION_STATE_EXITING:
          {
            // Do not attempt to restart, because user closed this session.
            vtkDebugMacro(<< "OpenXR event [XR_SESSION_STATE_EXITING]");
            vtkDebugMacro(<< "Exit render loop.");
            this->Done = true;
            break;
          }
          default:
            break;
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
          if (!xrManager.XrCheckOutput(vtkOpenXRManager::WarningOutput,
                xrGetCurrentInteractionProfile(
                  xrManager.GetSession(), xrManager.GetSubactionPaths()[hand], &state),
                "Failed to get interaction profile for hand " + hand))
          {
            continue;
          }

          XrPath interactionProfile = state.interactionProfile;

          if (!interactionProfile)
          {
            vtkDebugMacro(<< "No interaction profile set");
            continue;
          }

          uint32_t strLength;
          char profileString[XR_MAX_PATH_LENGTH];
          if (!xrManager.XrCheckOutput(vtkOpenXRManager::WarningOutput,
                xrPathToString(xrManager.GetXrRuntimeInstance(), interactionProfile,
                  XR_MAX_PATH_LENGTH, &strLength, profileString),
                "Failed to get interaction profile path string for hand " + hand))
          {
            continue;
          }

          vtkDebugMacro(<< "Interaction profile changed for " << hand << ": " << profileString);

          auto renWin = vtkOpenXRRenderWindow::SafeDownCast(this->RenderWindow);
          if (!renWin)
          {
            vtkErrorMacro("Unable to retrieve the OpenXR render window !");
            return;
          }

          std::string profile(&profileString[0]);
          renWin->SetCurrentInteractionProfile(hand, profile);
        }
        break;
      }
      default:
      {
        // Give a chance to the manager to handle connection events
        if (!xrManager.GetConnectionStrategy()->HandleXrEvent(eventData))
        {
          vtkWarningMacro(<< "Unhandled event type "
                          << vtkOpenXRUtilities::GetStructureTypeAsString(eventData.type));
        }
        break;
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindowInteractor::ConvertOpenXRPoseToWorldCoordinates(const XrPosef& xrPose,
  double pos[3],  // Output world position
  double wxyz[4], // Output world orientation quaternion
  double ppos[3], // Output physical position
  double wdir[3]) // Output world view direction (-Z)
{
  vtkOpenXRUtilities::SetMatrixFromXrPose(this->PoseToWorldMatrix, xrPose);
  this->ConvertPoseToWorldCoordinates(this->PoseToWorldMatrix, pos, wxyz, ppos, wdir);
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindowInteractor::PollXrActions()
{
  // Update the action states by syncing using the active action set
  vtkOpenXRManager::GetInstance().SyncActions();

  // Iterate over all actions and update their data
  MapAction::iterator it;
  for (it = this->MapActionStruct_Name.begin(); it != this->MapActionStruct_Name.end(); ++it)
  {
    ActionData* actionData = it->second;

    // Update the state of the actions for left and right hands separately.
    for (uint32_t hand :
      { vtkOpenXRManager::ControllerIndex::Left, vtkOpenXRManager::ControllerIndex::Right })
    {
      vtkOpenXRManager::GetInstance().UpdateActionData(actionData->ActionStruct, hand);
    }
  }

  auto renWin = vtkOpenXRRenderWindow::SafeDownCast(this->RenderWindow);
  if (!renWin)
  {
    vtkErrorMacro("Unable to retrieve the OpenXR render window !");
    return;
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
    // XXX GetHandPose should be replaced by the use of generic API for retrieving devices poses
    // (see DeviceHandles in vtkVRRenderWindow) in a future refactoring of OpenXR classes.
    XrPosef* handPose = this->GetHandPose(hand);
    if (handPose)
    {
      this->ConvertOpenXRPoseToWorldCoordinates(*handPose, pos, wxyz, ppos, wdir);
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

      // Update DeviceToPhysical matrices, this is a read-write access!
      vtkMatrix4x4* devicePose = renWin->GetDeviceToPhysicalMatrixForDevice(edHand->GetDevice());
      if (devicePose)
      {
        vtkOpenXRUtilities::SetMatrixFromXrPose(devicePose, *handPose);
      }
    }
  }

  // All actions are now updated, handle them now
  for (it = this->MapActionStruct_Name.begin(); it != this->MapActionStruct_Name.end(); ++it)
  {
    ActionData* actionData = it->second;

    for (uint32_t hand :
      { vtkOpenXRManager::ControllerIndex::Left, vtkOpenXRManager::ControllerIndex::Right })
    {
      vtkEventDataDevice3D* eventData = eventDatas[hand];

      if (eventData)
      {
        eventData->SetInput(actionData->DeviceInput);
        eventData->SetType(actionData->EventId);

        this->HandleAction(*actionData, hand, eventData);
      }
    }
  }

  // Handle head movement
  // XXX This is a temporary solution to stick with the OpenVR behavior.
  // Move3DEvent is emitted by left and right controllers, and the headset.
  // This is used in vtkOpenXRInteractorStyle for "grounded" movement.
  // In future refactoring of OpenXR classes, we could add a specific method in
  // vtkOpenXRManager to retrieve the "real" head pose (for now we use the left
  // eye direction retrieved in vtkOpenXRRenderWindow::UpdateHMDMatrixPose,
  // that is close).
  // Retrieve headset pose matrix in physical coordinates and convert to position and orientation
  // in world coordinates
  vtkMatrix4x4* poseMatrix =
    renWin->GetDeviceToPhysicalMatrixForDevice(vtkEventDataDevice::HeadMountedDisplay);
  if (poseMatrix == nullptr)
  {
    // Can be undefined at the beginning
    return;
  }
  // XXX In future developments, consider adding a function extracting position and orientation in
  // world coordinates directly from a pose matrix in world coordinates
  this->ConvertPoseToWorldCoordinates(poseMatrix, pos, wxyz, ppos, wdir);

  // Generate "head movement" event
  vtkNew<vtkEventDataDevice3D> edd;
  edd->SetWorldPosition(pos);
  edd->SetWorldOrientation(wxyz);
  edd->SetWorldDirection(wdir);
  edd->SetDevice(vtkEventDataDevice::HeadMountedDisplay);
  this->InvokeEvent(vtkCommand::Move3DEvent, edd);
}

//------------------------------------------------------------------------------
XrPosef* vtkOpenXRRenderWindowInteractor::GetHandPose(uint32_t hand)
{
  if (this->MapActionStruct_Name.count("handpose") == 0)
  {
    return nullptr;
  }

  ActionData* adHandPose = this->MapActionStruct_Name["handpose"];
  return &(adHandPose->ActionStruct.PoseLocations[hand].pose);
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
      if (!this->XrCheckOutput(vtkOpenXRManager::ErrorOutput, xrGetActionStateFloat(Session, &info,
      &action_t.States[hand]._float), "Failed to get float value"))
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

    ed->SetTrackPadPosition(vec2f.currentState.x, vec2f.currentState.y);

    this->ApplyAction(actionData, ed);
  }
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindowInteractor::AddAction(
  const std::string& path, const vtkCommand::EventIds& eid, bool vtkNotUsed(isAnalog))
{
  this->AddAction(path, eid);
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindowInteractor::AddAction(
  const std::string& path, const vtkCommand::EventIds& eid)
{
  if (this->MapActionStruct_Name.count(path) == 0)
  {
    ActionData* am = new ActionData();
    this->MapActionStruct_Name[path] = am;
  }

  auto* am = this->MapActionStruct_Name[path];
  am->EventId = eid;
  am->UseFunction = false;
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindowInteractor::AddAction(const std::string& path, bool vtkNotUsed(isAnalog),
  const std::function<void(vtkEventData*)>& func)
{
  this->AddAction(path, func);
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindowInteractor::AddAction(
  const std::string& path, const std::function<void(vtkEventData*)>& func)
{
  if (this->MapActionStruct_Name.count(path) == 0)
  {
    ActionData* am = new ActionData();
    this->MapActionStruct_Name[path] = am;
  }

  auto* am = this->MapActionStruct_Name[path];
  am->UseFunction = true;
  am->Function = func;
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindowInteractor::Initialize()
{
  if (this->Initialized)
  {
    return;
  }

  // Start with superclass initialization
  this->Superclass::Initialize();

  vtkOpenXRRenderWindow* renWin = vtkOpenXRRenderWindow::SafeDownCast(this->RenderWindow);

  // Make sure the render window is initialized
  renWin->Initialize();

  if (!renWin->GetVRInitialized())
  {
    return;
  }

  // Complex gesture actions are handled by the interactor directly (why?)
  this->AddAction(
    "complexgestureaction", [this](vtkEventData* ed) { this->HandleComplexGestureEvents(ed); });

  // Create an entry for pose actions that are used to retrieve
  // Orientation and locations of trackers
  this->AddAction("handpose", vtkCommand::Move3DEvent);
  // Prevent unbound action warning
  this->AddAction("handposegrip", [](vtkEventData*) {});

  std::string fullpath = vtksys::SystemTools::CollapseFullPath(
    this->ActionManifestDirectory + this->ActionManifestFileName);

  if (!this->LoadActions(fullpath))
  {
    vtkErrorMacro(<< "Failed to load actions.");
    this->Initialized = false;
    return;
  }

  // All action sets have been created, so
  // We can now attach the action sets to the session
  if (!vtkOpenXRManager::GetInstance().AttachSessionActionSets())
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
  vtkOpenXRManager::GetInstance().CreateActionSet(this->ActionSetName, localizedActionSetName);

  // We must select an action set to create actions
  // For instance only one action set so select it
  // Improvement: select each action set and create all actions
  // that belong to it
  vtkOpenXRManager::GetInstance().SelectActiveActionSet(0);

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

    // If the action is an output, add it so that it will
    // connect to its binding without user having to specify.
    // Vibration is the only supported output
    if (type == "vibration")
    {
      if (this->MapActionStruct_Name.count(name) == 0)
      {
        ActionData* am = new ActionData();
        this->MapActionStruct_Name[name] = am;
      }
    }

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
    // `GetActionTypeFromString` casts `0` when it fails, so the variant in
    // this case is not "valid", but `clang-tidy` doesn't know that.
    // NOLINTNEXTLINE(bugprone-non-zero-enum-to-bool-conversion)
    if (!xrActionType)
    {
      return false;
    }

    // Create the action using the selected action set
    Action_t actionStruct;
    actionStruct.ActionType = xrActionType;
    if (!vtkOpenXRManager::GetInstance().CreateOneAction(actionStruct, name, localizedName))
    {
      return false;
    }

    // Store it to retrieve actions by their name
    this->MapActionStruct_Name[name]->ActionStruct = actionStruct;
    this->MapActionStruct_Name[name]->Name = name;
  }

  Json::Value defaultBindings = root["default_bindings"];
  if (defaultBindings.isNull())
  {
    vtkErrorMacro(<< "Parse openxr_actions: Missing default_bindings node");
    return false;
  }

  // look in the same directory as the actionFilename
  std::string path = vtksys::SystemTools::GetFilenamePath(actionFilename);

  for (Json::Value::ArrayIndex i = 0; i < defaultBindings.size(); ++i)
  {
    Json::Value binding = defaultBindings[i];
    std::string bindingUrl = binding["binding_url"].asString();
    std::string bindingFilename = vtksys::SystemTools::CollapseFullPath(path + "/" + bindingUrl);
    if (!this->LoadDefaultBinding(bindingFilename))
    {
      return false;
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
  auto fillActionSuggestedBindings = [&](const std::string& path, const Json::Value& jsonValue)
  {
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

      XrPath xrPath = vtkOpenXRManager::GetInstance().GetXrPath(path);
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

  // Look under haptics for any outputs
  Json::Value haptics = actionSet["haptics"];
  for (Json::Value::ArrayIndex i = 0; i < haptics.size(); i++)
  {
    Json::Value haptic = haptics[i];

    // The path for this action
    std::string path = haptic["path"].asString();

    // Iterate over all outputs
    fillActionSuggestedBindings(path, haptic);
  }

  // Submit all suggested bindings
  return vtkOpenXRManager::GetInstance().SuggestActions(
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
void vtkOpenXRRenderWindowInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "vtkOpenXRRenderWindowInteractor"
     << "\n";
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
bool vtkOpenXRRenderWindowInteractor::ApplyVibration(const std::string& actionName, const int hand,
  const float amplitude, const float duration, const float frequency)
{
  ActionData* actionData = GetActionDataFromName(actionName);
  if (actionData == nullptr)
  {
    vtkWarningMacro(
      << "vtkOpenXRRenderWindowInteractor: Attempt to ApplyVibration using action data with name"
      << actionName << " that does not exist.");
    return false;
  }

  return vtkOpenXRManager::GetInstance().ApplyVibration(
    actionData->ActionStruct, hand, amplitude, duration, frequency);
}
VTK_ABI_NAMESPACE_END
