/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleTrackballCamera.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
 * panning, and ctrl + left button for spinning.  (With fewer mouse buttons,
 * ctrl + shift + left button is for zooming, and shift + left button is for
 * panning.)
 *
 * @sa
 * vtkInteractorStyleTrackballActor vtkInteractorStyleJoystickCamera
 * vtkInteractorStyleJoystickActor
*/

#ifndef vtkInteractorStyleTrackballCamera_h
#define vtkInteractorStyleTrackballCamera_h

#include "vtkInteractionStyleModule.h" // For export macro
#include "vtkInteractorStyle.h"

class VTKINTERACTIONSTYLE_EXPORT vtkInteractorStyleTrackballCamera : public vtkInteractorStyle
{
public:
  static vtkInteractorStyleTrackballCamera *New();
  vtkTypeMacro(vtkInteractorStyleTrackballCamera,vtkInteractorStyle);
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

  //@{
  /**
   * Set the apparent sensitivity of the interactor style to mouse motion.
   */
  vtkSetMacro(MotionFactor,double);
  vtkGetMacro(MotionFactor,double);
  //@}

protected:
  vtkInteractorStyleTrackballCamera();
  ~vtkInteractorStyleTrackballCamera() VTK_OVERRIDE;

  double MotionFactor;

  virtual void Dolly(double factor);

private:
  vtkInteractorStyleTrackballCamera(const vtkInteractorStyleTrackballCamera&) VTK_DELETE_FUNCTION;
  void operator=(const vtkInteractorStyleTrackballCamera&) VTK_DELETE_FUNCTION;
};

#endif
