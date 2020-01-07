/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkOpenVRInteractorStyle.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenVRInteractorStyle.h"

#include "vtkCompositeDataSet.h"
#include "vtkDataObjectTreeIterator.h"

#include "vtkBillboardTextActor3D.h"
#include "vtkCoordinate.h"
#include "vtkTextActor.h"
#include "vtkTextActor3D.h"

#include "vtkAssemblyPath.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkCellPicker.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkHardwareSelector.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkMapper.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenVRCamera.h"
#include "vtkOpenVRControlsHelper.h"
#include "vtkOpenVRHardwarePicker.h"
#include "vtkOpenVRModel.h"
#include "vtkOpenVROverlay.h"
#include "vtkOpenVRRenderWindow.h"
#include "vtkOpenVRRenderWindowInteractor.h"
#include "vtkPlane.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPropPicker.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSphereSource.h"
#include "vtkStringArray.h"
#include "vtkTextProperty.h"
#include "vtkTimerLog.h"

#include "vtkOpenVRMenuRepresentation.h"
#include "vtkOpenVRMenuWidget.h"

#include <sstream>

// Map controller inputs to interaction states
vtkStandardNewMacro(vtkOpenVRInteractorStyle);

//----------------------------------------------------------------------------
vtkOpenVRInteractorStyle::vtkOpenVRInteractorStyle()
{
  for (int d = 0; d < vtkEventDataNumberOfDevices; ++d)
  {
    this->InteractionState[d] = VTKIS_NONE;
    this->InteractionProps[d] = nullptr;
    this->ClippingPlanes[d] = nullptr;

    for (int i = 0; i < vtkEventDataNumberOfInputs; i++)
    {
      this->InputMap[d][i] = -1;
      this->ControlsHelpers[d][i] = nullptr;
    }
  }

  // Create default inputs mapping
  this->MapInputToAction(
    vtkEventDataDevice::RightController, vtkEventDataDeviceInput::Trigger, VTKIS_POSITION_PROP);
  this->MapInputToAction(
    vtkEventDataDevice::RightController, vtkEventDataDeviceInput::TrackPad, VTKIS_DOLLY);
  this->MapInputToAction(
    vtkEventDataDevice::RightController, vtkEventDataDeviceInput::ApplicationMenu, VTKIS_MENU);

  this->MapInputToAction(vtkEventDataDevice::LeftController,
    vtkEventDataDeviceInput::ApplicationMenu, VTKIS_TOGGLE_DRAW_CONTROLS);
  this->MapInputToAction(
    vtkEventDataDevice::LeftController, vtkEventDataDeviceInput::Trigger, VTKIS_LOAD_CAMERA_POSE);

  this->AddTooltipForInput(vtkEventDataDevice::RightController,
    vtkEventDataDeviceInput::ApplicationMenu, "Application Menu");

  this->MenuCommand = vtkCallbackCommand::New();
  this->MenuCommand->SetClientData(this);
  this->MenuCommand->SetCallback(vtkOpenVRInteractorStyle::MenuCallback);

  this->Menu->SetRepresentation(this->MenuRepresentation);
  this->Menu->PushFrontMenuItem("exit", "Exit", this->MenuCommand);
  this->Menu->PushFrontMenuItem("togglelabel", "Toggle Controller Labels", this->MenuCommand);
  this->Menu->PushFrontMenuItem("clipmode", "Clipping Mode", this->MenuCommand);
  this->Menu->PushFrontMenuItem("probemode", "Probe Mode", this->MenuCommand);
  this->Menu->PushFrontMenuItem("grabmode", "Grab Mode", this->MenuCommand);

  vtkNew<vtkPolyDataMapper> pdm;
  this->PickActor->SetMapper(pdm);
  this->PickActor->GetProperty()->SetLineWidth(4);
  this->PickActor->GetProperty()->RenderLinesAsTubesOn();
  this->PickActor->GetProperty()->SetRepresentationToWireframe();
  this->PickActor->DragableOff();

  this->HoverPickOff();
  this->GrabWithRayOff();

  vtkNew<vtkCellPicker> exactPicker;
  this->SetInteractionPicker(exactPicker);
}

