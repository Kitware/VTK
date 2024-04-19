// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkInteractorStyleTrackballCamera
 * @brief   interactive manipulation of the camera
 *
 * vtkInteractorStyleTrackballCamera allows the user to interactively
 * manipulate (rotate, pan, etc.) the camera, the viewpoint of the scene.  In
 * trackball interaction, the magnitude of the mouse motion is proportional
 * to the camera motion associated with a particular mouse binding. For
 * example, small left-button motions cause small changes in the rotation of
 * the camera around its focal point. For a 3-button mouse, the left button
 * is for rotation, the right button for zooming, the middle button for
 * panning, ctrl + left button for spinning, and shift + right button for
 * environment rotation. (With fewer mouse buttons, ctrl + shift + left button
 * is for zooming, and shift + left button is for panning.)
 *
 * @sa
 * vtkInteractorStyleTrackballActor vtkInteractorStyleJoystickCamera
 * vtkInteractorStyleJoystickActor
 */

#ifndef vtkInteractorStyleTrackballCamera_h
#define vtkInteractorStyleTrackballCamera_h

#include "vtkInteractionStyleModule.h" // For export macro
#include "vtkInteractorStyle.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class VTKINTERACTIONSTYLE_EXPORT VTK_MARSHALAUTO vtkInteractorStyleTrackballCamera
  : public vtkInteractorStyle
{
public:
  static vtkInteractorStyleTrackballCamera* New();
  vtkTypeMacro(vtkInteractorStyleTrackballCamera, vtkInteractorStyle);
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
  void EnvironmentRotate() override;

  ///@{
  /**
   * Set the apparent sensitivity of the interactor style to mouse motion.
   */
  vtkSetMacro(MotionFactor, double);
  vtkGetMacro(MotionFactor, double);
  ///@}

protected:
  vtkInteractorStyleTrackballCamera();
  ~vtkInteractorStyleTrackballCamera() override;

  double MotionFactor;

  virtual void Dolly(double factor);

private:
  vtkInteractorStyleTrackballCamera(const vtkInteractorStyleTrackballCamera&) = delete;
  void operator=(const vtkInteractorStyleTrackballCamera&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
