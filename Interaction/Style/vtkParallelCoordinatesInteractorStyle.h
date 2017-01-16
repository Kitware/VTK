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
/**
 * @class   vtkParallelCoordinatesInteractorStyle
 * @brief   interactive manipulation of the camera specialized for parallel coordinates
 *
 * vtkParallelCoordinatesInteractorStyle allows the user to interactively manipulate
 * (rotate, pan, zoom etc.) the camera.
 * Several events are overloaded from its superclass
 * vtkInteractorStyleTrackballCamera, hence the mouse bindings are different.
 * (The bindings keep the camera's view plane normal perpendicular to the x-y plane.)
 * In summary, the mouse events are as follows:
 * + Left Mouse button triggers window level events
 * + CTRL Left Mouse spins the camera around its view plane normal
 * + SHIFT Left Mouse pans the camera
 * + CTRL SHIFT Left Mouse dollys (a positional zoom) the camera
 * + Middle mouse button pans the camera
 * + Right mouse button dollys the camera.
 * + SHIFT Right Mouse triggers pick events
 *
 * Note that the renderer's actors are not moved; instead the camera is moved.
 *
 * @sa
 * vtkInteractorStyle vtkInteractorStyleTrackballActor
 * vtkInteractorStyleJoystickCamera vtkInteractorStyleJoystickActor
*/

#ifndef vtkParallelCoordinatesInteractorStyle_h
#define vtkParallelCoordinatesInteractorStyle_h

#include "vtkInteractionStyleModule.h" // For export macro
#include "vtkInteractorStyleTrackballCamera.h"

class vtkViewport;

class VTKINTERACTIONSTYLE_EXPORT vtkParallelCoordinatesInteractorStyle : public vtkInteractorStyleTrackballCamera
{
public:
  static vtkParallelCoordinatesInteractorStyle *New();
  vtkTypeMacro(vtkParallelCoordinatesInteractorStyle, vtkInteractorStyleTrackballCamera);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  enum {
    INTERACT_HOVER=0,
    INTERACT_INSPECT,
    INTERACT_ZOOM,
    INTERACT_PAN
  };

  //@{
  /**
   * Get the cursor positions in pixel coords
   */
  vtkGetVector2Macro(CursorStartPosition,int);
  vtkGetVector2Macro(CursorCurrentPosition,int);
  vtkGetVector2Macro(CursorLastPosition,int);
  //@}

  //@{
  /**
   * Get the cursor positions in a given coordinate system
   */
  void GetCursorStartPosition(vtkViewport *viewport, double pos[2]);
  void GetCursorCurrentPosition(vtkViewport *viewport, double pos[2]);
  void GetCursorLastPosition(vtkViewport *viewport, double pos[2]);
  //@}

  //@{
  /**
   * Event bindings controlling the effects of pressing mouse buttons
   * or moving the mouse.
   */
  void OnMouseMove() VTK_OVERRIDE;
  void OnLeftButtonDown() VTK_OVERRIDE;
  void OnLeftButtonUp() VTK_OVERRIDE;
  void OnMiddleButtonDown() VTK_OVERRIDE;
  void OnMiddleButtonUp() VTK_OVERRIDE;
  void OnRightButtonDown() VTK_OVERRIDE;
  void OnRightButtonUp() VTK_OVERRIDE;
  void OnLeave() VTK_OVERRIDE;
  //@}

  //@{
  virtual void StartInspect(int x, int y);
  virtual void Inspect(int x, int y);
  virtual void EndInspect();
  //@}

  //@{
  void StartZoom() VTK_OVERRIDE;
  void Zoom() VTK_OVERRIDE;
  void EndZoom() VTK_OVERRIDE;
  //@}

  //@{
  void StartPan() VTK_OVERRIDE;
  void Pan() VTK_OVERRIDE;
  void EndPan() VTK_OVERRIDE;
  //@}

  /**
   * Override the "fly-to" (f keypress) for images.
   */
  void OnChar() VTK_OVERRIDE;

protected:
  vtkParallelCoordinatesInteractorStyle();
  ~vtkParallelCoordinatesInteractorStyle() VTK_OVERRIDE;

  int CursorStartPosition[2];
  int CursorCurrentPosition[2];
  int CursorLastPosition[2];

private:
  vtkParallelCoordinatesInteractorStyle(const vtkParallelCoordinatesInteractorStyle&) VTK_DELETE_FUNCTION;
  void operator=(const vtkParallelCoordinatesInteractorStyle&) VTK_DELETE_FUNCTION;
};

#endif
