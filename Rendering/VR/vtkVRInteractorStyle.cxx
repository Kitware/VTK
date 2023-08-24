// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkVRInteractorStyle.h"

#include "vtkAssemblyPath.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCellPicker.h"
#include "vtkInformation.h"
#include "vtkMatrix3x3.h"
#include "vtkPlane.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkQuaternion.h"
#include "vtkRenderer.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSphereSource.h"
#include "vtkTextActor3D.h"
#include "vtkTextProperty.h"
#include "vtkTimerLog.h"
#include "vtkTransform.h"
#include "vtkVRControlsHelper.h"
#include "vtkVRHardwarePicker.h"
#include "vtkVRMenuRepresentation.h"
#include "vtkVRMenuWidget.h"
#include "vtkVRModel.h"
#include "vtkVRRenderWindow.h"
#include "vtkVRRenderWindowInteractor.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkVRInteractorStyle::vtkVRInteractorStyle()
{
  this->InteractionProps.resize(vtkEventDataNumberOfDevices);
  this->ClippingPlanes.resize(vtkEventDataNumberOfDevices);

  for (int d = 0; d < vtkEventDataNumberOfDevices; ++d)
  {
    this->InteractionState[d] = VTKIS_NONE;

    for (int i = 0; i < vtkEventDataNumberOfInputs; i++)
    {
      this->ControlsHelpers[d][i] = nullptr;
    }
  }

  // Create default inputs mapping
  this->MapInputToAction(vtkCommand::Select3DEvent, VTKIS_POSITION_PROP);

  this->MenuCommand->SetClientData(this);
  this->MenuCommand->SetCallback(vtkVRInteractorStyle::MenuCallback);

  this->Menu->SetRepresentation(this->MenuRepresentation);
  this->Menu->PushFrontMenuItem("exit", "Exit", this->MenuCommand);
  this->Menu->PushFrontMenuItem("clipmode", "Clipping Mode", this->MenuCommand);
  this->Menu->PushFrontMenuItem("probemode", "Probe Mode", this->MenuCommand);
  this->Menu->PushFrontMenuItem("grabmode", "Grab Mode", this->MenuCommand);

  vtkNew<vtkPolyDataMapper> pdm;
  this->PickActor->SetMapper(pdm);
  this->PickActor->GetProperty()->SetLineWidth(4);
  this->PickActor->GetProperty()->RenderLinesAsTubesOn();
  this->PickActor->GetProperty()->SetRepresentationToWireframe();
  this->PickActor->DragableOff();

  vtkNew<vtkCellPicker> exactPicker;
  this->SetInteractionPicker(exactPicker);
}

//------------------------------------------------------------------------------
vtkVRInteractorStyle::~vtkVRInteractorStyle()
{
  for (int d = 0; d < vtkEventDataNumberOfDevices; ++d)
  {
    for (int i = 0; i < vtkEventDataNumberOfInputs; i++)
    {
      if (this->ControlsHelpers[d][i])
      {
        this->ControlsHelpers[d][i]->Delete();
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkVRInteractorStyle::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "HoverPick: " << this->HoverPick << endl;
  os << indent << "GrabWithRay: " << this->GrabWithRay << endl;
}

//------------------------------------------------------------------------------
// Generic events binding
//------------------------------------------------------------------------------
void vtkVRInteractorStyle::OnSelect3D(vtkEventData* edata)
{
  vtkEventDataDevice3D* bd = edata->GetAsEventDataDevice3D();
  if (!bd)
  {
    return;
  }

  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];
  this->FindPokedRenderer(x, y);

  decltype(this->InputMap)::key_type key(vtkCommand::Select3DEvent, bd->GetAction());
  auto it = this->InputMap.find(key);
  if (it == this->InputMap.end())
  {
    return;
  }

  int state = it->second;

  // if grab mode then convert event data into where the ray is intersecting geometry
  switch (bd->GetAction())
  {
    case vtkEventDataAction::Press:
    case vtkEventDataAction::Touch:
      this->StartAction(state, bd);
      break;
    case vtkEventDataAction::Release:
    case vtkEventDataAction::Untouch:
      this->EndAction(state, bd);
      break;
    default:
      break;
  }
}

//------------------------------------------------------------------------------
void vtkVRInteractorStyle::OnNextPose3D(vtkEventData* edata)
{
  vtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();
  if (!edd)
  {
    return;
  }
  if (edd->GetAction() == vtkEventDataAction::Press)
  {
    this->LoadNextCameraPose();
  }
}

//------------------------------------------------------------------------------
void vtkVRInteractorStyle::Movement3D(int interactionState, vtkEventData* edata)
{
  vtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();
  if (!edd)
  {
    return;
  }

  // Retrieve device type
  int idev = static_cast<int>(edd->GetDevice());

  // Update current state
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];
  this->FindPokedRenderer(x, y);

  // Set current state and interaction prop
  this->InteractionProp = this->InteractionProps[idev];

  double const* pos = edd->GetTrackPadPosition();

  if (edd->GetAction() == vtkEventDataAction::Press)
  {
    this->StartAction(interactionState, edd);
    this->LastTrackPadPosition[0] = 0.0;
    this->LastTrackPadPosition[1] = 0.0;
    this->LastGroundMovementTrackPadPosition[0] = 0.0;
    this->LastGroundMovementTrackPadPosition[1] = 0.0;
    this->LastElevationTrackPadPosition[0] = 0.0;
    this->LastElevationTrackPadPosition[1] = 0.0;
    return;
  }

  if (edd->GetAction() == vtkEventDataAction::Release)
  {
    this->EndAction(interactionState, edd);
    return;
  }

  // If the input event is from a joystick and is away from the center then
  // call start. When the joystick returns to the center, call end.
  if ((edd->GetInput() == vtkEventDataDeviceInput::Joystick ||
        edd->GetInput() == vtkEventDataDeviceInput::TrackPad) &&
    this->InteractionState[idev] != interactionState && fabs(pos[1]) > 0.1)
  {
    this->StartAction(interactionState, edd);
    this->LastTrackPadPosition[0] = 0.0;
    this->LastTrackPadPosition[1] = 0.0;
    this->LastGroundMovementTrackPadPosition[0] = 0.0;
    this->LastGroundMovementTrackPadPosition[1] = 0.0;
    this->LastElevationTrackPadPosition[0] = 0.0;
    this->LastElevationTrackPadPosition[1] = 0.0;
    return;
  }

  if (this->InteractionState[idev] == interactionState)
  {
    // Stop when returning to the center on the joystick
    if ((edd->GetInput() == vtkEventDataDeviceInput::Joystick ||
          edd->GetInput() == vtkEventDataDeviceInput::TrackPad) &&
      fabs(pos[1]) < 0.1)
    {
      this->EndAction(interactionState, edd);
      return;
    }

    // Do the 3D movement corresponding to the interaction state
    switch (interactionState)
    {
      case VTKIS_DOLLY:
        this->Dolly3D(edd);
        break;
      case VTKIS_GROUNDMOVEMENT:
        this->GroundMovement3D(edd);
        break;
      case VTKIS_ELEVATION:
        this->Elevation3D(edd);
        break;
      default:
        break;
    }

    this->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
    return;
  }
}

//------------------------------------------------------------------------------
void vtkVRInteractorStyle::OnViewerMovement3D(vtkEventData* edata)
{
  if (this->Style == vtkVRInteractorStyle::FLY_STYLE)
  {
    this->Movement3D(VTKIS_DOLLY, edata);
  }
  else if (this->Style == vtkVRInteractorStyle::GROUNDED_STYLE)
  {
    this->Movement3D(VTKIS_GROUNDMOVEMENT, edata);
  }
}

//------------------------------------------------------------------------------
void vtkVRInteractorStyle::OnElevation3D(vtkEventData* edata)
{
  if (this->Style == vtkVRInteractorStyle::GROUNDED_STYLE)
  {
    this->Movement3D(VTKIS_ELEVATION, edata);
  }
}

//------------------------------------------------------------------------------
void vtkVRInteractorStyle::OnMove3D(vtkEventData* edata)
{
  vtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();
  if (!edd)
  {
    return;
  }

  // Retrieve device type
  int idev = static_cast<int>(edd->GetDevice());

  if (edd->GetDevice() == vtkEventDataDevice::HeadMountedDisplay)
  {
    edd->GetWorldDirection(this->HeadsetDir);
  }

  // Update current state
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  // Set current state and interaction prop
  this->InteractionProp = this->InteractionProps[idev];

  auto interactionState = this->InteractionState[idev];
  switch (interactionState)
  {
    case VTKIS_POSITION_PROP:
      this->FindPokedRenderer(x, y);
      this->PositionProp(edd);
      this->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
      break;
    case VTKIS_DOLLY:
    case VTKIS_GROUNDMOVEMENT:
    case VTKIS_ELEVATION:
      this->FindPokedRenderer(x, y);
      this->Movement3D(interactionState, edd);
      this->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
      break;
    case VTKIS_CLIP:
      this->FindPokedRenderer(x, y);
      this->Clip(edd);
      this->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
      break;
    default:
      vtkDebugMacro(<< "OnMove3D: unknown interaction state " << idev << ": "
                    << this->InteractionState[idev]);
      break;
  }

  // Update rays
  this->UpdateRay(edd->GetDevice());
}

//------------------------------------------------------------------------------
void vtkVRInteractorStyle::OnMenu3D(vtkEventData* edata)
{
  vtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();
  if (!edd)
  {
    return;
  }

  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];
  this->FindPokedRenderer(x, y);

  if (edd->GetAction() == vtkEventDataAction::Press)
  {
    this->StartAction(VTKIS_MENU, edd);
    return;
  }

  if (edd->GetAction() == vtkEventDataAction::Release)
  {
    this->EndAction(VTKIS_MENU, edd);
    return;
  }
}

