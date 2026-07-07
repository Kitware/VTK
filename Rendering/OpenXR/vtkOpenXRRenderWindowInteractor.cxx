// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOpenXRRenderWindowInteractor.h"

#include "vtkObjectFactory.h"
#include "vtkOpenGLState.h"
#include "vtkOpenXRInteractorStyle.h"
#include "vtkOpenXRManager.h"
#include "vtkOpenXRRenderWindow.h"
#include "vtkOpenXRUtilities.h"
#include "vtkResourceFileLocator.h"
#include "vtkStringFormatter.h"
#include "vtkVersion.h"

#include "vtk_jsoncpp.h"
#include <memory>
#include <string>
#include <vtksys/FStream.hxx>
#include <vtksys/SystemTools.hxx>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOpenXRRenderWindowInteractor);

//------------------------------------------------------------------------------

class vtkOpenXRRenderWindowInteractor::vtkInternal
{
private:
  vtkOpenXRRenderWindowInteractor* Interactor;

public:
  vtkInternal(vtkOpenXRRenderWindowInteractor* rwi)
    : Interactor(rwi)
  {
  }

  /**
   * Return the XrPosef for the action named "handpose"
   * and the hand \p hand or return nullptr if "handpose"
   * does not exist in the map.
   */
  XrPosef* GetHandPose(uint32_t hand);

  void ConvertOpenXRPoseToWorldCoordinates(const XrPosef& xrPose,
    double pos[3],   // Output world position
    double wxyz[4],  // Output world orientation quaternion
    double ppos[3],  // Output physical position
    double wdir[3]); // Output world view direction (-Z)

  /**
   * Process OpenXR specific events.
   */
  void ProcessXrEvents();

  /**
   * Update the action states using the OpenXRManager
   * and handle all actions.
   */
  void PollXrActions();

  struct ActionData;

  XrActionType GetActionTypeFromString(const std::string& type);
  bool LoadActions(const std::string& actionFilename);
  bool LoadDefaultBinding(const std::string& bindingFilename);
  ActionData* GetActionDataFromName(const std::string& actionName);

  void HandleAction(const ActionData& actionData, int hand, vtkEventDataDevice3D* ed);
  void HandleBooleanAction(const ActionData& actionData, int hand, vtkEventDataDevice3D* ed);
  void HandlePoseAction(const ActionData& actionData, int hand, vtkEventDataDevice3D* ed);
  void HandleVector2fAction(const ActionData& actionData, int hand, vtkEventDataDevice3D* ed);
  void ApplyAction(const ActionData& actionData, vtkEventDataDevice3D* ed);

  struct ActionData
  {
    std::string Name;

    vtkEventDataDeviceInput DeviceInput = vtkEventDataDeviceInput::Unknown;

    // This structure is defined in vtkOpenXRManager
    // And hold OpenXR related data
    // NOLINTNEXTLINE(bugprone-invalid-enum-default-initialization)
    vtk::detail::vtkOpenXRManager::Action_t ActionStruct{ XR_NULL_HANDLE };

    vtkCommand::EventIds EventId;
    std::function<void(vtkEventData*)> Function;
    bool UseFunction = false;
  };

  using MapAction = std::map<std::string, std::unique_ptr<ActionData>>;
  MapAction MapActionStruct_Name;
};

