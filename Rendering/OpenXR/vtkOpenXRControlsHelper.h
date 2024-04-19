// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenXRControlsHelper
 * @brief   Tooltip helper explaining controls
 * Helper class to draw one tooltip per button around the controller.
 *
 * @sa
 * vtkVRPanelRepresentation
 */

#ifndef vtkOpenXRControlsHelper_h
#define vtkOpenXRControlsHelper_h

#include "vtkRenderingOpenXRModule.h" // For export macro
#include "vtkVRControlsHelper.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGOPENXR_EXPORT vtkOpenXRControlsHelper : public vtkVRControlsHelper
{
public:
  /**
   * Instantiate the class.
   */
  static vtkOpenXRControlsHelper* New();
  vtkTypeMacro(vtkOpenXRControlsHelper, vtkVRControlsHelper);

protected:
  vtkOpenXRControlsHelper() = default;
  ~vtkOpenXRControlsHelper() override = default;
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void InitControlPosition() override;

private:
  vtkOpenXRControlsHelper(const vtkOpenXRControlsHelper&) = delete;
  void operator=(const vtkOpenXRControlsHelper&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
