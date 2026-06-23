// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTrackballZoom
 * @brief   Zooms camera with vertical mouse movement.
 *
 * vtkTrackballZoom allows the user to interactively
 * manipulate the camera, the viewpoint of the scene.
 * Moving the mouse down zooms in. Up zooms out.
 *
 * When zooming in Perspective projection, this manipulator by default, dollys
 * (rather than zooms) i.e. moves the camera further (or farther) in the view
 * direction. This is default behavior. You can change that to use zoom instead
 * i.e. change view angle on camera, by setting UseDollyForPerspectiveProjection
 * to off.
 */

#ifndef vtkTrackballZoom_h
#define vtkTrackballZoom_h

#include "vtkCameraManipulator.h"

#include "vtkInteractionStyleModule.h" // needed for export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKINTERACTIONSTYLE_EXPORT vtkTrackballZoom : public vtkCameraManipulator
{
public:
  static vtkTrackballZoom* New();
  vtkTypeMacro(vtkTrackballZoom, vtkCameraManipulator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Unimplemented methods from vtkCameraManipulator.
   */
  void StartInteraction() override {};
  void EndInteraction() override {};
  void OnKeyDown(vtkRenderWindowInteractor* vtkNotUsed(rwi)) override {}
  void OnKeyUp(vtkRenderWindowInteractor* vtkNotUsed(rwi)) override {}
  void OnButtonUp(int, int, vtkRenderer*, vtkRenderWindowInteractor*) override {}
  ///@}

  ///@{
  /**
   * Event bindings controlling the effects of pressing mouse buttons
   * or moving the mouse.
   */
  void OnMouseMove(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi) override;
  void OnButtonDown(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi) override;
  ///@}

  /**
   * Set this to false (true by default) to not dolly in case of perspective
   * projection and use zoom i.e. change view angle, instead.
   */
  vtkSetMacro(UseDollyForPerspectiveProjection, bool);
  vtkGetMacro(UseDollyForPerspectiveProjection, bool);
  vtkBooleanMacro(UseDollyForPerspectiveProjection, bool);

protected:
  vtkTrackballZoom();
  ~vtkTrackballZoom() override;

  // Subclass can access it
  double ZoomScale = 0.0;

private:
  vtkTrackballZoom(const vtkTrackballZoom&) = delete;
  void operator=(const vtkTrackballZoom&) = delete;

  bool UseDollyForPerspectiveProjection = true;
};
VTK_ABI_NAMESPACE_END
#endif
