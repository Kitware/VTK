// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenXRInteractorStyle
 * @brief   extended from vtkInteractorStyle3D to override command methods
 */

#ifndef vtkOpenXRInteractorStyle_h
#define vtkOpenXRInteractorStyle_h

#include "vtkRenderingOpenXRModule.h" // For export macro
#include "vtkVRInteractorStyle.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGOPENXR_EXPORT vtkOpenXRInteractorStyle : public vtkVRInteractorStyle
{
public:
  static vtkOpenXRInteractorStyle* New();
  vtkTypeMacro(vtkOpenXRInteractorStyle, vtkVRInteractorStyle);

  /**
   * Setup default actions defined with an action path and a corresponding command.
   */
  void SetupActions(vtkRenderWindowInteractor* iren) override;

  /**
   * Creates a new ControlsHelper suitable for use with this class.
   */
  vtkVRControlsHelper* MakeControlsHelper() override { return nullptr; };

  // likely to be removed
  void LoadNextCameraPose() override {}

protected:
  vtkOpenXRInteractorStyle() = default;
  ~vtkOpenXRInteractorStyle() override = default;

private:
  vtkOpenXRInteractorStyle(const vtkOpenXRInteractorStyle&) = delete;
  void operator=(const vtkOpenXRInteractorStyle&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