//----------------------------------------------------------------------------
vtkOpenVRInteractorStyle::~vtkOpenVRInteractorStyle()
{
  for (int d = 0; d < vtkEventDataNumberOfDevices; ++d)
  {
    if (this->ClippingPlanes[d])
    {
      this->ClippingPlanes[d]->Delete();
      this->ClippingPlanes[d] = nullptr;
    }
  }
  for (int d = 0; d < vtkEventDataNumberOfDevices; ++d)
  {
    for (int i = 0; i < vtkEventDataNumberOfInputs; i++)
    {
      if (this->ControlsHelpers[d][i])
      {
        this->ControlsHelpers[d][i]->Delete();
        this->ControlsHelpers[d][i] = nullptr;
      }
    }
  }
  this->MenuCommand->Delete();
}

void vtkOpenVRInteractorStyle::SetInteractor(vtkRenderWindowInteractor* iren)
{
  this->Superclass::SetInteractor(iren);
}

//----------------------------------------------------------------------------
void vtkOpenVRInteractorStyle::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkOpenVRInteractorStyle::MenuCallback(
  vtkObject* vtkNotUsed(object), unsigned long, void* clientdata, void* calldata)
{
  std::string name = static_cast<const char*>(calldata);
  vtkOpenVRInteractorStyle* self = static_cast<vtkOpenVRInteractorStyle*>(clientdata);

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
    self->MapInputToAction(
      vtkEventDataDevice::RightController, vtkEventDataDeviceInput::Trigger, VTKIS_CLIP);
  }
  if (name == "grabmode")
  {
    self->MapInputToAction(
      vtkEventDataDevice::RightController, vtkEventDataDeviceInput::Trigger, VTKIS_POSITION_PROP);
  }
  if (name == "probemode")
  {
    self->MapInputToAction(
      vtkEventDataDevice::RightController, vtkEventDataDeviceInput::Trigger, VTKIS_PICK);
  }
}

//----------------------------------------------------------------------------
// Generic events binding
//----------------------------------------------------------------------------
void vtkOpenVRInteractorStyle::OnMove3D(vtkEventData* edata)
{
  vtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();
  if (!edd)
  {
    return;
  }

  // joystick moves?
  int idev = static_cast<int>(edd->GetDevice());

  // Update current state
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  // Set current state and interaction prop
  this->InteractionProp = this->InteractionProps[idev];

  switch (this->InteractionState[idev])
  {
    case VTKIS_POSITION_PROP:
      this->FindPokedRenderer(x, y);
      this->PositionProp(edd);
      this->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
      break;
    case VTKIS_DOLLY:
      this->FindPokedRenderer(x, y);
      this->Dolly3D(edata);
      this->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
      break;
    case VTKIS_CLIP:
      this->FindPokedRenderer(x, y);
      this->Clip(edd);
      this->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
      break;
  }

  // Update rays
  this->UpdateRay(edd->GetDevice());
}

//----------------------------------------------------------------------------
void vtkOpenVRInteractorStyle::OnButton3D(vtkEventData* edata)
{
  vtkEventDataDevice3D* bd = edata->GetAsEventDataDevice3D();
  if (!bd)
  {
    return;
  }

  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];
  this->FindPokedRenderer(x, y);

  int state = this->InputMap[static_cast<int>(bd->GetDevice())][static_cast<int>(bd->GetInput())];
  if (state == -1)
  {
    return;
  }

  // right trigger press/release
  if (bd->GetAction() == vtkEventDataAction::Press)
  {
    this->StartAction(state, bd);
  }
  if (bd->GetAction() == vtkEventDataAction::Release)
  {
    this->EndAction(state, bd);
  }
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Interaction entry points
//----------------------------------------------------------------------------
void vtkOpenVRInteractorStyle::StartPick(vtkEventDataDevice3D* edata)
{
  this->HideBillboard();
  this->HidePickActor();

  this->InteractionState[static_cast<int>(edata->GetDevice())] = VTKIS_PICK;

  // update ray
  this->UpdateRay(edata->GetDevice());
}
//----------------------------------------------------------------------------
void vtkOpenVRInteractorStyle::EndPick(vtkEventDataDevice3D* edata)
{
  // perform probe
  this->ProbeData(edata->GetDevice());

  this->InteractionState[static_cast<int>(edata->GetDevice())] = VTKIS_NONE;

  // turn off ray
  this->UpdateRay(edata->GetDevice());
}

