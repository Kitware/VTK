/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleTrackballCamera.h
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
// .NAME vtkInteractorStyleTrackballCamera - interactive manipulation of the camera
// .SECTION Description
// vtkInteractorStyleTrackballCamera allows the user to interactively
// manipulate (rotate, pan, etc.) the camera, the viewpoint of the scene.
// The position of the mouse relative to the center of the scene determines
// the speed at which the camera moves.  When the mouse stops moving, so
// does the camera.
// For a 3-button mouse, the left button is for rotation, the right button
// for zooming, the middle button for panning, and ctrl + left button for
// spinning.  (With fewer mouse buttons, ctrl + shift + left button is
// for zooming, and shift + left button is for panning.)

// .SECTION See Also
// vtkInteractorStyleTrackballActor vtkInteractorStyleJoystickCamera
// vtkInteractorStyleJoystickActor

#ifndef __vtkInteractorStyleTrackballCamera_h
#define __vtkInteractorStyleTrackballCamera_h

#include "vtkInteractorStyle.h"

class VTK_RENDERING_EXPORT vtkInteractorStyleTrackballCamera : public vtkInteractorStyle
{
public:
  static vtkInteractorStyleTrackballCamera *New();
  vtkTypeRevisionMacro(vtkInteractorStyleTrackballCamera,vtkInteractorStyle);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkInteractorStyleTrackballCamera();
  ~vtkInteractorStyleTrackballCamera();

  // Description:
  // Event bindings controlling the effects of pressing mouse buttons
  // or moving the mouse.
  virtual void OnMouseMove       (int ctrl, int shift, int x, int y);
  virtual void OnLeftButtonDown  (int ctrl, int shift, int x, int y);
  virtual void OnLeftButtonUp    (int ctrl, int shift, int x, int y);
  virtual void OnMiddleButtonDown(int ctrl, int shift, int x, int y);
  virtual void OnMiddleButtonUp  (int ctrl, int shift, int x, int y);
  virtual void OnRightButtonDown (int ctrl, int shift, int x, int y);
  virtual void OnRightButtonUp   (int ctrl, int shift, int x, int y);

  // These methods for the different interactions in different modes
  // are overridden in subclasses to perform the correct motion. Since
  // they are called by OnTimer, they do not have mouse coord parameters
  // (use interactor's GetEventPosition and GetLastEventPosition)
  virtual void Rotate();
  virtual void Spin();
  virtual void Pan();
  virtual void Dolly();
  
  float MotionFactor;

private:
  vtkInteractorStyleTrackballCamera(const vtkInteractorStyleTrackballCamera&);  // Not implemented.
  void operator=(const vtkInteractorStyleTrackballCamera&);  // Not implemented.
};

#endif
