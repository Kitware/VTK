/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleJoystickCamera.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkInteractorStyleJoystickCamera - interactive manipulation of the camera

// .SECTION Description
// vtkInteractorStyleJoystickCamera allows the user to move (rotate, pan,
// etc.) the camera, the point of view for the scene.  The position of the
// mouse relative to the center of the scene determines the speed at which
// the camera moves, and the speed of the mouse movement determines the
// acceleration of the camera, so the camera continues to move even if the
// mouse if not moving.
// For a 3-button mouse, the left button is for rotation, the right button
// for zooming, the middle button for panning, and ctrl + left button for
// spinning.  (With fewer mouse buttons, ctrl + shift + left button is
// for zooming, and shift + left button is for panning.)

// .SECTION See Also
// vtkInteractorStyleJoystickActor vtkInteractorStyleTrackballCamera
// vtkInteractorStyleTrackballActor

#ifndef __vtkInteractorStyleJoystickCamera_h
#define __vtkInteractorStyleJoystickCamera_h

#include "vtkInteractorStyle.h"


#define VTK_INTERACTOR_STYLE_CAMERA_NONE    0
#define VTK_INTERACTOR_STYLE_CAMERA_ROTATE  1
#define VTK_INTERACTOR_STYLE_CAMERA_PAN     2
#define VTK_INTERACTOR_STYLE_CAMERA_ZOOM    3
#define VTK_INTERACTOR_STYLE_CAMERA_SPIN    4

class VTK_RENDERING_EXPORT vtkInteractorStyleJoystickCamera : public vtkInteractorStyle
{
public:
  static vtkInteractorStyleJoystickCamera *New();
  vtkTypeRevisionMacro(vtkInteractorStyleJoystickCamera, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Event bindings controlling the effects of pressing mouse buttons
  // or moving the mouse.
  void OnMouseMove  (int ctrl, int shift, int x, int y);
  void OnLeftButtonDown(int ctrl, int shift, int x, int y);
  void OnLeftButtonUp  (int ctrl, int shift, int x, int y);
  void OnMiddleButtonDown(int ctrl, int shift, int x, int y);
  void OnMiddleButtonUp  (int ctrl, int shift, int x, int y);
  void OnRightButtonDown(int ctrl, int shift, int x, int y);
  void OnRightButtonUp  (int ctrl, int shift, int x, int y);
  void OnTimer(void); 

protected:
  vtkInteractorStyleJoystickCamera();
  ~vtkInteractorStyleJoystickCamera();
  
  int State;
  float MotionFactor;
private:
  vtkInteractorStyleJoystickCamera(const vtkInteractorStyleJoystickCamera&);  // Not implemented.
  void operator=(const vtkInteractorStyleJoystickCamera&);  // Not implemented.
};

#endif