//----------------------------------------------------------------------------
void vtkOpenVRInteractorStyle::StartLoadCamPose(vtkEventDataDevice3D* edata)
{
  int iDevice = static_cast<int>(edata->GetDevice());
  this->InteractionState[iDevice] = VTKIS_LOAD_CAMERA_POSE;
}
//----------------------------------------------------------------------------
void vtkOpenVRInteractorStyle::EndLoadCamPose(vtkEventDataDevice3D* edata)
{
  this->LoadNextCameraPose();

  int iDevice = static_cast<int>(edata->GetDevice());
  this->InteractionState[iDevice] = VTKIS_NONE;
}

//----------------------------------------------------------------------------
bool vtkOpenVRInteractorStyle::HardwareSelect(vtkEventDataDevice controller, bool actorPassOnly)
{
  vtkRenderer* ren = this->CurrentRenderer;
  vtkOpenVRRenderWindow* renWin =
    vtkOpenVRRenderWindow::SafeDownCast(this->Interactor->GetRenderWindow());
  vtkOpenVRRenderWindowInteractor* iren =
    static_cast<vtkOpenVRRenderWindowInteractor*>(this->Interactor);

  if (!ren || !renWin || !iren)
  {
    return false;
  }

  vtkOpenVRModel* cmodel = renWin->GetTrackedDeviceModel(controller);
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
  vr::TrackedDevicePose_t& tdPose = renWin->GetTrackedDevicePose(cmodel->TrackedDevice);
  iren->ConvertPoseToWorldCoordinates(tdPose, p0, wxyz, dummy_ppos, wdir);

  this->HardwarePicker->PickProp(p0, wxyz, ren, ren->GetViewProps(), actorPassOnly);

  cmodel->SetVisibility(true);

  return true;
}

//----------------------------------------------------------------------------
void vtkOpenVRInteractorStyle::StartPositionProp(vtkEventDataDevice3D* edata)
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

//----------------------------------------------------------------------------
void vtkOpenVRInteractorStyle::EndPositionProp(vtkEventDataDevice3D* edata)
{
  vtkEventDataDevice dev = edata->GetDevice();
  this->InteractionState[static_cast<int>(dev)] = VTKIS_NONE;
  this->InteractionProps[static_cast<int>(dev)] = nullptr;
}

//----------------------------------------------------------------------------
void vtkOpenVRInteractorStyle::StartClip(vtkEventDataDevice3D* ed)
{
  if (this->CurrentRenderer == nullptr)
  {
    return;
  }

  vtkEventDataDevice dev = ed->GetDevice();
  this->InteractionState[static_cast<int>(dev)] = VTKIS_CLIP;

  if (!this->ClippingPlanes[static_cast<int>(dev)])
  {
    this->ClippingPlanes[static_cast<int>(dev)] = vtkPlane::New();
  }

  vtkActorCollection* ac;
  vtkActor *anActor, *aPart;
  vtkAssemblyPath* path;
  if (this->CurrentRenderer != 0)
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

//----------------------------------------------------------------------------
void vtkOpenVRInteractorStyle::EndClip(vtkEventDataDevice3D* ed)
{
  vtkEventDataDevice dev = ed->GetDevice();
  this->InteractionState[static_cast<int>(dev)] = VTKIS_NONE;

  vtkActorCollection* ac;
  vtkActor *anActor, *aPart;
  vtkAssemblyPath* path;
  if (this->CurrentRenderer != 0)
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

//----------------------------------------------------------------------------
void vtkOpenVRInteractorStyle::StartDolly3D(vtkEventDataDevice3D* ed)
{
  if (this->CurrentRenderer == nullptr)
  {
    return;
  }
  vtkEventDataDevice dev = ed->GetDevice();
  this->InteractionState[static_cast<int>(dev)] = VTKIS_DOLLY;
  this->LastDolly3DEventTime->StartTimer();

  // this->GrabFocus(this->EventCallbackCommand);
}
//----------------------------------------------------------------------------
void vtkOpenVRInteractorStyle::EndDolly3D(vtkEventDataDevice3D* ed)
{
  vtkEventDataDevice dev = ed->GetDevice();
  this->InteractionState[static_cast<int>(dev)] = VTKIS_NONE;

  this->LastDolly3DEventTime->StopTimer();
}

//----------------------------------------------------------------------------
void vtkOpenVRInteractorStyle::ToggleDrawControls()
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
          // this->ControlsHelpers[iDevice][iInput]->SetEnabled(false);
          this->CurrentRenderer->AddViewProp(this->ControlsHelpers[d][i]);
        }

        this->ControlsHelpers[d][i]->SetEnabled(!this->ControlsHelpers[d][i]->GetEnabled());
      }
    }
  }
}