//------------------------------------------------------------------------------
// Interaction entry points
//------------------------------------------------------------------------------
void vtkVRInteractorStyle::StartPick(vtkEventDataDevice3D* edata)
{
  this->HideBillboard();
  this->HidePickActor();

  this->InteractionState[static_cast<int>(edata->GetDevice())] = VTKIS_PICK;

  // update ray
  this->UpdateRay(edata->GetDevice());
}

//------------------------------------------------------------------------------
void vtkVRInteractorStyle::EndPick(vtkEventDataDevice3D* edata)
{
  // perform probe
  this->ProbeData(edata->GetDevice());

  this->InteractionState[static_cast<int>(edata->GetDevice())] = VTKIS_NONE;

  // Update ray
  this->UpdateRay(edata->GetDevice());
}

//------------------------------------------------------------------------------
void vtkVRInteractorStyle::StartLoadCamPose(vtkEventDataDevice3D* edata)
{
  int iDevice = static_cast<int>(edata->GetDevice());
  this->InteractionState[iDevice] = VTKIS_LOAD_CAMERA_POSE;
}

//------------------------------------------------------------------------------
void vtkVRInteractorStyle::EndLoadCamPose(vtkEventDataDevice3D* edata)
{
  this->LoadNextCameraPose();

  int iDevice = static_cast<int>(edata->GetDevice());
  this->InteractionState[iDevice] = VTKIS_NONE;
}

//------------------------------------------------------------------------------
void vtkVRInteractorStyle::StartPositionProp(vtkEventDataDevice3D* edata)
{
  if (this->GrabWithRay)
  {
    if (!this->HardwareSelect(edata->GetDevice(), true))
    {
      return;
    }

    vtkSelection* selection = this->HardwarePicker->GetSelection();

    if (!selection || selection->GetNumberOfNodes() == 0)
    {
      return;
    }

    vtkSelectionNode* node = selection->GetNode(0);
    this->InteractionProp =
      vtkProp3D::SafeDownCast(node->GetProperties()->Get(vtkSelectionNode::PROP()));
  }
  else
  {
    double pos[3];
    edata->GetWorldPosition(pos);
    this->FindPickedActor(pos, nullptr);
  }

  if (this->InteractionProp == nullptr)
  {
    return;
  }

  this->InteractionState[static_cast<int>(edata->GetDevice())] = VTKIS_POSITION_PROP;
  this->InteractionProps[static_cast<int>(edata->GetDevice())] = this->InteractionProp;

  // Don't start action if a controller is already positioning the prop
  int rc = static_cast<int>(vtkEventDataDevice::RightController);
  int lc = static_cast<int>(vtkEventDataDevice::LeftController);
  if (this->InteractionProps[rc] == this->InteractionProps[lc])
  {
    this->EndPositionProp(edata);
    return;
  }
}

