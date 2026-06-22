// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTrackballEnvironmentRotate
 * @brief   Rotates the environment with xy mouse movement.
 *
 * vtkTrackballEnvironmentRotate allows the user to rotate the renderer environment.
 */

#ifndef vtkTrackballEnvironmentRotate_h
#define vtkTrackballEnvironmentRotate_h

#include "vtkCameraManipulator.h"

#include "vtkInteractionStyleModule.h" // needed for export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKINTERACTIONSTYLE_EXPORT vtkTrackballEnvironmentRotate : public vtkCameraManipulator
{
public:
  static vtkTrackballEnvironmentRotate* New();
  vtkTypeMacro(vtkTrackballEnvironmentRotate, vtkCameraManipulator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Unimplemented methods from vtkCameraManipulator.
   */
  void StartInteraction() override {};
  void EndInteraction() override {};
  void OnKeyDown(vtkRenderWindowInteractor* vtkNotUsed(rwi)) override {}
  void OnKeyUp(vtkRenderWindowInteractor* vtkNotUsed(rwi)) override {}
  void OnButtonDown(int vtkNotUsed(x), int vtkNotUsed(y), vtkRenderer* vtkNotUsed(ren),
    vtkRenderWindowInteractor* vtkNotUsed(rwi)) override
  {
  }
  void OnButtonUp(int vtkNotUsed(x), int vtkNotUsed(y), vtkRenderer* vtkNotUsed(ren),
    vtkRenderWindowInteractor* vtkNotUsed(rwi)) override
  {
  }
  ///@}

  ///@{
  /**
   * Event bindings controlling the effects of pressing mouse buttons
   * or moving the mouse.
   */
  void OnMouseMove(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi) override;
  ///@}

protected:
  vtkTrackballEnvironmentRotate() = default;
  ~vtkTrackballEnvironmentRotate() override = default;

private:
  vtkTrackballEnvironmentRotate(const vtkTrackballEnvironmentRotate&) = delete;
  void operator=(const vtkTrackballEnvironmentRotate&) = delete;

  void EnvironmentRotate(vtkRenderer* ren, vtkRenderWindowInteractor* rwi);
};
VTK_ABI_NAMESPACE_END
#endif
