/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleJoystickActor.h
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
// .NAME vtkInteractorStyleJoystickActor - manipulate objects in the scene independently of one another
// .SECTION Description
// The class vtkInteractorStyleJoystickActor allows the user to interact
// with (rotate, zoom, etc.) separate objects in the scene independent of
// each other.  The position of the mouse relative to the center of the
// object determines the speed of the object's motion.  The mouse's velocity
// detemines the acceleration of the object's motion, so the object will
// continue moving even when the mouse is not moving.
// For a 3-button mouse, the left button is for rotation, the right button
// for zooming, the middle button for panning, and ctrl + left button for
// spinning.  (With fewer mouse buttons, ctrl + shift + left button is
// for zooming, and shift + left button is for panning.)
// .SECTION See Also
// vtkInteractorStyleJoystickCamera vtkInteractorStyleTrackballActor
// vtkInteractorStyleTrackballCamera

#ifndef __vtkInteractorStyleJoystickActor_h
#define __vtkInteractorStyleJoystickActor_h

#include "vtkInteractorStyle.h"
#include "vtkCellPicker.h"

// Motion flags

class VTK_RENDERING_EXPORT vtkInteractorStyleJoystickActor : public vtkInteractorStyle
{
public:
  static vtkInteractorStyleJoystickActor *New();

  vtkTypeRevisionMacro(vtkInteractorStyleJoystickActor,vtkInteractorStyle);
  void PrintSelf(ostream& os, vtkIndent indent);

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

protected:
  vtkInteractorStyleJoystickActor();
  ~vtkInteractorStyleJoystickActor();

  // These methods for the different interactions in different modes
  // are overridden in subclasses to perform the correct motion. Since
  // they might be called from OnTimer, they do not have mouse coord parameters
  // (use GetLastPos and interactor GetEventPosition)
  virtual void Rotate();
  virtual void Spin();
  virtual void Pan();
  virtual void Dolly();
  virtual void UniformScale();

  void FindPickedActor(int x, int y);

  void Prop3DTransform(vtkProp3D *prop3D, double *boxCenter,
                       int numRotation, double **rotate,
                       double *scale);

  void Prop3DTransform(vtkProp3D *prop3D,float *boxCenter,
                       int NumRotation,double **rotate,
                       double *scale);
  
  float MotionFactor;
  float RadianToDegree;                 // constant: for conv from deg to rad
  vtkProp3D *InteractionProp;
  double ViewUp[3];
  double ViewLook[3];
  double ViewRight[3];
  float ObjCenter[3];
  float DispObjCenter[3];
  float Radius;
  float NewPickPoint[4];
  float OldPickPoint[4];
  float MotionVector[3];
  double ViewPoint[3];
  double ViewFocus[3];

  vtkCellPicker *InteractionPicker;

private:
  vtkInteractorStyleJoystickActor(const vtkInteractorStyleJoystickActor&);  // Not implemented.
  void operator=(const vtkInteractorStyleJoystickActor&);  // Not implemented.
};

#endif