//------------------------------------------------------------------------------
void vtkVRInteractorStyle::EndPositionProp(vtkEventDataDevice3D* edata)
{
  vtkEventDataDevice dev = edata->GetDevice();
  this->InteractionState[static_cast<int>(dev)] = VTKIS_NONE;
  this->InteractionProps[static_cast<int>(dev)] = nullptr;
}

//------------------------------------------------------------------------------
void vtkVRInteractorStyle::StartClip(vtkEventDataDevice3D* ed)
{
  if (this->CurrentRenderer == nullptr)
  {
    return;
  }

  vtkEventDataDevice dev = ed->GetDevice();
  this->InteractionState[static_cast<int>(dev)] = VTKIS_CLIP;

  if (!this->ClippingPlanes[static_cast<int>(dev)])
  {
    this->ClippingPlanes[static_cast<int>(dev)] = vtkSmartPointer<vtkPlane>::New();
  }

  vtkActorCollection* ac;
  vtkActor *anActor, *aPart;
  vtkAssemblyPath* path;
  if (this->CurrentRenderer != nullptr)
  {
    ac = this->CurrentRenderer->GetActors();
    vtkCollectionSimpleIterator ait;
    for (ac->InitTraversal(ait); (anActor = ac->GetNextActor(ait));)
    {
      for (anActor->InitPathTraversal(); (path = anActor->GetNextPath());)
      {
        aPart = static_cast<vtkActor*>(path->GetLastNode()->GetViewProp());
        if (aPart->GetMapper())
        {
          aPart->GetMapper()->AddClippingPlane(this->ClippingPlanes[static_cast<int>(dev)]);
          continue;
        }
      }
    }
  }
  else
  {
    vtkWarningMacro(<< "no current renderer on the interactor style.");
  }
}

//------------------------------------------------------------------------------
void vtkVRInteractorStyle::EndClip(vtkEventDataDevice3D* ed)
{
  vtkEventDataDevice dev = ed->GetDevice();
  this->InteractionState[static_cast<int>(dev)] = VTKIS_NONE;

  vtkActorCollection* ac;
  vtkActor *anActor, *aPart;
  vtkAssemblyPath* path;
  if (this->CurrentRenderer != nullptr)
  {
    ac = this->CurrentRenderer->GetActors();
    vtkCollectionSimpleIterator ait;
    for (ac->InitTraversal(ait); (anActor = ac->GetNextActor(ait));)
    {
      for (anActor->InitPathTraversal(); (path = anActor->GetNextPath());)
      {
        aPart = static_cast<vtkActor*>(path->GetLastNode()->GetViewProp());
        if (aPart->GetMapper())
        {
          aPart->GetMapper()->RemoveClippingPlane(this->ClippingPlanes[static_cast<int>(dev)]);
          continue;
        }
      }
    }
  }
  else
  {
    vtkWarningMacro(<< "no current renderer on the interactor style.");
  }
}

//------------------------------------------------------------------------------
void vtkVRInteractorStyle::StartMovement3D(int interactionState, vtkEventDataDevice3D* ed)
{
  if (this->CurrentRenderer == nullptr)
  {
    return;
  }
  vtkEventDataDevice dev = ed->GetDevice();
  this->InteractionState[static_cast<int>(dev)] = interactionState;
}

//------------------------------------------------------------------------------
void vtkVRInteractorStyle::EndMovement3D(vtkEventDataDevice3D* ed)
{
  vtkEventDataDevice dev = ed->GetDevice();
  this->InteractionState[static_cast<int>(dev)] = VTKIS_NONE;
}

//------------------------------------------------------------------------------
// Complex gesture interaction methods
//------------------------------------------------------------------------------
void vtkVRInteractorStyle::OnPan()
{
  int rc = static_cast<int>(vtkEventDataDevice::RightController);
  int lc = static_cast<int>(vtkEventDataDevice::LeftController);

  if (!this->InteractionProps[rc] && !this->InteractionProps[lc])
  {
    this->InteractionState[rc] = VTKIS_PAN;
    this->InteractionState[lc] = VTKIS_PAN;

    int pointer = this->Interactor->GetPointerIndex();

    this->FindPokedRenderer(this->Interactor->GetEventPositions(pointer)[0],
      this->Interactor->GetEventPositions(pointer)[1]);

    if (this->CurrentRenderer == nullptr)
    {
      return;
    }

    vtkCamera* camera = this->CurrentRenderer->GetActiveCamera();
    vtkRenderWindowInteractor3D* rwi = static_cast<vtkRenderWindowInteractor3D*>(this->Interactor);

    double t[3] = { rwi->GetTranslation3D()[0] - rwi->GetLastTranslation3D()[0],
      rwi->GetTranslation3D()[1] - rwi->GetLastTranslation3D()[1],
      rwi->GetTranslation3D()[2] - rwi->GetLastTranslation3D()[2] };

    double* ptrans = rwi->GetPhysicalTranslation(camera);

    rwi->SetPhysicalTranslation(camera, ptrans[0] + t[0], ptrans[1] + t[1], ptrans[2] + t[2]);

    // clean up
    if (this->Interactor->GetLightFollowCamera())
    {
      this->CurrentRenderer->UpdateLightsGeometryToFollowCamera();
    }
  }
}

//------------------------------------------------------------------------------
void vtkVRInteractorStyle::OnPinch()
{
  int rc = static_cast<int>(vtkEventDataDevice::RightController);
  int lc = static_cast<int>(vtkEventDataDevice::LeftController);

  if (!this->InteractionProps[rc] && !this->InteractionProps[lc])
  {
    this->InteractionState[rc] = VTKIS_ZOOM;
    this->InteractionState[lc] = VTKIS_ZOOM;

    int pointer = this->Interactor->GetPointerIndex();

    this->FindPokedRenderer(this->Interactor->GetEventPositions(pointer)[0],
      this->Interactor->GetEventPositions(pointer)[1]);

    if (this->CurrentRenderer == nullptr)
    {
      return;
    }

    double dyf = this->Interactor->GetScale() / this->Interactor->GetLastScale();
    vtkCamera* camera = this->CurrentRenderer->GetActiveCamera();
    vtkRenderWindowInteractor3D* rwi = static_cast<vtkRenderWindowInteractor3D*>(this->Interactor);
    double physicalScale = rwi->GetPhysicalScale();

    this->SetScale(camera, physicalScale / dyf);
  }
}

