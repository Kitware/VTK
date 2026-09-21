// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkJoystickFly
 * @brief   Fly camera towards or away from the object
 *
 * vtkJoystickFly moves the camera towards or away from the focal point.
 * This class invokes the FlyAnimationStepEvent event at the end of each fly animation step. The
 * interactor is passed to the observer as callData. The observer should call the
 * `interactor->ProcessEvents()` method or a suitable method from the GUI framework to process
 * pending events and update the fly animation.
 */

#ifndef vtkJoystickFly_h
#define vtkJoystickFly_h

#include "vtkCameraManipulator.h"

#include "vtkCommand.h"                // for vtkCommand::UserEvent
#include "vtkInteractionStyleModule.h" // needed for export macro
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkRenderer;

class VTKINTERACTIONSTYLE_EXPORT VTK_MARSHALAUTO vtkJoystickFly : public vtkCameraManipulator
{
public:
  vtkTypeMacro(vtkJoystickFly, vtkCameraManipulator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum : vtkTypeUInt16
  {
    FlyAnimationStepEvent = vtkCommand::UserEvent + 1,
  };

  ///@{
  /**
   * Unimplemented methods from vtkCameraManipulator.
   */
  void StartInteraction() override {}
  void EndInteraction() override {}
  void OnKeyDown(vtkRenderWindowInteractor* vtkNotUsed(rwi)) override {}
  void OnKeyUp(vtkRenderWindowInteractor* vtkNotUsed(rwi)) override {}
  ///@}

  ///@{
  /**
   * Event bindings controlling the effects of pressing mouse buttons
   * or moving the mouse.
   */
  void OnMouseMove(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi) override;
  void OnButtonDown(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi) override;
  void OnButtonUp(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi) override;
  ///@}

  ///@{
  /**
   * Set and get the speed of flying.
   */
  vtkSetClampMacro(FlySpeed, double, 1, 30);
  vtkGetMacro(FlySpeed, double);
  ///@}

protected:
  vtkJoystickFly();
  ~vtkJoystickFly() override;

  // Subclasses set to 1 for fly in and -1 for fly out.
  int In = -1;

private:
  vtkJoystickFly(const vtkJoystickFly&) = delete;
  void operator=(const vtkJoystickFly&) = delete;

  int FlyFlag = 0;

  double FlySpeed = 20.0;
  double LastRenderTime = 0.1;

  void Fly(vtkRenderer* ren, vtkRenderWindowInteractor* rwi, double scale, double speed);
};
VTK_ABI_NAMESPACE_END
#endif