void vtkOpenVRInteractorStyle::SetDrawControls(bool val)
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
          // this->ControlsHelpers[iDevice][iInput]->SetEnabled(false);
          this->CurrentRenderer->AddViewProp(this->ControlsHelpers[d][i]);
        }

        this->ControlsHelpers[d][i]->SetEnabled(val);
      }
    }
  }
}

//----------------------------------------------------------------------------
// Interaction methods
//----------------------------------------------------------------------------
void vtkOpenVRInteractorStyle::ProbeData(vtkEventDataDevice controller)
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

void vtkOpenVRInteractorStyle::EndPickCallback(vtkSelection* sel)
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

//----------------------------------------------------------------------------
void vtkOpenVRInteractorStyle::LoadNextCameraPose()
{
  vtkOpenVRRenderWindow* renWin =
    vtkOpenVRRenderWindow::SafeDownCast(this->Interactor->GetRenderWindow());
  if (!renWin)
  {
    return;
  }
  vtkOpenVROverlay* ovl = renWin->GetDashboardOverlay();
  ovl->LoadNextCameraPose();
}

//----------------------------------------------------------------------------
void vtkOpenVRInteractorStyle::PositionProp(vtkEventData* ed)
{
  if (this->InteractionProp == nullptr || !this->InteractionProp->GetDragable())
  {
    return;
  }
  this->Superclass::PositionProp(ed);
}

//----------------------------------------------------------------------------
void vtkOpenVRInteractorStyle::Clip(vtkEventDataDevice3D* ed)
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
  // plane->SetOrigin(wpos[0], wpos[1], wpos[2]);

  double r[3];
  double up[3];
  up[0] = 0;
  up[1] = -1;
  up[2] = 0;
  vtkMath::RotateVectorByWXYZ(up, ori, r);
  // plane->SetNormal(r);

  vtkEventDataDevice dev = ed->GetDevice();
  int idev = static_cast<int>(dev);
  this->ClippingPlanes[idev]->SetNormal(r);
  this->ClippingPlanes[idev]->SetOrigin(wpos[0], wpos[1], wpos[2]);
}