//------------------------------------------------------------------------------
void vtkVRInteractorStyle::OnRotate()
{
  int rc = static_cast<int>(vtkEventDataDevice::RightController);
  int lc = static_cast<int>(vtkEventDataDevice::LeftController);

  // Rotate only when one controller is not interacting
  if (!this->InteractionProps[rc] && !this->InteractionProps[lc])
  {
    this->InteractionState[rc] = VTKIS_ROTATE;
    this->InteractionState[lc] = VTKIS_ROTATE;

    double angle = this->Interactor->GetRotation() - this->Interactor->GetLastRotation();

    // rotate the world, aka rotate the physicalViewDirection about the physicalViewUp
    vtkVRRenderWindow* renWin =
      vtkVRRenderWindow::SafeDownCast(this->Interactor->GetRenderWindow());
    if (!renWin)
    {
      return;
    }

    double* vup = renWin->GetPhysicalViewUp();
    double* dop = renWin->GetPhysicalViewDirection();
    double newDOP[3];
    double wxyz[4];
    wxyz[0] = vtkMath::RadiansFromDegrees(angle);
    wxyz[1] = vup[0];
    wxyz[2] = vup[1];
    wxyz[3] = vup[2];
    vtkMath::RotateVectorByWXYZ(dop, wxyz, newDOP);
    renWin->SetPhysicalViewDirection(newDOP);
  }
}

//------------------------------------------------------------------------------
// Interaction methods
//------------------------------------------------------------------------------
void vtkVRInteractorStyle::ProbeData(vtkEventDataDevice controller)
{
  // Invoke start pick method if defined
  this->InvokeEvent(vtkCommand::StartPickEvent, nullptr);

  if (!this->HardwareSelect(controller, false))
  {
    return;
  }

  // Invoke end pick method if defined
  if (this->HandleObservers && this->HasObserver(vtkCommand::EndPickEvent))
  {
    this->InvokeEvent(vtkCommand::EndPickEvent, this->HardwarePicker->GetSelection());
  }
  else
  {
    this->EndPickCallback(this->HardwarePicker->GetSelection());
  }
}

//------------------------------------------------------------------------------
void vtkVRInteractorStyle::PositionProp(vtkEventData* ed, double* lwpos, double* lwori)
{
  if (this->InteractionProp == nullptr || !this->InteractionProp->GetDragable())
  {
    return;
  }
  this->Superclass::PositionProp(ed, lwpos, lwori);
}

//------------------------------------------------------------------------------
void vtkVRInteractorStyle::Clip(vtkEventDataDevice3D* ed)
{
  if (this->CurrentRenderer == nullptr)
  {
    return;
  }

  const double* wpos = ed->GetWorldPosition();
  const double* wori = ed->GetWorldOrientation();

  double ori[4];
  ori[0] = vtkMath::RadiansFromDegrees(wori[0]);
  ori[1] = wori[1];
  ori[2] = wori[2];
  ori[3] = wori[3];

  // we have a position and a normal, that defines our plane
  double r[3];
  double up[3];
  up[0] = 0;
  up[1] = -1;
  up[2] = 0;
  vtkMath::RotateVectorByWXYZ(up, ori, r);

  vtkEventDataDevice dev = ed->GetDevice();
  int idev = static_cast<int>(dev);
  this->ClippingPlanes[idev]->SetNormal(r);
  this->ClippingPlanes[idev]->SetOrigin(wpos[0], wpos[1], wpos[2]);
}

//------------------------------------------------------------------------------
void vtkVRInteractorStyle::GroundMovement3D(vtkEventDataDevice3D* edd)
{
  if (this->CurrentRenderer == nullptr)
  {
    return;
  }

  vtkVRRenderWindowInteractor* rwi = vtkVRRenderWindowInteractor::SafeDownCast(this->Interactor);

  // Get joystick position
  if (edd->GetType() == vtkCommand::ViewerMovement3DEvent)
  {
    edd->GetTrackPadPosition(this->LastGroundMovementTrackPadPosition);
  }

  // Get current translation of the scene
  double* sceneTrans = rwi->GetPhysicalTranslation(this->CurrentRenderer->GetActiveCamera());

  // Get the physical view up vector (in world coordinates)
  double* physicalViewUp = rwi->GetPhysicalViewUp();
  vtkMath::Normalize(physicalViewUp);

  this->LastGroundMovement3DEventTime->StopTimer();

  // Compute travelled distance during elapsed time
  double physicalScale = rwi->GetPhysicalScale();
  double distanceTravelledWorld = this->DollyPhysicalSpeed * /* m/sec */
    physicalScale *                                          /* world/physical */
    this->LastGroundMovement3DEventTime->GetElapsedTime();   /* sec */

  this->LastGroundMovement3DEventTime->StartTimer();

  // Get the translation according to the headset view direction vector
  // projected on the "XY" (ground) plan.
  double viewTrans[3] = { physicalViewUp[0], physicalViewUp[1], physicalViewUp[2] };
  vtkMath::MultiplyScalar(viewTrans, vtkMath::Dot(this->HeadsetDir, physicalViewUp));
  vtkMath::Subtract(this->HeadsetDir, viewTrans, viewTrans);
  vtkMath::Normalize(viewTrans);

  // Get the translation according to the headset "right" direction vector
  // projected on the "XY" (ground) plan.
  double rightTrans[3] = { 0.0, 0.0, 0.0 };
  vtkMath::Cross(viewTrans, physicalViewUp, rightTrans);
  vtkMath::Normalize(rightTrans);

  // Scale the view direction translation according to the up / down thumbstick position.
  double scaledDistanceViewDir =
    this->LastGroundMovementTrackPadPosition[1] * distanceTravelledWorld;
  vtkMath::MultiplyScalar(viewTrans, scaledDistanceViewDir);

  // Scale the right direction translation according to the left / right thumbstick position.
  double scaledDistanceRightDir =
    this->LastGroundMovementTrackPadPosition[0] * distanceTravelledWorld;
  vtkMath::MultiplyScalar(rightTrans, scaledDistanceRightDir);

  // Compute and set new translation of the scene
  double newSceneTrans[3] = { 0.0, 0.0, 0.0 };
  vtkMath::Add(viewTrans, rightTrans, newSceneTrans);
  vtkMath::Subtract(sceneTrans, newSceneTrans, newSceneTrans);
  rwi->SetPhysicalTranslation(
    this->CurrentRenderer->GetActiveCamera(), newSceneTrans[0], newSceneTrans[1], newSceneTrans[2]);

  if (this->AutoAdjustCameraClippingRange)
  {
    this->CurrentRenderer->ResetCameraClippingRange();
  }
}

