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
// (rotate, pan, zoom etc.) the camera. vtkInteractorStyleImage is specially
// designed to work with images that are being rendered with
// vtkImageActor. Several events are overloaded from its superclass
// vtkInteractorStyle, hence the mouse bindings are different. (The bindings
// keep the camera's view plane normal perpendicular to the x-y plane.) In
// summary the mouse events for 2D image interaction are as follows:
// - Left Mouse button triggers window level events
// - CTRL Left Mouse spins the camera around its view plane normal
// - SHIFT Left Mouse pans the camera
// - CTRL SHIFT Left Mouse dollys (a positional zoom) the camera
// - Middle mouse button pans the camera
// - Right mouse button dollys the camera.
// - SHIFT Right Mouse triggers pick events
//
// If SetInteractionModeToImageSlicing() is called, then some of the mouse
// events are changed as follows:
// - CTRL Left Mouse slices through the image
// - SHIFT Middle Mouse slices through the image
// - CTRL Right Mouse spins the camera
//
// If SetInteractionModeToImage3D() is called, then some of the mouse
// events are changed as follows:
// - SHIFT Left Mouse rotates the camera for oblique slicing
// - SHIFT Middle Mouse slices through the image
// - CTRL Right Mouse also slices through the image
//
// In all modes, the following key bindings are in effect:
// - R Reset the Window/Level
// - X Reset to a sagittal view
// - Y Reset to a coronal view
// - Z Reset to an axial view
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
#define VTKIS_SLICE        1026

// Style flags

#define VTKIS_IMAGE2D 2
#define VTKIS_IMAGE3D 3
#define VTKIS_IMAGE_SLICING 4

class vtkImageProperty;

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
  virtual void OnMiddleButtonDown();
  virtual void OnMiddleButtonUp();
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
  virtual void Slice();
  
  // Interaction mode entry points used internally.  
  virtual void StartWindowLevel();
  virtual void EndWindowLevel();
  virtual void StartPick();
  virtual void EndPick();
  virtual void StartSlice();
  virtual void EndSlice();

  // Description:
  // Set/Get current mode to 2D or 3D.  The default is 2D.  In 3D mode,
  // it is possible to rotate the camera to view oblique slices.  In Slicing
  // mode, it is possible to slice through the data, but not to generate oblique
  // views by rotating the camera.
  vtkSetClampMacro(InteractionMode, int, VTKIS_IMAGE2D, VTKIS_IMAGE_SLICING);
  vtkGetMacro(InteractionMode, int);
  void SetInteractionModeToImage2D() {
    this->SetInteractionMode(VTKIS_IMAGE2D); }
  void SetInteractionModeToImage3D() {
    this->SetInteractionMode(VTKIS_IMAGE3D); }
  void SetInteractionModeToImageSlicing() {
    this->SetInteractionMode(VTKIS_IMAGE_SLICING); }

  // Description:
  // Set the orientations that will be used when the X, Y, or Z
  // keys are pressed.  See SetImageOrientation for more information.
  vtkSetVector3Macro(XViewRightVector, double);
  vtkGetVector3Macro(XViewRightVector, double);
  vtkSetVector3Macro(XViewUpVector, double);
  vtkGetVector3Macro(XViewUpVector, double);
  vtkSetVector3Macro(YViewRightVector, double);
  vtkGetVector3Macro(YViewRightVector, double);
  vtkSetVector3Macro(YViewUpVector, double);
  vtkGetVector3Macro(YViewUpVector, double);
  vtkSetVector3Macro(ZViewRightVector, double);
  vtkGetVector3Macro(ZViewRightVector, double);
  vtkSetVector3Macro(ZViewUpVector, double);
  vtkGetVector3Macro(ZViewUpVector, double);

  // Description:
  // Set the view orientation, in terms of the horizontal and
  // vertical directions of the computer screen.  The first
  // vector gives the direction that will correspond to moving
  // horizontally left-to-right across the screen, and the
  // second vector gives the direction that will correspond to
  // moving bottom-to-top up the screen.  This method changes
  // the position of the camera to provide the desired view.
  void SetImageOrientation(const double leftToRight[3],
                           const double bottomToTop[3]);

protected:
  vtkInteractorStyleImage();
  ~vtkInteractorStyleImage();

  void SetCurrentImageToNthImage(int i);

  int WindowLevelStartPosition[2];
  int WindowLevelCurrentPosition[2];
  double WindowLevelInitial[2];
  vtkImageProperty *CurrentImageProperty;
 
  int InteractionMode;
  double XViewRightVector[3];
  double XViewUpVector[3];
  double YViewRightVector[3];
  double YViewUpVector[3];
  double ZViewRightVector[3];
  double ZViewUpVector[3];

private:
  vtkInteractorStyleImage(const vtkInteractorStyleImage&);  // Not implemented.
  void operator=(const vtkInteractorStyleImage&);  // Not implemented.
};

#endif
