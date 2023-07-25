// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkZSpaceRenderWindowInteractor.h"

#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkEventData.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRendererCollection.h"
#include "vtkTextActor.h"
#include "vtkTransform.h"
#include "vtkZSpaceHardwarePicker.h"
#include "vtkZSpaceInteractorStyle.h"
#include "vtkZSpaceSDKManager.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkZSpaceRenderWindowInteractor);

//------------------------------------------------------------------------------
// Construct object so that light follows camera motion.
vtkZSpaceRenderWindowInteractor::vtkZSpaceRenderWindowInteractor()
{
  vtkNew<vtkZSpaceInteractorStyle> style;
  this->SetInteractorStyle(style);
}

//------------------------------------------------------------------------------
void vtkZSpaceRenderWindowInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkZSpaceRenderWindowInteractor::ProcessEvents()
{
  vtkZSpaceSDKManager* sdkManager = vtkZSpaceSDKManager::GetInstance();
  sdkManager->Update();

  // Compute stylus position and orientation
  vtkTransform* stylusT = sdkManager->GetStylusTransformRowMajor();

  double pos[3];
  stylusT->GetPosition(pos);
  double wxyz[4];
  stylusT->GetOrientationWXYZ(wxyz);

  // Offset stylus world position with the glasses position
  vtkCamera* camera =
    static_cast<vtkRenderer*>(this->GetRenderWindow()->GetRenderers()->GetItemAsObject(0))
      ->GetActiveCamera();
  double* camPos = camera->GetPosition();
  pos[0] += camPos[0];
  pos[1] += camPos[1];
  pos[2] += camPos[2];

  this->SetWorldEventPosition(pos[0], pos[1], pos[2], this->PointerIndex);
  this->SetWorldEventOrientation(wxyz[0], wxyz[1], wxyz[2], wxyz[3], this->PointerIndex);

  vtkNew<vtkEventDataDevice3D> ed3d;
  ed3d->SetWorldPosition(pos);
  ed3d->SetWorldOrientation(wxyz);
  // We only have one stylus
  ed3d->SetDevice(vtkEventDataDevice::RightController);

  switch (sdkManager->GetLeftButtonState())
  {
    case vtkZSpaceSDKManager::Down:
      this->OnLeftButtonDown(ed3d);
      break;
    case vtkZSpaceSDKManager::Up:
      this->OnLeftButtonUp(ed3d);
      break;
    default:
      break;
  }

  switch (sdkManager->GetMiddleButtonState())
  {
    case vtkZSpaceSDKManager::Down:
      this->OnMiddleButtonDown(ed3d);
      break;
    case vtkZSpaceSDKManager::Up:
      this->OnMiddleButtonUp(ed3d);
      break;
    default:
      break;
  }

  switch (sdkManager->GetRightButtonState())
  {
    case vtkZSpaceSDKManager::Down:
      this->OnRightButtonDown(ed3d);
      break;
    case vtkZSpaceSDKManager::Up:
      this->OnRightButtonUp(ed3d);
      break;
    default:
      break;
  }

  // Always a move event
  ed3d->SetType(vtkCommand::Move3DEvent);
  this->InvokeEvent(vtkCommand::Move3DEvent, ed3d);
}

//------------------------------------------------------------------------------
void vtkZSpaceRenderWindowInteractor::OnMiddleButtonDown(vtkEventDataDevice3D* ed3d)
{
  vtkZSpaceSDKManager::GetInstance()->SetMiddleButtonState(vtkZSpaceSDKManager::Pressed);

  ed3d->SetAction(vtkEventDataAction::Press);

  this->InvokeEvent(vtkCommand::PositionProp3DEvent, ed3d);
}

//------------------------------------------------------------------------------
void vtkZSpaceRenderWindowInteractor::OnMiddleButtonUp(vtkEventDataDevice3D* ed3d)
{
  vtkZSpaceSDKManager::GetInstance()->SetMiddleButtonState(vtkZSpaceSDKManager::None);

  ed3d->SetAction(vtkEventDataAction::Release);

  this->InvokeEvent(vtkCommand::PositionProp3DEvent, ed3d);
}

//------------------------------------------------------------------------------
void vtkZSpaceRenderWindowInteractor::OnRightButtonDown(vtkEventDataDevice3D* ed3d)
{
  vtkZSpaceSDKManager::GetInstance()->SetRightButtonState(vtkZSpaceSDKManager::Pressed);

  ed3d->SetType(vtkCommand::Select3DEvent);
  ed3d->SetAction(vtkEventDataAction::Press);

  // Start selecting some vtkWidgets that respond to this event
  this->InvokeEvent(vtkCommand::Select3DEvent, ed3d);
}

//------------------------------------------------------------------------------
void vtkZSpaceRenderWindowInteractor::OnRightButtonUp(vtkEventDataDevice3D* ed3d)
{
  vtkZSpaceSDKManager::GetInstance()->SetRightButtonState(vtkZSpaceSDKManager::None);

  ed3d->SetType(vtkCommand::Select3DEvent);
  ed3d->SetAction(vtkEventDataAction::Release);

  // End selecting some vtkWidgets that respond to this event
  this->InvokeEvent(vtkCommand::Select3DEvent, ed3d);
}

//------------------------------------------------------------------------------
void vtkZSpaceRenderWindowInteractor::OnLeftButtonDown(vtkEventDataDevice3D* ed3d)
{
  vtkZSpaceSDKManager::GetInstance()->SetLeftButtonState(vtkZSpaceSDKManager::Pressed);

  ed3d->SetAction(vtkEventDataAction::Press);

  this->InvokeEvent(vtkCommand::Pick3DEvent, ed3d);
}

//------------------------------------------------------------------------------
void vtkZSpaceRenderWindowInteractor::OnLeftButtonUp(vtkEventDataDevice3D* ed3d)
{
  vtkZSpaceSDKManager::GetInstance()->SetLeftButtonState(vtkZSpaceSDKManager::None);

  ed3d->SetAction(vtkEventDataAction::Release);

  this->InvokeEvent(vtkCommand::Pick3DEvent, ed3d);
}

//------------------------------------------------------------------------------
void vtkZSpaceRenderWindowInteractor::ExitCallback()
{
  if (this->HasObserver(vtkCommand::ExitEvent))
  {
    this->InvokeEvent(vtkCommand::ExitEvent, nullptr);
  }

  this->TerminateApp();
}

//------------------------------------------------------------------------------
vtkEventDataDevice vtkZSpaceRenderWindowInteractor::GetPointerDevice()
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
void vtkZSpaceRenderWindowInteractor::StartEventLoop()
{
  while (!this->Done)
  {
    this->ProcessEvents();
    this->Render();
  }
}

VTK_ABI_NAMESPACE_END