//------------------------------------------------------------------------------
void vtkVRInteractorStyle::Elevation3D(vtkEventDataDevice3D* edd)
{
  if (this->CurrentRenderer == nullptr)
  {
    return;
  }

  vtkVRRenderWindowInteractor* rwi = vtkVRRenderWindowInteractor::SafeDownCast(this->Interactor);

  // Get joystick position
  if (edd->GetType() == vtkCommand::Elevation3DEvent)
  {
    edd->GetTrackPadPosition(this->LastElevationTrackPadPosition);
  }

  // Get current translation of the scene
  double* sceneTrans = rwi->GetPhysicalTranslation(this->CurrentRenderer->GetActiveCamera());

  // Get the physical view up vector (in world coordinates)
  double* physicalViewUp = rwi->GetPhysicalViewUp();
  vtkMath::Normalize(physicalViewUp);

  this->LastElevation3DEventTime->StopTimer();

  // Compute travelled distance during elapsed time
  double physicalScale = rwi->GetPhysicalScale();
  double distanceTravelledWorld = this->DollyPhysicalSpeed * /* m/sec */
    physicalScale *                                          /* world/physical */
    this->LastElevation3DEventTime->GetElapsedTime();        /* sec */

  this->LastElevation3DEventTime->StartTimer();

  // Get the translation according to the "Z" (up) world coordinates axis,
  // scaled according to the up / down thumbstick position.
  double scaledDistance = this->LastElevationTrackPadPosition[1] * distanceTravelledWorld;
  double upTrans[3] = { physicalViewUp[0], physicalViewUp[1], physicalViewUp[2] };
  vtkMath::MultiplyScalar(upTrans, scaledDistance);

  // Compute and set new translation of the scene
  double newSceneTrans[3] = { 0.0, 0.0, 0.0 };
  vtkMath::Subtract(sceneTrans, upTrans, newSceneTrans);
  rwi->SetPhysicalTranslation(
    this->CurrentRenderer->GetActiveCamera(), newSceneTrans[0], newSceneTrans[1], newSceneTrans[2]);

  if (this->AutoAdjustCameraClippingRange)
  {
    this->CurrentRenderer->ResetCameraClippingRange();
  }
}

//------------------------------------------------------------------------------
// Utility routines
//------------------------------------------------------------------------------
void vtkVRInteractorStyle::MapInputToAction(
  vtkCommand::EventIds eid, vtkEventDataAction action, int state)
{
  if (state < VTKIS_NONE)
  {
    return;
  }

  decltype(this->InputMap)::key_type key(eid, action);
  auto it = this->InputMap.find(key);
  if (it != this->InputMap.end())
  {
    if (it->second == state)
    {
      return;
    }
  }

  this->InputMap[key] = state;

  this->Modified();
}

//------------------------------------------------------------------------------
void vtkVRInteractorStyle::MapInputToAction(vtkCommand::EventIds eid, int state)
{
  this->MapInputToAction(eid, vtkEventDataAction::Press, state);
  this->MapInputToAction(eid, vtkEventDataAction::Release, state);
}

//------------------------------------------------------------------------------
void vtkVRInteractorStyle::StartAction(int state, vtkEventDataDevice3D* edata)
{
  switch (state)
  {
    case VTKIS_POSITION_PROP:
      this->StartPositionProp(edata);
      break;
    case VTKIS_DOLLY:
      this->StartMovement3D(state, edata);
      this->LastDolly3DEventTime->StartTimer();
      break;
    case VTKIS_GROUNDMOVEMENT:
      this->StartMovement3D(state, edata);
      this->LastGroundMovement3DEventTime->StartTimer();
      break;
    case VTKIS_ELEVATION:
      this->StartMovement3D(state, edata);
      this->LastElevation3DEventTime->StartTimer();
      break;
    case VTKIS_CLIP:
      this->StartClip(edata);
      break;
    case VTKIS_PICK:
      this->StartPick(edata);
      break;
    case VTKIS_LOAD_CAMERA_POSE:
      this->StartLoadCamPose(edata);
      break;
    case VTKIS_MENU:
      // Menu is only displayed upon action end (e.g. button release)
      break;
    default:
      vtkDebugMacro(<< "StartAction: unknown state " << state);
      break;
  }
}

//------------------------------------------------------------------------------
void vtkVRInteractorStyle::EndAction(int state, vtkEventDataDevice3D* edata)
{
  switch (state)
  {
    case VTKIS_POSITION_PROP:
      this->EndPositionProp(edata);
      break;
    case VTKIS_DOLLY:
      this->EndMovement3D(edata);
      this->LastDolly3DEventTime->StopTimer();
      break;
    case VTKIS_GROUNDMOVEMENT:
      this->EndMovement3D(edata);
      this->LastGroundMovement3DEventTime->StopTimer();
      break;
    case VTKIS_ELEVATION:
      this->EndMovement3D(edata);
      this->LastElevation3DEventTime->StopTimer();
      break;
    case VTKIS_CLIP:
      this->EndClip(edata);
      break;
    case VTKIS_PICK:
      this->EndPick(edata);
      break;
    case VTKIS_MENU:
      this->Menu->SetInteractor(this->Interactor);
      this->Menu->Show(edata);
      break;
    case VTKIS_LOAD_CAMERA_POSE:
      this->EndLoadCamPose(edata);
      break;
    case VTKIS_TOGGLE_DRAW_CONTROLS:
      this->ToggleDrawControls();
      break;
    case VTKIS_EXIT:
      if (this->Interactor)
      {
        this->Interactor->ExitCallback();
      }
      break;
    default:
      vtkDebugMacro(<< "EndAction: unknown state " << state);
      break;
  }

  // Reset complex gesture state because a button has been released
  for (int d = 0; d < vtkEventDataNumberOfDevices; ++d)
  {
    switch (this->InteractionState[d])
    {
      case VTKIS_PAN:
      case VTKIS_ZOOM:
      case VTKIS_ROTATE:
        this->InteractionState[d] = VTKIS_NONE;
        break;
      default:
        vtkDebugMacro(<< "EndAction: unknown interaction state " << d << ": "
                      << this->InteractionState[d]);
        break;
    }
  }
}

