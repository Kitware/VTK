// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOpenVRInteractorStyle.h"

#include "vtkObjectFactory.h"
#include "vtkOpenVRControlsHelper.h"
#include "vtkOpenVROverlay.h"
#include "vtkOpenVRRenderWindow.h"
#include "vtkOpenVRRenderWindowInteractor.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOpenVRInteractorStyle);

//------------------------------------------------------------------------------
void vtkOpenVRInteractorStyle::SetupActions(vtkRenderWindowInteractor* iren)
{
  vtkOpenVRRenderWindowInteractor* oiren = vtkOpenVRRenderWindowInteractor::SafeDownCast(iren);

  if (oiren)
  {
    oiren->AddAction("/actions/vtk/in/Elevation", vtkCommand::Elevation3DEvent, true);
    oiren->AddAction("/actions/vtk/in/Movement", vtkCommand::ViewerMovement3DEvent, true);
    oiren->AddAction("/actions/vtk/in/NextCameraPose", vtkCommand::NextPose3DEvent, false);
    oiren->AddAction("/actions/vtk/in/PositionProp", vtkCommand::PositionProp3DEvent, false);
    oiren->AddAction("/actions/vtk/in/ShowMenu", vtkCommand::Menu3DEvent, false);
    oiren->AddAction("/actions/vtk/in/StartElevation", vtkCommand::Elevation3DEvent, false);
    oiren->AddAction("/actions/vtk/in/StartMovement", vtkCommand::ViewerMovement3DEvent, false);
    oiren->AddAction("/actions/vtk/in/TriggerAction", vtkCommand::Select3DEvent, false);
  }
}

//------------------------------------------------------------------------------
void vtkOpenVRInteractorStyle::LoadNextCameraPose()
{
  vtkOpenVRRenderWindow* renWin =
    vtkOpenVRRenderWindow::SafeDownCast(this->Interactor->GetRenderWindow());

  if (renWin)
  {
    vtkOpenVROverlay* ovl = renWin->GetDashboardOverlay();
    ovl->LoadNextCameraPose();
  }
}

//------------------------------------------------------------------------------
vtkVRControlsHelper* vtkOpenVRInteractorStyle::MakeControlsHelper()
{
  vtkVRControlsHelper* helper = vtkOpenVRControlsHelper::New();
  return helper;
}
VTK_ABI_NAMESPACE_END
