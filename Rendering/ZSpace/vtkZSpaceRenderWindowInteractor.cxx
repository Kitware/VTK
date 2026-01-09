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

  std::vector<vtkZSpaceSDKManager::ButtonIds> stylusButtons = { vtkZSpaceSDKManager::LeftButton,
    vtkZSpaceSDKManager::MiddleButton, vtkZSpaceSDKManager::RightButton };
  for (auto buttonId : stylusButtons)
  {
    vtkZSpaceSDKManager::ButtonState buttonState = sdkManager->GetButtonState(buttonId);
    this->ProcessNextButtonState(buttonId, buttonState);
    this->DispatchStylusEvents(buttonId, buttonState, ed3d);
  }

  // Always a move event
  ed3d->SetType(vtkCommand::Move3DEvent);
  this->InvokeEvent(vtkCommand::Move3DEvent, ed3d);
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
  else if (this->PointerIndex == 1)
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

//------------------------------------------------------------------------------
void vtkZSpaceRenderWindowInteractor::ProcessNextButtonState(
  vtkZSpaceSDKManager::ButtonIds buttonId, vtkZSpaceSDKManager::ButtonState buttonState)
{
  bool changeNextButtonState =
    buttonState == vtkZSpaceSDKManager::Up || buttonState == vtkZSpaceSDKManager::Down;
  if (!changeNextButtonState)
  {
    return;
  }

  // Switch next button state to be `Pressed` if the previous one is `Down` otherwise `None` by
  // default. See `vtkZSpaceSDKManager` documentation for more information.
  vtkZSpaceSDKManager::ButtonState nextButtonState = vtkZSpaceSDKManager::None;
  if (buttonState == vtkZSpaceSDKManager::Down)
  {
    nextButtonState = vtkZSpaceSDKManager::Pressed;
  }

  vtkZSpaceSDKManager::GetInstance()->SetButtonState(buttonId, nextButtonState);
}

//------------------------------------------------------------------------------
void vtkZSpaceRenderWindowInteractor::DispatchStylusEvents(vtkZSpaceSDKManager::ButtonIds buttonId,
  vtkZSpaceSDKManager::ButtonState buttonState, vtkEventDataDevice3D* ed3d)
{
  if (buttonState == vtkZSpaceSDKManager::None || buttonState == vtkZSpaceSDKManager::Pressed)
  {
    return;
  }

  if (vtkZSpaceSDKManager::GetInstance()->GetUseDefaultBehavior(buttonId))
  {
    this->CallDefaultStylusEvents(buttonId, buttonState, ed3d);
  }
  else
  {
    this->CallCustomStylusEvent(buttonId, buttonState);
  }
}

//------------------------------------------------------------------------------
void vtkZSpaceRenderWindowInteractor::CallDefaultStylusEvents(
  vtkZSpaceSDKManager::ButtonIds buttonId, vtkZSpaceSDKManager::ButtonState buttonState,
  vtkEventDataDevice3D* ed3d)
{
  vtkEventDataAction eventAction = vtkEventDataAction::Unknown;
  switch (buttonState)
  {
    case vtkZSpaceSDKManager::Up:
      eventAction = vtkEventDataAction::Release;
      break;
    case vtkZSpaceSDKManager::Down:
      eventAction = vtkEventDataAction::Press;
      break;
    default:
      break;
  }

  unsigned long eventType = vtkCommand::NoEvent;
  switch (buttonId)
  {
    case vtkZSpaceSDKManager::LeftButton:
      eventType = vtkCommand::Pick3DEvent;
      break;
    case vtkZSpaceSDKManager::MiddleButton:
      eventType = vtkCommand::PositionProp3DEvent;
      break;
    case vtkZSpaceSDKManager::RightButton:
      eventType = vtkCommand::Select3DEvent;
      break;
    default:
      break;
  }

  ed3d->SetType(eventType);
  ed3d->SetAction(eventAction);
  this->InvokeEvent(eventType, ed3d);
}

//------------------------------------------------------------------------------
void vtkZSpaceRenderWindowInteractor::CallCustomStylusEvent(
  vtkZSpaceSDKManager::ButtonIds buttonId, vtkZSpaceSDKManager::ButtonState buttonState)
{
  vtkZSpaceSDKManager::StylusEventData eventData(buttonId, buttonState);
  this->InvokeEvent(StylusButtonEvent, &eventData);
}

VTK_ABI_NAMESPACE_END
