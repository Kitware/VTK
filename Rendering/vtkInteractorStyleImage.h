/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleImage.h
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
// .NAME vtkInteractorStyleImage - interactive manipulation of the camera
// .SECTION Description
// vtkInteractorStyleImage allows the user to interactively
// manipulate (rotate, pan, etc.) the camera, the viewpoint of the scene.
// For a 3-button mouse, the left button is for rotation, the right button
// for zooming, the middle button for panning, and ctrl + left button for
// spinning.  (With fewer mouse buttons, ctrl + shift + left button is
// for zooming, and shift + left button is for panning.)

// .SECTION See Also
// vtkInteractorStyleTrackballActor vtkInteractorStyleJoystickCamera
// vtkInteractorStyleJoystickActor

#ifndef __vtkInteractorStyleImage_h
#define __vtkInteractorStyleImage_h

#include "vtkInteractorStyle.h"


#define VTK_INTERACTOR_STYLE_IMAGE_NONE    0
#define VTK_INTERACTOR_STYLE_IMAGE_WINDOW_LEVEL  1
#define VTK_INTERACTOR_STYLE_IMAGE_PAN     2
#define VTK_INTERACTOR_STYLE_IMAGE_ZOOM    3
#define VTK_INTERACTOR_STYLE_IMAGE_SPIN    4

class VTK_RENDERING_EXPORT vtkInteractorStyleImage : public vtkInteractorStyle
{
public:
  static vtkInteractorStyleImage *New();
  vtkTypeRevisionMacro(vtkInteractorStyleImage, vtkInteractorStyle);
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

  // Description:
  // Some useful information for handling window level
  vtkGetVector2Macro(WindowLevelStartPosition,int);
  vtkGetVector2Macro(WindowLevelCurrentPosition,int);
  
protected:
  vtkInteractorStyleImage();
  ~vtkInteractorStyleImage();

  void WindowLevelXY(int dx, int dy);
  void PanXY(int x, int y, int oldX, int oldY);
  void DollyXY(int dx, int dy);
  void SpinXY(int dx, int dy, int oldX, int oldY);
  
  int WindowLevelStartPosition[2];
  int WindowLevelCurrentPosition[2];
  int State;
  float MotionFactor;
  float RadianToDegree; // constant: for conv from deg to rad
private:
  vtkInteractorStyleImage(const vtkInteractorStyleImage&);  // Not implemented.
  void operator=(const vtkInteractorStyleImage&);  // Not implemented.
};

#endif