//------------------------------------------------------------------------------
void vtkVRInteractorStyle::AddTooltipForInput(
  vtkEventDataDevice device, vtkEventDataDeviceInput input, const std::string& text)
{
  int iInput = static_cast<int>(input);
  int iDevice = static_cast<int>(device);

  std::string controlName;
  std::string controlText;
  int drawSide = -1;
  int buttonSide = -1;

  // Setup default text and layout
  switch (input)
  {
    case vtkEventDataDeviceInput::Trigger:
      controlName = "trigger";
      drawSide = vtkVRControlsHelper::Left;
      buttonSide = vtkVRControlsHelper::Back;
      controlText = "Trigger :\n";
      break;
    case vtkEventDataDeviceInput::TrackPad:
      controlName = "trackpad";
      drawSide = vtkVRControlsHelper::Right;
      buttonSide = vtkVRControlsHelper::Front;
      controlText = "Trackpad :\n";
      break;
    case vtkEventDataDeviceInput::Grip:
      controlName = "lgrip";
      drawSide = vtkVRControlsHelper::Right;
      buttonSide = vtkVRControlsHelper::Back;
      controlText = "Grip :\n";
      break;
    case vtkEventDataDeviceInput::ApplicationMenu:
      controlName = "button";
      drawSide = vtkVRControlsHelper::Left;
      buttonSide = vtkVRControlsHelper::Front;
      controlText = "Application Menu :\n";
      break;
    default:
      vtkWarningMacro(<< "AddTooltipForInput: unknown input type " << static_cast<int>(input));
      break;
  }

  controlText += text;

  // Clean already existing helpers
  if (this->ControlsHelpers[iDevice][iInput] != nullptr)
  {
    if (this->CurrentRenderer)
    {
      this->CurrentRenderer->RemoveViewProp(this->ControlsHelpers[iDevice][iInput]);
    }
    this->ControlsHelpers[iDevice][iInput]->Delete();
    this->ControlsHelpers[iDevice][iInput] = nullptr;
  }

  // Create an input helper and add it to the renderer
  vtkVRControlsHelper* inputHelper = this->MakeControlsHelper();
  inputHelper->SetTooltipInfo(controlName.c_str(), buttonSide, drawSide, controlText.c_str());

  this->ControlsHelpers[iDevice][iInput] = inputHelper;
  this->ControlsHelpers[iDevice][iInput]->SetDevice(device);

  if (this->CurrentRenderer)
  {
    this->ControlsHelpers[iDevice][iInput]->SetRenderer(this->CurrentRenderer);
    this->ControlsHelpers[iDevice][iInput]->BuildRepresentation();
    this->CurrentRenderer->AddViewProp(this->ControlsHelpers[iDevice][iInput]);
  }
}

//------------------------------------------------------------------------------
// Handle Ray drawing and update
//------------------------------------------------------------------------------
void vtkVRInteractorStyle::ShowRay(vtkEventDataDevice controller)
{
  vtkVRRenderWindow* renWin = vtkVRRenderWindow::SafeDownCast(this->Interactor->GetRenderWindow());
  if (!renWin ||
    (controller != vtkEventDataDevice::LeftController &&
      controller != vtkEventDataDevice::RightController))
  {
    return;
  }

  vtkVRModel* cmodel = renWin->GetModelForDevice(controller);
  if (cmodel)
  {
    cmodel->SetShowRay(true);
  }
}

//------------------------------------------------------------------------------
void vtkVRInteractorStyle::HideRay(vtkEventDataDevice controller)
{
  vtkVRRenderWindow* renWin = vtkVRRenderWindow::SafeDownCast(this->Interactor->GetRenderWindow());
  if (!renWin ||
    (controller != vtkEventDataDevice::LeftController &&
      controller != vtkEventDataDevice::RightController))
  {
    return;
  }

  vtkVRModel* cmodel = renWin->GetModelForDevice(controller);
  if (cmodel)
  {
    cmodel->SetShowRay(false);
  }
}

//------------------------------------------------------------------------------
void vtkVRInteractorStyle::UpdateRay(vtkEventDataDevice controller)
{
  if (!this->Interactor)
  {
    return;
  }

  vtkRenderer* ren = this->CurrentRenderer;
  vtkVRRenderWindow* renWin = vtkVRRenderWindow::SafeDownCast(this->Interactor->GetRenderWindow());
  vtkVRRenderWindowInteractor* iren = vtkVRRenderWindowInteractor::SafeDownCast(this->Interactor);

  if (!ren || !renWin || !iren)
  {
    return;
  }

  vtkVRModel* mod = renWin->GetModelForDevice(controller);
  if (!mod)
  {
    return;
  }

  int idev = static_cast<int>(controller);

  // Keep the same ray if a controller is interacting with a prop
  if (this->InteractionProps[idev] != nullptr)
  {
    return;
  }

  // Check if interacting with a widget
  vtkPropCollection* props = ren->GetViewProps();

  vtkIdType nbProps = props->GetNumberOfItems();
  for (vtkIdType i = 0; i < nbProps; i++)
  {
    vtkWidgetRepresentation* rep = vtkWidgetRepresentation::SafeDownCast(props->GetItemAsObject(i));

    if (rep && rep->IsA("vtkQWidgetRepresentation") && rep->GetInteractionState() != 0)
    {
      mod->SetShowRay(true);
      mod->SetRayLength(ren->GetActiveCamera()->GetClippingRange()[1]);
      mod->SetRayColor(0.0, 0.0, 1.0);
      return;
    }
  }

  if (this->GetGrabWithRay() || this->InteractionState[idev] == VTKIS_PICK)
  {
    mod->SetShowRay(true);
  }
  else
  {
    mod->SetShowRay(false);
    return;
  }

  // Set length to its max if interactive picking is off
  if (!this->HoverPick)
  {
    mod->SetRayColor(1.0, 0.0, 0.0);
    mod->SetRayLength(ren->GetActiveCamera()->GetClippingRange()[1]);
    return;
  }

  // Compute controller position and world orientation
  double p0[3];   // Ray start point
  double wxyz[4]; // Controller orientation
  double dummy_ppos[3];
  double wdir[3];

  vtkMatrix4x4* devicePose = renWin->GetDeviceToPhysicalMatrixForDevice(controller);

  if (!devicePose)
  {
    return;
  }

  iren->ConvertPoseToWorldCoordinates(devicePose, p0, wxyz, dummy_ppos, wdir);

  // Compute ray length.
  this->InteractionPicker->Pick3DRay(p0, wxyz, ren);

  // If something is picked, set the length accordingly
  vtkProp3D* prop = this->InteractionPicker->GetProp3D();
  if (prop)
  {
    double p1[3];
    this->InteractionPicker->GetPickPosition(p1);
    mod->SetRayLength(sqrt(vtkMath::Distance2BetweenPoints(p0, p1)));
    mod->SetRayColor(0.0, 1.0, 0.0);
  }
  // Otherwise set the length to its max
  else
  {
    mod->SetRayLength(ren->GetActiveCamera()->GetClippingRange()[1]);
    mod->SetRayColor(1.0, 0.0, 0.0);
  }
}

