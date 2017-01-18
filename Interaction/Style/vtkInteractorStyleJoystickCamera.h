/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleJoystickCamera.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

class VTKINTERACTIONSTYLE_EXPORT vtkInteractorStyleJoystickCamera : public vtkInteractorStyle
{
public:
  static vtkInteractorStyleJoystickCamera *New();
  vtkTypeMacro(vtkInteractorStyleJoystickCamera,vtkInteractorStyle);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Event bindings controlling the effects of pressing mouse buttons
   * or moving the mouse.
   */
  void OnMouseMove() VTK_OVERRIDE;
  void OnLeftButtonDown() VTK_OVERRIDE;
  void OnLeftButtonUp() VTK_OVERRIDE;
  void OnMiddleButtonDown() VTK_OVERRIDE;
  void OnMiddleButtonUp() VTK_OVERRIDE;
  void OnRightButtonDown() VTK_OVERRIDE;
  void OnRightButtonUp() VTK_OVERRIDE;
  void OnMouseWheelForward() VTK_OVERRIDE;
  void OnMouseWheelBackward() VTK_OVERRIDE;
  //@}

  // These methods for the different interactions in different modes
  // are overridden in subclasses to perform the correct motion. Since
  // they are called by OnTimer, they do not have mouse coord parameters
  // (use interactor's GetEventPosition and GetLastEventPosition)
  void Rotate() VTK_OVERRIDE;
  void Spin() VTK_OVERRIDE;
  void Pan() VTK_OVERRIDE;
  void Dolly() VTK_OVERRIDE;

protected:
  vtkInteractorStyleJoystickCamera();
  ~vtkInteractorStyleJoystickCamera() VTK_OVERRIDE;

  virtual void Dolly(double factor);

private:
  vtkInteractorStyleJoystickCamera(const vtkInteractorStyleJoystickCamera&) VTK_DELETE_FUNCTION;
  void operator=(const vtkInteractorStyleJoystickCamera&) VTK_DELETE_FUNCTION;
};

#endif
