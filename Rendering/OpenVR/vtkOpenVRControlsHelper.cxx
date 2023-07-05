// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOpenVRControlsHelper.h"

#include "vtkObjectFactory.h"
#include "vtkOpenVRModel.h"
#include "vtkOpenVRRenderWindow.h"
#include "vtkRenderer.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOpenVRControlsHelper);

//------------------------------------------------------------------------------
void vtkOpenVRControlsHelper::InitControlPosition()
{
  if (!this->Renderer->GetRenderWindow()->GetInteractor())
  {
    return;
  }

  vtkOpenVRRenderWindow* renWin =
    static_cast<vtkOpenVRRenderWindow*>(this->Renderer->GetRenderWindow());
  if (!renWin)
  {
    return;
  }

  // Get the active controller device
  vtkEventDataDevice controller = this->Device;

  // Get the active controller model
  vtkVRModel* mod = renWin->GetModelForDevice(controller);

  // Hide controls tooltips if the controller is off
  if (!mod)
  {
    this->LabelVisible = false;
    return;
  }

  // Compute the component position offset. It corresponds to the vector from the
  // controller origin to the button origin, expressed in local coordinates.
  uint32_t nbOfComponents =
    renWin->GetOpenVRRenderModels()->GetComponentCount(mod->GetName().c_str());
  // for all existing components
  for (uint32_t i = 0; i < nbOfComponents; i++)
  {
    char componentName[100];
    // get the component name
    renWin->GetOpenVRRenderModels()->GetComponentName(
      mod->GetName().c_str(), i, componentName, 100);
    std::string strComponentName = std::string(componentName);

    if (strComponentName == this->ComponentName)
    {
      vr::RenderModel_ControllerMode_State_t pState;
      vr::RenderModel_ComponentState_t pComponentState;
      vr::VRControllerState_t cstate;

      // Get the controller state
      renWin->GetHMD()->GetControllerState(
        renWin->GetDeviceHandleForDevice(controller), &cstate, sizeof(cstate));

      // Get the component state
      renWin->GetOpenVRRenderModels()->GetComponentState(
        mod->GetName().c_str(), this->ComponentName.c_str(), &cstate, &pState, &pComponentState);

      vr::HmdMatrix34_t mTrackingToLocal = pComponentState.mTrackingToComponentLocal;

      // Save position offset
      this->ControlPositionLC[0] = mTrackingToLocal.m[0][3];
      this->ControlPositionLC[1] = mTrackingToLocal.m[1][3];
      this->ControlPositionLC[2] = mTrackingToLocal.m[2][3];

      break; // Don't need to check other components. break.
    }
  }
}
VTK_ABI_NAMESPACE_END
