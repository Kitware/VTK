/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleTrackballActor.h
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
// .NAME vtkInteractorStyleTrackballActor - manipulate objects in the scene independent of each other
// .SECTION Description
// vtkInteractorStyleTrackballActor allows the user to interact with (rotate,
// pan, etc.) objects in the scene indendent of each other.  The position
// of the mouse relative to the center of the object determine's the speed
// at which the object moves.  When the mouse stops moving, so does the
// object being manipulated.
// For a 3-button mouse, the left button is for rotation, the right button
// for zooming, the middle button for panning, and ctrl + left button for
// spinning.  (With fewer mouse buttons, ctrl + shift + left button is
// for zooming, and shift + left button is for panning.)

// .SECTION See Also
// vtkInteractorStyleTrackballCamera vtkInteractorStyleJoystickActor
// vtkInteractorStyleJoystickCamera

#ifndef __vtkInteractorStyleTrackballActor_h
#define __vtkInteractorStyleTrackballActor_h

#include "vtkInteractorStyle.h"
#include "vtkCellPicker.h"

#define VTK_INTERACTOR_STYLE_ACTOR_NONE    0
#define VTK_INTERACTOR_STYLE_ACTOR_ROTATE  1
#define VTK_INTERACTOR_STYLE_ACTOR_PAN     2
#define VTK_INTERACTOR_STYLE_ACTOR_ZOOM    3
#define VTK_INTERACTOR_STYLE_ACTOR_SPIN    4
#define VTK_INTERACTOR_STYLE_ACTOR_SCALE   5

class VTK_RENDERING_EXPORT vtkInteractorStyleTrackballActor : public vtkInteractorStyle
{
public:
  static vtkInteractorStyleTrackballActor *New();
  vtkTypeRevisionMacro(vtkInteractorStyleTrackballActor, vtkObject);
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

protected:
  vtkInteractorStyleTrackballActor();
  ~vtkInteractorStyleTrackballActor();

  void RotateXY(int x, int y, int oldX, int oldY);
  void PanXY(int x, int y, int oldX, int oldY);
  void DollyXY(int dx, int dy);
  void SpinXY(int dx, int dy, int oldX, int oldY);
  void ScaleXY(int x, int y, int oldX, int oldY);
  void FindPickedActor(int x, int y);
  void Prop3DTransform(vtkProp3D *prop3D, double *boxCenter,
                      int numRotation, double **rotate,
                      double *scale);
  void Prop3DTransform(vtkProp3D *prop3D,float *boxCenter,
                      int NumRotation,double **rotate,
                      double *scale);
  
  int State;
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
//  vtkAbstractPropPicker *InteractionPicker;
  vtkCellPicker *InteractionPicker;
private:
  vtkInteractorStyleTrackballActor(const vtkInteractorStyleTrackballActor&);  // Not implemented.
  void operator=(const vtkInteractorStyleTrackballActor&);  // Not implemented.
};

#endif
