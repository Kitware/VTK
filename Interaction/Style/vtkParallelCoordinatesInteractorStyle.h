/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParallelCoordinatesInteractorStyle.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// .NAME vtkParallelCoordinatesInteractorStyle - interactive manipulation of the camera specialized for parallel coordinates
// .SECTION Description
// vtkParallelCoordinatesInteractorStyle allows the user to interactively manipulate
// (rotate, pan, zoomm etc.) the camera.
// Several events are overloaded from its superclass
// vtkParallelCoordinatesInteractorStyle, hence the mouse bindings are
// different. (The bindings
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

#ifndef __vtkParallelCoordinatesInteractorStyle_h
#define __vtkParallelCoordinatesInteractorStyle_h

#include "vtkInteractionStyleModule.h" // For export macro
#include "vtkInteractorStyleTrackballCamera.h"

class vtkViewport;

class VTKINTERACTIONSTYLE_EXPORT vtkParallelCoordinatesInteractorStyle : public vtkInteractorStyleTrackballCamera
{
public:
  static vtkParallelCoordinatesInteractorStyle *New();
  vtkTypeMacro(vtkParallelCoordinatesInteractorStyle, vtkInteractorStyleTrackballCamera);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  enum {
    INTERACT_HOVER=0,
    INTERACT_INSPECT,
    INTERACT_ZOOM,
    INTERACT_PAN
  };
  //ETX

  // Description:
  // Get the cursor positions in pixel coords
  vtkGetVector2Macro(CursorStartPosition,int);
  vtkGetVector2Macro(CursorCurrentPosition,int);
  vtkGetVector2Macro(CursorLastPosition,int);

  // Description:
  // Get the cursor positions in a given coordinate system
  void GetCursorStartPosition(vtkViewport *viewport, double pos[2]);
  void GetCursorCurrentPosition(vtkViewport *viewport, double pos[2]);
  void GetCursorLastPosition(vtkViewport *viewport, double pos[2]);

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
  virtual void OnLeave();

  virtual void StartInspect(int x, int y);
  virtual void Inspect(int x, int y);
  virtual void EndInspect();

  virtual void StartZoom();
  virtual void Zoom();
  virtual void EndZoom();

  virtual void StartPan();
  virtual void Pan();
  virtual void EndPan();

  // Description:
  // Override the "fly-to" (f keypress) for images.
  virtual void OnChar();
protected:
  vtkParallelCoordinatesInteractorStyle();
  ~vtkParallelCoordinatesInteractorStyle();

  int CursorStartPosition[2];
  int CursorCurrentPosition[2];
  int CursorLastPosition[2];

private:
  vtkParallelCoordinatesInteractorStyle(const vtkParallelCoordinatesInteractorStyle&);  // Not implemented.
  void operator=(const vtkParallelCoordinatesInteractorStyle&);  // Not implemented.
};

#endif
