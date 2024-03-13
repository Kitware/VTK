// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkInteractorStyleJoystickCamera
 * @brief   interactive manipulation of the camera
 *
 *
 * vtkInteractorStyleJoystickCamera allows the user to move (rotate, pan,
 * etc.) the camera, the point of view for the scene.  The position of the
 * mouse relative to the center of the scene determines the speed at which
 * the camera moves, and the speed of the mouse movement determines the
 * acceleration of the camera, so the camera continues to move even if the
 * mouse if not moving.
 * For a 3-button mouse, the left button is for rotation, the right button
 * for zooming, the middle button for panning, and ctrl + left button for
 * spinning.  (With fewer mouse buttons, ctrl + shift + left button is
 * for zooming, and shift + left button is for panning.)
 *
 * @sa
 * vtkInteractorStyleJoystickActor vtkInteractorStyleTrackballCamera
 * vtkInteractorStyleTrackballActor
 */

#ifndef vtkInteractorStyleJoystickCamera_h
#define vtkInteractorStyleJoystickCamera_h

#include "vtkInteractionStyleModule.h" // For export macro
#include "vtkInteractorStyle.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class VTKINTERACTIONSTYLE_EXPORT VTK_MARSHALAUTO vtkInteractorStyleJoystickCamera
  : public vtkInteractorStyle
{
public:
  static vtkInteractorStyleJoystickCamera* New();
  vtkTypeMacro(vtkInteractorStyleJoystickCamera, vtkInteractorStyle);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Event bindings controlling the effects of pressing mouse buttons
   * or moving the mouse.
   */
  void OnMouseMove() override;
  void OnLeftButtonDown() override;
  void OnLeftButtonUp() override;
  void OnMiddleButtonDown() override;
  void OnMiddleButtonUp() override;
  void OnRightButtonDown() override;
  void OnRightButtonUp() override;
  void OnMouseWheelForward() override;
  void OnMouseWheelBackward() override;
  ///@}

  // These methods for the different interactions in different modes
  // are overridden in subclasses to perform the correct motion. Since
  // they are called by OnTimer, they do not have mouse coord parameters
  // (use interactor's GetEventPosition and GetLastEventPosition)
  void Rotate() override;
  void Spin() override;
  void Pan() override;
  void Dolly() override;

protected:
  vtkInteractorStyleJoystickCamera();
  ~vtkInteractorStyleJoystickCamera() override;

  virtual void Dolly(double factor);

private:
  vtkInteractorStyleJoystickCamera(const vtkInteractorStyleJoystickCamera&) = delete;
  void operator=(const vtkInteractorStyleJoystickCamera&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
