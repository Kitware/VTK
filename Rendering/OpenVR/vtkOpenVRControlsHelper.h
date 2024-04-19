// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenVRControlsHelper
 * @brief   Tooltip helper explaining controls
 * Helper class to draw one tooltip per button around the controller.
 *
 * @sa
 * vtkOpenVRPanelRepresentation
 */

#ifndef vtkOpenVRControlsHelper_h
#define vtkOpenVRControlsHelper_h

#include "vtkRenderingOpenVRModule.h" // For export macro
#include "vtkVRControlsHelper.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGOPENVR_EXPORT vtkOpenVRControlsHelper : public vtkVRControlsHelper
{
public:
  /**
   * Instantiate the class.
   */
  static vtkOpenVRControlsHelper* New();
  vtkTypeMacro(vtkOpenVRControlsHelper, vtkVRControlsHelper);

protected:
  vtkOpenVRControlsHelper() = default;
  ~vtkOpenVRControlsHelper() override = default;

  void InitControlPosition() override;

private:
  vtkOpenVRControlsHelper(const vtkOpenVRControlsHelper&) = delete;
  void operator=(const vtkOpenVRControlsHelper&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
