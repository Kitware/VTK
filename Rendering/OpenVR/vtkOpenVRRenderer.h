// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenVRRenderer
 * @brief   OpenVR renderer
 *
 * vtkOpenVRRenderer is a concrete implementation of the abstract class
 * vtkVRRenderer.
 */

#ifndef vtkOpenVRRenderer_h
#define vtkOpenVRRenderer_h

#include "vtkRenderingOpenVRModule.h" // For export macro
#include "vtkVRRenderer.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGOPENVR_EXPORT vtkOpenVRRenderer : public vtkVRRenderer
{
public:
  static vtkOpenVRRenderer* New();
  vtkTypeMacro(vtkOpenVRRenderer, vtkVRRenderer);

  /**
   * Create a new Camera suitable for use with this type of Renderer.
   */
  VTK_NEWINSTANCE vtkCamera* MakeCamera() override;

protected:
  vtkOpenVRRenderer() = default;
  ~vtkOpenVRRenderer() override = default;

private:
  vtkOpenVRRenderer(const vtkOpenVRRenderer&) = delete;
  void operator=(const vtkOpenVRRenderer&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
