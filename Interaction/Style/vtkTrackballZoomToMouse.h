// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTrackballZoomToMouse
 * @brief   Zooms camera with vertical mouse movement to mouse position.
 *
 * vtkTrackballZoomToMouse is a redifinition of a vtkTrackballZoom
 * allowing the user to zoom at the point projected under the mouse position.
 */

#ifndef vtkTrackballZoomToMouse_h
#define vtkTrackballZoomToMouse_h

#include "vtkTrackballZoom.h"

#include "vtkInteractionStyleModule.h" // needed for export macro
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class VTKINTERACTIONSTYLE_EXPORT VTK_MARSHALAUTO vtkTrackballZoomToMouse : public vtkTrackballZoom
{
public:
  static vtkTrackballZoomToMouse* New();
  vtkTypeMacro(vtkTrackballZoomToMouse, vtkTrackballZoom);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Event bindings controlling the effects of pressing mouse buttons
   * or moving the mouse.
   */
  void OnMouseMove(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi) override;
  void OnButtonDown(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi) override;
  ///@}

protected:
  vtkTrackballZoomToMouse();
  ~vtkTrackballZoomToMouse() override;

private:
  vtkTrackballZoomToMouse(const vtkTrackballZoomToMouse&) = delete;
  void operator=(const vtkTrackballZoomToMouse&) = delete;

  int ZoomPosition[2] = { 0, 0 };
};
VTK_ABI_NAMESPACE_END
#endif