//------------------------------------------------------------------------------
void vtkVRInteractorStyle::ShowBillboard(const std::string& text)
{
  vtkVRRenderWindow* renWin = vtkVRRenderWindow::SafeDownCast(this->Interactor->GetRenderWindow());
  vtkRenderer* ren = this->CurrentRenderer;
  if (!renWin || !ren)
  {
    return;
  }

  renWin->UpdateHMDMatrixPose();
  double dop[3];
  ren->GetActiveCamera()->GetDirectionOfProjection(dop);
  double vr[3];
  double* vup = renWin->GetPhysicalViewUp();
  double dtmp[3];
  double vupdot = vtkMath::Dot(dop, vup);
  if (fabs(vupdot) < 0.999)
  {
    dtmp[0] = dop[0] - vup[0] * vupdot;
    dtmp[1] = dop[1] - vup[1] * vupdot;
    dtmp[2] = dop[2] - vup[2] * vupdot;
    vtkMath::Normalize(dtmp);
  }
  else
  {
    renWin->GetPhysicalViewDirection(dtmp);
  }
  vtkMath::Cross(dtmp, vup, vr);
  vtkNew<vtkMatrix4x4> rot;
  for (int i = 0; i < 3; ++i)
  {
    rot->SetElement(0, i, vr[i]);
    rot->SetElement(1, i, vup[i]);
    rot->SetElement(2, i, -dtmp[i]);
  }
  rot->Transpose();
  double orient[3];
  vtkTransform::GetOrientation(orient, rot);
  vtkTextProperty* prop = this->TextActor3D->GetTextProperty();
  this->TextActor3D->SetOrientation(orient);
  this->TextActor3D->RotateX(-30.0);

  double tpos[3];
  double scale = renWin->GetPhysicalScale();
  ren->GetActiveCamera()->GetPosition(tpos);
  tpos[0] += (0.7 * scale * dop[0] - 0.1 * scale * vr[0] - 0.4 * scale * vup[0]);
  tpos[1] += (0.7 * scale * dop[1] - 0.1 * scale * vr[1] - 0.4 * scale * vup[1]);
  tpos[2] += (0.7 * scale * dop[2] - 0.1 * scale * vr[2] - 0.4 * scale * vup[2]);
  this->TextActor3D->SetPosition(tpos);
  // scale should cover 10% of FOV
  double fov = ren->GetActiveCamera()->GetViewAngle();
  double tsize = 0.1 * 2.0 * atan(fov * 0.5); // 10% of fov
  tsize /= 200.0;                             // about 200 pixel texture map
  scale *= tsize;
  this->TextActor3D->SetScale(scale, scale, scale);
  this->TextActor3D->SetInput(text.c_str());
  this->CurrentRenderer->AddActor(this->TextActor3D);

  prop->SetFrame(1);
  prop->SetFrameColor(1.0, 1.0, 1.0);
  prop->SetBackgroundOpacity(1.0);
  prop->SetBackgroundColor(0.0, 0.0, 0.0);
  prop->SetFontSize(14);
}

//------------------------------------------------------------------------------
void vtkVRInteractorStyle::HideBillboard()
{
  this->CurrentRenderer->RemoveActor(this->TextActor3D);
}

//------------------------------------------------------------------------------
void vtkVRInteractorStyle::ShowPickSphere(double* pos, double radius, vtkProp3D* prop)
{
  this->PickActor->GetProperty()->SetColor(this->PickColor);

  this->Sphere->SetCenter(pos);
  this->Sphere->SetRadius(radius);
  this->PickActor->GetMapper()->SetInputConnection(this->Sphere->GetOutputPort());
  if (prop)
  {
    this->PickActor->SetPosition(prop->GetPosition());
    this->PickActor->SetScale(prop->GetScale());
  }
  else
  {
    this->PickActor->SetPosition(0.0, 0.0, 0.0);
    this->PickActor->SetScale(1.0, 1.0, 1.0);
  }
  this->CurrentRenderer->AddActor(this->PickActor);
}

//------------------------------------------------------------------------------
void vtkVRInteractorStyle::ShowPickCell(vtkCell* cell, vtkProp3D* prop)
{
  vtkNew<vtkPolyData> pd;
  vtkNew<vtkPoints> pdpts;
  pdpts->SetDataTypeToDouble();
  vtkNew<vtkCellArray> lines;

  this->PickActor->GetProperty()->SetColor(this->PickColor);

  int nedges = cell->GetNumberOfEdges();

  if (nedges)
  {
    for (int edgenum = 0; edgenum < nedges; ++edgenum)
    {
      vtkCell* edge = cell->GetEdge(edgenum);
      vtkPoints* pts = edge->GetPoints();
      int npts = edge->GetNumberOfPoints();
      lines->InsertNextCell(npts);
      for (int ep = 0; ep < npts; ++ep)
      {
        vtkIdType newpt = pdpts->InsertNextPoint(pts->GetPoint(ep));
        lines->InsertCellPoint(newpt);
      }
    }
  }
  else if (cell->GetCellType() == VTK_LINE || cell->GetCellType() == VTK_POLY_LINE)
  {
    vtkPoints* pts = cell->GetPoints();
    int npts = cell->GetNumberOfPoints();
    lines->InsertNextCell(npts);
    for (int ep = 0; ep < npts; ++ep)
    {
      vtkIdType newpt = pdpts->InsertNextPoint(pts->GetPoint(ep));
      lines->InsertCellPoint(newpt);
    }
  }
  else
  {
    return;
  }

  pd->SetPoints(pdpts.Get());
  pd->SetLines(lines.Get());

  if (prop)
  {
    this->PickActor->SetPosition(prop->GetPosition());
    this->PickActor->SetScale(prop->GetScale());
    this->PickActor->SetUserMatrix(prop->GetUserMatrix());
  }
  else
  {
    this->PickActor->SetPosition(0.0, 0.0, 0.0);
    this->PickActor->SetScale(1.0, 1.0, 1.0);
  }
  this->PickActor->SetOrientation(prop->GetOrientation());
  static_cast<vtkPolyDataMapper*>(this->PickActor->GetMapper())->SetInputData(pd);
  this->CurrentRenderer->AddActor(this->PickActor);
}