//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Multitouch interaction methods
//----------------------------------------------------------------------------
void vtkOpenVRInteractorStyle::OnPan()
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
    double physicalScale = rwi->GetPhysicalScale();

    rwi->SetPhysicalTranslation(camera, ptrans[0] + t[0] * physicalScale,
      ptrans[1] + t[1] * physicalScale, ptrans[2] + t[2] * physicalScale);

    // clean up
    if (this->Interactor->GetLightFollowCamera())
    {
      this->CurrentRenderer->UpdateLightsGeometryToFollowCamera();
    }
  }
}
//----------------------------------------------------------------------------
void vtkOpenVRInteractorStyle::OnPinch()
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
//----------------------------------------------------------------------------
void vtkOpenVRInteractorStyle::OnRotate()
{
  int rc = static_cast<int>(vtkEventDataDevice::RightController);
  int lc = static_cast<int>(vtkEventDataDevice::LeftController);

  // Rotate only when one controller is not interacting
  if ((this->InteractionProps[rc] || this->InteractionProps[lc]) &&
    (!this->InteractionProps[rc] || !this->InteractionProps[lc]))
  {
    this->InteractionState[rc] = VTKIS_ROTATE;
    this->InteractionState[lc] = VTKIS_ROTATE;

    double angle = this->Interactor->GetRotation() - this->Interactor->GetLastRotation();

    if (this->InteractionProps[rc])
    {
      this->InteractionProps[rc]->RotateY(angle);
    }
    if (this->InteractionProps[lc])
    {
      this->InteractionProps[lc]->RotateY(angle);
    }
  }
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Utility routines
//----------------------------------------------------------------------------
void vtkOpenVRInteractorStyle::MapInputToAction(
  vtkEventDataDevice device, vtkEventDataDeviceInput input, int state)
{
  if (input >= vtkEventDataDeviceInput::NumberOfInputs || state < VTKIS_NONE)
  {
    return;
  }

  int oldstate = this->InputMap[static_cast<int>(device)][static_cast<int>(input)];
  if (oldstate == state)
  {
    return;
  }

  this->InputMap[static_cast<int>(device)][static_cast<int>(input)] = state;
  this->AddTooltipForInput(device, input);

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkOpenVRInteractorStyle::StartAction(int state, vtkEventDataDevice3D* edata)
{
  switch (state)
  {
    case VTKIS_POSITION_PROP:
      this->StartPositionProp(edata);
      break;
    case VTKIS_DOLLY:
      this->StartDolly3D(edata);
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
  }
}
//----------------------------------------------------------------------------
void vtkOpenVRInteractorStyle::EndAction(int state, vtkEventDataDevice3D* edata)
{
  switch (state)
  {
    case VTKIS_POSITION_PROP:
      this->EndPositionProp(edata);
      break;
    case VTKIS_DOLLY:
      this->EndDolly3D(edata);
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
  }

  // Reset multitouch state because a button has been released
  for (int d = 0; d < vtkEventDataNumberOfDevices; ++d)
  {
    switch (this->InteractionState[d])
    {
      case VTKIS_PAN:
      case VTKIS_ZOOM:
      case VTKIS_ROTATE:
        this->InteractionState[d] = VTKIS_NONE;
        break;
    }
  }
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Handle Ray drawing and update
//----------------------------------------------------------------------------
void vtkOpenVRInteractorStyle::ShowRay(vtkEventDataDevice controller)
{
  vtkOpenVRRenderWindow* renWin =
    vtkOpenVRRenderWindow::SafeDownCast(this->Interactor->GetRenderWindow());
  if (!renWin ||
    (controller != vtkEventDataDevice::LeftController &&
      controller != vtkEventDataDevice::RightController))
  {
    return;
  }
  vtkOpenVRModel* cmodel = renWin->GetTrackedDeviceModel(controller);
  if (cmodel)
  {
    cmodel->SetShowRay(true);
  }
}
//----------------------------------------------------------------------------
void vtkOpenVRInteractorStyle::HideRay(vtkEventDataDevice controller)
{
  vtkOpenVRRenderWindow* renWin =
    vtkOpenVRRenderWindow::SafeDownCast(this->Interactor->GetRenderWindow());
  if (!renWin ||
    (controller != vtkEventDataDevice::LeftController &&
      controller != vtkEventDataDevice::RightController))
  {
    return;
  }
  vtkOpenVRModel* cmodel = renWin->GetTrackedDeviceModel(controller);
  if (cmodel)
  {
    cmodel->SetShowRay(false);
  }
}
//----------------------------------------------------------------------------
void vtkOpenVRInteractorStyle::UpdateRay(vtkEventDataDevice controller)
{
  if (!this->Interactor)
  {
    return;
  }

  vtkRenderer* ren = this->CurrentRenderer;
  vtkOpenVRRenderWindow* renWin =
    vtkOpenVRRenderWindow::SafeDownCast(this->Interactor->GetRenderWindow());
  vtkOpenVRRenderWindowInteractor* iren =
    static_cast<vtkOpenVRRenderWindowInteractor*>(this->Interactor);

  if (!ren || !renWin || !iren)
  {
    return;
  }

  vr::TrackedDeviceIndex_t idx = renWin->GetTrackedDeviceIndexForDevice(controller);
  if (idx == vr::k_unTrackedDeviceIndexInvalid)
  {
    return;
  }
  vtkOpenVRModel* mod = renWin->GetTrackedDeviceModel(idx);
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

    if (rep && rep->GetInteractionState() != 0)
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
  vr::TrackedDevicePose_t& tdPose = renWin->GetTrackedDevicePose(mod->TrackedDevice);
  iren->ConvertPoseToWorldCoordinates(tdPose, p0, wxyz, dummy_ppos, wdir);

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

  return;
}
//----------------------------------------------------------------------------

void vtkOpenVRInteractorStyle::ShowBillboard(const std::string& text)
{
  vtkOpenVRRenderWindow* renWin =
    vtkOpenVRRenderWindow::SafeDownCast(this->Interactor->GetRenderWindow());
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

void vtkOpenVRInteractorStyle::HideBillboard()
{
  this->CurrentRenderer->RemoveActor(this->TextActor3D);
}

void vtkOpenVRInteractorStyle::ShowPickSphere(double* pos, double radius, vtkProp3D* prop)
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

void vtkOpenVRInteractorStyle::ShowPickCell(vtkCell* cell, vtkProp3D* prop)
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

void vtkOpenVRInteractorStyle::HidePickActor()
{
  if (this->CurrentRenderer)
  {
    this->CurrentRenderer->RemoveActor(this->PickActor);
  }
}

//----------------------------------------------------------------------------
void vtkOpenVRInteractorStyle::AddTooltipForInput(
  vtkEventDataDevice device, vtkEventDataDeviceInput input)
{
  this->AddTooltipForInput(device, input, "");
}
//----------------------------------------------------------------------------
void vtkOpenVRInteractorStyle::AddTooltipForInput(
  vtkEventDataDevice device, vtkEventDataDeviceInput input, const std::string& text)
{
  int iInput = static_cast<int>(input);
  int iDevice = static_cast<int>(device);
  int state = this->InputMap[iDevice][iInput];

  // if (state == -1)
  // {
  //   vtkWarningMacro(<< "Input " << iInput <<
  //     " not found. Inputs need to be mapped to actions first.");
  //   return;
  // }

  vtkStdString controlName = vtkStdString();
  vtkStdString controlText = vtkStdString();
  int drawSide = -1;
  int buttonSide = -1;

  // Setup default text and layout
  switch (input)
  {
    case vtkEventDataDeviceInput::Trigger:
      controlName = "trigger";
      drawSide = vtkOpenVRControlsHelper::Left;
      buttonSide = vtkOpenVRControlsHelper::Back;
      controlText = "Trigger :\n";
      break;
    case vtkEventDataDeviceInput::TrackPad:
      controlName = "trackpad";
      drawSide = vtkOpenVRControlsHelper::Right;
      buttonSide = vtkOpenVRControlsHelper::Front;
      controlText = "Trackpad :\n";
      break;
    case vtkEventDataDeviceInput::Grip:
      controlName = "lgrip";
      drawSide = vtkOpenVRControlsHelper::Right;
      buttonSide = vtkOpenVRControlsHelper::Back;
      controlText = "Grip :\n";
      break;
    case vtkEventDataDeviceInput::ApplicationMenu:
      controlName = "button";
      drawSide = vtkOpenVRControlsHelper::Left;
      buttonSide = vtkOpenVRControlsHelper::Front;
      controlText = "Application Menu :\n";
      break;
  }

  if (text != "")
  {
    controlText += text;
  }
  else
  {
    // Setup default action text
    switch (state)
    {
      case VTKIS_POSITION_PROP:
        controlText += "Pick objects to\nadjust their pose";
        break;
      case VTKIS_DOLLY:
        controlText += "Apply translation\nto the camera";
        break;
      case VTKIS_CLIP:
        controlText += "Clip objects";
        break;
      case VTKIS_PICK:
        controlText += "Probe data";
        break;
      case VTKIS_LOAD_CAMERA_POSE:
        controlText += "Load next\ncamera pose.";
        break;
      case VTKIS_TOGGLE_DRAW_CONTROLS:
        controlText += "Toggle control visibility";
        break;
      case VTKIS_EXIT:
        controlText += "Exit";
        break;
      default:
        controlText = "No action assigned\nto this input.";
        break;
    }
  }

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
  vtkOpenVRControlsHelper* inputHelper = vtkOpenVRControlsHelper::New();
  inputHelper->SetTooltipInfo(controlName.c_str(), buttonSide, drawSide, controlText.c_str());

  this->ControlsHelpers[iDevice][iInput] = inputHelper;
  this->ControlsHelpers[iDevice][iInput]->SetDevice(device);

  if (this->CurrentRenderer)
  {
    this->ControlsHelpers[iDevice][iInput]->SetRenderer(this->CurrentRenderer);
    this->ControlsHelpers[iDevice][iInput]->BuildRepresentation();
    // this->ControlsHelpers[iDevice][iInput]->SetEnabled(false);
    this->CurrentRenderer->AddViewProp(this->ControlsHelpers[iDevice][iInput]);
  }
}
