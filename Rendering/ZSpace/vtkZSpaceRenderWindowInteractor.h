// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkZSpaceRenderWindowInteractor
 * @brief   Handle zSpace specific interactions.
 *
 * This class handle the zSpace specific interactions, done with the stylus.
 * It will internally update and retrieve the state of the zSpace devices
 * (through the zSpace manager instance, in the ProcessEvents method) and
 * emit events accordingly.
 */

#ifndef vtkZSpaceRenderWindowInteractor_h
#define vtkZSpaceRenderWindowInteractor_h

#include "vtkEventData.h" // For vtkEventDataDevice
#include "vtkRenderWindowInteractor3D.h"
#include "vtkRenderingZSpaceModule.h" // For export macro
#include "vtkZSpaceSDKManager.h"      // For ButtonIds, ButtonState

VTK_ABI_NAMESPACE_BEGIN

class VTKRENDERINGZSPACE_EXPORT vtkZSpaceRenderWindowInteractor : public vtkRenderWindowInteractor3D
{
public:
  /**
   * Construct object so that light follows camera motion.
   */
  static vtkZSpaceRenderWindowInteractor* New();
  vtkTypeMacro(vtkZSpaceRenderWindowInteractor, vtkRenderWindowInteractor3D);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Custom events for zSpace environment.
   */
  enum CustomEvents
  {
    StylusButtonEvent = vtkCommand::UserEvent + 6703
  };

  /**
   * These methods correspond to the Exit, User and Pick
   * callbacks. They allow for the Style to invoke them.
   */
  virtual void ExitCallback();

  /**
   * Update WorldEventPosition and WorldEventOrientation, then
   * call event functions depending on the zSpace buttons states.
   */
  void ProcessEvents() override;

  /*
   * Return the pointer index as a device
   */
  vtkEventDataDevice GetPointerDevice();

protected:
  vtkZSpaceRenderWindowInteractor();
  ~vtkZSpaceRenderWindowInteractor() override = default;

  /**
   * This will start up the event loop and never return. If you call this
   * method it will loop processing events until the application is exited.
   */
  void StartEventLoop() override;

private:
  vtkZSpaceRenderWindowInteractor(const vtkZSpaceRenderWindowInteractor&) = delete;
  void operator=(const vtkZSpaceRenderWindowInteractor&) = delete;

  /**
   * Change the button state of the given `buttonId` depending on the current state. This ensures
   * that the events are triggered once when the states are either `Pressed` or `Up`.
   */
  void ProcessNextButtonState(
    vtkZSpaceSDKManager::ButtonIds buttonId, vtkZSpaceSDKManager::ButtonState buttonState);

  /**
   * Function call to invoke the default stylus event as well as custom stylus event if needed.
   */
  void DispatchStylusEvents(vtkZSpaceSDKManager::ButtonIds buttonId,
    vtkZSpaceSDKManager::ButtonState buttonState, vtkEventDataDevice3D* ed3d);

  /**
   * Invoke the events for the default behavior of the stylus. In this case:
   * - The left button will pick a cell and show its properties
   * - The middle button will grab a data object in the scene if possible.
   * - The right button will
   */
  void CallDefaultStylusEvents(vtkZSpaceSDKManager::ButtonIds buttonId,
    vtkZSpaceSDKManager::ButtonState buttonState, vtkEventDataDevice3D* ed3d);

  /**
   * Call a `vtkCommand::UserEvent` with the `buttonId` and `buttonState` attached to it.
   */
  void CallCustomStylusEvent(
    vtkZSpaceSDKManager::ButtonIds buttonId, vtkZSpaceSDKManager::ButtonState buttonState);
};

VTK_ABI_NAMESPACE_END

#endif