//------------------------------------------------------------------------------
// Construct object so that light follows camera motion.
vtkOpenXRRenderWindowInteractor::vtkOpenXRRenderWindowInteractor()
  : Internal(std::make_unique<vtkInternal>(this))
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
vtkOpenXRRenderWindowInteractor::~vtkOpenXRRenderWindowInteractor() = default;

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindowInteractor::DoOneEvent(
  vtkVRRenderWindow* renWin, vtkRenderer* vtkNotUsed(ren))
{
  this->Internal->ProcessXrEvents();

  if (this->Done || !vtk::detail::vtkOpenXRManager::GetInstance().IsSessionRunning())
  {
    return;
  }

  this->Internal->PollXrActions();

  if (this->RecognizeGestures)
  {
    this->RecognizeComplexGesture(nullptr);
  }

  // Start a render
  this->InvokeEvent(vtkCommand::RenderEvent);
  renWin->Render();
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindowInteractor::vtkInternal::ProcessXrEvents()
{
  auto& xrManager = vtk::detail::vtkOpenXRManager::GetInstance();

  XrEventDataBuffer eventData{};
  while (xrManager.PollEvent(eventData))
  {
    switch (eventData.type)
    {
      // We lost some data
      case XR_TYPE_EVENT_DATA_EVENTS_LOST:
      {
        vtkDebugWithObjectMacro(this->Interactor,
          << "OpenXR event [XR_TYPE_EVENT_DATA_EVENTS_LOST] : some events data lost!");
        // do we care if the runtime loses events?
        break;
      }

      //
      case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
      {
        vtkWarningWithObjectMacro(this->Interactor,
          << "OpenXR event [XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING] : exit render loop.");
        this->Interactor->Done = true;
        return;
      }

      //
      case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
      {
        const auto stateEvent =
          *reinterpret_cast<const XrEventDataSessionStateChanged*>(&eventData);
        if (stateEvent.session != xrManager.GetSession())
        {
          vtkErrorWithObjectMacro(this->Interactor,
            << "OpenXR event [XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED] : session is "
               "different than this->Session. Aborting.");
          this->Interactor->Done = true;
          return;
        }
        switch (stateEvent.state)
        {
          case XR_SESSION_STATE_READY:
          {
            vtkDebugWithObjectMacro(
              this->Interactor, << "OpenXR event [XR_SESSION_STATE_READY] : Begin session");
            xrManager.BeginSession();
            break;
          }
          case XR_SESSION_STATE_STOPPING:
            vtkDebugWithObjectMacro(
              this->Interactor, << "OpenXR event [XR_SESSION_STATE_STOPPING]");
            [[fallthrough]];
          case XR_SESSION_STATE_LOSS_PENDING:
            // Session was lost, so start over and poll for new systemId.
            vtkDebugWithObjectMacro(
              this->Interactor, << "OpenXR event [XR_SESSION_STATE_LOSS_PENDING]");
            [[fallthrough]];
          case XR_SESSION_STATE_EXITING:
          {
            // Do not attempt to restart, because user closed this session.
            vtkDebugWithObjectMacro(this->Interactor, << "OpenXR event [XR_SESSION_STATE_EXITING]");
            vtkDebugWithObjectMacro(this->Interactor, << "Exit render loop.");
            this->Interactor->Done = true;
            break;
          }
          default:
            break;
        }
        break;
      }
      case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING:
      {
        vtkDebugWithObjectMacro(
          this->Interactor, << "OpenXR event [XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING]");
        const auto stateEvent =
          *reinterpret_cast<const XrEventDataReferenceSpaceChangePending*>(&eventData);
        (void)stateEvent;
        break;
      }

      case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
      {
        vtkDebugWithObjectMacro(
          this->Interactor, << "OpenXR event [XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED]");
        const auto stateEvent =
          *reinterpret_cast<const XrEventDataInteractionProfileChanged*>(&eventData);
        (void)stateEvent;

        XrInteractionProfileState state{ XR_TYPE_INTERACTION_PROFILE_STATE };

        for (uint32_t hand : { vtk::detail::vtkOpenXRManager::ControllerIndex::Left,
               vtk::detail::vtkOpenXRManager::ControllerIndex::Right })
        {
          if (!xrManager.XrCheckOutput(vtk::detail::vtkOpenXRManager::WarningOutput,
                xrGetCurrentInteractionProfile(
                  xrManager.GetSession(), xrManager.GetSubactionPaths()[hand], &state),
                "Failed to get interaction profile for hand " + vtk::to_string(hand)))
          {
            continue;
          }

          XrPath interactionProfile = state.interactionProfile;

          if (!interactionProfile)
          {
            vtkDebugWithObjectMacro(this->Interactor, << "No interaction profile set");
            continue;
          }

          uint32_t strLength;
          char profileString[XR_MAX_PATH_LENGTH];
          if (!xrManager.XrCheckOutput(vtk::detail::vtkOpenXRManager::WarningOutput,
                xrPathToString(xrManager.GetXrRuntimeInstance(), interactionProfile,
                  XR_MAX_PATH_LENGTH, &strLength, profileString),
                "Failed to get interaction profile path string for hand " + vtk::to_string(hand)))
          {
            continue;
          }

          vtkDebugWithObjectMacro(this->Interactor,
            << "Interaction profile changed for " << hand << ": " << profileString);

          auto renWin = vtkOpenXRRenderWindow::SafeDownCast(this->Interactor->RenderWindow);
          if (!renWin)
          {
            vtkErrorWithObjectMacro(
              this->Interactor, "Unable to retrieve the OpenXR render window !");
            return;
          }

          std::string profile(&profileString[0]);
          renWin->SetCurrentInteractionProfile(hand, profile);
        }
        break;
      }
      default:
      {
        break;
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindowInteractor::vtkInternal::ConvertOpenXRPoseToWorldCoordinates(
  const XrPosef& xrPose,
  double pos[3],  // Output world position
  double wxyz[4], // Output world orientation quaternion
  double ppos[3], // Output physical position
  double wdir[3]) // Output world view direction (-Z)
{
  vtkOpenXRUtilities::SetMatrixFromXrPose(this->Interactor->PoseToWorldMatrix, xrPose);
  this->Interactor->ConvertPoseToWorldCoordinates(
    this->Interactor->PoseToWorldMatrix, pos, wxyz, ppos, wdir);
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindowInteractor::vtkInternal::PollXrActions()
{
  // Update the action states by syncing using the active action set
  vtk::detail::vtkOpenXRManager::GetInstance().SyncActions();

  // Iterate over all actions and update their data
  MapAction::iterator it;
  for (it = this->MapActionStruct_Name.begin(); it != this->MapActionStruct_Name.end(); ++it)
  {
    ActionData* actionData = it->second.get();

    // Update the state of the actions for left and right hands separately.
    for (uint32_t hand : { vtk::detail::vtkOpenXRManager::ControllerIndex::Left,
           vtk::detail::vtkOpenXRManager::ControllerIndex::Right })
    {
      vtk::detail::vtkOpenXRManager::GetInstance().UpdateActionData(actionData->ActionStruct, hand);
    }
  }

  auto renWin = vtkOpenXRRenderWindow::SafeDownCast(this->Interactor->RenderWindow);
  if (!renWin)
  {
    vtkErrorWithObjectMacro(this->Interactor, "Unable to retrieve the OpenXR render window !");
    return;
  }

  // Construct the event data that contains position and orientation of each hand
  double pos[3] = { 0.0 };
  double ppos[3] = { 0.0 };
  double wxyz[4] = { 0.0 };
  double wdir[3] = { 0.0 };
  std::array<vtkSmartPointer<vtkEventDataDevice3D>, 2> eventDatas;
  for (const uint32_t hand : { vtk::detail::vtkOpenXRManager::ControllerIndex::Left,
         vtk::detail::vtkOpenXRManager::ControllerIndex::Right })
  {
    // XXX GetHandPose should be replaced by the use of generic API for retrieving devices poses
    // (see DeviceHandles in vtkVRRenderWindow) in a future refactoring of OpenXR classes.
    XrPosef* handPose = this->GetHandPose(hand);
    if (handPose)
    {
      this->ConvertOpenXRPoseToWorldCoordinates(*handPose, pos, wxyz, ppos, wdir);
      auto edHand = vtkEventDataDevice3D::New();
      edHand->SetDevice(hand == vtk::detail::vtkOpenXRManager::ControllerIndex::Right
          ? vtkEventDataDevice::RightController
          : vtkEventDataDevice::LeftController);
      edHand->SetWorldPosition(pos);
      edHand->SetWorldOrientation(wxyz);
      edHand->SetWorldDirection(wdir);
      eventDatas[hand].TakeReference(edHand);

      // We should remove this and use event data directly
      int pointerIndex = static_cast<int>(edHand->GetDevice());
      this->Interactor->SetPhysicalEventPosition(ppos[0], ppos[1], ppos[2], pointerIndex);
      this->Interactor->SetWorldEventPosition(pos[0], pos[1], pos[2], pointerIndex);
      this->Interactor->SetWorldEventOrientation(wxyz[0], wxyz[1], wxyz[2], wxyz[3], pointerIndex);

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
    ActionData* actionData = it->second.get();

    for (uint32_t hand : { vtk::detail::vtkOpenXRManager::ControllerIndex::Left,
           vtk::detail::vtkOpenXRManager::ControllerIndex::Right })
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
  this->Interactor->ConvertPoseToWorldCoordinates(poseMatrix, pos, wxyz, ppos, wdir);

  // Generate "head movement" event
  vtkNew<vtkEventDataDevice3D> edd;
  edd->SetWorldPosition(pos);
  edd->SetWorldOrientation(wxyz);
  edd->SetWorldDirection(wdir);
  edd->SetDevice(vtkEventDataDevice::HeadMountedDisplay);
  this->Interactor->InvokeEvent(vtkCommand::Move3DEvent, edd);
}

//------------------------------------------------------------------------------
XrPosef* vtkOpenXRRenderWindowInteractor::vtkInternal::GetHandPose(uint32_t hand)
{
  if (this->MapActionStruct_Name.count("handpose") == 0)
  {
    return nullptr;
  }

  ActionData* adHandPose = this->MapActionStruct_Name["handpose"].get();
  return &(adHandPose->ActionStruct.PoseLocations[hand].pose);
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindowInteractor::vtkInternal::HandleAction(
  const ActionData& actionData, const int hand, vtkEventDataDevice3D* ed)
{
  const auto& actionT = actionData.ActionStruct;
  switch (actionT.ActionType)
  {
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
void vtkOpenXRRenderWindowInteractor::vtkInternal::ApplyAction(
  const ActionData& actionData, vtkEventDataDevice3D* ed)
{
  this->Interactor->SetPointerIndex(static_cast<int>(ed->GetDevice()));

  if (actionData.UseFunction)
  {
    actionData.Function(ed);
  }
  else
  {
    this->Interactor->InvokeEvent(actionData.EventId, ed);
  }
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindowInteractor::vtkInternal::HandleBooleanAction(
  const ActionData& actionData, const int hand, vtkEventDataDevice3D* ed)
{
  XrActionStateBoolean value = actionData.ActionStruct.States[hand]._boolean;

  // Set the active state of the model
  vtkOpenXRRenderWindow::SafeDownCast(this->Interactor->RenderWindow)
    ->SetModelActiveState(hand, value.isActive);

  // Do nothing if the controller is inactive
  if (!value.isActive)
  {
    return;
  }

  if (value.changedSinceLastSync)
  {
    vtkDebugWithObjectMacro(this->Interactor, << "Boolean action \"" << actionData.Name
                                              << "\" is triggered with value " << value.currentState
                                              << " for hand " << hand);

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
void vtkOpenXRRenderWindowInteractor::vtkInternal::HandlePoseAction(
  const ActionData& actionData, const int hand, vtkEventDataDevice3D* ed)
{
  XrActionStatePose pose = actionData.ActionStruct.States[hand]._pose;

  // Set the active state of the model
  vtkOpenXRRenderWindow::SafeDownCast(this->Interactor->RenderWindow)
    ->SetModelActiveState(hand, pose.isActive);
  // Do nothing if the controller is inactive
  if (!pose.isActive)
  {
    return;
  }

  this->ApplyAction(actionData, ed);
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderWindowInteractor::vtkInternal::HandleVector2fAction(
  const ActionData& actionData, const int hand, vtkEventDataDevice3D* ed)
{
  XrActionStateVector2f vec2f = actionData.ActionStruct.States[hand]._vec2f;

  // Set the active state of the model
  vtkOpenXRRenderWindow::SafeDownCast(this->Interactor->RenderWindow)
    ->SetModelActiveState(hand, vec2f.isActive);
  // Do nothing if the controller is inactive
  if (!vec2f.isActive)
  {
    return;
  }

  if (vec2f.changedSinceLastSync)
  {
    vtkDebugWithObjectMacro(this->Interactor, << "Vector2f : " << actionData.Name
                                              << ", x = " << vec2f.currentState.x
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
  if (this->Internal->MapActionStruct_Name.count(path) == 0)
  {
    this->Internal->MapActionStruct_Name[path] = std::make_unique<vtkInternal::ActionData>();
  }

  auto& am = this->Internal->MapActionStruct_Name[path];
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
  if (this->Internal->MapActionStruct_Name.count(path) == 0)
  {
    this->Internal->MapActionStruct_Name[path] = std::make_unique<vtkInternal::ActionData>();
  }

  auto& am = this->Internal->MapActionStruct_Name[path];
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

  if (!this->Internal->LoadActions(fullpath))
  {
    vtkErrorMacro(<< "Failed to load actions.");
    this->Initialized = false;
    return;
  }

  // All action sets have been created, so
  // We can now attach the action sets to the session
  if (!vtk::detail::vtkOpenXRManager::GetInstance().AttachSessionActionSets())
  {
    this->Initialized = false;
    return;
  }
}

//------------------------------------------------------------------------------
bool vtkOpenXRRenderWindowInteractor::vtkInternal::LoadActions(const std::string& actionFilename)
{
  vtkDebugWithObjectMacro(this->Interactor, << "LoadActions from file : " << actionFilename);
  // As OpenXR does not yet have a way to pass a file to create actions
  // We need to create them programmatically, so we parse it with JsonCpp
  Json::Value root;

  // Open the file
  vtksys::ifstream file;
  file.open(actionFilename.c_str());
  if (!file.is_open())
  {
    vtkErrorWithObjectMacro(
      this->Interactor, << "Unable to open openXR action file : " << actionFilename);
    return false;
  }

  Json::CharReaderBuilder builder;

  std::string formattedErrors;
  // parse the entire data into the Json::Value root
  if (!Json::parseFromStream(builder, file, &root, &formattedErrors))
  {
    // Report failures and their locations in the document
    vtkErrorWithObjectMacro(this->Interactor, << "Failed to parse action file with errors :" << endl
                                              << formattedErrors);
    return false;
  }

  // Create an action set
  std::string localizedActionSetName = "VTK actions";
  vtk::detail::vtkOpenXRManager::GetInstance().CreateActionSet(
    this->Interactor->ActionSetName, localizedActionSetName);

  // We must select an action set to create actions
  // For instance only one action set so select it
  // Improvement: select each action set and create all actions
  // that belong to it
  vtk::detail::vtkOpenXRManager::GetInstance().SelectActiveActionSet(0);

  // Create actions
  Json::Value actions = root["actions"];
  if (actions.isNull())
  {
    vtkErrorWithObjectMacro(this->Interactor, << "Parse openxr_actions: Missing actions node");
    return false;
  }
  Json::Value localization = root["localization"];
  if (localization.isNull())
  {
    vtkErrorWithObjectMacro(this->Interactor, << "Parse openxr_actions: Missing localization node");
    return false;
  }
  localization = localization[0];

  for (Json::Value::ArrayIndex i = 0; i < actions.size(); ++i)
  {
    // Create one action per json value
    const Json::Value& action = actions[i];

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
        this->MapActionStruct_Name[name] = std::make_unique<vtkInternal::ActionData>();
      }
    }

    // Check if the action is used by the interactor style
    // or ourself. If that's the case, create it
    // Else do nothing
    if (this->MapActionStruct_Name.count(name) == 0)
    {
      vtkWarningWithObjectMacro(this->Interactor,
        << "An action with name " << name
        << " is available but the interactor style or the interactor does not use it.");
      continue;
    }

    vtkDebugWithObjectMacro(this->Interactor, << "Creating an action of type=" << type
                                              << ", with name=" << name
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
    vtk::detail::vtkOpenXRManager::Action_t actionStruct;
    actionStruct.ActionType = xrActionType;
    if (!vtk::detail::vtkOpenXRManager::GetInstance().CreateOneAction(
          actionStruct, name, localizedName))
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
    vtkErrorWithObjectMacro(
      this->Interactor, << "Parse openxr_actions: Missing default_bindings node");
    return false;
  }

  // look in the same directory as the actionFilename
  std::string path = vtksys::SystemTools::GetFilenamePath(actionFilename);

  for (Json::Value::ArrayIndex i = 0; i < defaultBindings.size(); ++i)
  {
    const Json::Value& binding = defaultBindings[i];
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
XrActionType vtkOpenXRRenderWindowInteractor::vtkInternal::GetActionTypeFromString(
  const std::string& type)
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
    vtkErrorWithObjectMacro(this->Interactor, << "Unrecognized action type: " << type);
    return (XrActionType)0;
  }
}

//------------------------------------------------------------------------------
bool vtkOpenXRRenderWindowInteractor::vtkInternal::LoadDefaultBinding(
  const std::string& bindingFilename)
{
  Json::Value root;
  // Open the file
  vtksys::ifstream file;
  file.open(bindingFilename.c_str());
  if (!file.is_open())
  {
    vtkErrorWithObjectMacro(
      this->Interactor, << "Unable to open openXR binding file : " << bindingFilename);
    return false;
  }

  Json::CharReaderBuilder builder;
  std::string formattedErrors;
  // parse the entire data into the Json::Value root
  if (!Json::parseFromStream(builder, file, &root, &formattedErrors))
  {
    // Report failures and their locations in the document
    vtkErrorWithObjectMacro(
      this->Interactor, << "Failed to parse binding file with errors :" << endl
                        << formattedErrors);
    return false;
  }

  // Get the interaction profile name
  std::string interactionProfile = root["interaction_profile"].asString();

  Json::Value bindings = root["bindings"];

  const Json::Value& actionSet = bindings[this->Interactor->ActionSetName];
  if (actionSet.isNull())
  {
    vtkErrorWithObjectMacro(
      this->Interactor, << "Selected action set : " << this->Interactor->ActionSetName
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
    vtkDebugWithObjectMacro(
      this->Interactor, << "Add action : " << action << ", with path : " << path);
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

      auto& actionT = actionData->ActionStruct;

      if (actionT.Action == XR_NULL_HANDLE)
      {
        vtkErrorWithObjectMacro(this->Interactor,
          << "Action " << action << ", with path : " << path << " has a null handle !");
        return;
      }

      XrPath xrPath = vtk::detail::vtkOpenXRManager::GetInstance().GetXrPath(path);
      actionSuggestedBindings.push_back({ actionT.Action, xrPath });
    }
  };

  // First, look after all sources inputs, ie. boolean/float/vector2f actions
  const Json::Value& sources = actionSet["sources"];
  for (Json::Value::ArrayIndex i = 0; i < sources.size(); ++i)
  {
    const Json::Value& source = sources[i];

    // The path for this action
    std::string path = source["path"].asString();

    // Iterate over all inputs and append to the path the selected input
    // For example, if the input is "click", then append click to the path
    // if the input is "position", add nothing as openxr binds the position as a vector2f
    // (for example if we want to retrieve the position of the trackpad as a vector2f directly)
    const Json::Value& inputs = source["inputs"];
    for (auto const& inputStr : inputs.getMemberNames())
    {
      const Json::Value& action = inputs[inputStr];
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
  const Json::Value& haptics = actionSet["haptics"];
  for (Json::Value::ArrayIndex i = 0; i < haptics.size(); i++)
  {
    const Json::Value& haptic = haptics[i];

    // The path for this action
    std::string path = haptic["path"].asString();

    // Iterate over all outputs
    fillActionSuggestedBindings(path, haptic);
  }

  // Submit all suggested bindings
  return vtk::detail::vtkOpenXRManager::GetInstance().SuggestActions(
    interactionProfile, actionSuggestedBindings);
}

//------------------------------------------------------------------------------
vtkOpenXRRenderWindowInteractor::vtkInternal::ActionData*
vtkOpenXRRenderWindowInteractor::vtkInternal::GetActionDataFromName(const std::string& actionName)
{
  // Check if action data exists
  if (this->MapActionStruct_Name.count(actionName) == 0)
  {
    vtkWarningWithObjectMacro(this->Interactor,
      << "vtkOpenXRRenderWindowInteractor: Attempt to get an action data with name " << actionName
      << " that does not exist in the map.");
    return nullptr;
  }
  return this->MapActionStruct_Name[actionName].get();
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
  vtkInternal::ActionData* actionData = this->Internal->GetActionDataFromName(actionName);
  if (actionData == nullptr)
  {
    vtkWarningMacro(
      << "vtkOpenXRRenderWindowInteractor: Attempt to ApplyVibration using action data with name"
      << actionName << " that does not exist.");
    return false;
  }

  return vtk::detail::vtkOpenXRManager::GetInstance().ApplyVibration(
    actionData->ActionStruct, hand, amplitude, duration, frequency);
}
VTK_ABI_NAMESPACE_END
