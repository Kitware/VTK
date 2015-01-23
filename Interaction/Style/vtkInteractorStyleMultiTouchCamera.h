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
// .NAME vtkInteractorStyleMultiTouchCamera - interactive manipulation of the camera
// .SECTION Description
// vtkInteractorStyleMultiTouchCamera allows the user to interactively
// manipulate (rotate, pan, etc.) the camera, the viewpoint of the scene.  In
// trackball interaction, the magnitude of the mouse motion is proportional
// to the camera motion associated with a particular mouse binding. For
// example, small left-button motions cause small changes in the rotation of
// the camera around its focal point.

// .SECTION See Also
// vtkInteractorStyleTrackballActor vtkInteractorStyleJoystickCamera
// vtkInteractorStyleJoystickActor

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
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Event bindings controlling the effects of pressing mouse buttons
  // or moving the mouse.
  virtual void OnMouseMove();
  virtual void OnLeftButtonDown();
  virtual void OnLeftButtonUp();

  // These methods for the different interactions in different modes
  // are overridden in subclasses to perform the correct motion. Since
  // they are called by OnTimer, they do not have mouse coord parameters
  // (use interactor's GetEventPosition and GetLastEventPosition)
  virtual void AdjustCamera();

  // Description:
  // Set the apparent sensitivity of the interactor style to mouse motion.
  vtkSetMacro(MotionFactor,double);
  vtkGetMacro(MotionFactor,double);

protected:
  vtkInteractorStyleMultiTouchCamera();
  ~vtkInteractorStyleMultiTouchCamera();

  int PointersDownCount;
  int PointersDown[VTKI_MAX_POINTERS];

  double MotionFactor;

private:
  vtkInteractorStyleMultiTouchCamera(const vtkInteractorStyleMultiTouchCamera&);  // Not implemented.
  void operator=(const vtkInteractorStyleMultiTouchCamera&);  // Not implemented.
};

#endif
