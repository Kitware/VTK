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
// .NAME vtkInteractorStyleImage - interactive manipulation of the camera specialized for images
// .SECTION Description
// vtkInteractorStyleImage allows the user to interactively manipulate
// (rotate, pan, zoomm etc.) the camera.  vtkInteractorStyleImage is specially
// designed to work with images that are being rendered with
// vtkImageActor. Several events are overloaded from its superclass
// vtkInteractorStyle, hence the mouse bindings are different. (The bindings
// keep the camera's view plane normal perpendicular to the x-y plane.) In
// summary the mouse events are as follows:
// + Left Mouse button triggers window level events
// + CTRL Left Mouse spins the camera around its view plane normal
// + SHIFT Left Mouse pans the camera
// + CTRL SHIFT Left Mouse dollys (a positional zoom) the camera
// + Middle mouse button pans the camera
// + Right mouse button dollys the camera.
// + SHIFT Right Mouse triggers pick events
//
// Note that the renderer's actors are not moved; instead the camera is moved.

// .SECTION See Also
// vtkInteractorStyle vtkInteractorStyleTrackballActor 
// vtkInteractorStyleJoystickCamera vtkInteractorStyleJoystickActor

#ifndef __vtkInteractorStyleImage_h
#define __vtkInteractorStyleImage_h

#include "vtkInteractorStyle.h"


#define VTK_INTERACTOR_STYLE_IMAGE_NONE    0
#define VTK_INTERACTOR_STYLE_IMAGE_WINDOW_LEVEL  1
#define VTK_INTERACTOR_STYLE_IMAGE_PAN     2
#define VTK_INTERACTOR_STYLE_IMAGE_ZOOM    3
#define VTK_INTERACTOR_STYLE_IMAGE_SPIN    4
#define VTK_INTERACTOR_STYLE_IMAGE_PICK    5

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
  // Override the "fly-to" (f keypress) for images.
  void OnChar   (int ctrl, int shift, char keycode, int repeatcount);

  // Description:
  // Some useful information for handling window level
  vtkGetVector2Macro(WindowLevelStartPosition,int);
  vtkGetVector2Macro(WindowLevelCurrentPosition,int);
  
  // Description:
  // Some useful information for interaction
  vtkSetClampMacro(State,int,VTK_INTERACTOR_STYLE_IMAGE_NONE,VTK_INTERACTOR_STYLE_IMAGE_PICK);
  vtkGetMacro(State,int);
  
protected:
  vtkInteractorStyleImage();
  ~vtkInteractorStyleImage();

  void WindowLevelXY(int dx, int dy);
  void PanXY(int x, int y, int oldX, int oldY);
  void DollyXY(int dx, int dy);
  void SpinXY(int dx, int dy, int oldX, int oldY);
  void PickXY(int x, int y);
  
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
