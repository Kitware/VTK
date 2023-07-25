// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenVRDefaultOverlay
 * @brief   OpenVR overlay
 *
 * vtkOpenVRDefaultOverlay support for VR overlays
 */

#ifndef vtkOpenVRDefaultOverlay_h
#define vtkOpenVRDefaultOverlay_h

#include "vtkOpenVROverlay.h"
#include "vtkRenderingOpenVRModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGOPENVR_EXPORT vtkOpenVRDefaultOverlay : public vtkOpenVROverlay
{
public:
  static vtkOpenVRDefaultOverlay* New();
  vtkTypeMacro(vtkOpenVRDefaultOverlay, vtkOpenVROverlay);

  /**
   * Render the overlay, we set some opf the spots based on current settings
   */
  void Render() override;

protected:
  vtkOpenVRDefaultOverlay() = default;
  ~vtkOpenVRDefaultOverlay() override = default;

  void SetupSpots() override;

private:
  vtkOpenVRDefaultOverlay(const vtkOpenVRDefaultOverlay&) = delete;
  void operator=(const vtkOpenVRDefaultOverlay&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
