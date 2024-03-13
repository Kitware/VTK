// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2009 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
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
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkViewport;

class VTKINTERACTIONSTYLE_EXPORT VTK_MARSHALAUTO vtkParallelCoordinatesInteractorStyle
  : public vtkInteractorStyleTrackballCamera
{
public:
  static vtkParallelCoordinatesInteractorStyle* New();
  vtkTypeMacro(vtkParallelCoordinatesInteractorStyle, vtkInteractorStyleTrackballCamera);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum
  {
    INTERACT_HOVER = 0,
    INTERACT_INSPECT,
    INTERACT_ZOOM,
    INTERACT_PAN
  };

  ///@{
  /**
   * Get the cursor positions in pixel coords
   */
  vtkGetVector2Macro(CursorStartPosition, int);
  vtkGetVector2Macro(CursorCurrentPosition, int);
  vtkGetVector2Macro(CursorLastPosition, int);
  ///@}

  ///@{
  /**
   * Get the cursor positions in a given coordinate system
   */
  void GetCursorStartPosition(vtkViewport* viewport, double pos[2]);
  void GetCursorCurrentPosition(vtkViewport* viewport, double pos[2]);
  void GetCursorLastPosition(vtkViewport* viewport, double pos[2]);
  ///@}

  ///@{
  /**
   * Event bindings controlling the effects of pressing mouse buttons
   * or moving the mouse.
   */
  void OnMouseMove() override;
  void OnLeftButtonDown() override;
  void OnLeftButtonUp() override;
  void OnMiddleButtonDown() override;
  void OnMiddleButtonUp() override;
  void OnRightButtonDown() override;
  void OnRightButtonUp() override;
  void OnLeave() override;
  ///@}

  ///@{
  virtual void StartInspect(int x, int y);
  virtual void Inspect(int x, int y);
  virtual void EndInspect();
  ///@}

  ///@{
  void StartZoom() override;
  void Zoom() override;
  void EndZoom() override;
  ///@}

  ///@{
  void StartPan() override;
  void Pan() override;
  void EndPan() override;
  ///@}

  /**
   * Override the "fly-to" (f keypress) for images.
   */
  void OnChar() override;

protected:
  vtkParallelCoordinatesInteractorStyle();
  ~vtkParallelCoordinatesInteractorStyle() override;

  int CursorStartPosition[2];
  int CursorCurrentPosition[2];
  int CursorLastPosition[2];

private:
  vtkParallelCoordinatesInteractorStyle(const vtkParallelCoordinatesInteractorStyle&) = delete;
  void operator=(const vtkParallelCoordinatesInteractorStyle&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