//------------------------------------------------------------------------------
void vtkVRInteractorStyle::HidePickActor()
{
  if (this->CurrentRenderer)
  {
    this->CurrentRenderer->RemoveActor(this->PickActor);
  }
}

//------------------------------------------------------------------------------
void vtkVRInteractorStyle::ToggleDrawControls()
{
  if (this->CurrentRenderer == nullptr)
  {
    return;
  }

  // Enable helpers
  for (int d = 0; d < vtkEventDataNumberOfDevices; ++d)
  {
    // No helper for HMD
    if (static_cast<vtkEventDataDevice>(d) == vtkEventDataDevice::HeadMountedDisplay)
    {
      continue;
    }

    for (int i = 0; i < vtkEventDataNumberOfInputs; i++)
    {
      if (this->ControlsHelpers[d][i])
      {
        if (this->ControlsHelpers[d][i]->GetRenderer() != this->CurrentRenderer)
        {
          vtkRenderer* ren = this->ControlsHelpers[d][i]->GetRenderer();
          if (ren)
          {
            ren->RemoveViewProp(this->ControlsHelpers[d][i]);
          }
          this->ControlsHelpers[d][i]->SetRenderer(this->CurrentRenderer);
          this->ControlsHelpers[d][i]->BuildRepresentation();
          this->CurrentRenderer->AddViewProp(this->ControlsHelpers[d][i]);
        }

        this->ControlsHelpers[d][i]->SetEnabled(!this->ControlsHelpers[d][i]->GetEnabled());
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkVRInteractorStyle::SetDrawControls(bool val)
{
  if (this->CurrentRenderer == nullptr)
  {
    return;
  }

  // Enable helpers
  for (int d = 0; d < vtkEventDataNumberOfDevices; ++d)
  {
    // No helper for HMD
    if (static_cast<vtkEventDataDevice>(d) == vtkEventDataDevice::HeadMountedDisplay)
    {
      continue;
    }

    for (int i = 0; i < vtkEventDataNumberOfInputs; i++)
    {
      if (this->ControlsHelpers[d][i])
      {
        if (this->ControlsHelpers[d][i]->GetRenderer() != this->CurrentRenderer)
        {
          vtkRenderer* ren = this->ControlsHelpers[d][i]->GetRenderer();
          if (ren)
          {
            ren->RemoveViewProp(this->ControlsHelpers[d][i]);
          }
          this->ControlsHelpers[d][i]->SetRenderer(this->CurrentRenderer);
          this->ControlsHelpers[d][i]->BuildRepresentation();
          this->CurrentRenderer->AddViewProp(this->ControlsHelpers[d][i]);
        }

        this->ControlsHelpers[d][i]->SetEnabled(val);
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkVRInteractorStyle::SetInteractor(vtkRenderWindowInteractor* iren)
{
  this->Superclass::SetInteractor(iren);

  if (iren)
  {
    this->SetupActions(iren);
  }
}

//------------------------------------------------------------------------------
void vtkVRInteractorStyle::EndPickCallback(vtkSelection* sel)
{
  if (!sel)
  {
    return;
  }

  vtkSelectionNode* node = sel->GetNode(0);
  if (!node || !node->GetProperties()->Has(vtkSelectionNode::PROP()))
  {
    return;
  }

  vtkProp3D* prop = vtkProp3D::SafeDownCast(node->GetProperties()->Get(vtkSelectionNode::PROP()));
  if (!prop)
  {
    return;
  }
  this->ShowPickSphere(prop->GetCenter(), prop->GetLength() / 2.0, nullptr);
}

//------------------------------------------------------------------------------
void vtkVRInteractorStyle::MenuCallback(
  vtkObject* vtkNotUsed(object), unsigned long, void* clientdata, void* calldata)
{
  std::string name = static_cast<const char*>(calldata);
  vtkVRInteractorStyle* self = static_cast<vtkVRInteractorStyle*>(clientdata);

  if (name == "exit")
  {
    if (self->Interactor)
    {
      self->Interactor->ExitCallback();
    }
  }
  if (name == "togglelabel")
  {
    self->ToggleDrawControls();
  }
  if (name == "clipmode")
  {
    self->MapInputToAction(vtkCommand::Select3DEvent, VTKIS_CLIP);
  }
  if (name == "grabmode")
  {
    self->MapInputToAction(vtkCommand::Select3DEvent, VTKIS_POSITION_PROP);
  }
  if (name == "probemode")
  {
    self->MapInputToAction(vtkCommand::Select3DEvent, VTKIS_PICK);
  }
}

//------------------------------------------------------------------------------
bool vtkVRInteractorStyle::HardwareSelect(vtkEventDataDevice controller, bool actorPassOnly)
{
  vtkRenderer* ren = this->CurrentRenderer;
  vtkVRRenderWindow* renWin = vtkVRRenderWindow::SafeDownCast(this->Interactor->GetRenderWindow());
  vtkVRRenderWindowInteractor* iren = static_cast<vtkVRRenderWindowInteractor*>(this->Interactor);

  if (!ren || !renWin || !iren)
  {
    return false;
  }

  vtkVRModel* cmodel = renWin->GetModelForDevice(controller);

  if (!cmodel)
  {
    return false;
  }

  cmodel->SetVisibility(false);

  // Compute controller position and world orientation
  double p0[3];   // Ray start point
  double wxyz[4]; // Controller orientation
  double dummy_ppos[3];
  double wdir[3];

  vtkMatrix4x4* devicePose = renWin->GetDeviceToPhysicalMatrixForDevice(controller);

  if (!devicePose)
  {
    return false;
  }

  iren->ConvertPoseToWorldCoordinates(devicePose, p0, wxyz, dummy_ppos, wdir);
  this->HardwarePicker->PickProp(p0, wxyz, ren, ren->GetViewProps(), actorPassOnly);
  cmodel->SetVisibility(true);

  return true;
}
VTK_ABI_NAMESPACE_END
