/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleImage.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkInteractorStyleImage - interactive manipulation of the camera specialized for images
// .SECTION Description
// vtkInteractorStyleImage allows the user to interactively manipulate
// (rotate, pan, zoomm etc.) the camera. vtkInteractorStyleImage is specially
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

#include "vtkInteractorStyleTrackballCamera.h"

// Motion flags

#define VTKIS_WINDOW_LEVEL 1024
#define VTKIS_PICK         1025

class VTK_RENDERING_EXPORT vtkInteractorStyleImage : public vtkInteractorStyleTrackballCamera
{
public:
  static vtkInteractorStyleImage *New();
  vtkTypeMacro(vtkInteractorStyleImage, vtkInteractorStyleTrackballCamera);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Some useful information for handling window level
  vtkGetVector2Macro(WindowLevelStartPosition,int);
  vtkGetVector2Macro(WindowLevelCurrentPosition,int);
  
  // Description:
  // Event bindings controlling the effects of pressing mouse buttons
  // or moving the mouse.
  virtual void OnMouseMove();
  virtual void OnLeftButtonDown();
  virtual void OnLeftButtonUp();
  virtual void OnRightButtonDown();
  virtual void OnRightButtonUp();

  // Description:
  // Override the "fly-to" (f keypress) for images.
  virtual void OnChar();

  // These methods for the different interactions in different modes
  // are overridden in subclasses to perform the correct motion. Since
  // they might be called from OnTimer, they do not have mouse coord parameters
  // (use interactor's GetEventPosition and GetLastEventPosition)
  virtual void WindowLevel();
  virtual void Pick();
  
  // Interaction mode entry points used internally.  
  virtual void StartWindowLevel();
  virtual void EndWindowLevel();
  virtual void StartPick();
  virtual void EndPick();

protected:
  vtkInteractorStyleImage();
  ~vtkInteractorStyleImage();

  int WindowLevelStartPosition[2];
  int WindowLevelCurrentPosition[2];
 
private:
  vtkInteractorStyleImage(const vtkInteractorStyleImage&);  // Not implemented.
  void operator=(const vtkInteractorStyleImage&);  // Not implemented.
};

#endif
