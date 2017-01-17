/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleMultiTouchCamera.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkInteractorStyleMultiTouchCamera
 * @brief   multitouch manipulation of the camera
 *
 * vtkInteractorStyleMultiTouchCamera allows the user to interactively
 * manipulate (rotate, pan, etc.) the camera, the viewpoint of the scene
 * using multitouch gestures in addition to regular gestures
 *
 * @sa
 * vtkInteractorStyleTrackballActor vtkInteractorStyleJoystickCamera
 * vtkInteractorStyleJoystickActor
*/

#ifndef vtkInteractorStyleMultiTouchCamera_h
#define vtkInteractorStyleMultiTouchCamera_h

#include "vtkInteractionStyleModule.h" // For export macro
#include "vtkRenderWindowInteractor.h" // for max pointers
#include "vtkInteractorStyleTrackballCamera.h"

class VTKINTERACTIONSTYLE_EXPORT vtkInteractorStyleMultiTouchCamera : public vtkInteractorStyleTrackballCamera
{
public:
  static vtkInteractorStyleMultiTouchCamera *New();
  vtkTypeMacro(vtkInteractorStyleMultiTouchCamera,vtkInteractorStyleTrackballCamera);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Event bindings for gestures
   */
  void OnRotate() VTK_OVERRIDE;
  void OnPinch() VTK_OVERRIDE;
  void OnPan() VTK_OVERRIDE;
  //@}

protected:
  vtkInteractorStyleMultiTouchCamera();
  ~vtkInteractorStyleMultiTouchCamera() VTK_OVERRIDE;

private:
  vtkInteractorStyleMultiTouchCamera(const vtkInteractorStyleMultiTouchCamera&) VTK_DELETE_FUNCTION;
  void operator=(const vtkInteractorStyleMultiTouchCamera&) VTK_DELETE_FUNCTION;
};

#endif
